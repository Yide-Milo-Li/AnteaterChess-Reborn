#include "anteater/rules.h"
#include <assert.h>
#include <stddef.h>

static void test_parse_move_request_fields_accepts_standard_fields(void) {
    AcMoveRequest request;

    assert(ac_parse_move_request_fields("E2", "E4", AC_PROMOTION_CHOICE_QUEEN, &request) == 0);
    assert(ac_position_equal(request.from, ac_create_position(6, 4)) == 1);
    assert(ac_position_equal(request.to, ac_create_position(4, 4)) == 1);
    assert(request.promotion == AC_PROMOTION_CHOICE_QUEEN);
}

static void test_parse_move_request_fields_accepts_trimmed_lowercase_fields(void) {
    AcMoveRequest request;

    assert(ac_parse_move_request_fields("  e2  ", "  e4", AC_PROMOTION_CHOICE_ROOK, &request) == 0);
    assert(ac_position_equal(request.from, ac_create_position(6, 4)) == 1);
    assert(ac_position_equal(request.to, ac_create_position(4, 4)) == 1);
    assert(request.promotion == AC_PROMOTION_CHOICE_ROOK);
}

static void test_parse_move_request_fields_rejects_invalid_coordinates(void) {
    AcMoveRequest request;

    assert(ac_parse_move_request_fields("K2", "E4", AC_PROMOTION_CHOICE_QUEEN, &request) != 0);
    assert(ac_position_equal(request.from, ac_create_position(-1, -1)) == 1);
    assert(ac_position_equal(request.to, ac_create_position(-1, -1)) == 1);
    assert(request.promotion == AC_PROMOTION_CHOICE_NONE);

    assert(ac_parse_move_request_fields("E2", "E 4", AC_PROMOTION_CHOICE_QUEEN, &request) != 0);
    assert(ac_parse_move_request_fields(NULL, "E4", AC_PROMOTION_CHOICE_QUEEN, &request) != 0);
}

static void test_parse_move_request_fields_rejects_invalid_promotion(void) {
    AcMoveRequest request;

    assert(ac_parse_move_request_fields("E2", "E4", (AcPromotionChoice)99, &request) != 0);
    assert(ac_position_equal(request.from, ac_create_position(-1, -1)) == 1);
    assert(ac_position_equal(request.to, ac_create_position(-1, -1)) == 1);
    assert(request.promotion == AC_PROMOTION_CHOICE_NONE);
}

static void test_parse_move_request_fields_rejects_null_output(void) {
    assert(ac_parse_move_request_fields("E2", "E4", AC_PROMOTION_CHOICE_QUEEN, NULL) != 0);
}

int main(void) {
    test_parse_move_request_fields_accepts_standard_fields();
    test_parse_move_request_fields_accepts_trimmed_lowercase_fields();
    test_parse_move_request_fields_rejects_invalid_coordinates();
    test_parse_move_request_fields_rejects_invalid_promotion();
    test_parse_move_request_fields_rejects_null_output();
    return 0;
}
