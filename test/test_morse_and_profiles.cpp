#include <unity.h>
#include "morse_table.h"
#include "profiles.h"

// ==========================================
// Morse Encode/Decode Round-Trip Tests
// ==========================================

void test_encode_decode_roundtrip_letters(void) {
    char buf[8];
    for (char ch = 'A'; ch <= 'Z'; ch++) {
        int len = morseEncode(ch, buf);
        TEST_ASSERT_GREATER_THAN(0, len);
        char decoded = morseDecode(buf);
        TEST_ASSERT_EQUAL_CHAR(ch, decoded);
    }
}

void test_encode_decode_roundtrip_digits(void) {
    char buf[8];
    for (char ch = '0'; ch <= '9'; ch++) {
        int len = morseEncode(ch, buf);
        TEST_ASSERT_GREATER_THAN(0, len);
        char decoded = morseDecode(buf);
        TEST_ASSERT_EQUAL_CHAR(ch, decoded);
    }
}

void test_encode_lowercase_maps_to_uppercase(void) {
    char buf_upper[8], buf_lower[8];
    morseEncode('A', buf_upper);
    morseEncode('a', buf_lower);
    TEST_ASSERT_EQUAL_STRING(buf_upper, buf_lower);
}

void test_encode_space_returns_space(void) {
    char buf[8];
    int len = morseEncode(' ', buf);
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL_CHAR(' ', buf[0]);
}

void test_encode_unknown_char_returns_zero(void) {
    char buf[8];
    int len = morseEncode(3, buf); // ETX control char — not in table
    TEST_ASSERT_EQUAL(0, len);
    TEST_ASSERT_EQUAL_CHAR('\0', buf[0]);
}

// ==========================================
// Known Morse Patterns
// ==========================================

void test_encode_E_is_dot(void) {
    char buf[8];
    morseEncode('E', buf);
    TEST_ASSERT_EQUAL_STRING(".", buf);
}

void test_encode_T_is_dash(void) {
    char buf[8];
    morseEncode('T', buf);
    TEST_ASSERT_EQUAL_STRING("-", buf);
}

void test_encode_A_is_dot_dash(void) {
    char buf[8];
    morseEncode('A', buf);
    TEST_ASSERT_EQUAL_STRING(".-", buf);
}

void test_encode_N_is_dash_dot(void) {
    char buf[8];
    morseEncode('N', buf);
    TEST_ASSERT_EQUAL_STRING("-.", buf);
}

void test_encode_S_is_three_dots(void) {
    char buf[8];
    morseEncode('S', buf);
    TEST_ASSERT_EQUAL_STRING("...", buf);
}

void test_encode_O_is_three_dashes(void) {
    char buf[8];
    morseEncode('O', buf);
    TEST_ASSERT_EQUAL_STRING("---", buf);
}

void test_encode_SOS_pattern(void) {
    char buf_s[8], buf_o[8];
    morseEncode('S', buf_s);
    morseEncode('O', buf_o);
    TEST_ASSERT_EQUAL_STRING("...", buf_s);
    TEST_ASSERT_EQUAL_STRING("---", buf_o);
}

void test_encode_1_is_dot_dash_dash_dash_dash(void) {
    char buf[8];
    morseEncode('1', buf);
    TEST_ASSERT_EQUAL_STRING(".----", buf);
}

void test_encode_0_is_five_dashes(void) {
    char buf[8];
    morseEncode('0', buf);
    TEST_ASSERT_EQUAL_STRING("-----", buf);
}

void test_encode_5_is_five_dots(void) {
    char buf[8];
    morseEncode('5', buf);
    TEST_ASSERT_EQUAL_STRING(".....", buf);
}

// ==========================================
// Decode Edge Cases
// ==========================================

void test_decode_empty_pattern_returns_root(void) {
    // Empty pattern stays at root — which is a space
    char decoded = morseDecode("");
    // Root of tree is space character
    TEST_ASSERT_EQUAL_CHAR(' ', decoded);
}

void test_decode_invalid_char_returns_null(void) {
    char decoded = morseDecode("x");
    TEST_ASSERT_EQUAL_CHAR('\0', decoded);
}

void test_decode_very_long_pattern_returns_null(void) {
    // 7 dots — exceeds tree depth (6 levels)
    char decoded = morseDecode(".......");
    TEST_ASSERT_EQUAL_CHAR('\0', decoded);
}

void test_decode_dot(void) {
    TEST_ASSERT_EQUAL_CHAR('E', morseDecode("."));
}

void test_decode_dash(void) {
    TEST_ASSERT_EQUAL_CHAR('T', morseDecode("-"));
}

void test_decode_dot_dash(void) {
    TEST_ASSERT_EQUAL_CHAR('A', morseDecode(".-"));
}

// ==========================================
// Profile Tests
// ==========================================

void test_profile_P1_all_letters_nonzero(void) {
    const uint8_t* p = getProfile(1);
    TEST_ASSERT_NOT_NULL(p);
    // Letters A-Z are at indices 32-57
    for (int i = 32; i < CHAR_COUNT; i++) {
        TEST_ASSERT_EQUAL(50, pgm_read_byte(p + i));
    }
}

void test_profile_P1_punctuation_zero(void) {
    const uint8_t* p = getProfile(1);
    // Indices 0-31 are punctuation/numbers — all zero in P1
    for (int i = 0; i < 32; i++) {
        TEST_ASSERT_EQUAL(0, pgm_read_byte(p + i));
    }
}

void test_profile_P3_numbers_only(void) {
    const uint8_t* p = getProfile(3);
    TEST_ASSERT_NOT_NULL(p);
    // Numbers '0'-'9' are ASCII 48-57, indices 15-24
    for (int i = 15; i <= 24; i++) {
        TEST_ASSERT_EQUAL(50, pgm_read_byte(p + i));
    }
    // Letters should be zero
    for (int i = 32; i < CHAR_COUNT; i++) {
        TEST_ASSERT_EQUAL(0, pgm_read_byte(p + i));
    }
}

void test_profile_P6_beginner_five_letters(void) {
    const uint8_t* p = getProfile(6);
    TEST_ASSERT_NOT_NULL(p);
    // A(32), E(36), M(44), N(45), T(51) should be nonzero
    TEST_ASSERT_GREATER_THAN(0, pgm_read_byte(p + 32)); // A
    TEST_ASSERT_GREATER_THAN(0, pgm_read_byte(p + 36)); // E
    TEST_ASSERT_GREATER_THAN(0, pgm_read_byte(p + 44)); // M
    TEST_ASSERT_GREATER_THAN(0, pgm_read_byte(p + 45)); // N
    TEST_ASSERT_GREATER_THAN(0, pgm_read_byte(p + 51)); // T

    // Count nonzero letters (indices 32-57)
    int count = 0;
    for (int i = 32; i < CHAR_COUNT; i++) {
        if (pgm_read_byte(p + i) > 0) count++;
    }
    TEST_ASSERT_EQUAL(5, count);
}

void test_profile_invalid_returns_null(void) {
    TEST_ASSERT_NULL(getProfile(0));
    TEST_ASSERT_NULL(getProfile(10));
    TEST_ASSERT_NULL(getProfile(255));
}

void test_profile_all_valid_indices(void) {
    for (uint8_t i = 1; i <= 9; i++) {
        const uint8_t* p = getProfile(i);
        TEST_ASSERT_NOT_NULL(p);
    }
}

void test_profile_probabilities_in_range(void) {
    for (uint8_t i = 1; i <= 9; i++) {
        const uint8_t* p = getProfile(i);
        for (int j = 0; j < CHAR_COUNT; j++) {
            uint8_t val = pgm_read_byte(p + j);
            TEST_ASSERT_LESS_OR_EQUAL(100, val);
        }
    }
}

void test_profile_each_has_at_least_one_nonzero(void) {
    for (uint8_t i = 1; i <= 9; i++) {
        const uint8_t* p = getProfile(i);
        int sum = 0;
        for (int j = 0; j < CHAR_COUNT; j++) {
            sum += pgm_read_byte(p + j);
        }
        TEST_ASSERT_GREATER_THAN(0, sum);
    }
}

// ==========================================
// Test Runner
// ==========================================

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Morse encode/decode
    RUN_TEST(test_encode_decode_roundtrip_letters);
    RUN_TEST(test_encode_decode_roundtrip_digits);
    RUN_TEST(test_encode_lowercase_maps_to_uppercase);
    RUN_TEST(test_encode_space_returns_space);
    RUN_TEST(test_encode_unknown_char_returns_zero);

    // Known patterns
    RUN_TEST(test_encode_E_is_dot);
    RUN_TEST(test_encode_T_is_dash);
    RUN_TEST(test_encode_A_is_dot_dash);
    RUN_TEST(test_encode_N_is_dash_dot);
    RUN_TEST(test_encode_S_is_three_dots);
    RUN_TEST(test_encode_O_is_three_dashes);
    RUN_TEST(test_encode_SOS_pattern);
    RUN_TEST(test_encode_1_is_dot_dash_dash_dash_dash);
    RUN_TEST(test_encode_0_is_five_dashes);
    RUN_TEST(test_encode_5_is_five_dots);

    // Decode edge cases
    RUN_TEST(test_decode_empty_pattern_returns_root);
    RUN_TEST(test_decode_invalid_char_returns_null);
    RUN_TEST(test_decode_very_long_pattern_returns_null);
    RUN_TEST(test_decode_dot);
    RUN_TEST(test_decode_dash);
    RUN_TEST(test_decode_dot_dash);

    // Profiles
    RUN_TEST(test_profile_P1_all_letters_nonzero);
    RUN_TEST(test_profile_P1_punctuation_zero);
    RUN_TEST(test_profile_P3_numbers_only);
    RUN_TEST(test_profile_P6_beginner_five_letters);
    RUN_TEST(test_profile_invalid_returns_null);
    RUN_TEST(test_profile_all_valid_indices);
    RUN_TEST(test_profile_probabilities_in_range);
    RUN_TEST(test_profile_each_has_at_least_one_nonzero);

    return UNITY_END();
}
