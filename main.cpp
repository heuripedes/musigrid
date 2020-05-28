#include <SDL.h>
#include <assert.h>
#include <cctype>
#include <cfloat>
#include <cstdlib>
#include <map>
#include <memory>
#include <vector>

#include "libtcod/src/libtcod.hpp"

#include "machine.hpp"

#define GRID_W 80
#define GRID_H 20

struct Point {
  int x, y;
};

#define ALPHABET "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ*#:%!?;=$."
static Point cursor = {GRID_W / 2, GRID_H / 2};
static char cursor_char = sizeof(ALPHABET) - 1;

int main() {
  printf("sizeof(Cell) = %zu\n", sizeof(Cell));

  TCOD_key_t key = {TCODK_NONE, 0};
  TCOD_mouse_t mouse;

  TCODConsole::setCustomFont(
      "/home/higor/code/musigrid/libtcod/data/fonts/terminal10x16_gs_tc.png",
      TCOD_FONT_LAYOUT_TCOD);

  TCODConsole::initRoot(80, 25, "libtcod C++ sample");
  SDL_SetWindowResizable(TCOD_sys_get_sdl_window(), SDL_FALSE);
  atexit(TCOD_quit);

  TCODSystem::setFps(60);

  float secs_then = TCODSystem::getElapsedSeconds();
  float secs_acc = 0;

  unsigned frames = 0;
  unsigned ticks = 0;

  unsigned speed = 2;

  Machine machine;
  machine.init(GRID_W, GRID_H);

  while (!TCODConsole::isWindowClosed()) {
    TCOD_event_t ev;

    float secs_now = TCODSystem::getElapsedSeconds();
    float delta = secs_now - secs_then;
    secs_then = secs_now;
    secs_acc += delta;

    while ((ev = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse)) !=
           TCOD_EVENT_NONE) {
      switch (ev) {
      case TCOD_EVENT_KEY_PRESS: {
        Point p = cursor;
        p.x += (key.vk == TCODK_RIGHT) - (key.vk == TCODK_LEFT);
        p.y += (key.vk == TCODK_DOWN) - (key.vk == TCODK_UP);

        if (machine.is_valid(p.x, p.y))
          cursor = p;

        cursor_char += (key.vk == TCODK_PAGEUP) - (key.vk == TCODK_PAGEDOWN);
        cursor_char %= sizeof(ALPHABET);
        break;
      }
      case TCOD_EVENT_KEY_RELEASE: {
        // auto cursor_cell = machine.get_cell(cursor.x,
        // cursor.y);//&cells[cursor.y][cursor.x];

        if (key.vk == TCODK_ENTER)
          machine.new_cell(cursor.x, cursor.y, ALPHABET[cursor_char]);

        if (key.vk == TCODK_INSERT)
          machine.new_cell(cursor.x, cursor.y, toupper(ALPHABET[cursor_char]));

        if (key.vk == TCODK_DELETE)
          machine.new_cell(cursor.x, cursor.y, '.');
        break;
      }
      default:
        break;
      }
    }

    unsigned bpm = speed * 60;

    bool is_tick = frames % (speed * 4) == 0;
    bool is_beat = secs_acc > (60 / (float)bpm);

    if (is_beat)
      secs_acc -= (60 / (float)bpm);

    if (is_tick) {
      ticks++;
      /*
            // clear flags
            for (int y = 0; y < GRID_H; ++y) {
              for (int x = 0; x < GRID_W; ++x) {
                auto cell = &cells[y][x];
                cell->flags = 0;
              }
            }

            tick(ticks);
            */

      machine.tick();
    }

    auto root = TCODConsole::root;

    // draw grid
    for (int y = 0; y < GRID_H; ++y) {
      for (int x = 0; x < GRID_W; ++x) {
        if (x % 10 == 0 && y % 10 == 0)
          root->putCharEx(x, y, '+', TCODColor::darkestGrey, TCODColor::black);
        else if (x % 2 == 0 && y % 2 == 0)
          root->putCharEx(x, y, '.', TCODColor::darkestGrey, TCODColor::black);
        else
          root->putCharEx(x, y, '.', root->getDefaultBackground(),
                          root->getDefaultBackground());
      }
    }

    // render chars
    for (int y = 0; y < machine.grid_h(); ++y) {
      for (int x = 0; x < machine.grid_w(); ++x) {
        auto cell = machine.get_cell(x, y);
        char ch = cell->c;

        if (cursor.x == x && cursor.y == y && cell->c == '.')
          ch = int_to_b36(cursor_char, false);

        if (cell->flags & CF_WAS_READ) {
          if (cell->flags & CF_IS_LOCKED)
            root->putCharEx(x, y, ch, TCODColor::white,
                            root->getDefaultBackground());
          else
            root->putCharEx(x, y, ch, TCODColor::cyan,
                            root->getDefaultBackground());
        } else if (cell->flags & CF_WAS_WRITTEN)
          root->putCharEx(x, y, ch, TCODColor::black, TCODColor::white);
        else if (isupper(ch))
          root->putCharEx(x, y, ch, TCODColor::black, TCODColor::cyan);
        else if (cell->flags & CF_IS_LOCKED)
          root->putCharEx(x, y, ch, TCODColor::grey,
                          root->getDefaultBackground());
        else if (ch != '.') {
          root->putCharEx(x, y, ch, TCODColor::grey,
                          root->getDefaultBackground());
          // root->setCharBackground(x, y, root->getDefaultBackground());
          // root->setCharForeground(x, y,
          //                         (cell->flags & CF_WAS_BANGED)
          //                             ? TCODColor::white
          //                             : TCODColor::grey);
        }
      }
    }

    auto cursor_cell = machine.get_cell(cursor.x, cursor.y);

#if 1
    root->setCharBackground(cursor.x, cursor.y, TCODColor::yellow);
    root->setCharForeground(cursor.x, cursor.y, TCODColor::black);

    if (frames % 30 > 15)
      root->putCharEx(cursor.x, cursor.y, ALPHABET[cursor_char], TCODColor::black, TCODColor::yellow);
    else
      root->putCharEx(cursor.x, cursor.y, cursor_cell->c, TCODColor::yellow, TCODColor::black);
#else
    const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ*#:%!?;=$.....";
    const int menu_cols = 10;
    const int menu_rows = sizeof(chars) / menu_cols;

    int menu_pos = 0;
    for (int i = 0; i < sizeof(chars); ++i) {
      if (cursor_cell->c == chars[i] || toupper(cursor_cell->c) == chars[i]) {
        menu_pos = i;
        break;
      }
    }

    int cursor_col = menu_pos % menu_cols;
    int cursor_row = menu_pos / menu_rows;

    int menu_x = cursor.x;
    int menu_y = cursor.y;

    if (menu_x + menu_cols > GRID_W)
      menu_x = GRID_W - menu_cols;

    if (menu_y + menu_rows > GRID_H)
      menu_y = GRID_H - menu_rows;

    for (int y = 0; y < menu_rows; ++y) {
      for (int x = 0; x < menu_cols; ++x) {
        int idx = y * menu_cols + x;
        char c = '.';

        if (idx < sizeof(chars))
          c = chars[idx];

        root->putChar(menu_x + x, menu_y + y, c);
        if (cursor_cell->c == c) {
          root->setCharBackground(menu_x + x, menu_y + y, TCODColor::yellow);
          root->setCharForeground(menu_x + x, menu_y + y, TCODColor::black);
        } else
          root->setCharBackground(menu_x + x, menu_y + y, TCODColor::grey);
      }
    }

#endif

    root->printf(0, 21, "      %02i,%02i %8uf", cursor.x, cursor.y, ticks);
    root->printf(0, 22, "            %8u%c", bpm, is_beat ? '*' : ' ');

    TCODConsole::flush();

    frames++;
  }

  TCOD_quit();
  return 0;
}
