#include "system.hpp"

void System::set_size(int width, int height) {
  video_size.w = width;
  video_size.h = height;
}

void System::handle_input(Input input) {
  auto pressed = input.get_pressed(old_input);
  auto released = old_input.get_pressed(input);

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
    p.y += input.down - input.top;

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

