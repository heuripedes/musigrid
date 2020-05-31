#pragma once

#include "machine.hpp"
#include "terminal.hpp"
#include <array>
#include <memory>

struct System {
  static constexpr const std::array<char, 46> CURSOR_CHARS = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B',
      'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
      'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      '*', '#', ':', '%', '!', '?', ';', '=', '$', '.'};

  struct Vec2i {
    union {
      // clang-format off
      struct { int x, y; };
      struct { int w, h; };
      // clang-format on
    };
    Vec2i() = default;
  };

  struct Input {
    bool up, down, left, right;
    bool ins, del, pgup, pgdown;
    bool enter, backspace;
    Input() = default;

    Input get_pressed(const Input &prev) const {
      Input out;
      out.up = !prev.up && up;
      out.down = !prev.down && down;
      out.left = !prev.left && left;
      out.ins = !prev.ins && ins;
      out.del = !prev.del && del;
      out.pgup = !prev.pgup && pgup;
      out.pgdown = !prev.pgdown && pgdown;
      out.enter = !prev.enter && enter;
      out.backspace = !prev.backspace && backspace;

      return out;
    }
  };

  struct InsertMenu {
    static constexpr const std::array<std::array<char, 10>, 5> ITEMS = {
        {{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
         {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'},
         {'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'},
         {'U', 'V', 'W', 'X', 'Y', 'Z', '*', '#', ':', '%'},
         {'!', '?', ';', '=', '$', '.', '.', '.', '.', '.'}}};

    static constexpr int items_cols() { return ITEMS[0].size(); }
    static constexpr int items_rows() { return ITEMS.size(); }

    const System &system;
    Vec2i pos;
    Vec2i cursor;
    bool is_open = false;
    bool ucase = true;

    InsertMenu(const System &s) : system(s) {}

    void open(int menu_x, int menu_y, char current_char) {
      is_open = true;
      cursor.x = cursor.y = 0;

      if (menu_x + items_cols() > system.machine.grid_w())
        menu_x = system.machine.grid_w() - items_cols();

      if (menu_y + items_rows() + 1 > system.machine.grid_h())
        menu_y = system.machine.grid_h() - items_rows() - 1;

      pos.x = menu_x;
      pos.y = menu_y;

      for (int y = 0; y < items_rows(); y++) {
        for (int x = 0; x < items_cols(); x++) {
          if (ITEMS[y][x] == current_char ||
              ITEMS[y][x] == toupper(current_char)) {
            cursor.x = x;
            cursor.y = y;
          }
        }
      }
    }

    char accept() {
      is_open = false;
      auto c = ITEMS[cursor.y][cursor.x];
      return ucase ? toupper(c) : tolower(c);
    }

    void toggle_case() { ucase = !ucase; }

    void cancel() { is_open = false; }

    void move_cursor(int dx, int dy) {
      cursor.x = cursor.x + dx;
      cursor.y = cursor.y + dy;

      if (cursor.x >= items_cols())
        cursor.x = 0;
      if (cursor.x < 0)
        cursor.x = items_cols() - 1;
      if (cursor.y >= items_rows())
        cursor.y = 0;
      if (cursor.y < 0)
        cursor.y = items_rows() - 1;
    }
  };

  Machine machine;
  Terminal term;

  Vec2i video_size;
  Vec2i cursor;

  Input old_input;

  InsertMenu insert_menu;

  void set_size(int width, int height);
  void handle_input(Input input);

  void draw() {
    term.clear();

    // draw grid
    for (int y = 0; y < machine.grid_h(); ++y) {
      for (int x = 0; x < machine.grid_w(); ++x) {
        if (x % 10 == 0 && y % 10 == 0)
          term.putc('+', x, y, 5, 0);
        else if (x % 2 == 0 && y % 2 == 0)
          term.putc('.', x, y, 5, 0);
        else
          term.putc(' ', x, y, 1, 0);
      }
    }

    auto cursor_cell = machine.get_cell(cursor.x, cursor.y);

    for (int y = 0; y < machine.grid_h(); ++y) {
      for (int x = 0; x < machine.grid_w(); ++x) {
        auto cell = machine.get_cell(x, y);
        char ch = cell->c;

        if (cursor_cell->c == cell->c && cell->c != '.') {
          term.putc(ch, x, y, 7, 0);
          continue;
        }

        if (cell->flags & CF_WAS_TICKED)
          term.putc(ch, x, y, 0, 6);
        else if (cell->flags & CF_WAS_READ)
          term.putc(ch, x, y, (cell->flags & CF_IS_LOCKED) ? 1 : 6, 0);
        else if (cell->flags & CF_WAS_WRITTEN)
          term.putc(ch, x, y, 0, 1);
        else if (ch != '.' || cell->flags & CF_IS_LOCKED)
          term.putc(ch, x, y, (cell->flags & CF_WAS_READ) ? 1 : 3, 0);
      }
    }

    if (!insert_menu.is_open) {
      term.putc(cursor_cell->c == '.' ? '@' : (char)cursor_cell->c, cursor.x,
                cursor.y, 0, 7);
    } else {
      int draw_x = insert_menu.pos.x;
      int draw_y = insert_menu.pos.y;

      term.fg = 6;
      term.bg = 7;
      term.print(draw_x, draw_y++, "INS -");

      term.reset_color();

      for (int y = 0; y < insert_menu.items_rows(); ++y) {
        for (int x = 0; x < insert_menu.items_cols(); ++x) {
          char c = insert_menu.ucase ? toupper(insert_menu.ITEMS[y][x])
                                     : tolower(insert_menu.ITEMS[y][x]);

          if (x == insert_menu.cursor.x && y == insert_menu.cursor.y) {
            term.fg = 7;
            term.bg = 0;
          } else {
            term.fg = 1;
            term.bg = 3;
          }

          term.putc(c, draw_x + x, draw_y + y);
        }
      }
    }

    term.reset_color();
    term.print(0, machine.grid_h() + 0, " %10s   %02i,%02i %8uf",
               machine.cell_descs[cursor.y][cursor.x], cursor.x, cursor.y,
               machine.ticks);
    term.print(0, machine.grid_h() + 1, " %10s   %2s %2s %8u%c", "", "", "",
               machine.bpm, machine.ticks % 4 == 0 ? '*' : ' ');
  }
};
