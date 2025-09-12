#include "wave_image.h"
#include <cstdlib>

namespace sv {

void WaveImage::SetPixel(int x, int y) {
  if (x < 0 || x >= width_) return;
  if (y < 0 || y >= height_) return;
  buffer_[y * width_ * x] = true;
}

void WaveImage::DrawLine(int x0, int y0, int x1, int y1) {
  if (x0 < 0 || x1 < 0 || x0 >= width_ || x1 >= width_) return;
  if (y0 < 0 || y1 < 0 || y0 >= height_ || y1 >= height_) return;
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
  // nicode value is 0x28xx, with xx being the 8-bits above.
  uint16_t braille = 0x2800;
  if (x >= width_ / 2 || y >= height_ / 4) return braille;
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
  WaveImage img(cfg.char_w * 2, cfg.char_h * 4);

  int p0_idx = cfg.left_idx;
  while (p0_idx > 0 && wave[p0_idx].time > cfg.left_time) {
    p0_idx--;
  }
  // Draw stable signal up to the first point.
  if (wave[p0_idx].time > cfg.left_time) {
  }


  return img;
}
} // namespace sv
