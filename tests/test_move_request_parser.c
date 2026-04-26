#include <assert.h>
#include <stddef.h>

#include "core/position.h"
#include "input/move_request_parser.h"

static void test_parse_move_request_fields_accepts_standard_fields(void) {
    MoveRequest request;

    assert(parseMoveRequestFields("E2", "E4",
        PROMOTION_CHOICE_QUEEN, &request) == 0);
    assert(positionEqual(request.from, createPosition(6, 4)) == 1);
    assert(positionEqual(request.to, createPosition(4, 4)) == 1);
    assert(request.promotion == PROMOTION_CHOICE_QUEEN);
}

static void test_parse_move_request_fields_accepts_trimmed_lowercase_fields(void) {
    MoveRequest request;

    assert(parseMoveRequestFields("  e2  ", "  e4",
        PROMOTION_CHOICE_ROOK, &request) == 0);
    assert(positionEqual(request.from, createPosition(6, 4)) == 1);
    assert(positionEqual(request.to, createPosition(4, 4)) == 1);
    assert(request.promotion == PROMOTION_CHOICE_ROOK);
}

static void test_parse_move_request_fields_rejects_invalid_coordinates(void) {
    MoveRequest request;

    assert(parseMoveRequestFields("K2", "E4",
        PROMOTION_CHOICE_QUEEN, &request) != 0);
    assert(positionEqual(request.from, createPosition(-1, -1)) == 1);
    assert(positionEqual(request.to, createPosition(-1, -1)) == 1);
    assert(request.promotion == PROMOTION_CHOICE_NONE);

    assert(parseMoveRequestFields("E2", "E 4",
        PROMOTION_CHOICE_QUEEN, &request) != 0);
    assert(parseMoveRequestFields(NULL, "E4",
        PROMOTION_CHOICE_QUEEN, &request) != 0);
}

static void test_parse_move_request_fields_rejects_invalid_promotion(void) {
    MoveRequest request;

    assert(parseMoveRequestFields("E2", "E4",
        (PromotionChoice)99, &request) != 0);
    assert(positionEqual(request.from, createPosition(-1, -1)) == 1);
    assert(positionEqual(request.to, createPosition(-1, -1)) == 1);
    assert(request.promotion == PROMOTION_CHOICE_NONE);
}

static void test_parse_move_request_fields_rejects_null_output(void) {
    assert(parseMoveRequestFields("E2", "E4",
        PROMOTION_CHOICE_QUEEN, NULL) != 0);
}

int main(void) {
    test_parse_move_request_fields_accepts_standard_fields();
    test_parse_move_request_fields_accepts_trimmed_lowercase_fields();
    test_parse_move_request_fields_rejects_invalid_coordinates();
    test_parse_move_request_fields_rejects_invalid_promotion();
    test_parse_move_request_fields_rejects_null_output();
    return 0;
}
