#pragma once

#include <assert.h>
#include <vector>

static inline char int_to_b36(int v, bool upper) {
  if (upper)
    return "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[v % 36];

  return "0123456789abcdefghijklmnopqrstuvwxyz"[v % 36];
}

static inline int b36_to_int(char ch) {
  if (ch >= '0' && ch <= '9')
    return ch - '0';

  if (ch >= 'a' && ch <= 'z')
    return 10 + ch - 'a';

  if (ch >= 'A' && ch <= 'Z')
    return 10 + ch - 'A';

  return -1;
}

static inline bool is_operator_ch(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '*' ||
         ch == '#' || ch == ':' || ch == '%' || ch == '!' || ch == '?' ||
         ch == ';' || ch == '=' || ch == '$';
}

enum CellFlags {
  CF_WAS_TICKED = 1 << 0, // we have already ticked this cell
  CF_WAS_BANGED = 1 << 1, // something banged this cell
  CF_WAS_READ = 1 << 2,
  CF_WAS_WRITTEN = 1 << 3,
  CF_IS_LOCKED = 1 << 4,
};

struct Cell {
  char c = '.';
  unsigned char flags;
  int to_int() const { return b36_to_int(c); }
  void from_int(int v, bool upper) { c = int_to_b36(v, upper); }
};

struct Machine {
  std::vector<std::vector<Cell>> cells;
  char variables[36];

  unsigned ticks = 0;

  void init(int width, int height);
  void reset();
  void tick();

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
      cell->flags |= CF_WAS_READ;
      cell->flags |= CF_IS_LOCKED;
      return cell->c;
    }

    return '.';
  }

  void write_locked(int x, int y, char c, const char *desc) {
    auto cell = get_cell(x, y);

    if (cell) {
      cell->flags |= CF_WAS_WRITTEN;
      cell->flags |= CF_IS_LOCKED;
      cell->c = c;
    }
  }

  char read_cell(int x, int y, const char *desc) {
    auto cell = get_cell(x, y);
    if (cell) {
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

  void write_cell(int x, int y, char c) {
    auto cell = get_cell(x, y);
    if (cell) {
      cell->c = c;
      cell->flags |= CF_WAS_WRITTEN;
    }
  }
};