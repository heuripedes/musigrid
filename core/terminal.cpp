#include "terminal.hpp"
#include "util.hpp"

#include <cstdlib>
#include <stdint.h>

#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STBI_PNG_ONLY
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STB_IMAGE_STATIC

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#endif
#ifdef __clang__
#pragma clang diagnostic ignored "-Wduplicated-branches"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#else
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#ifdef __clang__
#pragma clang diagnostic push
#else
#pragma GCC diagnostic pop
#endif

void Terminal::configure(int w, int h) {
  cols = w;
  rows = h;
  cursor_x = 0;
  cursor_y = 0;
  fg = default_fg = 1;
  bg = default_bg = 0;

  // orca theme
  colors[0] = Color{0x00, 0x00, 0x00, 0xff}; // black
  colors[1] = Color{0xff, 0xff, 0xff, 0xff}; // white
  colors[2] = Color{0xdd, 0xdd, 0xdd, 0xff}; // gray4
  colors[3] = Color{0x77, 0x77, 0x77, 0xff}; // gray3
  colors[4] = Color{0x44, 0x44, 0x44, 0xff}; // gray2
  colors[5] = Color{0x22, 0x22, 0x22, 0xff}; // gray1
  colors[6] = Color{0x72, 0xde, 0xc2, 0xff}; // cyan
  colors[7] = Color{0xfb, 0xb5, 0x45, 0xff}; // yellow

  buffer.resize(rows * cols);
  back_buffer.resize(rows * cols);

  for (auto &c : buffer) {
    c.ch = 32 + rand() % (96 - 32);
    c.fg = rand() % 8;
    c.bg = rand() % 8;
  }
}

void Terminal::set_font(std::string name) {
#define if_font(NAME, CHAR_W, CHAR_H)                                          \
  if (name == #NAME) {                                                         \
    extern uint8_t NAME##_data[];                                              \
    extern unsigned NAME##_size;                                               \
    int channels;                                                              \
    font = stbi_load_from_memory(NAME##_data, NAME##_size, &font_w, &font_h,   \
                                 &channels, 4);                                \
    char_w = CHAR_W;                                                           \
    char_h = CHAR_H;                                                           \
  }
  // clang-format off
  if_font(unscii8, 8, 8)
  else if_font(unscii8_alt, 8, 8)
  else if_font(unscii8_fantasy, 8, 8)
  else if_font(unscii8_mcr, 8, 8)
  else if_font(unscii8_tall, 8, 16)
  else if_font(unscii8_thin, 8, 8)
  else if_font(unscii16, 8, 16)
  else { abort(); }
  // clang-format on
#undef if_font
}

void Terminal::putcs(int x, int y, const char *str) {
  int ox = x;

  while (*str) {
    if (*str == '\n') {
      y++;
      x = ox;
    } else if (*str == '\r') {
      x = ox;
    } else if (*str == '\t') {
      x += 8;
    } else {
      putc(*str, x, y, fg, bg);
      x++;
    }

    str++;

    if (x >= cols) {
      // if we hit the edge, skip the rest of the line.
      while (*str && (*str != '\r' && *str != '\n')) {
        ++str;
      }
      x = cols - 1;
    }
  }
  cursor_x = x;
  cursor_y = y;
}

void Terminal::draw_buffer(uint8_t *out, size_t out_pitch) {
  const int out_cols = (out_pitch / sizeof(uint32_t)) / char_w;
  const int font_pitch = font_w * sizeof(uint32_t);
  const auto max_cols = std::min(cols, out_cols);

  for (int cell_y = 0; cell_y < rows; ++cell_y) {
    for (int cell_x = 0; cell_x < max_cols; ++cell_x) {
      const auto cell = buffer[cell_y * cols + cell_x];
      auto ch = cell.ch;

      // if (back_buffer[cell_y * cols + cell_x] == cell)
        // continue;

      if (ch < ' ' || ch > 127)
        ch = '?';

      ch = ch - ' ';

      const int char_row = ch / 32;
      const int char_col = ch % 32;

      const int dst_x = cell_x * char_w;
      const int dst_y = cell_y * char_h;

      const int src_x = char_col * char_w;
      const int src_y = char_row * char_h;

      const auto fg = colors[cell.fg];
      const auto bg = colors[cell.bg];

      // fast path for empty space
      if (src_x == 0 && src_y == 0) {
        for (int cy = 0; cy < char_h; ++cy) {
          // clang-format off
          const uint8_t bg_color[] = {bg.b, bg.g, bg.r, 255};
          uint32_t pixel = *(uint32_t *)bg_color;
          uint32_t *dst = (uint32_t *)(out + (dst_y + cy) * out_pitch + dst_x * sizeof(uint32_t));
          // clang-format on

          std::fill(dst, dst + char_w, pixel);
        }
      } else {
        for (int cy = 0; cy < char_h; ++cy) {
          // clang-format off
          const uint8_t *src = font + (src_y + cy) * font_pitch + src_x * sizeof(uint32_t);
          uint8_t *dst = out + (dst_y + cy) * out_pitch + dst_x * sizeof(uint32_t);
          // clang-format on

          for (int cx = 0; cx < char_w; ++cx) {
            // drawing background is the most common case.
            if (!*src) {
              *dst++ = bg.b;
              *dst++ = bg.g;
              *dst++ = bg.r;
            } else {
              *dst++ = fg.b;
              *dst++ = fg.g;
              *dst++ = fg.r;
            }
            *dst++ = 255;

            src += 4;
          }
        }
      }
    }
  }

  back_buffer = buffer;
}
