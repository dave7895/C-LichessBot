#include "../src/engine.h"
#include <catch2/catch.hpp>
#include <libchess/position.hpp>
#include <string>
const std::pair<std::string, std::string> positions[] = {
    {"rn2kb1r/pp2pp1p/3p4/q7/1p1Pbpn1/B6K/P1P1P2P/RN1Q1BNR b kq - 0 13",
     "a5h5"},
    {"rnbqkbnQ/p2pp3/Bp6/2p5/7R/8/PPPP1PP1/RNB1K1N1 w Qq - 0 10", "h8h5"}};
TEST_CASE("Check if engine can find mate in one", "[mate]") {
  for (const auto &[fen, bestMove] : positions) {
    libchess::Position pos{fen};
    INFO(fen);
    CHECK(static_cast<std::string>(
              topLevelNegamax(pos, 3, true, -INT32_MAX, INT32_MAX)) == bestMove);
  }
}
