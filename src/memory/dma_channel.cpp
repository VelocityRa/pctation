#include <memory/dma_channel.hpp>

#include <util/log.hpp>

namespace memory {

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

}  // namespace memory
