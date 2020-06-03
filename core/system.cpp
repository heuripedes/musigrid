#include "system.hpp"
#include "machine.hpp"
#include "util.hpp"

const std::array<std::array<char, 10>, 5> System::InsertMenu::ITEMS = {
    {{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
     {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'},
     {'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'},
     {'U', 'V', 'W', 'X', 'Y', 'Z', '*', '#', ':', '%'},
     {'!', '?', ';', '=', '$', '.', '.', '.', '.', '.'}}};

System::System() {}

void System::set_size(int width, int height) {
  video_size.x = width;
  video_size.y = height;

  auto rows = height / term.char_h;
  auto cols = width / term.char_w;

  term.configure(cols, rows);
  term.fg = 1;
  term.bg = 0;

  if (machine.grid_h() == 0)
    machine.init(cols, rows - 2);
  else
    machine.set_size(cols, rows - 2);
}

void System::handle_input(const SimpleInput &new_input) {
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
  } else if (options_menu.is_open) {
    options_menu.move_cursor(input.down - input.up);

    if (input.enter) {
      auto option = options_menu.accept_int();

      if (option == UI_OPT_RESUME) {
      } else if (option == UI_OPT_FONT) {
        font_menu = PopupList("Select font", {
                                                 "unscii16",
                                                 "unscii8_alt",
                                                 "unscii8_fantasy",
                                                 "unscii8_mcr",
                                                 "unscii8",
                                                 "unscii8_tall",
                                                 "unscii8_thin",
                                             });
        font_menu.open(options_menu.x + 1, 1 + options_menu.selected);
      } else if (option == UI_OPT_SOUNDFONT) {
      }
    } else if (input.backspace || input.del)
      options_menu.cancel();
  } else if (font_menu.is_open) {
    font_menu.move_cursor(input.down - input.up);

    if (input.enter || input.ins) {
      auto font = font_menu.accept_str();
      term.set_font(font);
      set_size(video_size.x, video_size.y);
    } else if (input.backspace || input.del)
      font_menu.cancel();

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
    } else if (pressed.enter) {
      options_menu.open(0, 0);
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

  if (insert_menu.is_open) {
    insert_menu.draw(term);
  } else if (options_menu.is_open) {
    options_menu.draw(term);
  } else if (font_menu.is_open) {
    font_menu.draw(term);
  } else {
    term.putc(cursor_cell->c == '.' ? '@' : (char)cursor_cell->c, cursor.x,
              cursor.y, 0, 7);
  }

  term.reset_color();
  term.print(0, grid_h + 0, " %10s   %02i,%02i %8uf",
             machine.cell_descs[cursor.y][cursor.x], cursor.x, cursor.y,
             machine.ticks);
  term.print(0, grid_h + 1, " %10s   %2s %2s %8u%c", "", "", "", machine.bpm,
             machine.ticks % 4 == 0 ? '*' : ' ');
}
