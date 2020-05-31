#include "machine.hpp"

#include <SDL.h>
#include <assert.h>
#include <condition_variable>
#include <cstdlib>
#include <ctype.h>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <vector>

#include "terminal.hpp"

#define GRID_W (640 / 8)
#define GRID_H ((480 / 16) - 2)

struct Point {
  int x, y;
  Point() = default;
};

#define ALPHABET "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ*#:%!?;=$."
static Point cursor = Point{GRID_W / 2, GRID_H / 2};
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

SDL_AudioDeviceID audio_dev;

Machine machine;
Terminal term;

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

static void init() {
  term.configure(GRID_W, GRID_H + 2);
  term.set_font("unscii16");
  term.fg = 1;
  term.bg = 0;

  machine.init(GRID_W, GRID_H);
}

static void draw(unsigned bpm, bool is_beat) {
  // draw grid
  for (int y = 0; y < GRID_H; ++y) {
    for (int x = 0; x < GRID_W; ++x) {
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
        char c = insert_menu.ucase ? toupper(insert_menu.items[y][x])
                                   : tolower(insert_menu.items[y][x]);

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
  term.print(0, machine.grid_h() + 1, " %10s   %2s %2s %8u%c", "", "", "", bpm,
             is_beat ? '*' : ' ');
}

int main(int, char *[]) {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer);

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, 640, 480);

  SDL_AudioSpec spec;
  spec.channels = 2;
  spec.freq = 44100;
  spec.format = AUDIO_S16;
  spec.samples = 256;
  spec.callback = audio_callback;

  audio_dev = SDL_OpenAudioDevice(NULL, false, &spec, NULL, 0);
  SDL_PauseAudioDevice(audio_dev, 0);

  init();

  bool running = true;
  bool first_frame = true;

  float secs_then = SDL_GetTicks() / 1000.0f;
  float secs_acc = 0;

  unsigned frames = 0;

  unsigned speed = 2;

  while (running) {
    float secs_now = SDL_GetTicks() / 1000.0f;
    float delta = secs_now - secs_then;
    secs_then = secs_now;
    secs_acc += delta;

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_KEYDOWN: {
        auto key = ev.key.keysym;
        if (insert_menu.is_open == false) {
          Point p = cursor;
          p.x += (key.scancode == SDL_SCANCODE_RIGHT) -
                 (key.scancode == SDL_SCANCODE_LEFT);
          p.y += (key.scancode == SDL_SCANCODE_DOWN) -
                 (key.scancode == SDL_SCANCODE_UP);

          if (machine.is_valid(p.x, p.y))
            cursor = p;

          cursor_char += (key.scancode == SDL_SCANCODE_PAGEUP) -
                         (key.scancode == SDL_SCANCODE_PAGEDOWN);
          cursor_char %= sizeof(ALPHABET);
        } else {
          insert_menu.move_cursor((key.scancode == SDL_SCANCODE_RIGHT) -
                                      (key.scancode == SDL_SCANCODE_LEFT),
                                  (key.scancode == SDL_SCANCODE_DOWN) -
                                      (key.scancode == SDL_SCANCODE_UP));
        }
        break;
      }
      case SDL_KEYUP: {
        auto key = ev.key.keysym;
        if (insert_menu.is_open == false) {
          switch (key.scancode) {
          case SDL_SCANCODE_INSERT: {
            auto cur_char = machine.get_cell(cursor.x, cursor.y)->c;
            insert_menu.open(cursor.x, cursor.y,
                             cur_char == '.' ? Cell::Glyph('O') : cur_char);
            break;
          }
          case SDL_SCANCODE_DELETE:
            machine.new_cell(cursor.x, cursor.y, '.');
            break;
          default:
            break;
          }
        } else {
          switch (key.scancode) {
          case SDL_SCANCODE_DELETE:
            insert_menu.cancel();
            break;
          case SDL_SCANCODE_RETURN:
          case SDL_SCANCODE_INSERT:
            machine.new_cell(cursor.x, cursor.y, insert_menu.accept());
            break;
          case SDL_SCANCODE_PAGEDOWN:
          case SDL_SCANCODE_PAGEUP:
            insert_menu.toggle_case();
            break;
          default:
            break;
          }
        }
        break;
      }
      case SDL_QUIT:
        goto break_main_loop;
      }
    }

    if (!first_frame) {
      term.clear();
    }
    first_frame = false;

    unsigned bpm = speed * 60;

    bool is_tick = frames % (speed * 4) == 0;
    bool is_beat = secs_acc > (60 / (float)bpm);

    if (is_beat)
      secs_acc -= (60 / (float)bpm);

    if (is_tick) {
      std::unique_lock<std::mutex> lk(audio.mut);
      machine.tick();

      lk.unlock();
      audio.read_cv.notify_one();
    }

    draw(bpm, is_beat);

    uint8_t *pixels = nullptr;
    int pitch;

    SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);
    term.draw_buffer(pixels, pitch);
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_Delay(1000 / 30);
  }
break_main_loop:
  running = false;

  {
    std::unique_lock<std::mutex> lk(audio.mut);
    audio.quit = true;
    lk.unlock();
    audio.read_cv.notify_one();
    SDL_PauseAudioDevice(audio_dev, true);
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
