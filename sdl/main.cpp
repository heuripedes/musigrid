#include "../core/machine.hpp"
#include "../core/terminal.hpp"
#include "../core/system.hpp"

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_events.h>
#include <SDL_hints.h>
#include <SDL_scancode.h>
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

SDL_AudioDeviceID audio_dev;


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

int main(int, char *[]) {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_Window *window;
  SDL_Renderer *renderer;

  SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");
  SDL_CreateWindowAndRenderer(640, 480, 0, &window, &renderer);

  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, 640, 480);

  // init();

  System system;
  system.set_size(640, 480);

  SDL_AudioSpec spec;
  spec.channels = 2;
  spec.freq = Machine::AUDIO_SAMPLE_RATE;
  spec.format = AUDIO_S16;
  spec.samples = system.machine.audio_samples.size() / 2;
  spec.callback = audio_callback;

  audio_dev = SDL_OpenAudioDevice(NULL, false, &spec, &spec, 0);
  SDL_PauseAudioDevice(audio_dev, 0);

  audio.buffer.storage.resize(spec.size);

  bool running = true;

  while (running) {
    SDL_Event ev;
    System::Input input;
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        auto key = ev.key.keysym;
        auto val = ev.type == SDL_KEYDOWN;
        // clang-format off
        switch (key.scancode) {
          case SDL_SCANCODE_LEFT: input.left = val; break;
          case SDL_SCANCODE_RIGHT: input.right = val; break;
          case SDL_SCANCODE_DOWN: input.down = val; break;
          case SDL_SCANCODE_UP: input.up = val; break;
          case SDL_SCANCODE_INSERT: input.ins = val; break;
          case SDL_SCANCODE_DELETE: input.del = val; break;
          case SDL_SCANCODE_PAGEUP: input.pgup = val; break;
          case SDL_SCANCODE_PAGEDOWN: input.pgdown = val; break;
          default: break;
        }
        // clang-format on
        break;
      }
      case SDL_QUIT:
        goto break_main_loop;
      }
    }

    system.handle_input(input);
    system.machine.run();

    audio_write((uint8_t*)system.machine.audio_samples.data(), system.machine.audio_samples.size() * sizeof(int16_t));

    system.draw();

    uint8_t *pixels = nullptr;
    int pitch;

    SDL_LockTexture(texture, NULL, (void **)&pixels, &pitch);
    system.term.draw_buffer(pixels, pitch);
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
