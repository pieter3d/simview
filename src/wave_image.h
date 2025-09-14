#pragma once

#include "radix.h"
#include "wave_data.h"

#include <cstdint>
#include <vector>

namespace sv {

class WaveImage {
 public:
  WaveImage(int w, int h) : width_(w), height_(h), buffer_(w * h) {}
  bool GetPixel(int x, int y) const { return buffer_[y * width_ * x]; }
  void DrawLine(int x0, int y0, int x1, int y1);
  uint16_t GetBrailleChar(int x, int y);

 private:
  void SetPixel(int x, int y);
  const int width_;
  const int height_;
  std::vector<bool> buffer_;
};

enum class AnalogWaveType { kSampleAndHold = 0, kLinear };

struct WaveImageConfig {
  int char_w;
  int char_h;
  // Hint indicator where the sample is that's near the left edge of the image.
  int left_idx = 0;
  uint64_t left_time;
  uint64_t right_time;
  Radix radix = Radix::kBinary;
  AnalogWaveType analog_type;
};

WaveImage RenderWaves(const WaveImageConfig &cfg, const std::vector<WaveData::Sample> &wave);

// Returns a close-enough ASCII character for the given braille pattern.
char BraillePatternToAscii(uint8_t b);

} // namespace sv
