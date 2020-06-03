#pragma once
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

struct Color {
  uint8_t r, g, b, a;
};

struct Terminal {
  Color colors[8];

  struct Cell {
    char ch;
    uint8_t fg : 4;
    uint8_t bg : 4;

    bool operator==(const Cell &o) const {
      return ch == o.ch && fg == o.fg && bg == o.bg;
    }
  };

  uint8_t fg, bg;
  uint8_t default_fg, default_bg;

  int cols;
  int rows;

  // where the next character will be written
  int cursor_x;
  int cursor_y;

  int char_w;
  int char_h;
  uint8_t *font;
  int font_w;
  int font_h;

  std::vector<Cell> buffer;
  std::vector<Cell> back_buffer;

  Terminal() { set_font("unscii16"); }

  void configure(int w, int h);
  void set_font(std::string name);

  void reset_color() {
    fg = default_fg;
    bg = default_bg;
  }

  void clear() {
    for (auto &c : buffer) {
      c.bg = default_bg;
      c.fg = default_fg;
      c.ch = ' ';
    }
  }

  void putc(char c, int x, int y) { putc(c, x, y, fg, bg); }

  void putc(char c, int x, int y, uint8_t fg, uint8_t bg) {
    if (x >= cols || y >= rows)
      return;
    buffer[y * cols + x] = Cell{c, fg, bg};
  }

  void putc(char c) {
    putc(c, cursor_x, cursor_y);
    cursor_x = (cursor_x + 1) % cols;
  }

  void putcs(int x, int y, const char *str);

  void print(const char *fmt, ...) {
    char buf[1024];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    putcs(cursor_x, cursor_y, buf);
  }

  void print(int x, int y, const char *fmt, ...) {
    char buf[1024];
    va_list va;
    va_start(va, fmt);
    vsnprintf(buf, sizeof(buf), fmt, va);
    va_end(va);

    putcs(x, y, buf);
  }

  void draw_buffer(uint8_t *out, size_t pitch);
};
