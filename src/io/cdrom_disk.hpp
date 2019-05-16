#pragma once

#include <util/types.hpp>

#include <fstream>
#include <string>
#include <vector>

constexpr u32 SECTORS_PER_SECOND = 75;
constexpr u32 SECTOR_SIZE = 2352;

namespace io {

// Represents a position in mm:ss:ff (minute-second-frame) format
struct CdromPosition {
  // track ('TT') is always 01
  u8 minutes{};  // 'MM'
  u8 seconds{};  // 'SS'
  u8 frames{};   // 'FF'

  CdromPosition() = default;
  constexpr CdromPosition(u8 minutes, u8 seconds, u8 frames)
      : minutes(minutes),
        seconds(seconds),
        frames(frames) {}

  static CdromPosition from_lba(u32 lba) {
    u8 minutes = (u8)((u32)lba / 60 / SECTORS_PER_SECOND);
    u8 seconds = (u8)((u32)lba % (60 * SECTORS_PER_SECOND) / SECTORS_PER_SECOND);
    u8 frames = (u8)((u32)lba % SECTORS_PER_SECOND);
    return CdromPosition(minutes, seconds, frames);
  }
  u32 to_lba() const {
    return (minutes * 60 * SECTORS_PER_SECOND) + (seconds * SECTORS_PER_SECOND) + frames;
  }

  CdromPosition operator+(const CdromPosition& lhs) const { return from_lba(to_lba() + lhs.to_lba()); }
  CdromPosition operator-(const CdromPosition& lhs) const { return from_lba(to_lba() - lhs.to_lba()); }
  void physical_to_logical() { *this = *this - CdromPosition(0, 2, 0); }
  void logical_to_physical() { *this = *this + CdromPosition(0, 2, 0); }
};

using CdromSize = CdromPosition;

static constexpr auto CDROM_INDEX_1_POS = CdromPosition(0, 2, 0);
static constexpr auto PREGAP_FRAME_COUNT = SECTORS_PER_SECOND * 2;

struct CdromTrack {
  enum class DataType {
    Invalid,
    Audio,
    Data,
  };

  DataType type{ DataType::Invalid };
  std::string filepath;
  u32 number{};

  CdromSize pregap{};
  CdromPosition start{};

  u32 offset{};  // File offset in sectors/frames
  u32 frame_count{};

  std::ifstream file;
};

class CdromDisk {
 public:
  buffer read(CdromPosition pos);

  void init_from_bin(const std::string& bin_path);
  void init_from_cue(const std::string& cue_path);

  const CdromTrack& track(u32 track_number) const { return m_tracks[track_number]; }
  size_t get_track_count() const { return m_tracks.size(); }

  CdromSize size() const {
    u32 sectors{};
    for (auto& t : m_tracks)
      sectors += t.frame_count;
    return CdromPosition::from_lba(sectors) + CDROM_INDEX_1_POS;
  }

  CdromPosition get_track_start(u32 track_number) const {
    u32 start{};

    const auto track_count = get_track_count();

    if (track_count > 0) {
      // Add Pregap if necessary
      if (m_tracks[0].type == CdromTrack::DataType::Data)
        start += PREGAP_FRAME_COUNT;

      if (track_count > 1) {
        for (u32 t = 0; t < track_number - 1; t++)
          start += m_tracks[t].frame_count;
      }
    }

    return CdromPosition::from_lba(start);
  }

  // Returns nullptr if the position is out of bounds of laoded tracks
  CdromTrack* get_track_by_pos(CdromPosition pos) {
    for (auto i = 0; i < m_tracks.size(); ++i) {
      const auto pos_lba = pos.to_lba();
      const auto start = get_track_start(i).to_lba();
      const auto size = m_tracks[i].frame_count;

      if (pos_lba >= start && pos_lba < start + size)
        return &m_tracks[i];
    }
    return nullptr;
  }

 private:
  void create_track_for_bin(const std::string& bin_path);

  std::string filepath;
  std::vector<CdromTrack> m_tracks;
};

}  // namespace io

namespace util {

inline u8 bcd_to_dec(u8 val) {
  return ((val / 16 * 10) + (val % 16));
}

inline u8 dec_to_bcd(u8 val) {
  return ((val / 10 * 16) + (val % 10));
}

}  // namespace util
