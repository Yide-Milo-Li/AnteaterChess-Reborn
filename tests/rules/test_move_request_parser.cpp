#include "anteater/rules.hpp"
#include <assert.h>
#include <stddef.h>

using namespace ac;

static void test_parse_move_request_fields_accepts_standard_fields(void) {
    MoveRequest request;

    assert(parse_move_request_fields("E2", "E4", PromotionChoice::Queen, &request) == Status::Ok);
    assert(position_equal(request.from, create_position(6, 4)) == 1);
    assert(position_equal(request.to, create_position(4, 4)) == 1);
    assert(request.promotion == PromotionChoice::Queen);
}

static void test_parse_move_request_fields_accepts_trimmed_lowercase_fields(void) {
    MoveRequest request;

    assert(parse_move_request_fields("  e2  ", "  e4", PromotionChoice::Rook, &request) == Status::Ok);
    assert(position_equal(request.from, create_position(6, 4)) == 1);
    assert(position_equal(request.to, create_position(4, 4)) == 1);
    assert(request.promotion == PromotionChoice::Rook);
}

static void test_parse_move_request_fields_rejects_invalid_coordinates(void) {
    MoveRequest request;

    assert(parse_move_request_fields("K2", "E4", PromotionChoice::Queen, &request) != Status::Ok);
    assert(position_equal(request.from, create_position(-1, -1)) == 1);
    assert(position_equal(request.to, create_position(-1, -1)) == 1);
    assert(request.promotion == PromotionChoice::None);

    assert(parse_move_request_fields("E2", "E 4", PromotionChoice::Queen, &request) != Status::Ok);
    assert(parse_move_request_fields(NULL, "E4", PromotionChoice::Queen, &request) != Status::Ok);
}

static void test_parse_move_request_fields_rejects_invalid_promotion(void) {
    MoveRequest request;

    assert(parse_move_request_fields("E2", "E4", (PromotionChoice)99, &request) != Status::Ok);
    assert(position_equal(request.from, create_position(-1, -1)) == 1);
    assert(position_equal(request.to, create_position(-1, -1)) == 1);
    assert(request.promotion == PromotionChoice::None);
}

static void test_parse_move_request_fields_rejects_null_output(void) {
    assert(parse_move_request_fields("E2", "E4", PromotionChoice::Queen, NULL) != Status::Ok);
}

int main(void) {
    test_parse_move_request_fields_accepts_standard_fields();
    test_parse_move_request_fields_accepts_trimmed_lowercase_fields();
    test_parse_move_request_fields_rejects_invalid_coordinates();
    test_parse_move_request_fields_rejects_invalid_promotion();
    test_parse_move_request_fields_rejects_null_output();
    return 0;
}
