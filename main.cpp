#include "libtcod/src/libtcod/console.h"
#include "libtcod/src/libtcod/console.hpp"
#include "libtcod/src/libtcod/console_init.h"
#include "libtcod/src/libtcod/console_types.h"
#include "libtcod/src/libtcod/sys.h"
#include <SDL.h>
#include <SDL_hints.h>
#include <SDL_video.h>
#include <assert.h>
#include <cctype>
#include <map>
#include <memory>
#include <vector>

#define GRID_W 80
#define GRID_H 20

struct Point {
  int x, y;
};

static std::array<char, 36> upper, lower;
struct Cell {
  char c = ' ';
  bool skip = false;
  bool banged = false;
  bool on_bang = false;
  std::vector<Point> inpos;  // inputs
  std::vector<Point> optpos; // optional inputs
  std::vector<Point> outpos; // outputs

  int to_value() const {
    char ch = toupper(c);
    if (ch >= '0' && ch <= '9')
      return ch - '0';
    if (ch >= 'A' && ch <= 'Z')
      return 10 + ch - 'A';

    assert(false && "this shouldnt happen");
  }
};

static std::array<std::array<Cell, GRID_W>, GRID_H> cells;
static Point cursor = {GRID_W / 2, GRID_H / 2};
static char cursor_char = 0;

bool is_valid(Point p) {
  return p.x >= 0 && p.y >= 0 && p.y < cells.size() && p.x < cells[0].size();
}

#include "libtcod/src/libtcod.hpp"

Cell make(char ch) {
  Cell c;
  c.c = ch;

  if (islower(ch))
    return c;

  switch (ch) {
  case 'A':
    c.inpos = {Point{-1, 0}, Point{1, 0}};
    c.outpos = {Point{0, 1}};
    break;
  case 'C':
    break;
  }

  return c;
}

void bang_around(int x, int y) {
  for (int cy = -1; cy <= 1; cy++) {
    for (int cx = -1; cx <= 1; cx++) {
      if ((cx < 0 && (cy < 0 || cy > 0) || (cx > 0 && (cy < 0 || cy > 0))))
        continue;
      if (cx + x == x && cy + y == y) {
        cells[cy + y][cx + x].c = '*';
        cells[cy + y][cx + x].skip = true;
      } else if (is_valid({cx + x, cy + y}))
        cells[cy + y][cx + x].banged = true;
    }
  }
}

int main() {
  TCOD_key_t key = {TCODK_NONE, 0};
  TCOD_mouse_t mouse;

  memcpy(upper.data(), " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", upper.size());
  memcpy(lower.data(), " 0123456789abcdefghijklmnopqrstuvwxyz", lower.size());

  TCODConsole::setCustomFont(
      "/home/higor/code/musigrid/libtcod/data/fonts/terminal10x16_gs_tc.png",
      TCOD_FONT_LAYOUT_TCOD);

  TCODConsole::initRoot(80, 25, "libtcod C++ sample");
  SDL_SetWindowResizable(TCOD_sys_get_sdl_window(), SDL_FALSE);
  atexit(TCOD_quit);

  unsigned frames = 0;
  unsigned ticks = 0;
  unsigned bpm = 120;
  while (!TCODConsole::isWindowClosed()) {
    TCOD_event_t ev;
    while ((ev = TCODSystem::checkForEvent(TCOD_EVENT_ANY, &key, &mouse)) !=
           TCOD_EVENT_NONE) {
      switch (ev) {
      case TCOD_EVENT_KEY_PRESS: {
        Point p = cursor;
        p.x += (key.vk == TCODK_RIGHT) - (key.vk == TCODK_LEFT);
        p.y += (key.vk == TCODK_DOWN) - (key.vk == TCODK_UP);

        if (is_valid(p))
          cursor = p;

        cursor_char += (key.vk == TCODK_PAGEUP) - (key.vk == TCODK_PAGEDOWN);
        if (cursor_char < 0)
          cursor_char = lower.size() - 1;
        cursor_char %= lower.size();

        break;
      }
      case TCOD_EVENT_KEY_RELEASE: {
        auto cursor_cell = &cells[cursor.y][cursor.x];
        if (key.vk == TCODK_ENTER)
          *cursor_cell = make(lower[cursor_char]);

        if (key.vk == TCODK_INSERT && isalpha(lower[cursor_char]))
          *cursor_cell = make(upper[cursor_char]);

        if (key.vk == TCODK_DELETE)
          *cursor_cell = make(' ');

        break;
      }
      }
    }

    bool is_tick = frames % ((60 * 60) / bpm) == 0;

    if (is_tick) {
      ticks++;
    }

    // clear flags
    for (int y = 0; y < GRID_H; ++y) {
      for (int x = 0; x < GRID_W; ++x) {
        auto cell = &cells[y][x];
        cell->banged = false;
        cell->skip = false;
      }
    }

    for (int y = 0; y < GRID_H; ++y) {
      for (int x = 0; x < GRID_W; ++x) {
        auto cell = &cells[y][x];

        if (!is_tick && !cell->banged)
          continue;

        char effective_c = cell->banged ? toupper(cell->c) : cell->c;

        if (effective_c == ' ' || islower(effective_c) ||
            isdigit(effective_c) || cell->skip)
          continue;

        if (cell->c == '*') {
          cell->c = ' ';
          continue;
        }

        std::vector<Cell *> inputs, outputs;

        for (auto p : cell->inpos) {
          inputs.push_back(is_valid({x + p.x, y + p.y})
                               ? &cells[y + p.y][x + p.x]
                               : nullptr);
        }

        for (auto p : cell->outpos) {
          outputs.push_back(is_valid({x + p.x, y + p.y})
                                ? &cells[y + p.y][x + p.x]
                                : nullptr);
        }

#define DEF_MOVE_OP(X, Y)                                                      \
  do {                                                                         \
    if (is_valid({x + X, y + Y}) && cells[y + Y][x + X].c == ' ') {            \
      cells[y + Y][x + X] = *cell;                                             \
      cells[y + Y][x + X].skip = true;                                         \
      cell->c = ' ';                                                           \
    } else                                                                     \
      bang_around(x, y);                                                       \
  } while (0)
#define DEF_ARITH_BINOP(OP)                                                    \
  do {                                                                         \
    bool ucase = false;                                                        \
    int a = 0;                                                                 \
    int b = 0;                                                                 \
    if (inputs[0] && inputs[0]->c != ' ')                                      \
      a = inputs[0]->to_value();                                               \
    if (inputs[1] && inputs[1]->c != ' ') {                                    \
      b = inputs[1]->to_value();                                               \
      ucase = ucase || isupper(inputs[1]->c);                                  \
    }                                                                          \
    int res = (a OP b) % 36;                                                   \
    if (outputs[0])                                                            \
      outputs[0]->c = ucase ? upper[res + 1] : lower[res + 1];                 \
  } while (0)

        switch (effective_c) {
        case 'E':
          DEF_MOVE_OP(1, 0);
          break;
        case 'W':
          DEF_MOVE_OP(-1, 0);
          break;
        case 'N':
          DEF_MOVE_OP(0, -1);
          break;
        case 'S':
          DEF_MOVE_OP(0, 1);
          break;

        case 'A':
          DEF_ARITH_BINOP(+);
          break;
        case 'B':
          DEF_ARITH_BINOP(-);
          break;
        case 'M':
          DEF_ARITH_BINOP(*);
          break;
        case 'C': {
          bool ucase = false;
          int rate = 0;
          int mod = 10;

          if (inputs[0]) {
            rate = inputs[0]->to_value();
            ucase = ucase || isupper(inputs[0]->c);
          }

          if (inputs[1]) {
            mod = inputs[1]->to_value();
            ucase = ucase || isupper(inputs[1]->c);
          }

          int res = 0;

          if (outputs[0]) {
            res = outputs[0]->to_value();
            if (ticks % rate == 0) {
              res = (res + 1) % mod;
            }

            res = res % 36;
            outputs[0]->c = ucase ? upper[res] : lower[res];
          }
          break;
        }


        }
      }
    }

    auto root = TCODConsole::root;
    root->clear();

    // draw grid
    for (int y = 0; y < GRID_H; ++y) {
      for (int x = 0; x < GRID_W; ++x) {
        if (x % 2 == 0 && y % 2 == 0)
          root->putCharEx(x, y, '.', TCODColor::darkestGrey, TCODColor::black);
      }
    }

    // render chars
    for (int y = 0; y < GRID_H; ++y) {
      for (int x = 0; x < GRID_W; ++x) {
        auto cell = &cells[y][x];
        char ch = cell->c;

        if (cursor.x == x && cursor.y == y && cell->c == ' ')
          ch = lower[cursor_char];

        if (ch == ' ')
          continue;

        root->setChar(x, y, ch);

        if (isupper(ch) && (cell->inpos.size() || cell->outpos.size())) {
          root->setCharBackground(x, y, TCODColor::cyan);
          root->setCharForeground(x, y, TCODColor::black);
        } else {
          root->setCharBackground(x, y, root->getDefaultBackground());
          root->setCharForeground(x, y, TCODColor::grey);
        }
      }
    }

    auto cursor_cell = &cells[cursor.y][cursor.x];

    root->setCharBackground(cursor.x, cursor.y, TCODColor::yellow);
    root->setCharForeground(cursor.x, cursor.y, TCODColor::black);

    if (cursor_cell->c == ' ')
      root->setChar(cursor.x, cursor.y,
                    cursor_char == 0 ? '@' : lower[cursor_char]);

    root->printf(0, 21, "%02i,%02i%c", cursor.x, cursor.y, is_tick ? '*': ' ');

    TCODConsole::flush();

    frames++;
  }

  TCOD_quit();
  return 0;
}
