#pragma once

#include <assert.h>
#include <cctype>
#include <map>
#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <vector>

static inline bool is_b36(char ch) {
  return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') ||
         (ch >= 'A' && ch <= 'Z');
}

static inline char int_to_b36(int v, bool upper) {
  v %= 36;
  assert(v >= 0 && v <= 35);
  if (upper)
    return "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[v % 36];

  return "0123456789abcdefghijklmnopqrstuvwxyz"[v % 36];
}

static inline int b36_to_int_raw(char ch) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';

  if (ch >= 'a' && ch <= 'z')
    return 10 + ch - 'a';

  if (ch >= 'A' && ch <= 'Z')
    return 10 + ch - 'A';

  return -1;
}

static inline int b36_to_int(char ch, int fallback) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';

  if (ch >= 'a' && ch <= 'z')
    return 10 + ch - 'a';

  if (ch >= 'A' && ch <= 'Z')
    return 10 + ch - 'A';

  return fallback;
}

static int note_octave0_to_key(char note, int octave0) {
  // clang-format off
  // Reference:
  //    https://www.barryrudolph.com/greg/midi.html
  //    https://newt.phys.unsw.edu.au/jw/notes.html
  //
  //                                    A,  B, C, D, E, F, G
  static const int midi_notes[] =  { 9, 11, 0, 2, 4, 5, 7 };
  // clang-format on

  bool sharp = islower(note);

  note = toupper(note);

  if (!isalpha(note))
    return -1;

  // Orca's README says:
  // "The midi operator interprets any letter above the chromatic scale as a
  // transpose value, for instance 3H, is equivalent to 4A."
  // also H -> A, I -> B
  while (note > 'I') {
    note -= 'I';
    octave0++;
  }

  // map H -> A and I -> B
  if (note > 'G')
    note -= 'G';

  // MIDI's first note is C-2, but Orca can't handle n < 0 so
  // we skip the firs two octaves and start at C0.
  int octave = octave0 + 2;

  int out = midi_notes[note - 'A'];
  out += octave * 12;
  out += sharp;

  return out;
}

enum CellFlags {
  CF_WAS_TICKED = 1 << 0, // we have already ticked this cell
  CF_WAS_BANGED = 1 << 1, // something banged this cell
  CF_WAS_READ = 1 << 2,
  CF_WAS_WRITTEN = 1 << 3,
  CF_IS_LOCKED = 1 << 4,
};

static inline bool is_operator_ch(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '*' ||
         ch == '#' || ch == ':' || ch == '%' || ch == '!' || ch == '?' ||
         ch == ';' || ch == '=' || ch == '$';
}

struct Cell {
  struct Glyph {
    char ch;

    Glyph() : ch('.') {}

    Glyph(char c) : ch('.') { operator=(c); }

    Glyph(int c) : ch('.') { operator=(c); }

    Glyph as_upper() const { return Glyph(toupper(ch)); }

    bool valid_c(char c) const {
      return c == '.' || (c >= '0' && c <= '9') || is_operator_ch(c);
    }

    operator char() const { return ch; }

    bool operator==(int c) const { return ch == c; }

    bool operator==(char c) const { return ch == c; }

    bool operator>(char c) const { return ch > c; }

    Glyph &operator=(char c) {
      assert(valid_c(c));
      if (valid_c(c))
        ch = c;
      return *this;
    }
  };

  Glyph c = '.';
  unsigned char flags;

  int to_int(int fallback) const {
    return is_b36(c) ? b36_to_int_raw(c) : fallback;
  }
  void from_int(int v, bool upper) { c = int_to_b36(v, upper); }
};

struct Note {
  int channel;
  int key;
  float velocity;
  int length;
};

struct tsf;
struct Machine {
  static const int AUDIO_SAMPLE_RATE = 44100;
  static const int FRAMES_PER_SECOND = 60;
  static const std::map<char, const char *> OPERATOR_NAMES;

  std::array<int16_t, AUDIO_SAMPLE_RATE / FRAMES_PER_SECOND * 2> audio_samples;
  size_t audio_sample_count = 0;

  std::vector<std::vector<Cell>> cells;
  std::vector<std::vector<const char *>> cell_descs;
  std::vector<Note> notes;

  tsf *sf = nullptr;

  std::map<Cell::Glyph, Cell::Glyph> variables;

  int bpm = 120;

  unsigned frames = 0;
  unsigned ticks = 0;

  Machine() { sf = nullptr; }

  ~Machine() {
    // if (sf)
    // tsf_close(sf);
  }

  bool load_string(const std::string &data);
  std::string to_string() const;

  void init(int width, int height);
  void reset();
  void run();

  int grid_w() const { return cells[0].size(); }
  int grid_h() const { return cells.size(); }

  Cell new_cell(int x, int y, char ch) {
    Cell c;
    c.c = ch;

    cells.at(y).at(x) = c;
    return c;
  }

  bool is_valid(int x, int y) const {
    return x >= 0 && y >= 0 && y < grid_h() && x < grid_w();
  }

  /* "private" */
  void tick();
  void tick_cell(char effective_c, int x, int y, Cell *cell);

  void move_operation(int x, int y, int X, int Y) {
    auto cell = &cells[y][x];
    if (is_valid(x + X, y + Y) && cells[y + Y][x + X].c == '.') {
      cells[y + Y][x + X] = *cell;
      cells[y + Y][x + X].flags |= CF_WAS_TICKED;
      cell->c = '.';
      cell->flags &= ~CF_WAS_TICKED;
    } else {
      cell->c = '*';
    }
  }

  Cell *get_cell(int x, int y) { return is_valid(x, y) ? &cells[y][x] : NULL; }

  char read_locked(int x, int y, const char *desc) {
    auto cell = get_cell(x, y);

    if (cell) {
      cell_descs[y][x] = desc;
      cell->flags |= CF_WAS_READ;
      cell->flags |= CF_IS_LOCKED;
      return cell->c;
    }

    return '.';
  }

  void write_locked(int x, int y, Cell::Glyph c, const char *desc) {
    auto cell = get_cell(x, y);

    if (cell) {
      cell_descs[y][x] = desc;
      cell->flags |= CF_WAS_WRITTEN;
      cell->flags |= CF_IS_LOCKED;
      cell->c = c;
    }
  }

  char read_cell(int x, int y, const char *desc) {
    auto cell = get_cell(x, y);
    if (cell) {
      cell_descs[y][x] = desc;
      cell->flags |= CF_WAS_READ;

      return cell->c;
    }

    return '.';
  }

  char peek_cell(int x, int y) {
    auto cell = get_cell(x, y);

    if (cell)
      return cell->c;

    return '.';
  }

  void lock_cell(int x, int y) {
    auto cell = get_cell(x, y);
    if (cell)
      cell->flags |= CF_IS_LOCKED;
  }

  void write_cell(int x, int y, char c, const char *desc) {
    auto cell = get_cell(x, y);
    if (cell) {
      cell_descs[y][x] = desc;
      cell->c = c;
      cell->flags |= CF_WAS_WRITTEN;
    }
  }
};
