#include "system.hpp"
#include "machine.hpp"
#include "util.hpp"

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

void System::handle_input(const SimpleInput &new_input) {
  // auto pressed = input.get_pressed(old_input);
  // auto released = old_input.get_pressed(input);
  Input &input = old_input;

  input.left = new_input.left;
  input.right = new_input.right;
  input.down = new_input.down;
  input.up = new_input.up;
  input.del = new_input.del;
  input.ins = new_input.ins;
  input.pgup = new_input.pgup;
  input.pgdown = new_input.pgdown;
  input.enter = new_input.enter;
  input.backspace = new_input.backspace;

  auto &pressed = input;

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

  const auto grid_h = machine.grid_h();
  const auto grid_w = machine.grid_w();

  // draw grid
  for (int y = 0; y < grid_h; ++y) {
    for (int x = 0; x < grid_w; ++x) {
      if (x % 10 == 0 && y % 10 == 0)
        term.putc('+', x, y, 4, 0);
      else if (x % 2 == 0 && y % 2 == 0)
        term.putc('.', x, y, 4, 0);
      else
        term.putc(' ', x, y, 1, 0);
    }
  }

  auto cursor_cell = machine.get_cell(cursor.x, cursor.y);

  for (int y = 0; y < grid_h; ++y) {
    for (int x = 0; x < grid_w; ++x) {
      auto cell = machine.get_cell(x, y);
      char ch = cell->c;

      // highlighted cell
      if (UNLIKELY(cursor_cell->c == ch && !(ch == '.' || ch == '#')))
        term.putc(ch, x, y, 7, 0);
      // read unlocked or read written
      else if (cell->flags & CF_WAS_READ &&
               ((cell->flags & CF_IS_LITERAL) == 0 ||
                cell->flags & CF_WAS_WRITTEN))
        term.putc(ch, x, y, 6, 0);
      // read locked
      else if (cell->flags & CF_WAS_READ && cell->flags & CF_IS_LITERAL)
        term.putc(ch, x, y, 1, 0);
      // write locked
      else if (cell->flags & CF_WAS_WRITTEN)
        term.putc(ch, x, y, 0, 1);
      // literals
      else if (cell->flags & CF_IS_LITERAL) {
        if (cell->flags & CF_WAS_BANGED && ch == '*')
          term.putc(ch, x, y, 1, 0);
        else
          term.putc(ch, x, y, 4, 0);
      }
      // operator, except for NSEW
      else if (cell->flags & CF_WAS_TICKED &&
               (ch != 'E' && ch != 'W' && ch != 'S' && ch != 'N' &&
                ch != '.')) {
        // blink these.
        if (cell->flags & CF_WAS_BANGED && (ch == '%' || ch == ':'))
          term.putc(' ', x, y, 0, 0);
        else if (ch == '*')
          term.putc(ch, x, y, 1, 0);
        else
          term.putc(ch, x, y, 0, 6);
      } else if (ch != '.')
        term.putc(ch, x, y, 4, 0);
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
  term.print(0, grid_h + 0, " %10s   %02i,%02i %8uf",
             machine.cell_descs[cursor.y][cursor.x], cursor.x, cursor.y,
             machine.ticks);
  term.print(0, grid_h + 1, " %10s   %2s %2s %8u%c", "", "", "", machine.bpm,
             machine.ticks % 4 == 0 ? '*' : ' ');
}
