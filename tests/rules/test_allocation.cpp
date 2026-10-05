#include "anteater/rules.hpp"
#include "../core/failing_resource.hpp"
#include <cassert>
#include <memory>

int main() {
    using namespace ac;
    Position position{};
    position_init(&position);
    const auto original = position;
    auto moves = std::make_unique<MoveList>();
    assert(generate_legal_moves(&position, moves.get()) == Status::Ok);
    const Move move = moves->moves[0];
    FailingResource resource;
    Undo undo{};
    resource.failNext();
    assert(position_apply(&position, move, &undo, &resource) == Status::OutOfMemory);
    assert(position == original && resource.live == 0);
    GameResult result = GameResult::WhiteWin;
    resource.failNext();
    assert(position_result(&position, &result, &resource) == Status::OutOfMemory);
    assert(result == GameResult::WhiteWin && resource.live == 0);
    MoveRequest request{move.from, move.to, PromotionChoice::None};
    Move resolved = move;
    resource.failNext();
    assert(resolve_move_request(&position, request, &resolved, &resource) == Status::OutOfMemory);
    assert(resolved == move && resource.live == 0);
    resource.failNext();
    assert(!validate_move(&position, move, &resource));
    assert(position == original && resource.live == 0);
    resource.failAt = 0;
    assert(position_apply(&position, move, &undo, &resource) == Status::Ok);
    assert(resource.live == 0);
    assert(position_unmake(&position, &undo) == Status::Ok);
    assert(position == original);
}
