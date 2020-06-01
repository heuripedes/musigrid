#include "machine.hpp"

#include <algorithm>
#include <stdint.h>
#include <string>

#include <ctype.h>
#include <stdlib.h>

#define TSF_IMPLEMENTATION
#include "tsf.h"

const std::map<char, const char *> Machine::OPERATOR_NAMES = {
    {'A', "add"},       {'B', "subtract"}, {'C', "clock"},     {'D', "delay"},
    {'E', "east"},      {'F', "if"},       {'G', "generator"}, {'H', "halt"},
    {'I', "increment"}, {'J', "jumper"},   {'K', "konkat"},    {'L', "less"},
    {'M', "multiply"},  {'N', "north"},    {'O', "read"},      {'P', "push"},
    {'Q', "query"},     {'R', "random"},   {'S', "south"},     {'T', "track"},
    {'U', "uclid"},     {'V', "variable"}, {'W', "west"},      {'X', "write"},
    {'Y', "jymper"},    {'Z', "lerp"},     {'*', "bang"},      {'#', "comment"},
    {':', "midi"},      {'%', "mono"},     {'!', "cc"},        {'?', "pb"},
    {';', "udp"},       {'=', "osc"},      {'$', "self"},
};

// This is a minimal SoundFont with a single loopin saw-wave
// sample/instrument/preset (484 bytes)
const static unsigned char MinimalSoundFont[] = {
#define TEN0 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    'R', 'I',  'F',  'F',  220,  1,    0,    0,    's',  'f',  'b',  'k',  'L',
    'I', 'S',  'T',  88,   1,    0,    0,    'p',  'd',  't',  'a',  'p',  'h',
    'd', 'r',  76,   TEN0, TEN0, TEN0, TEN0, 0,    0,    0,    0,    TEN0, 0,
    0,   0,    0,    0,    0,    0,    255,  0,    255,  0,    1,    TEN0, 0,
    0,   0,    'p',  'b',  'a',  'g',  8,    0,    0,    0,    0,    0,    0,
    0,   1,    0,    0,    0,    'p',  'm',  'o',  'd',  10,   TEN0, 0,    0,
    0,   'p',  'g',  'e',  'n',  8,    0,    0,    0,    41,   0,    0,    0,
    0,   0,    0,    0,    'i',  'n',  's',  't',  44,   TEN0, TEN0, 0,    0,
    0,   0,    0,    0,    0,    0,    TEN0, 0,    0,    0,    0,    0,    0,
    0,   1,    0,    'i',  'b',  'a',  'g',  8,    0,    0,    0,    0,    0,
    0,   0,    2,    0,    0,    0,    'i',  'm',  'o',  'd',  10,   TEN0, 0,
    0,   0,    'i',  'g',  'e',  'n',  12,   0,    0,    0,    54,   0,    1,
    0,   53,   0,    0,    0,    0,    0,    0,    0,    's',  'h',  'd',  'r',
    92,  TEN0, TEN0, 0,    0,    0,    0,    0,    0,    0,    50,   0,    0,
    0,   0,    0,    0,    0,    49,   0,    0,    0,    34,   86,   0,    0,
    60,  0,    0,    0,    1,    TEN0, TEN0, TEN0, TEN0, 0,    0,    0,    0,
    0,   0,    0,    'L',  'I',  'S',  'T',  112,  0,    0,    0,    's',  'd',
    't', 'a',  's',  'm',  'p',  'l',  100,  0,    0,    0,    86,   0,    119,
    3,   31,   7,    147,  10,   43,   14,   169,  17,   58,   21,   189,  24,
    73,  28,   204,  31,   73,   35,   249,  38,   46,   42,   71,   46,   250,
    48,  150,  53,   242,  55,   126,  60,   151,  63,   108,  66,   126,  72,
    207, 70,   86,   83,   100,  72,   74,   100,  163,  39,   241,  163,  59,
    175, 59,   179,  9,    179,  134,  187,  6,    186,  2,    194,  5,    194,
    15,  200,  6,    202,  96,   206,  159,  209,  35,   213,  213,  216,  45,
    220, 221,  223,  76,   227,  221,  230,  91,   234,  242,  237,  105,  241,
    8,   245,  118,  248,  32,   252};

bool Machine::load_string(const std::string &data) {
  size_t first_eol = data.find('\n');
  if (first_eol == std::string::npos && data.empty())
    return false;

  std::vector<Cell> row;
  for (unsigned i = 0; i < data.size(); ++i) {
    if (data[i] == '\n') {
      cells.push_back(std::move(row));
      row = std::vector<Cell>();
    } else {
      Cell cell;
      cell.c = data[i];
      row.push_back(cell);
    }
  }

  if (row.size())
    cells.push_back(std::move(row));

  init(grid_w(), grid_h());

  return true;
}

std::string Machine::to_string() const {
  std::string out;
  for (int y = 0; y < grid_h(); ++y) {
    for (int x = 0; x < grid_w(); ++x) {
      out.push_back(cells[y][x].c);
    }
    out.push_back('\n');
  }

  return out;
}

void Machine::init(int width, int height) {
  if (!sf)
    // sf = tsf_load_filename("/usr/share/soundfonts/FluidR3_GM.sf2");
    sf = tsf_load_memory(MinimalSoundFont, sizeof(MinimalSoundFont));

  assert(sf);

  for (int i = 0; i < 7; ++i) {
    // tsf_channel_set_presetnumber(sf, i, 0, 0);
    tsf_channel_set_bank(sf, i, 0);
  }

  tsf_set_output(sf, TSF_STEREO_INTERLEAVED, AUDIO_SAMPLE_RATE, 0);

  cells.resize(height);
  cell_descs.resize(height);
  for (auto &row : cells)
    row.resize(width);
  for (auto &row : cell_descs)
    row.resize(width);
#if 0
  const char *data[] = {"..........................................",
                        ".#.MIDI.example.#.........................",
                        "..........................................",
                        "...wC4....................................",
                        ".gD204TCAFE..################.............",
                        "...:02C.g....#..............#.............",
                        ".............#..Channel..1..#.............",
                        "...8C4.......#..Octave.234..#.............",
                        ".4D234TCAFE..#..Notes.CAFE..#.............",
                        "...:13E.4....#..............#.............",
                        ".............################.............",
                        "...4C4....................................",
                        ".1D424TCAFE...............................",
                        "...%24F.2.................................",
                        "..........................................",
                        "..........................................",
                        "..........................................",
                        "..........................................",
                        NULL};
  for (int y = 0; data[y] && y < grid_h(); ++y) {
    for (int x = 0; data[y][x] && x < grid_w(); ++x) {
      cells[y][x].c = data[y][x];
    }
  }
#endif
}

void Machine::reset() {
  int w = cells[0].size();
  int h = cells.size();

  cells.clear();
  init(w, h);

  frames = 0;
  ticks = 0;
}

void Machine::run() {
  if (frames % (bpm / 15) == 0)
    tick();

  tsf_render_short(sf, audio_samples.data(), audio_samples.size() / 2);
  frames++;
}

void Machine::tick() {
  // clear flags
  for (auto &row : cells)
    for (auto &cell : row)
      cell.flags = 0;

  // silence notes
  // tsf_note_off_all(sf);

  notes.erase(std::remove_if(notes.begin(), notes.end(),
                             [&](Note &note) {
                               auto val = --note.length < 1;
                               if (val)
                                 tsf_channel_note_off(sf, note.channel,
                                                      note.key);
                               return val;
                             }),
              notes.end());

  for (auto &row : cell_descs)
    for (auto &cell_desc : row)
      cell_desc = "empty";

  for (int y = 0; y < grid_h(); ++y) {
    for (int x = 0; x < grid_w(); ++x) {
      auto cell = &cells[y][x];

      if (cell->flags & CF_WAS_TICKED)
        continue;

      Cell::Glyph effective_c =
          (cell->flags & CF_WAS_BANGED) ? cell->c.as_upper() : cell->c;

      if (effective_c == '.' || islower(effective_c) || isdigit(effective_c) ||
          (cell->flags & CF_IS_LOCKED && effective_c != '*'))
        continue;

      cell_descs[y][x] = OPERATOR_NAMES.at(effective_c);

      tick_cell(effective_c, x, y, cell);
    }
  }

  ticks++;
}

void Machine::tick_cell(char effective_c, int x, int y, Cell *cell) {
  if (cell->flags & CF_WAS_TICKED)
    return;

  cell->flags |= CF_WAS_TICKED;

  switch (effective_c) {
  case 'A': {
    char ca = read_cell(x - 1, y, "A-a");
    char cb = read_locked(x + 1, y, "A-b");
    int a = b36_to_int(ca, 0);
    int b = b36_to_int(cb, 0);
    write_locked(x, y + 1, int_to_b36(a + b, isupper(cb)), "A-output");
    break;
  }
  case 'B': {
    char ca = read_cell(x - 1, y, "B-a");
    char cb = read_locked(x + 1, y, "B-b");
    int a = b36_to_int(ca, 0);
    int b = b36_to_int(cb, 0);

    if (b > a)
      write_locked(x, y + 1, int_to_b36(b - a, isupper(cb)), "B-output");
    else
      write_locked(x, y + 1, int_to_b36(a - b, isupper(cb)), "B-output");
    break;
  }
  case 'C': {
    char ratec = read_cell(x - 1, y, "C-rate");
    char modc = read_locked(x + 1, y, "C-mod");

    int rate = b36_to_int(ratec, 1);
    int mod = b36_to_int(modc, 10);

    if (rate < 1)
      rate = 1;

    if (mod < 2)
      write_locked(x, y + 1, '0', "C-output");
    else {
      char resc = peek_cell(x, y + 1);
      int res = b36_to_int(resc, 0);

      if (ticks % rate == 0)
        res = (res + 1) % mod;

      write_locked(x, y + 1, int_to_b36(res, isupper(modc)), "C-output");
    }
    break;
  }
  case 'D': {
    char ratec = read_cell(x - 1, y, "D-rate");
    char modc = read_locked(x + 1, y, "D-mod");

    int rate = b36_to_int(ratec, 1);
    int mod = b36_to_int(modc, 8);

    if (rate < 1)
      rate = 1;

    bool bang = mod != 0 && (mod == 1 || (ticks % (rate * mod) == 0));
    write_locked(x, y + 1, (bang ? '*' : '.'), "D-output");
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

    int x_ = b36_to_int(cx, 0);
    int y_ = b36_to_int(cy, 0);
    int len = b36_to_int(clen, 1);

    if (len < 1)
      len = 1;

    static const char *in_descs[] = {
        "G-in0", "G-in1", "G-in2", "G-in3", "G-in4", "G-in5", "G-in6", "G-in7",
        "G-in8", "G-in9", "G-inA", "G-inB", "G-inC", "G-inD", "G-inE", "G-inF",
        "G-inG", "G-inH", "G-inI", "G-inJ", "G-inK", "G-inL", "G-inM", "G-inN",
        "G-inO", "G-inP", "G-inQ", "G-inR", "G-inS", "G-inT", "G-inU", "G-inV",
        "G-inW", "G-inX", "G-inY", "G-inZ",
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
      write_locked(x + x_ + i, y + y_ + 1,
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

    int step = b36_to_int(stepc, 1);
    int mod = b36_to_int(modc, 10);

    if (mod < 1)
      mod = 10;

    char resc = peek_cell(x, y + 1);
    int res = b36_to_int(resc, 0);

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
    int len = b36_to_int(clen, 1);

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
      if (varc == '.')
        write_locked(x + 1 + i, y + 1, '.', out_descs[i]);
      else
        write_locked(x + 1 + i, y + 1, variables[varc], out_descs[i]);
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
  case 'M': {
    char ca = read_cell(x - 1, y, "M-a");
    char cb = read_locked(x + 1, y, "M-b");
    int a = b36_to_int(ca, 0);
    int b = b36_to_int(cb, 0);
    write_locked(x, y + 1, int_to_b36(a * b, isupper(cb)), "M-output");
    break;
  }
  case 'N':
    move_operation(x, y, 0, -1);
    break;

  case 'O': {
    char cx = read_cell(x - 2, y, "O-x");
    char cy = read_cell(x - 1, y, "O-y");

    int x_ = b36_to_int(cx, 0);
    int y_ = b36_to_int(cy, 0);

    write_locked(x, y + 1, read_locked(x + 1 + x_, y + y_, "O-read"),
                 "O-output");
    break;
  }
  case 'P': {
    char ckey = read_cell(x - 2, y, "P-key");
    char clen = read_cell(x - 1, y, "P-len");
    char cread = read_locked(x + 1, y, "P-read");

    int key = b36_to_int(ckey, 0);
    int len = b36_to_int(clen, 0);

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

    int x_ = b36_to_int(cx, 0);
    int y_ = b36_to_int(cy, 0);
    int len = b36_to_int(clen, 1);

    if (len < 1)
      len = 1;

    static const char *in_descs[] = {
        "Q-in0", "Q-in1", "Q-in2", "Q-in3", "Q-in4", "Q-in5", "Q-in6", "Q-in7",
        "Q-in8", "Q-in9", "Q-inA", "Q-inB", "Q-inC", "Q-inD", "Q-inE", "Q-inF",
        "Q-inG", "Q-inH", "Q-inI", "Q-inJ", "Q-inK", "Q-inL", "Q-inM", "Q-inN",
        "Q-inO", "Q-inP", "Q-inQ", "Q-inR", "Q-inS", "Q-inT", "Q-inU", "Q-inV",
        "Q-inW", "Q-inX", "Q-inY", "Q-inZ",
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
      write_locked(x + i - len + 1, y + 1,
                   read_locked(x + x_ + i + 1, y + y_, in_descs[i]),
                   out_descs[i]);
    }
    break;
  }
  case 'R': {
    char cmin = read_cell(x - 1, y, "R-min");
    char cmax = read_locked(x + 1, y, "R-max");

    int min_ = b36_to_int(cmin, 0);
    int max_ = b36_to_int(cmax, 35);

    int res = min_ + (random() / (float)RAND_MAX) * (max_ - min_ + 1);
    write_locked(x, y + 1, int_to_b36(res, isupper(cmax)), "R-output");
    break;
  }
  case 'S':
    move_operation(x, y, 0, 1);
    break;

  case 'T': {
    char ckey = read_cell(x - 2, y, "T-key");
    char clen = read_cell(x - 1, y, "T-len");

    int key = b36_to_int(ckey, 0);
    int len = b36_to_int(clen, 0);

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
      variables[cwrite] = cread;
    else if (cread != '.')
      write_locked(x, y + 1, variables[cread], "V-output");
    break;
  }
  case 'W':
    move_operation(x, y, -1, 0);
    break;
  case 'X': {
    char cx = read_cell(x - 2, y, "X-x");
    char cy = read_cell(x - 1, y, "X-y");

    int x_ = b36_to_int(cx, 0);
    int y_ = b36_to_int(cy, 0);

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

    int rate = b36_to_int(ratec, 1);
    int target = b36_to_int(targetc, 0);

    char resc = peek_cell(x, y + 1);
    int res = b36_to_int(resc, 0);

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
  case '*': {
    if ((cell->flags & CF_IS_LOCKED) == 0)
      cell->c = '.';

    Cell *neigh;

    // XXX: Orca and Orca-c only bang the neighbors to the north and west
    if ((neigh = get_cell(x, y - 1)) != NULL) {
      neigh->flags |= CF_WAS_BANGED;
      tick_cell(neigh->c.as_upper(), x, y - 1, neigh);
    }

    if ((neigh = get_cell(x - 1, y)) != NULL) {
      neigh->flags |= CF_WAS_BANGED;
      tick_cell(neigh->c.as_upper(), x - 1, y, neigh);
    }

    if ((neigh = get_cell(x, y + 1)) != NULL) {
      neigh->flags |= CF_WAS_BANGED;
    }

    if ((neigh = get_cell(x + 1, y)) != NULL) {
      neigh->flags |= CF_WAS_BANGED;
    }
    break;
  }
  case '#':
    for (int i = x; i < grid_w(); ++i) {
      auto cell = get_cell(i, y);
      if (!cell)
        continue;
      cell->flags |= CF_WAS_TICKED;
      cell->flags |= CF_IS_LOCKED;
      if (i > x && cell->c == '#')
        break;
    }
    break;
  case ':': {
    char channelc = read_locked(x + 1, y, ":-channel");
    char octavec = read_locked(x + 2, y, ":-octave");
    char notec = read_locked(x + 3, y, ":-note");
    char velocityc = read_locked(x + 4, y, ":-velocity");
    char lengthc = read_locked(x + 5, y, ":-length");

    int channel = b36_to_int(channelc, 0);
    int octave = b36_to_int(octavec, 0);
    int velocity = b36_to_int(velocityc, 15);
    int length = b36_to_int(lengthc, 1);

    if (cell->flags & CF_WAS_BANGED) {
      Note n;
      n.key = note_octave0_to_key(notec, octave);
      n.channel = channel;
      n.velocity = std::min(velocity / 16.0f, 16.0f);
      n.length = length; // length % g
      notes.push_back(n);
      // printf(": %c + %i -> %i\n", notec, octave, n.key);
      tsf_channel_note_on(sf, n.channel, n.key, n.velocity);
    }
    break;
  }
  case '%': {
    char channelc = read_locked(x + 1, y, "%-channel");
    char octavec = read_locked(x + 2, y, "%-octave");
    char notec = read_locked(x + 3, y, "%-note");
    char velocityc = read_locked(x + 4, y, "%-velocity");
    char lengthc = read_locked(x + 5, y, "%-length");

    int channel = b36_to_int(channelc, 0);
    int octave = b36_to_int(octavec, 0);
    int velocity = b36_to_int(velocityc, 15);
    int length = b36_to_int(lengthc, 1);

    if (cell->flags & CF_WAS_BANGED) {
      Note n;
      n.key = note_octave0_to_key(notec, octave);
      n.channel = channel;
      n.velocity = std::min(velocity / 16.0f, 16.0f);
      n.length = length; // length % g

      notes.erase(std::remove_if(notes.begin(), notes.end(),
                                 [&](Note &note) {
                                   if (n.channel == note.channel)
                                     tsf_channel_note_off(sf, note.channel,
                                                          note.key);
                                   return n.channel == note.channel;
                                 }),
                  notes.end());

      notes.push_back(n);
      // printf("%% %c + %i -> %i\n", notec, octave, n.key);
      tsf_channel_note_on(sf, n.channel, n.key, n.velocity);
      tsf_channel_set_pan(sf, n.channel, ticks % 2 == 0);
    }
    break;
  }
  }
}
