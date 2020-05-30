#include "../machine.hpp"
#include <gtest/gtest.h>
#include <memory>

Machine from_string(const std::string &data) {
  Machine m;
  m.load_string(data);
  return m;
}

struct OutputCompare {
  std::string input;
  std::string expected;
  std::string result;
  bool matched = false;
  Machine *m = nullptr;
  ~OutputCompare() {
    if (m)
      delete m;
  }

  void create() {
    m = new Machine();
    *m = from_string(input);
  }

  bool output_matches(bool tick = true) {
    if (!m)
      create();

    if (tick)
      m->tick();

    result = m->to_string();

    return matched = (expected == result);
  }
};

TEST(operator_a, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".A.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".A.\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_a, lhs) {
  OutputCompare c;
  c.input = "...\n"
            "1A.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1A.\n"
               ".1.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_a, operator_rhs) {
  OutputCompare c;
  c.input = "...\n"
            ".AC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".AC\n"
               ".C.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_a, rhs_lhs) {
  OutputCompare c;
  c.input = "...\n"
            "1AC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1AC\n"
               ".D.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_a, overflow) {
  OutputCompare c;
  c.input = "...\n"
            "1AZ\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1AZ\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_a, non_b36_rhs) {
  OutputCompare c;
  c.input = "...\n"
            "1A*\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1A*\n"
               ".1.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_b, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".B.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".B.\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_b, lhs) {
  OutputCompare c;
  c.input = "...\n"
            "1B.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1B.\n"
               ".1.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_b, operator_rhs) {
  OutputCompare c;
  c.input = "...\n"
            ".BC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".BC\n"
               ".C.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_b, rhs_lhs) {
  OutputCompare c;
  c.input = "...\n"
            "1BC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1BC\n"
               ".B.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_b, non_b36_rhs) {
  OutputCompare c;
  c.input = "...\n"
            "1B*\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1B*\n"
               ".1.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_c, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".C.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".C.\n"
               ".1.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_c, rate) {
  OutputCompare c;
  c.input = "...\n"
            "2C.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "2C.\n"
               ".1.\n"
               "...\n";
  c.create();
  c.m->tick();
  c.m->tick();
  EXPECT_TRUE(c.output_matches(false) && "after 2 ticks");

  c.expected = "...\n"
               "2C.\n"
               ".2.\n"
               "...\n";
  c.m->tick();
  c.m->tick();
  EXPECT_TRUE(c.output_matches(false) && "after 4 ticks");
}

TEST(operator_c, mod0) {
  OutputCompare c;
  c.input = "...\n"
            ".C0\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".C0\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_c, mod1) {
  OutputCompare c;
  c.input = "...\n"
            ".C1\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".C1\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_c, operator_mod) {
  OutputCompare c;
  c.input = "...\n"
            ".CC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".CC\n"
               ".B.\n"
               "...\n";
  c.create();

  for (int i = 0; i < 0xB; ++i)
    c.m->tick();

  EXPECT_TRUE(c.output_matches(false) && "should be B after 0xB iterations");

  c.expected = "...\n"
               ".CC\n"
               ".0.\n"
               "...\n";
  c.m->tick();
  EXPECT_TRUE(c.output_matches(false) && "should be 0 after 0xC iterations");
}

TEST(operator_d, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".D.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".D.\n"
               "...\n"
               "...\n";
  c.create();

  for (int i = 0; i < 8; ++i)
    c.m->tick();

  EXPECT_TRUE(c.output_matches(false) && "nothing after 7 ticks");

  c.expected = "...\n"
               ".D.\n"
               ".*.\n"
               "...\n";

  c.m->tick();
  EXPECT_TRUE(c.output_matches(false) && "bang after 8 ticks");
}

TEST(operator_d, mod0) {
  OutputCompare c;
  c.input = "...\n"
            ".D0\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".D0\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_d, mod1) {
  OutputCompare c;
  c.input = "...\n"
            ".D1\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".D1\n"
               ".*.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_e, move) {
  OutputCompare c;
  c.input = "...\n"
            ".E.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "..E\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_e, bang_at_edge) {
  OutputCompare c;
  c.input = "...\n"
            "..E\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "..*\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_e, bang_at_something) {
  OutputCompare c;
  c.input = "...\n"
            ".E1\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".*1\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_f, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".F.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".F.\n"
               ".*.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_f, one_arg) {
  OutputCompare c;
  c.input = "...\n"
            "1F.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1F.\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_f, two_diff_arg) {
  OutputCompare c;
  c.input = "...\n"
            "1F2\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1F2\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_f, two_equal_arg) {
  OutputCompare c;
  c.input = "...\n"
            "1F1\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1F1\n"
               ".*.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_g, empty) {
  OutputCompare c;
  c.input = "......\n"
            "x...G.\n"
            "......\n"
            "......";

  c.expected = "......\n"
               "x...G.\n"
               "......\n"
               "......\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_g, len1) {
  OutputCompare c;
  c.input = "......\n"
            "x..1Ga\n"
            "......\n"
            "......";

  c.expected = "......\n"
               "x..1Ga\n"
               "....a.\n"
               "......\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_g, len3) {
  OutputCompare c;
  c.input = "........\n"
            "x..3Ga.c\n"
            "........\n"
            "........";

  c.expected = "........\n"
               "x..3Ga.c\n"
               "....a.c.\n"
               "........\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_g, len1_offset) {
  OutputCompare c;
  c.input = "......\n"
            "x111Ga\n"
            "......\n"
            "......";

  c.expected = "......\n"
               "x111Ga\n"
               "......\n"
               ".....a\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_h, hold_bang) {
  OutputCompare c;
  c.input = ".*.\n"
            "*H*\n"
            ".*.\n"
            "...";

  c.expected = "...\n"
               ".H.\n"
               ".*.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_i, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".I.\n"
            "...\n"
            "...";

  c.expected = "...\n"
               ".I.\n"
               ".1.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_i, step2) {
  OutputCompare c;
  c.input = "...\n"
            "2I.\n"
            "...\n"
            "...";

  c.expected = "...\n"
               "2I.\n"
               ".2.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_i, mod0) {
  OutputCompare c;
  c.input = "...\n"
            ".I0\n"
            "...\n"
            "...";

  c.expected = "...\n"
               ".I0\n"
               ".1.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_i, mod1) {
  OutputCompare c;
  c.input = "...\n"
            ".I1\n"
            "...\n"
            "...";

  c.expected = "...\n"
               ".I1\n"
               ".0.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_i, wrap) {
  OutputCompare c;
  c.input = "...\n"
            ".I2\n"
            "...\n"
            "...";

  c.expected = "...\n"
               ".I2\n"
               ".1.\n"
               "...\n";

  c.create();
  c.m->tick();

  EXPECT_TRUE(c.output_matches(false) && "should be 1 after 1 tick");

  c.expected = "...\n"
               ".I2\n"
               ".0.\n"
               "...\n";

  c.m->tick();
  EXPECT_TRUE(c.output_matches(false) && "should be 0 after 2 ticks");
}

TEST(operator_j, some_value) {
  OutputCompare c;
  c.input = ".a.\n"
            ".J.\n"
            "...\n"
            "...";

  c.expected = ".a.\n"
               ".J.\n"
               ".a.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_k, one_var) {
  OutputCompare c;
  c.input = "....\n"
            ".Ka.\n"
            "....\n"
            "....";

  c.expected = "....\n"
               ".Ka.\n"
               "..Z.\n"
               "....\n";

  c.create();
  c.m->variables[b36_to_int('a')] = 'Z';

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_k, two_vars) {
  OutputCompare c;
  c.input = "....\n"
            "2Kax\n"
            "....\n"
            "....";

  c.expected = "....\n"
               "2Kax\n"
               "..ZC\n"
               "....\n";

  c.create();
  c.m->variables[b36_to_int('a')] = 'Z';
  c.m->variables[b36_to_int('x')] = 'C';

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_k, two_vars_len2) {
  OutputCompare c;
  c.input = "....\n"
            "2K.x\n"
            "....\n"
            "....";

  c.expected = "....\n"
               "2K.x\n"
               "...C\n"
               "....\n";

  c.create();
  c.m->variables[b36_to_int('a')] = 'Z';
  c.m->variables[b36_to_int('x')] = 'C';

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_l, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".L.\n"
            "...\n"
            "...";

  c.expected = "...\n"
               ".L.\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_l, non_b36_rhs) {
  OutputCompare c;
  c.input = "...\n"
            ".LC\n"
            "...\n"
            "...";

  c.expected = "...\n"
               ".LC\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_l, lhs_and_non_b36_rhs) {
  OutputCompare c;
  c.input = "...\n"
            "1LC\n"
            "...\n"
            "...";

  c.expected = "...\n"
               "1LC\n"
               ".1.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_l, inverted) {
  OutputCompare c;
  c.input = "...\n"
            "2L1\n"
            "...\n"
            "...";

  c.expected = "...\n"
               "2L1\n"
               ".1.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_m, empty) {
  OutputCompare c;
  c.input = "...\n"
            ".M.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".M.\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_m, lhs) {
  OutputCompare c;
  c.input = "...\n"
            "1M.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1M.\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_m, operator_rhs) {
  OutputCompare c;
  c.input = "...\n"
            ".MC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               ".MC\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_m, rhs_lhs) {
  OutputCompare c;
  c.input = "...\n"
            "1MC\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1MC\n"
               ".C.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_m, overflow) {
  OutputCompare c;
  c.input = "...\n"
            "2MZ\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "2MZ\n"
               ".Y.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_m, non_b36_rhs) {
  OutputCompare c;
  c.input = "...\n"
            "1M*\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "1M*\n"
               ".0.\n"
               "...\n";
  EXPECT_TRUE(c.output_matches());
}

TEST(operator_n, move) {
  OutputCompare c;
  c.input = "...\n"
            ".N.\n"
            "...\n"
            "...";
  c.expected = ".N.\n"
               "...\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_n, bang_at_edge) {
  OutputCompare c;
  c.input = ".N.\n"
            "...\n"
            "...\n"
            "...";
  c.expected = ".*.\n"
               "...\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_n, bang_at_something) {
  OutputCompare c;
  c.input = ".1.\n"
            ".N.\n"
            "...\n"
            "...";
  c.expected = ".1.\n"
               ".*.\n"
               "...\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_o, empty) {
  OutputCompare c;
  c.input = ".....\n"
            "..O..\n"
            ".....\n"
            ".....";
  c.expected = ".....\n"
               "..O..\n"
               ".....\n"
               ".....\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_o, read_xy) {
  OutputCompare c;
  c.input = ".....\n"
            "11Oab\n"
            "...cd\n"
            ".....";
  c.expected = ".....\n"
               "11Oab\n"
               "..dcd\n"
               ".....\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_p, empty) {
  OutputCompare c;
  c.input = ".....\n"
            "..P..\n"
            ".....\n"
            ".....";
  c.expected = ".....\n"
               "..P..\n"
               ".....\n"
               ".....\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_p, read_only_val) {
  OutputCompare c;
  c.input = ".....\n"
            "..Pab\n"
            "...cd\n"
            ".....";
  c.expected = ".....\n"
               "..Pab\n"
               "..acd\n"
               ".....\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_p, read_key1_len0) {
  OutputCompare c;
  c.input = ".....\n"
            "10Pab\n"
            "..acd\n"
            ".....";
  c.expected = ".....\n"
               "10Pab\n"
               "..acd\n"
               ".....\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_p, read_key1_len2) {
  OutputCompare c;
  c.input = ".....\n"
            "12Pab\n"
            "...cd\n"
            ".....";
  c.expected = ".....\n"
               "12Pab\n"
               "...ad\n"
               ".....\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_q, len4) {
  OutputCompare c;
  c.input = "..4Q1234\n"
            "........\n"
            "........\n"
            "........";
  c.expected = "..4Q1234\n"
               "1234....\n"
               "........\n"
               "........\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_q, len3_x1y1) {
  OutputCompare c;
  c.input = "113Q1234\n"
            "....5678\n"
            "........\n"
            "........";
  c.expected = "113Q1234\n"
               ".6785678\n"
               "........\n"
               "........\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_s, move) {
  OutputCompare c;
  c.input = "...\n"
            ".S.\n"
            "...\n"
            "...";
  c.expected = "...\n"
               "...\n"
               ".S.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_s, bang_at_edge) {
  OutputCompare c;
  c.input = "...\n"
            "...\n"
            "...\n"
            ".S.";
  c.expected = "...\n"
               "...\n"
               "...\n"
               ".*.\n";

  EXPECT_TRUE(c.output_matches());
}

TEST(operator_s, bang_at_something) {
  OutputCompare c;
  c.input = "...\n"
            ".S.\n"
            ".1.\n"
            "...";
  c.expected = "...\n"
               ".*.\n"
               ".1.\n"
               "...\n";

  EXPECT_TRUE(c.output_matches());
}
