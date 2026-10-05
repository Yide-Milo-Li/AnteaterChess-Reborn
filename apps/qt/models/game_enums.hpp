#pragma once
#include "anteater/types.hpp"
#include <QObject>
#include <QtQml/qqmlregistration.h>
namespace ac {
class GameEnums : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(Game)
    QML_UNCREATABLE("Game enums")
  public:
    enum Mode {
        HumanVsHuman = value(GameMode::HumanVsHuman),
        HumanVsComputer = value(GameMode::HumanVsComputer),
        ComputerVsComputer = value(GameMode::ComputerVsComputer)
    };
    Q_ENUM(Mode)
    enum Side { White = value(Color::White), Black = value(Color::Black), NoSide = value(Color::Empty) };
    Q_ENUM(Side)
    enum Level {
        Human = value(Difficulty::None),
        Easy = value(Difficulty::Easy),
        Medium = value(Difficulty::Medium),
        Hard = value(Difficulty::Hard),
        Tournament = value(Difficulty::Tournament)
    };
    Q_ENUM(Level)
    enum Promotion {
        NoPromotion = value(PromotionChoice::None),
        PromoteQueen = value(PromotionChoice::Queen),
        PromoteRook = value(PromotionChoice::Rook),
        PromoteBishop = value(PromotionChoice::Bishop),
        PromoteKnight = value(PromotionChoice::Knight)
    };
    Q_ENUM(Promotion)
};
} // namespace ac
