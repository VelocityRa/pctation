#pragma once

#include <util/types.hpp>

namespace memory {

union DmaChannelControl {
  u32 word{};

  struct {
    u32 transfer_direction : 1;
    u32 memory_address_step : 1;
    u32 _2_7 : 6;
    u32 chopping_enable : 1;
    u32 sync_mode : 2;
    u32 _11_15 : 5;
    u32 chopping_dma_window_size : 3;
    u32 _19 : 1;
    u32 chopping_cpu_window_size : 3;
    u32 _23 : 1;
    u32 enable : 1;
    u32 _25_27 : 3;
    u32 manual_trigger : 1;
    u32 _29_31 : 3;
  };
};

union DmaBlockControl {  // Fields are different depending on sync mode
  u32 word{};

  union {
    // Manual sync mode
    struct {
      u16 word_count;  // number of words to transfer
    } manual;
    // Request sync mode
    struct {
      u16 block_size;   // block size in words
      u16 block_count;  // number of blocks to transfer
    } request;
    // In Linked List mode this register is unused
  };
};

class DmaChannel {
 public:
  enum class TransferDirection {
    ToRam = 0,
    FromRam = 1,
  };
  enum class MemoryAddressStep {
    Forward = 0,
    Backward = 1,
  };
  enum class SyncMode {
    // Transfer starts when CPU writes to the manual_trigger bit and happens all at once
    // Used for CDROM, OTC
    Manual = 0,
    // Sync blocks to DMA requests
    // Used for MDEC, SPU, and GPU-data
    Request = 1,
    // Used for GPU-command-lists
    LinkedList = 2,
  };

  bool active() const;
  u32 transfer_word_count();
  void transfer_finished();
  TransferDirection transfer_direction() const {
    return static_cast<TransferDirection>(m_channel_control.transfer_direction);
  }
  bool to_ram() const { return transfer_direction() == TransferDirection::ToRam; }
  MemoryAddressStep memory_address_step() const {
    return static_cast<MemoryAddressStep>(m_channel_control.memory_address_step);
  }
  SyncMode sync_mode() const { return static_cast<SyncMode>(m_channel_control.sync_mode); }
  const char* sync_mode_str() const {
    switch (sync_mode()) {
      case SyncMode::Manual: return "Manual";
      case SyncMode::Request: return "Request";
      case SyncMode::LinkedList: return "Linked List";
      default: return "<Invalid>";
    }
  }

  DmaChannelControl m_channel_control;
  DmaBlockControl m_block_control;
  u32 m_base_addr{};
};

}  // namespace memory
