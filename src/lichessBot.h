#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <libchess/position.hpp> //from kz04px/libchess
#include <mutex>
#include <queue>
#include <random>
#include <string>
#include "playGames.h"
#include <cpr/cpr.h>
#include <simdjson.h>
#ifndef AUTH_HEADER
#define AUTH_HEADER
const std::string key = "LICHESS_TOKEN";
const std::string g_myId = "davidsguterbot";
const auto authHeader = cpr::Bearer{getenv(key.c_str())};
#endif
