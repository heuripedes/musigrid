#include "../core/machine.hpp"
#include "../core/terminal.hpp"

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_hints.h>
#include <assert.h>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctype.h>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <thread>
#include <vector>


#define GRID_W (640 / 8)
#define GRID_H ((480 / 16) - 2)

struct Point {
  int x, y;
  Point() = default;
};

template <typename T> struct CircularBuffer {
  using storage_type = std::vector<T>;
  storage_type storage;
  size_t used;
  size_t ip;
  size_t op;

  CircularBuffer() = default;
  CircularBuffer(const CircularBuffer &) = delete;

  bool is_full() const { return used == storage.size(); }
  bool is_empty() const { return used == 0; }

  size_t unused() const { return storage.size() - used; }

  void notify_copy(size_t amount) {
    assert(used + amount <= storage.size());
    ip = (ip + amount) % storage.size();
    used += amount;
  }

  std::tuple<T *, size_t> first_unused_contiguous() {
    size_t available = unused();
    if (available == 0)
      return {nullptr, 0};

    if (ip + available > storage.size())
      available = storage.size() - ip;

    return {storage.data() + ip, available};
  }

  size_t write(const T *data, size_t size) {
    size_t written = 0;

    assert(!is_full());
    for (written = 0; !is_full() && written < size; ++written) {
      storage[ip++] = data[written];
      ip %= storage.size();
      used++;
    }

    assert(used <= storage.size());
    return written;
  }

  size_t write_unsafe(const T *data, size_t size) {
    size_t written = 0;

    for (written = 0; written < size; ++written) {
      storage[ip++] = data[written];
      ip %= storage.size();
      used++;
    }

    used = std::min(used + written, storage.size());
    return written;
  }

  size_t write(T data) {
    write(&data, 1);
    return 1;
  }

  size_t write_unsafe(T data) {
    write_unsafe(&data, 1);
    return 1;
  }

  size_t read(T *data, size_t size) {
    size_t bytes_read = 0;

    assert(!is_empty());
    for (bytes_read = 0; !is_empty() && bytes_read < size; ++bytes_read) {
      data[bytes_read] = storage[op++];
      op %= storage.size();
      used--;
    }

    assert((ssize_t)used >= 0);
    return bytes_read;
  }
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
  std::condition_variable read_cv, write_cv;
  CircularBuffer<uint8_t> buffer;
  int notes = 0;
  bool quit = false;
} audio;

static void audio_callback(void *, uint8_t *data, int len) {
  size_t size = len;

  std::unique_lock<std::mutex> lk(audio.mut);
  audio.read_cv.wait(lk,
                     [&]() { return audio.quit || audio.buffer.used >= size; });

  if (audio.quit)
    goto zero_buffer;

  if (!audio.buffer.is_empty()) {
    size_t read = audio.buffer.read(data, size);
    size -= read;
    data += read;
  }

  if (size)
    fprintf(stderr, "buffer underrun: %zu unused bytes\n", size);

zero_buffer:
  lk.unlock();
  audio.write_cv.notify_one();
  std::fill(data, data + size, 0);
}

static void audio_write(uint8_t *data, size_t size) {

  while (size) {
    std::unique_lock<std::mutex> lk(audio.mut);
    audio.write_cv.wait(lk, [&]() { return !audio.buffer.is_full(); });

    size_t written = audio.buffer.write(data, size);
    data += written;
    size -= written;

    audio.read_cv.notify_one();
  }
}

static void init() {
  term.configure(GRID_W, GRID_H + 2);
  term.set_font("unscii16");
  term.fg = 1;
  term.bg = 0;

  machine.init(GRID_W, GRID_H);
}

static void draw() {
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
  term.print(0, machine.grid_h() + 1, " %10s   %2s %2s %8u%c", "", "", "", machine.bpm,
             machine.ticks % 4 == 0 ? '*' : ' ');
}

int main(int, char *[]) {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_Window *window;
  SDL_Renderer *renderer;

  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
  SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer);

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                        SDL_TEXTUREACCESS_STREAMING, 640, 480);

  init();

  SDL_AudioSpec spec;
  spec.channels = 2;
  spec.freq = Machine::AUDIO_SAMPLE_RATE;
  spec.format = AUDIO_S16;
  spec.samples = machine.audio_samples.size() / 2;
  spec.callback = audio_callback;

  audio_dev = SDL_OpenAudioDevice(NULL, false, &spec, &spec, 0);
  SDL_PauseAudioDevice(audio_dev, 0);

  audio.buffer.storage.resize(spec.size);

  bool running = true;

  while (running) {
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

    term.clear();

    static int frames = 0;
    frames++;

    machine.run();

    audio_write((uint8_t *)machine.audio_samples.data(),
                machine.audio_samples.size() * sizeof(int16_t));

    draw();

    uint8_t *pixels = nullptr;
    int pitch;

    SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);
    term.draw_buffer(pixels, pitch);
    SDL_UnlockTexture(texture);

    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
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
