#include "system.hpp"

const std::array<std::array<char, 10>, 5> System::InsertMenu::ITEMS = {
    {{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
     {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'},
     {'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'},
     {'U', 'V', 'W', 'X', 'Y', 'Z', '*', '#', ':', '%'},
     {'!', '?', ';', '=', '$', '.', '.', '.', '.', '.'}}};

void System::set_size(int width, int height) {
  video_size.x = width;
  video_size.y = height;

  term.set_font("unscii16");

  auto rows = height / term.char_h;
  auto cols = width / term.char_w;

  term.configure(cols, rows);
  term.fg = 1;
  term.bg = 0;

  machine.init(cols, rows - 2);
}

void System::handle_input(Input input) {
  auto pressed = input.get_pressed(old_input);
  // auto released = old_input.get_pressed(input);
  old_input = input;

  if (insert_menu.is_open) {
    insert_menu.move_cursor(input.right - input.left, input.down - input.up);

    if (pressed.del)
      insert_menu.cancel();
    else if (pressed.enter || pressed.ins)
      machine.new_cell(cursor.x, cursor.y, insert_menu.accept());
    else if (pressed.pgdown || pressed.pgup)
      insert_menu.toggle_case();

  } else {
    Vec2i p = cursor;
    p.x += input.right - input.left;
    p.y += input.down - input.up;

    if (machine.is_valid(p.x, p.y))
      cursor = p;

    if (pressed.ins) {
      auto cur_char = machine.get_cell(cursor.x, cursor.y)->c;
      insert_menu.open(cursor.x, cursor.y,
                       cur_char == '.' ? Cell::Glyph('O') : cur_char);
    } else if (pressed.del)
      machine.new_cell(cursor.x, cursor.y, '.');
  }
}

void System::draw() {
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
