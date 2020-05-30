#include "machine.hpp"

#include <SDL.h>
#include <SDL_audio.h>
#include <assert.h>
#include <cctype>
#include <cfloat>
#include <condition_variable>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <vector>

#include "libtcod/src/libtcod.hpp"

#include "libtcod/src/libtcod/color.hpp"
#include "libtcod/src/libtcod/console_printing.h"
#include "libtcod/src/libtcod/console_types.h"

#define GRID_W 48
#define GRID_H 15

struct Point {
  int x{0}, y{0};
};

#define ALPHABET "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ*#:%!?;=$."
static Point cursor = {GRID_W / 2, GRID_H / 2};
static char cursor_char = sizeof(ALPHABET) - 1;
static struct {
  bool is_open = false;
  bool ucase = false;
  Point pos;
  Point cursor;

  const std::array<std::array<char, 10>, 5> items{
      {{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'},
       {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'},
       {'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T'},
       {'U', 'V', 'W', 'X', 'Y', 'Z', '*', '#', ':', '%'},
       {'!', '?', ';', '=', '$', '.', '.', '.', '.', '.'}}};

  int items_cols() const { return items[0].size(); }
  int items_rows() const { return items.size(); }

  void open(int menu_x, int menu_y, char current_char) {
    is_open = true;
    cursor.x = cursor.y = 0;

    if (menu_x + items_cols() > GRID_W)
      menu_x = GRID_W - items_cols();

    if (menu_y + items_rows() + 1 > GRID_H)
      menu_y = GRID_H - items_rows() - 1;

    pos.x = menu_x;
    pos.y = menu_y;

    for (int y = 0; y < items_rows(); y++) {
      for (int x = 0; x < items_cols(); x++) {
        if (items[y][x] == current_char ||
            items[y][x] == toupper(current_char)) {
          cursor.x = x;
          cursor.y = y;
        }
      }
    }
  }

  char accept() {
    is_open = false;
    auto c = items[cursor.y][cursor.x];
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

} insert_menu;

void print(TCODConsole *cons, int x, int y, const char *str, TCODColor fg,
           TCODColor bg) {
  int tx = x;
  int ty = y;
  while (*str) {
    if (*str == '\n') {
      tx = x;
      ty++;
    }
    cons->putCharEx(tx++, ty, *str++, fg, bg);
  }
}

SDL_AudioDeviceID audio_dev;

Machine machine;

struct {
  std::mutex mut;
  std::condition_variable read_cv;
  int notes = 0;
  bool quit = false;
} audio;

static void audio_callback(void *userdata, Uint8 *stream, int len) {
  std::unique_lock<std::mutex> lk(audio.mut);

  audio.read_cv.wait(lk, [&]() {
    return audio.quit || tsf_active_voice_count(machine.sf) > 0;
  });

  if (audio.quit)
    memset(stream, 0, len);
  else {
    int sample_count = (len / (2 * sizeof(int16_t))); // 2 output channels
    tsf_render_short(machine.sf, (int16_t *)stream, sample_count, 0);
  }
}

int main(int, char *[]) {
  printf("sizeof(Cell) = %zu\n", sizeof(Cell));

  TCOD_key_t key = {TCODK_NONE, 0};
  TCOD_mouse_t mouse;

  TCODConsole::setCustomFont(
      "/home/higor/code/musigrid/libtcod/data/fonts/terminal10x16_gs_tc.png",
      TCOD_FONT_LAYOUT_TCOD);

  machine.init(GRID_W, GRID_H);

  TCODConsole::initRoot(machine.grid_w(), machine.grid_h() + 2,
                        "libtcod C++ sample");
  SDL_SetWindowResizable(TCOD_sys_get_sdl_window(), SDL_FALSE);
  {
    int ww;
    int wh;
    SDL_GetWindowSize(TCOD_sys_get_sdl_window(), &ww, &wh);
    printf("window size is %ix%i\n", ww, wh);
  }
  atexit(TCOD_quit);

  SDL_Init(SDL_INIT_AUDIO);
  SDL_AudioSpec spec;
  spec.channels = 2;
  spec.freq = 44100;
  spec.format = AUDIO_S16;
  spec.samples = 256;
  spec.callback = audio_callback;

  audio_dev = SDL_OpenAudioDevice(NULL, false, &spec, NULL, 0);
  SDL_PauseAudioDevice(audio_dev, 0);
  tsf_set_output(machine.sf, TSF_STEREO_INTERLEAVED, spec.freq, 0);

  TCODSystem::setFps(60);

  float secs_then = TCODSystem::getElapsedSeconds();
  float secs_acc = 0;

  unsigned frames = 0;
  unsigned ticks = 0;

  unsigned speed = 2;

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
        if (insert_menu.is_open == false) {
          Point p = cursor;
          p.x += (key.vk == TCODK_RIGHT) - (key.vk == TCODK_LEFT);
          p.y += (key.vk == TCODK_DOWN) - (key.vk == TCODK_UP);

          if (machine.is_valid(p.x, p.y))
            cursor = p;

          cursor_char += (key.vk == TCODK_PAGEUP) - (key.vk == TCODK_PAGEDOWN);
          cursor_char %= sizeof(ALPHABET);
        } else {
          insert_menu.move_cursor(
              (key.vk == TCODK_RIGHT) - (key.vk == TCODK_LEFT),
              (key.vk == TCODK_DOWN) - (key.vk == TCODK_UP));
        }
        break;
      }
      case TCOD_EVENT_KEY_RELEASE: {
        if (insert_menu.is_open == false) {
          switch (key.vk) {
          case TCODK_INSERT: {
            auto cur_char = machine.get_cell(cursor.x, cursor.y)->c;
            insert_menu.open(cursor.x, cursor.y,
                             cur_char == '.' ? Cell::Glyph('O') : cur_char);
            break;
          }
          case TCODK_DELETE:
            machine.new_cell(cursor.x, cursor.y, '.');
            break;
          default:
            break;
          }
        } else {
          switch (key.vk) {
          case TCODK_DELETE:
            insert_menu.cancel();
            break;
          case TCODK_ENTER:
          case TCODK_INSERT:
            machine.new_cell(cursor.x, cursor.y, insert_menu.accept());
            break;
          case TCODK_PAGEDOWN:
          case TCODK_PAGEUP:
            insert_menu.toggle_case();
            break;
          default:
            break;
          }
        }
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

      std::unique_lock<std::mutex> lk(audio.mut);
      machine.tick();
      audio.read_cv.notify_one();
    }

    auto root = TCODConsole::root;
    root->clear();

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
    auto cursor_cell = machine.get_cell(cursor.x, cursor.y);

    for (int y = 0; y < machine.grid_h(); ++y) {
      for (int x = 0; x < machine.grid_w(); ++x) {
        auto cell = machine.get_cell(x, y);
        char ch = cell->c;

        if (cursor_cell->c == cell->c && cell->c != '.') {
          root->putCharEx(x, y, ch, TCODColor::orange, TCODColor::black);
          continue;
        }

        if (cell->flags & CF_WAS_TICKED)
          root->putCharEx(x, y, ch, TCODColor::black, TCODColor::cyan);
        else if (cell->flags & CF_WAS_READ)
          root->putCharEx(x, y, ch,
                          (cell->flags & CF_IS_LOCKED) ? TCODColor::white
                                                       : TCODColor::cyan,
                          root->getDefaultBackground());
        else if (cell->flags & CF_WAS_WRITTEN)
          root->putCharEx(x, y, ch, TCODColor::black, TCODColor::white);
        else if (ch != '.' || cell->flags & CF_IS_LOCKED)
          root->putCharEx(x, y, ch,
                          (cell->flags & CF_WAS_READ) ? TCODColor::white
                                                      : TCODColor::grey,
                          root->getDefaultBackground());
      }
    }

    if (!insert_menu.is_open) {
      root->putCharEx(cursor.x, cursor.y,
                      cursor_cell->c == '.' ? '@' : (char)cursor_cell->c,
                      TCODColor::black, TCODColor::yellow);
    } else {
      int draw_x = insert_menu.pos.x;
      int draw_y = insert_menu.pos.y;

      print(root, draw_x, draw_y++, "INS -", TCODColor::yellow,
            TCODColor::blue);

      for (int y = 0; y < insert_menu.items_rows(); ++y) {
        for (int x = 0; x < insert_menu.items_cols(); ++x) {
          char c = insert_menu.ucase ? toupper(insert_menu.items[y][x])
                                     : tolower(insert_menu.items[y][x]);

          root->putChar(draw_x + x, draw_y + y, c);
          if (x == insert_menu.cursor.x && y == insert_menu.cursor.y) {
            root->setCharBackground(draw_x + x, draw_y + y, TCODColor::yellow);
            root->setCharForeground(draw_x + x, draw_y + y, TCODColor::black);
          } else
            root->setCharBackground(draw_x + x, draw_y + y, TCODColor::grey);
        }
      }
    }

    root->printf(0, machine.grid_h() + 0, " %10s   %02i,%02i %8uf",
                 machine.cell_descs[cursor.y][cursor.x], cursor.x, cursor.y,
                 ticks);
    root->printf(0, machine.grid_h() + 1, " %10s   %2s %2s %8u%c", "", "", "",
                 bpm, is_beat ? '*' : ' ');

    TCODConsole::flush();

    frames++;
  }

  {
    std::unique_lock<std::mutex> lk(audio.mut);
    audio.quit = true;
    lk.unlock();
    audio.read_cv.notify_one();
    SDL_PauseAudioDevice(audio_dev, true);
  }

  TCOD_quit();
  return 0;
}
