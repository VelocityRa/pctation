#include <memory/dma.hpp>

#include <cpu/interrupt.hpp>
#include <gpu/gpu.hpp>
#include <io/cdrom_drive.hpp>
#include <memory/dma_channel.hpp>
#include <memory/ram.hpp>
#include <util/log.hpp>

#include <gsl-lite.hpp>

namespace memory {

constexpr u32 RAM_ADDR_MASK = 0x1FFFFC;

DmaChannel const& Dma::channel_control(DmaPort port) const {
  const auto port_index = (u32)port;
  Expects(port_index < 7);
  return m_channels[port_index];
}

DmaChannel& Dma::channel_control(DmaPort port) {
  const auto port_index = (u32)port;
  Expects(port_index < 7);
  return m_channels[port_index];
}

void Dma::step() {
  if (m_irq_pending) {
    m_irq_pending = false;
    m_interrupts.trigger(cpu::IrqType::DMA);
  }
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

  // TODO: optimize for the few combinations that are actually used

  while (transfer_word_count > 0) {
    const auto addr_cur = addr & RAM_ADDR_MASK;

    switch (channel.transfer_direction()) {
      case DmaChannel::TransferDirection::ToRam: {
        u32 src_word{};

        switch (port) {
          case DmaPort::MdecOut:
            // TODO
            LOG_INFO("DMA transfer of word 0x{:08X} to MDEC-Out port", src_word);
            break;
          // Not supposed to read from anywhere for OTC, values are specific and depend on the address
          case DmaPort::Otc:
            if (transfer_word_count == 1)
              // Last OTC entry contains the "End of table" marker
              src_word = 0xFFFFFF;
            else
              // Each of the rest of the entries points to the previous one
              src_word = (addr - 4) & RAM_ADDR_MASK;
            break;
          case DmaPort::Gpu:
            src_word = m_gpu.get_vram_pos(m_gpu.m_vram_transfer_x, m_gpu.m_vram_transfer_y);
            m_gpu.advance_vram_transfer_pos();
            break;
          case DmaPort::Cdrom: src_word = m_cdrom.read_word(); break;
          default: LOG_WARN("DMA transfer to unimplemented port {} requested", static_cast<u8>(port));
        }
        m_ram.write<u32>(addr_cur, src_word);
        break;
      }
      case DmaChannel::TransferDirection::FromRam: {
        u32 src_word = m_ram.read<u32>(addr_cur);

        switch (port) {
          case DmaPort::MdecIn:
            // TODO
            LOG_INFO("DMA transfer of word 0x{:08X} to MDEC-In port", src_word);
            break;
          case DmaPort::Gpu:
            // Send packet (which is part of a GP0 command, likely data) to the GPU
            m_gpu.gp0(src_word);
            break;
          case DmaPort::Spu:
            // TODO
            LOG_INFO("DMA transfer of word 0x{:08X} to SPU port", src_word);
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

  transfer_finished(channel, port);
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
  transfer_finished(channel, port);
}

void Dma::transfer_finished(DmaChannel& channel, DmaPort port) {
  channel.transfer_finished();

  bool is_enabled = m_reg_interrupt.is_port_enabled(port);

  if (is_enabled) {
    m_reg_interrupt.set_port_flags(port, true);
    m_irq_pending = m_reg_interrupt.get_irq_master_flag();
  }
}

}  // namespace memory
