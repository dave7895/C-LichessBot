#include "../src/engine.h"
#include <catch2/catch.hpp>
#include <libchess/position.hpp>

const std::tuple<std::string, std::string, int> positions[] = {
    {"rnbqkbn1/p2pp3/Bp6/2p4Q/7R/8/PPPP1PP1/RNB1K1N1 b Qq - 1 10", "=",
     -mateScore},
    {"rnbqkbnQ/p2pp3/Bp6/2p5/7R/8/PPPP1PP1/RNB1K1N1 w Qq - 0 10", ">", 0},
    {"rnbqkb1r/ppppp1pp/5n2/5P2/8/8/PPPP1PPP/RNBQKBNR w KQkq - 1 3", ">", 0},
    {"rnbqkbn1/ppppp3/6p1/5P2/8/8/PPPP1PPr/RNB1KBNR w KQq - 0 5", "<", 0},
    {"rnbqkbn1/ppppp3/6P1/8/8/8/PPPP1PPr/RNB1KBNR b KQq - 0 5", ">", 0},
    {"rnb1kbR1/ppppB3/4p3/8/3P4/8/PPP2PB1/RN2K1N1 b Qq - 0 11", "<", 0},
    {"rnb1kbnr/4pppp/1q6/1Bp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 b kq - 3 11", "<", 0},
    {"rnb1kbnr/4pppp/8/1qp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 w kq - 0 12", ">", 0},
    {"rn2kbnr/3bpppp/1q6/1Bp5/p7/P1N1PN2/1PP2PPP/R1BQ1RK1 w kq - 4 12", ">", 0},
};

TEST_CASE("check if evaluation matches expectations", "[evaluation]") {
  for (const auto &[fen, relation, value] : positions) {
    libchess::Position pos{fen};
    const auto eval = escalating_eval(pos, 0);
    INFO(fen);
    if (relation == "=") {
      CHECK(eval == value);
    } else if (relation == "<") {
      CHECK(eval < value);
    } else if (relation == ">") {
      CHECK(eval > value);
    }
  }
}
