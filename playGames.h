#ifndef PLAYGAMES_H
#define PLAYGAMES_H
#include "lichessBot.h"
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
bool wrapperCallback(std::string data, game &thisGame,
                     simdjson::ondemand::parser &gameParser,
                     simdjson::ondemand::document &state, int &currentDepth);
bool streamEventsCallback(std::string data, intptr_t);
// bool streamGame(game specificGame, std::stringstream ss);

bool streamGameUtility(std::string id);
bool fillGameStreamBuffer(std::string data, intptr_t,
                          simdjson::ondemand::parser &gameParser,
                          simdjson::ondemand::document &state,
                          std::queue<std::string> &sq, std::mutex &mut,
                          std::condition_variable &condvar);
std::string sviewval_to_str(
    simdjson::simdjson_result<simdjson::fallback::ondemand::value> view);
libchess::Move calculateMove(libchess::Position pos, int depth=1);
void wrapperStreamgame(std::string gameId);

#endif
