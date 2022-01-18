#include <iostream>
#include <string>
#include <algorithm>
#include <future>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>
#include <libchess/position.hpp> //from kz04px/libchess
//#define SIMDJSON_DEVELOPMENT_CHECKS 1
#include <simdjson.h>
#include <cpr/cpr.h>
#include "playGames.h"
#ifndef AUTH_HEADER
#define AUTH_HEADER
const std::string key = "LICHESS_TOKEN";
const std::string g_myId = "davidsguterbot";
const auto authHeader = cpr::Bearer{getenv(key.c_str())};
#endif
