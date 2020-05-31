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
      int x, y;
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
    static const std::array<std::array<char, 10>, 5> ITEMS;

    static constexpr int items_cols() { return ITEMS[0].size(); }
    static constexpr int items_rows() { return ITEMS.size(); }

    const System *system = nullptr;
    Vec2i pos;
    Vec2i cursor;
    bool is_open = false;
    bool ucase = true;

    InsertMenu() {};
    InsertMenu(const System *s) : system(s) {}

    void open(int menu_x, int menu_y, char current_char) {
      is_open = true;
      cursor.x = cursor.y = 0;

      if (menu_x + items_cols() > system->machine.grid_w())
        menu_x = system->machine.grid_w() - items_cols();

      if (menu_y + items_rows() + 1 > system->machine.grid_h())
        menu_y = system->machine.grid_h() - items_rows() - 1;

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
  Vec2i cursor = {0, 0};

  Input old_input;

  InsertMenu insert_menu{this};

  System() = default;

  void set_size(int width, int height);
  void handle_input(Input input);

  void draw();
};
