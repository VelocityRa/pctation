#include <memory/dma.hpp>

#include <gpu/gpu.hpp>
#include <memory/ram.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

namespace memory {

constexpr u32 RAM_ADDR_MASK = 0x1FFFFC;

bool DmaChannel::active() const {
  const auto enable = m_channel_control.enable;

  if (sync_mode() == SyncMode::Manual) {
    const auto trigger = m_channel_control.manual_trigger;
    return enable && trigger;
  } else
    return enable;
}

u32 DmaChannel::transfer_word_count() {
  const auto manual = m_block_control.manual;
  const auto request = m_block_control.request;

  switch (sync_mode()) {
    case DmaChannel::SyncMode::Manual: return manual.word_count;
    case DmaChannel::SyncMode::Request: return request.block_size * request.block_count;
    case DmaChannel::SyncMode::LinkedList:
      // fallthrough
    default:
      LOG_ERROR("Invalid sync mode");
      assert(0);
      return 0;
  }
}

void DmaChannel::transfer_finished() {
  m_channel_control.enable = false;
  m_channel_control.manual_trigger = false;

  // TODO: interrupts
}

u32 Dma::read_reg(DmaRegister reg) const {
  const u32 reg_offset = (u32)reg;
  const auto major = (reg_offset & 0x70) >> 4;
  const auto minor = reg_offset & 0xF;

  if (0 <= major && major <= 6) {
    // Per-channel registers
    const auto& channel = channel_control(static_cast<DmaPort>(major));

    switch (minor) {
      case 0: return channel.m_base_addr;
      case 4: return channel.m_block_control.word;
      case 8: return channel.m_channel_control.word;
      default: LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", reg_offset); return 0;
    }
  } else if (major == 7) {
    // Common registers
    switch (minor) {
      case 0: return m_control;
      case 4: return m_interrupt.word;
      default: LOG_WARN("Unhandled read from DMA at offset 0x{:08X}", reg_offset); return 0;
    }
  } else {
    LOG_WARN("Unhandled read from DMA at 0x{:08X}", reg_offset);
    return 0;
  }
}
void Dma::set_reg(DmaRegister reg, u32 val) {
  const u32 reg_offset = (u32)reg;
  const auto major = (reg_offset & 0x70) >> 4;
  const auto minor = reg_offset & 0xF;

  if (0 <= major && major <= 6) {  // Per-channel registers
    const auto port = static_cast<DmaPort>(major);
    auto& channel = channel_control(port);

    switch (minor) {
      case 0: channel.m_base_addr = val; break;
      case 4: channel.m_block_control.word = val; break;
      case 8: channel.m_channel_control.word = val; break;
      default:
        LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, reg_offset);
        break;
    }

    if (channel.active())
      do_transfer(port);

  } else if (major == 7) {  // Common registers
    switch (minor) {
      case 0: m_control = val; break;
      case 4: m_interrupt.word = val; break;
      default:
        LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, reg_offset);
        break;
    }
  } else
    LOG_WARN("Unhandled write to DMA register: 0x{:08X} at offset 0x{:08X}", val, reg_offset);
}

DmaChannel const& Dma::channel_control(DmaPort port) const {
  const auto port_index = (u32)port;
  Ensures(port_index < 7);
  return m_channels[port_index];
}

DmaChannel& Dma::channel_control(DmaPort port) {
  const auto port_index = (u32)port;
  Ensures(port_index < 7);
  return m_channels[port_index];
}

void Dma::do_transfer(DmaPort port) {
  auto& channel = channel_control(port);

  switch (channel.sync_mode()) {
    case DmaChannel::SyncMode::Manual:
    case DmaChannel::SyncMode::Request: do_block_transfer(port); break;
    case DmaChannel::SyncMode::LinkedList: do_linked_list_transfer(port); break;
    default: LOG_ERROR("Invalid sync mode"); assert(0);
  }
}

void Dma::do_block_transfer(DmaPort port) {
  auto& channel = channel_control(port);

  const s32 addr_step =
      (channel.memory_address_step() == DmaChannel::MemoryAddressStep::Forward) ? 4 : -4;

  address addr = channel.m_base_addr;

  u32 transfer_word_count = channel.transfer_word_count();

  LOG_DEBUG("Starting DMA block transfer: {} {} RAM, sync mode: {}", dma_port_to_str(port),
            channel.to_ram() ? "to" : "from", channel.sync_mode_str());

  while (transfer_word_count > 0) {
    const auto addr_cur = addr & RAM_ADDR_MASK;

    switch (channel.transfer_direction()) {
      case DmaChannel::TransferDirection::ToRam: {
        u32 src_word{};

        switch (port) {
          // Not supposed to read from anywhere for OTC, values are specific and depend on the address
          case DmaPort::Otc:
            if (transfer_word_count == 1)
              src_word = 0xFFFFFF;  // Last OTC entry contains the "End of table" marker
            else
              // TODO: Should this be RAM_ADDR_MASK?
              src_word =
                  (addr - 4) & 0x1FFFFF;  // Each of the rest of the entries points to the previous one
            break;
          default: LOG_WARN("DMA transfer to unimplemented port {} requested", static_cast<u8>(port));
        }
        m_ram.write<u32>(addr_cur, src_word);
        break;
      }
      case DmaChannel::TransferDirection::FromRam: {
        u32 src_word = m_ram.read<u32>(addr_cur);
        switch (port) {
          case DmaPort::Gpu:
            // Send packet (which is part of a GP0 command, likely data) to the GPU
            m_gpu.gp0(src_word);
            break;
          default:
            LOG_WARN("DMA transfer of word 0x{:08X} to unimplemented port {} requested", src_word,
                     static_cast<u8>(port));
        }
        break;
      }
      default: LOG_ERROR("Invalid DMA transfer direction"); assert(0);
    }
    addr += addr_step;
    transfer_word_count -= 1;
  }
  channel.transfer_finished();
}

void Dma::do_linked_list_transfer(DmaPort port) {
  auto& channel = channel_control(port);

  Expects(channel.transfer_direction() == DmaChannel::TransferDirection::FromRam);
  Expects(port == DmaPort::Gpu);

  address addr = channel.m_base_addr & RAM_ADDR_MASK;

  LOG_DEBUG("Starting DMA linked list transfer: RAM to GPU");

  while (true) {  // for each packet in the linked list
    const u32 packet_header = m_ram.read<u32>(addr);
    auto packet_word_count = packet_header >> 24;

    if (packet_word_count > 0)
      LOG_DEBUG("GPU packet at {:08X} (words: {})", addr, packet_word_count);

    while (packet_word_count > 0) {
      addr = (addr + 4) & RAM_ADDR_MASK;
      const u32 packet_data = m_ram.read<u32>(addr);

      // Send packet (which is a GP0 command) to the GPU
      m_gpu.gp0(packet_data);

      packet_word_count -= 1;
    }

    // Only check the top bit instead of the whole marker, that's what the hardware does
    if ((packet_header & 0x800000) != 0)
      break;

    addr = packet_header & RAM_ADDR_MASK;
  }
  channel.transfer_finished();
}

}  // namespace memory
