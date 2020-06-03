#include "util.hpp"
#include "terminal.hpp"

#include <ctype.h>

void PopupList::draw(Terminal &term) {
  int max_w = title.length();

  for (auto &item : items)
    max_w = std::max(max_w, (int)item.length());

  std::string separator(max_w, '-');

  term.bg = 7;
  term.fg = 0;

  term.putc('+', x, y, 0, 7);
  term.print(x + 1, y, "[%-*s]", max_w, title.c_str());
  term.putc('+', x + max_w + 3, y, 0, 7);

  for (int i = 0; i < items.size(); ++i) {
    term.putc('|', x, y + i + 1, 0, 7);

    if (i == selected) {
      term.bg = 6;
      term.fg = 0;
    } else
      term.reset_color();

    term.print(x + 1, y + i + 1, " %-*s ", max_w, items[i].c_str());

    term.putc('|', x + max_w + 3, y + i + 1, 0, 7);
  }

  term.bg = 7;
  term.fg = 0;
  term.print(x, y + items.size() + 1, "+%.*s+", max_w + 2,
             "---------------------------------");
  term.reset_color();
}
