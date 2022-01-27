#include "../src/engine.h"
#include <catch2/catch.hpp>
#include <libchess/position.hpp>
const std::pair<std::string, std::string> positions[] = {
    {"rnbqkbn1/p2pp2r/Bpp3P1/8/7R/8/PPPP1PP1/RNB1K1N1 w Qq - 0 8", "g6h7"},
    {"rnbqkbn1/ppppp3/6p1/5P2/8/8/PPPP1PPr/RNB1KBNR w KQq - 0 5", "h1h2"},
    {"rnb1kbnr/4pppp/1q6/1Bp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 b kq - 3 11", "c8d7"},
    {"rnb1kbnr/4pppp/8/1qp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 w kq - 0 12", "c3b5"}
  };

TEST_CASE("Check if engine can find obviously best captures", "[captures]") {
  for (const auto &[fen, bestMove] : positions) {
    libchess::Position pos{fen};
    INFO(fen);
    CHECK(static_cast<std::string>(
              topLevelNegamax(pos, 3, false, -INT32_MAX, INT32_MAX)) == bestMove);
  }
}
