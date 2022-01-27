#include "../src/engine.h"
#include <catch2/catch.hpp>
#include <libchess/position.hpp>

const std::vector<std::string> positions = {
    "rnbqkbn1/p2pp3/Bp6/2p4Q/7R/8/PPPP1PP1/RNB1K1N1 b Qq - 1 10",
    "rnbqkbnQ/p2pp3/Bp6/2p5/7R/8/PPPP1PP1/RNB1K1N1 w Qq - 0 10",
    "rnbqkb1r/ppppp1pp/5n2/5P2/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 3",
    "rnbqkbn1/ppppp3/6p1/5P2/8/8/PPPP1PPr/RNB1KBNR w KQq - 0 5",
    "rnbqkbn1/ppppp3/6P1/8/8/8/PPPP1PPr/RNB1KBNR b KQq - 0 5",
    "rnb1kbR1/ppppB3/4p3/8/3P4/8/PPP2PB1/RN2K1N1 b Qq - 0 11",
    "rnb1kbnr/4pppp/1q6/1Bp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 b kq - 3 11",
    "rnb1kbnr/4pppp/8/1qp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 w kq - 0 12",
    "rn2kbnr/3bpppp/1q6/1Bp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 w kq - 4 12",
};

TEST_CASE("Check if alphabeta evaluates the same as plain negamax",
          "[evaluation], [alphabeta]") {
  for (const auto fen : positions) {
    libchess::Position pos{fen};
    INFO(fen);
    CHECK(topLevelNegamax(pos, 3, true) == topLevelNegamax(pos, 3, false));
    CHECK(negamax(pos, 3, -INT32_MAX, INT32_MAX, 3) == negamaxNoAB(pos, 3));
  }
}
