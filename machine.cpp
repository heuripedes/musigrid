#include "machine.hpp"
#include <ctype.h>
#include <stdlib.h>

void Machine::init(int width, int height) {
  cells.resize(height);
  for (auto &row : cells)
    row.resize(width);
}

void Machine::reset() {
  int w = cells[0].size();
  int h = cells.size();

  cells.clear();
  init(w, h);
}

void Machine::tick() {
  for (int y = 0; y < grid_h(); ++y) {
    for (int x = 0; x < grid_w(); ++x) {
      auto cell = &cells[y][x];
      cell->flags = 0;
    }
  }

  for (int y = 0; y < grid_h(); ++y) {
    for (int x = 0; x < grid_w(); ++x) {
      auto cell = &cells[y][x];

      if (cell->flags & CF_WAS_TICKED)
        continue;

      cell->flags |= CF_WAS_TICKED;

      char effective_c =
          (cell->flags & CF_WAS_BANGED) ? toupper(cell->c) : cell->c;

      if (effective_c == '.' || islower(effective_c) || isdigit(effective_c) ||
          cell->flags & CF_IS_LOCKED)
        continue;

#define DEF_ARITH_BINOP(NAME, OPERATION)                                       \
  do {                                                                         \
    char ca = read_cell(x - 1, y, #NAME "-a");                                 \
    char cb = read_locked(x + 1, y, #NAME "-b");                               \
    int a = ca == '.' ? 0 : b36_to_int(ca);                                    \
    int b = cb == '.' ? 0 : b36_to_int(cb);                                    \
    write_locked(x, y + 1, int_to_b36(a OPERATION b, isupper(cb)),             \
                 #NAME "-output");                                             \
  } while (0)

      switch (effective_c) {
      case 'A':
        DEF_ARITH_BINOP(A, +);
        break;
      case 'B':
        DEF_ARITH_BINOP(B, -);
        break;
      case 'C': {
        char ratec = read_cell(x - 1, y, "C-rate");
        char modc = read_locked(x + 1, y, "C-mod");

        int rate = ratec == '.' ? 1 : b36_to_int(ratec);
        int mod = modc == '.' ? 10 : b36_to_int(modc);

        if (rate < 1)
          rate = 1;

        if (mod == 0)
          write_locked(x, y + 1, '0', "C-output");
        else {
          char resc = peek_cell(x, y + 1);
          int res = resc == '.' ? 0 : b36_to_int(resc);

          if (ticks % rate == 0)
            res = (res + 1) % mod;

          write_locked(x, y + 1, int_to_b36(res, isupper(modc)), "C-output");
        }
        break;
      }
      case 'D': {
        char ratec = read_cell(x - 1, y, "D-rate");
        char modc = read_locked(x + 1, y, "D-mod");

        int rate = ratec == '.' ? 1 : b36_to_int(ratec);
        int mod = modc == '.' ? 8 : b36_to_int(modc);

        if (rate < 1)
          rate = 1;

        if (mod == 0)
          write_locked(x, y + 1, '.', "D-output");
        else if (mod == 1)
          write_locked(x, y + 1, '*', "D-output");
        else {
          char res = (ticks % rate == 0 && ticks % mod == 0) ? '*' : '.';
          write_locked(x, y + 1, res, "D-output");
        }
        break;
      }
      case 'E':
        move_operation(x, y, 1, 0);
        break;
      case 'F': {
        char ca = read_cell(x - 1, y, "F-a");
        char cb = read_locked(x + 1, y, "F-b");
        write_locked(x, y + 1, ca == cb ? '*' : '.', "F-output");
        break;
      }
      case 'G': {
        char cx = read_cell(x - 3, y, "G-x");
        char cy = read_cell(x - 2, y, "G-y");
        char clen = read_cell(x - 1, y, "G-len");

        int x_ = cx == '.' ? 0 : b36_to_int(cx);
        int y_ = cy == '.' ? 1 : b36_to_int(cy);
        int len = clen == '.' ? 1 : b36_to_int(clen);

        if (len < 1)
          len = 1;

        static const char *in_descs[] = {
            "G-in0", "G-in1", "G-in2", "G-in3", "G-in4", "G-in5",
            "G-in6", "G-in7", "G-in8", "G-in9", "G-inA", "G-inB",
            "G-inC", "G-inD", "G-inE", "G-inF", "G-inG", "G-inH",
            "G-inI", "G-inJ", "G-inK", "G-inL", "G-inM", "G-inN",
            "G-inO", "G-inP", "G-inQ", "G-inR", "G-inS", "G-inT",
            "G-inU", "G-inV", "G-inW", "G-inX", "G-inY", "G-inZ",
        };

        static const char *out_descs[] = {
            "G-out0", "G-out1", "G-out2", "G-out3", "G-out4", "G-out5",
            "G-out6", "G-out7", "G-out8", "G-out9", "G-outA", "G-outB",
            "G-outC", "G-outD", "G-outE", "G-outF", "G-outG", "G-outH",
            "G-outI", "G-outJ", "G-outK", "G-outL", "G-outM", "G-outN",
            "G-outO", "G-outP", "G-outQ", "G-outR", "G-outS", "G-outT",
            "G-outU", "G-outV", "G-outW", "G-outX", "G-outY", "G-outZ",
        };

        for (int i = 0; i < len; ++i) {
          write_locked(x + x_ + 1 + i, y + y_,
                       read_locked(x + 1 + i, y, in_descs[i]), out_descs[i]);
        }

        break;
      }
      case 'H': {
        lock_cell(x, y + 1);
        break;
      }
      case 'I': {
        char stepc = read_cell(x - 1, y, "I-step");
        char modc = read_locked(x + 1, y, "I-mod");

        int step = stepc == '.' ? 1 : b36_to_int(stepc);
        int mod = modc == '.' ? 10 : b36_to_int(modc);

        if (mod < 1)
          mod = 10;

        char resc = peek_cell(x, y + 1);
        int res = resc == '.' ? 0 : b36_to_int(resc);

        res = (res + step) % mod;

        write_locked(x, y + 1, int_to_b36(res, isupper(modc)), "I-output");
        break;
      }
      case 'J': {
        write_locked(x, y + 1, read_cell(x, y - 1, "J-input"), "J-output");
        break;
      }
      case 'K': {
        char clen = read_cell(x - 1, y, "K-len");
        int len = clen == '.' ? 1 : b36_to_int(clen);

        if (len < 1)
          len = 1;

        static const char *in_descs[] = {
            "K-read0", "K-read1", "K-read2", "K-read3", "K-read4", "K-read5",
            "K-read6", "K-read7", "K-read8", "K-read9", "K-readA", "K-readB",
            "K-readC", "K-readD", "K-readE", "K-readF", "K-readG", "K-readH",
            "K-readI", "K-readJ", "K-readK", "K-readL", "K-readM", "K-readN",
            "K-readO", "K-readP", "K-readQ", "K-readR", "K-readS", "K-readT",
            "K-readU", "K-readV", "K-readW", "K-readX", "K-readY", "K-readZ",
        };

        static const char *out_descs[] = {
            "K-out0", "K-out1", "K-out2", "K-out3", "K-out4", "K-out5",
            "K-out6", "K-out7", "K-out8", "K-out9", "K-outA", "K-outB",
            "K-outC", "K-outD", "K-outE", "K-outF", "K-outG", "K-outH",
            "K-outI", "K-outJ", "K-outK", "K-outL", "K-outM", "K-outN",
            "K-outO", "K-outP", "K-outQ", "K-outR", "K-outS", "K-outT",
            "K-outU", "K-outV", "K-outW", "K-outX", "K-outY", "K-outZ",
        };

        for (int i = 0; i < len; ++i) {
          char varc = read_locked(x + 1 + i, y, in_descs[i]);
          int var = b36_to_int(varc);

          write_locked(x + 1 + i, y + 1, varc >= 0 ? variables[var] : '.',
                       out_descs[i]);
        }
        break;
      }
      case 'L': {
        char ca = read_cell(x - 1, y, "L-a");
        char cb = read_locked(x + 1, y, "L-b");

        char res = tolower(ca) < tolower(cb) ? ca : cb;

        write_locked(x, y + 1, isupper(cb) ? toupper(res) : tolower(res),
                     "L-output");
        break;
      }

      case 'M':
        DEF_ARITH_BINOP(M, *);
        break;
      case 'N':
        move_operation(x, y, 0, -1);
        break;

      case 'O': {
        char cx = read_cell(x - 2, y, "O-x");
        char cy = read_cell(x - 1, y, "O-y");

        int x_ = cx == '.' ? 0 : b36_to_int(cx);
        int y_ = cy == '.' ? 0 : b36_to_int(cy);

        write_locked(x, y + 1, read_locked(x + 1 + x_, y + y_, "O-read"),
                     "O-output");
        break;
      }
      case 'P': {
        char ckey = read_cell(x - 2, y, "P-key");
        char clen = read_cell(x - 1, y, "P-len");
        char cread = read_locked(x + 1, y, "P-read");

        int key = ckey == '.' ? 0 : b36_to_int(ckey);
        int len = clen == '.' ? 0 : b36_to_int(clen);

        if (len < 1)
          len = 1;

        key %= len;

        for (int i = 0; i < len; ++i)
          lock_cell(x + i, y + 1);

        write_locked(x + key, y + 1, cread, "P-output");
        break;
      }
      case 'Q': {
        char cx = read_cell(x - 3, y, "Q-x");
        char cy = read_cell(x - 2, y, "Q-y");
        char clen = read_cell(x - 1, y, "Q-len");

        int x_ = cx == '.' ? 0 : b36_to_int(cx);
        int y_ = cy == '.' ? 1 : b36_to_int(cy);
        int len = clen == '.' ? 1 : b36_to_int(clen);

        if (len < 1)
          len = 1;

        static const char *in_descs[] = {
            "Q-in0", "Q-in1", "Q-in2", "Q-in3", "Q-in4", "Q-in5",
            "Q-in6", "Q-in7", "Q-in8", "Q-in9", "Q-inA", "Q-inB",
            "Q-inC", "Q-inD", "Q-inE", "Q-inF", "Q-inG", "Q-inH",
            "Q-inI", "Q-inJ", "Q-inK", "Q-inL", "Q-inM", "Q-inN",
            "Q-inO", "Q-inP", "Q-inQ", "Q-inR", "Q-inS", "Q-inT",
            "Q-inU", "Q-inV", "Q-inW", "Q-inX", "Q-inY", "Q-inZ",
        };

        static const char *out_descs[] = {
            "Q-out0", "Q-out1", "Q-out2", "Q-out3", "Q-out4", "Q-out5",
            "Q-out6", "Q-out7", "Q-out8", "Q-out9", "Q-outA", "Q-outB",
            "Q-outC", "Q-outD", "Q-outE", "Q-outF", "Q-outG", "Q-outH",
            "Q-outI", "Q-outJ", "Q-outK", "Q-outL", "Q-outM", "Q-outN",
            "Q-outO", "Q-outP", "Q-outQ", "Q-outR", "Q-outS", "Q-outT",
            "Q-outU", "Q-outV", "Q-outW", "Q-outX", "Q-outY", "Q-outZ",
        };

        for (int i = 0; i < len; ++i) {
          write_locked(x + x_ + 1 + i - len, y + y_,
                       read_locked(x + 1 + i, y, in_descs[i]), out_descs[i]);
        }
        break;
      }
      case 'R': {
        char cmin = read_cell(x - 3, y, "R-min");
        char cmax = read_locked(x - 2, y, "R-max");

        int min_ = cmin == '.' ? 0 : b36_to_int(cmin);
        int max_ = cmax == '.' ? 1 : b36_to_int(cmax);

        int res = min_ + (random() / (float)RAND_MAX) * (max_ - min_);
        write_locked(x, y + 1, int_to_b36(res, isupper(cmax)), "R-output");
        break;
      }
      case 'S':
        move_operation(x, y, 0, 1);
        break;

      case 'T': {
        char ckey = read_cell(x - 2, y, "T-key");
        char clen = read_cell(x - 1, y, "T-len");

        int key = ckey == '.' ? 0 : b36_to_int(ckey);
        int len = clen == '.' ? 0 : b36_to_int(clen);

        if (len < 1)
          len = 1;

        key %= len;

        char cval = read_locked(x + 1 + key, y, "T-val");

        for (int i = 0; i < len; ++i)
          lock_cell(x + 1 + i, y);

        write_locked(x, y + 1, cval, "T-output");
        break;
      }
      case 'U': /* who the fuck knows? */
        break;
      case 'V': {
        char cwrite = read_cell(x - 1, y, "V-write");
        char cread = read_locked(x + 1, y, "V-read");

        if (cwrite != '.')
          variables[b36_to_int(cwrite)] = cread;
        else if (cread != '.')
          write_locked(x, y + 1, variables[b36_to_int(cread)], "V-output");
        break;
      }
      case 'W':
        move_operation(x, y, -1, 0);
        break;
      case 'X': {
        char cx = read_cell(x - 2, y, "X-x");
        char cy = read_cell(x - 1, y, "X-y");

        int x_ = cx == '.' ? 0 : b36_to_int(cx);
        int y_ = cy == '.' ? 0 : b36_to_int(cy);

        write_locked(x + x_, y + y_ + 1, read_locked(x + 1, y, "X-read"),
                     "X-output");
        break;
      }
      case 'Y': {
        write_locked(x + 1, y, read_cell(x - 1, y, "Y-input"), "Y-output");
        break;
      }
      case 'Z': {
        char ratec = read_cell(x - 1, y, "Z-rate");
        char targetc = read_locked(x + 1, y, "Z-target");

        int rate = ratec == '.' ? 1 : b36_to_int(ratec);
        int target = targetc == '.' ? 0 : b36_to_int(targetc);

        char resc = peek_cell(x, y + 1);
        int res = resc == '.' ? 0 : b36_to_int(resc);

        if (res > target) {
          res = res - rate;
          if (res < target)
            res = target;
        } else if (res < target) {
          res = res + rate;
          if (res > target)
            res = target;
        }
        write_locked(x, y + 1, int_to_b36(res, isupper(target)), "Z-output");
        break;
      }
      case '*':
        cell->c = '.';
        for (int cy = -1; cy <= 1; cy++) {
          for (int cx = -1; cx <= 1; cx++) {
            if ((cx < 0 && (cy < 0 || cy > 0) ||
                 (cx > 0 && (cy < 0 || cy > 0))))
              continue;
            if (cx + x != x && cy + y != y && is_valid(cx + x, cy + y))
              cells[cy + y][cx + x].flags |= CF_WAS_BANGED;
          }
        }
        break;
      case '#':
        for (int i = x; i < grid_w(); ++i) {
          lock_cell(i, y);
          if (i > x && cells[y][i].c == '#')
            break;
        }
        break;
      }
    }
  }

  ticks++;
}
