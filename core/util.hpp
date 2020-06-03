#pragma once
#include <initializer_list>
#include <string>
#include <vector>

#ifdef __GNUC__
#ifndef UNLIKELY
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#endif
#ifndef LIKELY
#define LIKELY(expr) __builtin_expect(!!(expr), 1)
#endif
#else
#ifndef UNLIKELY
#define UNLIKELY
#endif
#ifndef LIKELY
#define LIKELY
#endif
#endif

struct Terminal;
struct Input;

struct PopupList {
  int x = 0;
  int y = 0;
  int selected = 0;
  bool is_open = false;
  std::string title;
  std::vector<std::string> items;

  PopupList() {}
  PopupList(std::string tit, std::initializer_list<std::string> lst)
      : title(tit), items(lst) {}
  PopupList(const PopupList &) = delete;

  void open(int ox, int oy) {
    x = ox;
    y = oy;
    is_open = true;
  }

  void cancel() { is_open = false; };

  int accept_int() {
    is_open = false;
    return selected;
  }
  std::string accept_str() {
    is_open = false;
    return items[selected];
  }

  void move_cursor(int dy) {
    selected += dy;
    if (selected < 0)
      selected = items.size() - 1;
    else if (selected >= items.size())
      selected = 0;
  }

  void draw(Terminal &term);
};
