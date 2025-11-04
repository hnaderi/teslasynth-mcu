#include "synthesizer/helpers/assertions.hpp"
#include <envelope.hpp>
#include <unity.h>

void test_level_sanity(void) {
  EnvelopeLevel level;
  TEST_ASSERT_TRUE(level.is_zero());
  level = EnvelopeLevel(0.5);
  TEST_ASSERT_FALSE(level.is_zero());

  TEST_ASSERT_TRUE(EnvelopeLevel::max() == EnvelopeLevel(1));
}
void test_level_comparison(void) {
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) > EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) < EnvelopeLevel(0.02));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.02) >= EnvelopeLevel(0.01));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.01) <= EnvelopeLevel(0.02));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.1) == EnvelopeLevel(0.1));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.2) != EnvelopeLevel(0.1));

  TEST_ASSERT_TRUE(EnvelopeLevel(-1) == EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(-1000) == EnvelopeLevel(0));
  TEST_ASSERT_TRUE(EnvelopeLevel(101) == EnvelopeLevel(1));
  TEST_ASSERT_TRUE(EnvelopeLevel(1e4f) == EnvelopeLevel(100));
  TEST_ASSERT_TRUE(EnvelopeLevel(1e4f) == EnvelopeLevel(1.f));
  TEST_ASSERT_TRUE(EnvelopeLevel(100) == EnvelopeLevel(1.f));

  TEST_ASSERT_TRUE(EnvelopeLevel(0.800) != EnvelopeLevel(0.810));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.800) == EnvelopeLevel(0.801));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.800) == EnvelopeLevel(0.8001));

  TEST_ASSERT_TRUE(EnvelopeLevel(0.63) != EnvelopeLevel(0.64));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.63) != EnvelopeLevel(0.632));
  TEST_ASSERT_TRUE(EnvelopeLevel(0.63) == EnvelopeLevel(0.631));
}
void test_level_arithmetic(void) {
  assert_level_equal(EnvelopeLevel(0) + EnvelopeLevel(1), EnvelopeLevel(1));
  assert_level_equal(EnvelopeLevel(0.1) + EnvelopeLevel(0.1),
                     EnvelopeLevel(0.2));
  assert_level_equal(EnvelopeLevel(1) + EnvelopeLevel(0.01), EnvelopeLevel(1));
  assert_level_equal((EnvelopeLevel(1) += EnvelopeLevel(0.01)),
                     EnvelopeLevel(1));
  assert_level_equal((EnvelopeLevel(1) += EnvelopeLevel(1)), EnvelopeLevel(1));
  TEST_ASSERT_EQUAL(EnvelopeLevel(1) - EnvelopeLevel(0.4f), 0.6f);

  assert_level_equal((EnvelopeLevel(1) += 0.5f), EnvelopeLevel(1));
  assert_level_equal((EnvelopeLevel(1) += -0.5f), EnvelopeLevel(0.5));
  assert_level_equal((EnvelopeLevel(1) += -2.f), EnvelopeLevel(0));
}
void test_level_to_duration(void) {
  assert_duration_equal(EnvelopeLevel(0.01) * 1_ms, 10_us);
  assert_duration_equal(EnvelopeLevel(0.1) * 1_ms, 100_us);
  assert_duration_equal(EnvelopeLevel(1) * 1_ms, 1_ms);
}

void test_curve_lin_positive(void) {
  Curve curve =
      Curve(EnvelopeLevel(0), EnvelopeLevel(1), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(1_ms), EnvelopeLevel(0.1));
  assert_level_equal(curve.update(1_ms), EnvelopeLevel(0.2));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.will_reach_target(8_ms), 0_ms);
  assert_level_equal(curve.update(8_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  assert_duration_equal(curve.will_reach_target(10_ms), 10_ms);
  assert_level_equal(curve.update(10_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_positive2(void) {
  Curve curve =
      Curve(EnvelopeLevel(0.35), EnvelopeLevel(1), 65_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.4));
  assert_level_equal(curve.update(10_ms), EnvelopeLevel(0.5));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(500_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  assert_level_equal(curve.update(10_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_positive3(void) {
  Curve curve =
      Curve(EnvelopeLevel(0), EnvelopeLevel(1), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.5));
  TEST_ASSERT_FALSE(curve.is_target_reached());
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(1));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.2), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.6));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.will_reach_target(10_ms), 5_ms);
  assert_level_equal(curve.update(5_ms), EnvelopeLevel(0.2));
  TEST_ASSERT_TRUE(curve.is_target_reached());

  assert_duration_equal(curve.will_reach_target(10_ms), 10_ms);
  assert_level_equal(curve.update(10_ms), EnvelopeLevel(0.2));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_lin_negative2(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.7), 100_ms, CurveType::Lin);
  assert_level_equal(curve.update(50_ms), EnvelopeLevel(0.85));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(500_ms), EnvelopeLevel(0.7));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_lin_negative_small(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.65), 10_ms, CurveType::Lin);
  assert_level_equal(curve.update(5_ns), EnvelopeLevel(1));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(5_ns), EnvelopeLevel(1));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(10_ms), EnvelopeLevel(0.65));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_exp_positive(void) {
  Curve curve = Curve(EnvelopeLevel(0), EnvelopeLevel(1), 5_ms, CurveType::Exp);
  assert_level_equal(curve.update(369_us), EnvelopeLevel(0.4));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_duration_equal(curve.will_reach_target(5_ms), 369_us);
  TEST_ASSERT_FALSE(curve.will_reach_target(1660_us));
  assert_level_equal(curve.update(1660_us), EnvelopeLevel(0.94));
  TEST_ASSERT_FALSE(curve.is_target_reached());
  TEST_ASSERT_FALSE(curve.will_reach_target(2_ms));

  assert_duration_equal(curve.will_reach_target(2971_us), 0_us);
  assert_duration_equal(curve.will_reach_target(12971_us), 10_ms);
  assert_duration_equal(curve.will_reach_target(10_ms), 7029_us);

  assert_level_equal(curve.update(10_ms), EnvelopeLevel(1));
  assert_duration_equal(curve.will_reach_target(10_ms), 10_ms);
  TEST_ASSERT_TRUE(curve.is_target_reached());
}
void test_curve_exp_negative(void) {
  Curve curve =
      Curve(EnvelopeLevel(1), EnvelopeLevel(0.4), 50_ms, CurveType::Exp);
  assert_level_equal(curve.update(1_ms), EnvelopeLevel(0.922));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(16993_us), EnvelopeLevel(0.45));
  TEST_ASSERT_FALSE(curve.is_target_reached());

  assert_level_equal(curve.update(34_ms), EnvelopeLevel(0.4));
  TEST_ASSERT_TRUE(curve.is_target_reached());
}

void test_curve_constant(void) {
  Curve curve = Curve(EnvelopeLevel(0.6));
  TEST_ASSERT_TRUE_MESSAGE(curve.is_target_reached(),
                           "Constant curve reaches target immediately!");
  assert_duration_equal(curve.will_reach_target(10_s), 10_s);
  assert_level_equal(curve.update(0_us), EnvelopeLevel(0.6));
}
void test_curve_constant_zero(void) {
  Curve curve = Curve(EnvelopeLevel(0));
  TEST_ASSERT_TRUE_MESSAGE(curve.is_target_reached(),
                           "Constant curve reaches target immediately!");
  assert_duration_equal(curve.will_reach_target(10_s), 10_s);
  assert_level_equal(curve.update(0_us), EnvelopeLevel(0));
}

const ADSR lin_adsr{10_ms, 20_ms, EnvelopeLevel(0.5), 30_ms, CurveType::Lin};
const ADSR exp_adsr{10_ms, 20_ms, EnvelopeLevel(0.5), 30_ms, CurveType::Exp};
const ADSR const_adsr{10_ms, 20_ms, EnvelopeLevel(0.5), 30_ms,
                      CurveType::Const};

void test_envelope_lin_full(void) {
  Envelope env(lin_adsr);
  TEST_ASSERT_EQUAL(Envelope::Stage::Attack, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(5_ms, true), EnvelopeLevel(0.5));
  assert_level_equal(env.update(1_ms, true), EnvelopeLevel(0.6));
  TEST_ASSERT_EQUAL(Envelope::Stage::Attack, env.stage());
  assert_level_equal(env.update(4_ms, true), EnvelopeLevel(1));
  TEST_ASSERT_EQUAL(Envelope::Stage::Decay, env.stage());
  assert_level_equal(env.update(20_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  assert_level_equal(env.update(2000_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  assert_level_equal(env.update(3_ms, false), EnvelopeLevel(0.45));
  TEST_ASSERT_EQUAL(Envelope::Stage::Release, env.stage());
  assert_level_equal(env.update(3_ms, false), EnvelopeLevel(0.40));
  TEST_ASSERT_EQUAL(Envelope::Stage::Release, env.stage());
  assert_level_equal(env.update(24_ms, false), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Off, env.stage());
}
void test_envelope_exp_full(void) {
  Envelope env(exp_adsr);
  TEST_ASSERT_EQUAL(Envelope::Stage::Attack, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(10_ms, true), EnvelopeLevel(1));
  TEST_ASSERT_EQUAL(Envelope::Stage::Decay, env.stage());
  assert_level_equal(env.update(20_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  assert_level_equal(env.update(300_ms, true), EnvelopeLevel(0.5));
  assert_level_equal(env.update(300_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  TEST_ASSERT_TRUE(env.update(1_us, false) < EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(Envelope::Stage::Release, env.stage());
  auto lvl = env.update(10_ms, false);
  TEST_ASSERT_TRUE(lvl < EnvelopeLevel(0.5));
  TEST_ASSERT_TRUE(lvl > EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Release, env.stage());
  assert_level_equal(env.update(20_ms, false), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Off, env.stage());
}
void test_envelope_const_full(void) {
  Envelope env(const_adsr);
  TEST_ASSERT_EQUAL(Envelope::Stage::Attack, env.stage());
  assert_level_equal(env.update(3_ms, true), EnvelopeLevel(0.5));
  assert_level_equal(env.update(3000_ms, true), EnvelopeLevel(0.5));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  assert_level_equal(env.update(100_ns, false), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Off, env.stage());
}

void test_envelope_const_zero(void) {
  Envelope env(EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Attack, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(3_ms, true), EnvelopeLevel(0));
  assert_level_equal(env.update(3000_ms, true), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  assert_level_equal(env.update(100_ns, false), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Off, env.stage());
}
void test_envelope_const_value(void) {
  Envelope env(EnvelopeLevel(0.4));
  TEST_ASSERT_EQUAL(Envelope::Stage::Attack, env.stage());
  assert_level_equal(env.update(0_ms, true), EnvelopeLevel(0.4));
  assert_level_equal(env.update(3_ms, true), EnvelopeLevel(0.4));
  assert_level_equal(env.update(3000_ms, true), EnvelopeLevel(0.4));
  TEST_ASSERT_EQUAL(Envelope::Stage::Sustain, env.stage());
  assert_level_equal(env.update(0_us, false), EnvelopeLevel(0));
  TEST_ASSERT_EQUAL(Envelope::Stage::Off, env.stage());
}

void test_envelope_comparison(void) {
  TEST_ASSERT_TRUE(lin_adsr == lin_adsr);
  TEST_ASSERT_FALSE(lin_adsr != lin_adsr);
  TEST_ASSERT_TRUE(exp_adsr == exp_adsr);
  TEST_ASSERT_FALSE(exp_adsr != exp_adsr);
  TEST_ASSERT_TRUE(const_adsr == const_adsr);
  TEST_ASSERT_FALSE(const_adsr != const_adsr);

  TEST_ASSERT_TRUE(lin_adsr != exp_adsr);
  TEST_ASSERT_FALSE(lin_adsr == exp_adsr);
  TEST_ASSERT_TRUE(lin_adsr != const_adsr);
  TEST_ASSERT_FALSE(lin_adsr == const_adsr);
}

extern "C" void app_main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_level_sanity);
  RUN_TEST(test_level_comparison);
  RUN_TEST(test_level_arithmetic);
  RUN_TEST(test_level_to_duration);
  RUN_TEST(test_curve_lin_positive);
  RUN_TEST(test_curve_lin_positive2);
  RUN_TEST(test_curve_lin_positive3);
  RUN_TEST(test_curve_lin_negative);
  RUN_TEST(test_curve_lin_negative2);
  RUN_TEST(test_curve_lin_negative_small);
  RUN_TEST(test_curve_exp_positive);
  RUN_TEST(test_curve_exp_negative);
  RUN_TEST(test_curve_constant);
  RUN_TEST(test_curve_constant_zero);

  RUN_TEST(test_envelope_lin_full);
  RUN_TEST(test_envelope_exp_full);
  RUN_TEST(test_envelope_const_full);
  RUN_TEST(test_envelope_const_zero);
  RUN_TEST(test_envelope_const_value);
  RUN_TEST(test_envelope_comparison);
  UNITY_END();
}

int main(int argc, char **argv) { app_main(); }
