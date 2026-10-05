#pragma once
#include "anteater/types.hpp"

namespace ac {
// Configuration and timing policy is independent of the search implementation.
void init_default_game_config(GameConfig *config);
void init_game_config_for_mode(GameConfig *config, GameMode mode);
int get_default_ai_time_budget_ms(Difficulty difficulty);
int get_ai_time_budget_ms(const GameConfig *config, Difficulty difficulty);
int get_required_ai_turn_timer_seconds(const GameConfig *config);
int is_ai_turn_timer_setting_valid(const GameConfig *config);
bool is_valid_difficulty(Difficulty difficulty) noexcept;
Status validate_config(const GameConfig &config) noexcept;
int is_ai_turn(const GameConfig *config, Color color);
int search_depth_limit(Difficulty difficulty);

struct TournamentBudget {
    std::array<int, 2> remainingMs{}, poolMs{};
};
void initialize_tournament_budget(TournamentBudget *manager);
int tournament_budget_ms(const TournamentBudget *manager, Color color);
int tournament_expired(const TournamentBudget *manager, Color color);
void charge_tournament_budget(TournamentBudget *manager, Color color, int budgetMs, int elapsedMs);
} // namespace ac
