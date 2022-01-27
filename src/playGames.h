#ifndef PLAYGAMES_H
#define PLAYGAMES_H
#include "engine.h"
#include "apiInteraction.h"
#include <simdjson.h>
#include <mutex>
#include <queue>
#include <random>
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
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
void wrapperStreamgame(std::string gameId);
std::string sviewval_to_str(
    simdjson::simdjson_result<simdjson::fallback::ondemand::value> view);

#endif
