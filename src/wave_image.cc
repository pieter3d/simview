#include "wave_image.h"

#include <cstdlib>
#include <string_view>

namespace sv {

char BraillePatternToAscii(uint8_t b) {
  constexpr std::string_view lo_lut = " .,:;a&#@";
  constexpr std::string_view hi_lut = " '\":PM&#@";
  int num_bits = 0;
  int num_lo_bits = 0;
  int num_hi_bits = 0;
  for (int i = 0; i < 8; ++i) {
    if ((b >> i) & 1) {
      num_bits++;
      // Count the number of bits in the upper and lower half of the character.
      if (i <= 4 && i != 2) {
        num_hi_bits++;
      } else {
        num_lo_bits++;
      }
    }
  }
  return (num_lo_bits < num_hi_bits ? hi_lut : lo_lut)[num_bits];
}

void WaveImage::SetPixel(int x, int y) {
  if (x < 0 || x >= width_) return;
  if (y < 0 || y >= height_) return;
  buffer_[y * width_ + x] = true;
}

void WaveImage::DrawLine(int x0, int y0, int x1, int y1) {
  // Bresenham's Line Algorithm
  const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
  if (steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }

  const int dx = x1 - x0;
  const int dy = std::abs(y1 - y0);
  const int ystep = (y0 < y1) ? 1 : -1;
  int error = dx / 2;
  int y = y0;

  for (int x = x0; x <= x1; ++x) {
    if (steep) {
      SetPixel(y, x);
    } else {
      SetPixel(x, y);
    }
    error -= dy;
    if (error < 0) {
      y += ystep;
      error += dx;
    }
  }
}

uint16_t WaveImage::GetBrailleChar(int x, int y) {
  // Bit index as follows:
  // 0 3
  // 1 4
  // 2 5
  // 6 7
  // Unicode value is 0x28xx, with xx being the 8-bits above.
  uint16_t braille = 0x2800;
  if (x < 0 || y < 0 || x >= width_ / 2 || y >= height_ / 4) return braille;
  braille |= buffer_[(4 * y + 0) * width_ + (2 * x + 0)] << 0;
  braille |= buffer_[(4 * y + 1) * width_ + (2 * x + 0)] << 1;
  braille |= buffer_[(4 * y + 2) * width_ + (2 * x + 0)] << 2;
  braille |= buffer_[(4 * y + 0) * width_ + (2 * x + 1)] << 3;
  braille |= buffer_[(4 * y + 1) * width_ + (2 * x + 1)] << 4;
  braille |= buffer_[(4 * y + 2) * width_ + (2 * x + 1)] << 5;
  braille |= buffer_[(4 * y + 3) * width_ + (2 * x + 0)] << 6;
  braille |= buffer_[(4 * y + 3) * width_ + (2 * x + 1)] << 7;
  return braille;
}

WaveImage RenderWaves(const WaveImageConfig &cfg, const std::vector<WaveData::Sample> &wave) {
  // Braille patterns are 2x4 dots, "pixels".
  const int img_w = cfg.char_w * 2;
  const int img_h = cfg.char_h * 4;
  WaveImage img(img_w, img_h);

  int p0_idx = cfg.left_idx;
  while (p0_idx > 0 && wave[p0_idx].time > cfg.left_time) {
    p0_idx--;
  }
  // Make a list of all points and track the max value.
  std::vector<std::pair<double, double>> samples;
  double max = 0;
  double min = 0;
  for (int i = p0_idx; i < wave.size(); ++i) {
    double v = 0;
    switch (cfg.radix) {
    case Radix::kFloat:
      if (wave[i].value.size() <= 32) {
        v = BinStringToFp32(wave[i].value).value_or(0);
      } else {
        v = BinStringToFp64(wave[i].value).value_or(0);
      }
      break;
    case Radix::kSignedDecimal: v = BinStringToSigned(wave[i].value).value_or(0); break;
    default: v = BinStringToUnsigned(wave[i].value).value_or(0);
    }
    if (v > max) max = v;
    if (v < min) min = v;
    samples.push_back({static_cast<double>(wave[i].time) - cfg.left_time, v});
    if (wave[i].time >= cfg.right_time) break;
  }

  const double x_scale = (img_w - 1) / static_cast<double>(cfg.right_time - cfg.left_time);
  const double y_scale = max == min ? 1 : ((img_h - 1) / (max - min));
  for (int i = 1; i < samples.size(); ++i) {
    const int x0 = std::lround(samples[i - 1].first * x_scale);
    const int y0 = img_h - 1 - std::lround(samples[i - 1].second * y_scale);
    const int x1 = std::lround(samples[i].first * x_scale);
    const int y1 = img_h - 1 - std::lround(samples[i].second * y_scale);
    if (cfg.analog_type == AnalogWaveType::kLinear) {
      img.DrawLine(x0, y0, x1, y1);
    } else if (cfg.analog_type == AnalogWaveType::kSampleAndHold) {
      img.DrawLine(x0, y0, x1, y0);
      img.DrawLine(x1, y0, x1, y1);
    }
  }

  return img;
}
} // namespace sv
