#include <iostream>
#include <libchess/position.hpp> //from kz04px/libchess
#include <random>
struct game {
  std::string gameId;
  std::string variant;
  libchess::Position gamePos;
  libchess::Side mySide;
  game(){};
  game(std::string gameId, std::string variant, std::string fen)
      : gameId(gameId), variant(variant) /*,
         gamePos(libchess::Position{fen})*/
  {
    libchess::Position pos{fen};
    gamePos = pos;
  };
  game(std::string gameId, std::string variant, libchess::Position pos,
       libchess::Side side)
      : gameId(gameId), variant(variant), gamePos(pos), mySide(side){};
  game(std::string gameId, std::string variant, libchess::Position pos)
      : gameId(gameId), variant(variant), gamePos(pos),
        mySide(libchess::Side::White){};
};

libchess::Move topLevelNegamax(libchess::Position &pos, const int depth,
                               const bool alphabeta = true,
                               int alpha = -INT32_MAX,
                               const int beta = INT32_MAX);
int negamax(libchess::Position &pos, int depth, int alpha, int beta,
            const int maxDepth);
int negamaxNoAB(libchess::Position pos, int depth);
libchess::Move calculateMove(libchess::Position pos, int depth = 1);
int evaluate_position(libchess::Position const &pos, const int &depth);
int escalating_eval(libchess::Position const &pos, const int depth);
int verySimpleEval(libchess::Position pos);
int simpleEval(libchess::Position pos);
const int mateScore = 100000;
const std::array<int, 6> pieceToVal{100, 320, 330, 500, 900, 100};
