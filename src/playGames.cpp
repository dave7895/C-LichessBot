#include "playGames.h"

std::string sviewval_to_str(
    simdjson::simdjson_result<simdjson::fallback::ondemand::value> view) {
  return std::string{std::string_view{view}};
}

simdjson::ondemand::parser parser;
std::mutex stdoutmtx;
std::vector<std::future<bool>> runningGames;
std::vector<std::thread> runningGameThreads;
std::map<std::string, std::thread> runningGamesMap;
bool streamEventsCallback(std::string data, intptr_t) {
  std::cout << "hello from streamEvents\n";
  if (data.length() <= 1)
    return true;
  data.reserve(data.length() + simdjson::SIMDJSON_PADDING);
  std::cout << "length: " << data.length() << " size: " << data.size()
            << " capacity: " << data.capacity()
            << " Length of SIMDJSON_PADDING: " << simdjson::SIMDJSON_PADDING
            << "\n";
  simdjson::ondemand::document doc = parser.iterate(data);
  std::string_view eventTypeView = doc["type"];
  std::string eventType{eventTypeView};
  std::cout << eventType << "\n";
  std::vector<std::future<bool>> futVec;
  std::future<bool> currentFut;
  std::cout << eventType << "arrived in streamEventsCallback\n";
  if (eventType == "challenge") {
    std::string challengeId{std::string_view{doc["challenge"]["id"]}};
    if (std::string_view{doc["challenge"]["id"]} == g_myId)
      return true;
    std::string variant{
        doc["challenge"]["variant"]["key"].get_string().take_value()};
    std::cout << "received challenge from "
              << std::string{std::string_view{
                     doc["challenge"]["challenger"]["id"].value()}}
              << "\n";
    if (variant == "standard" || variant == "threeCheck")
      cpr::Response acc =
          cpr::Post(cpr::Url{"https://lichess.org/api/challenge/" +
                             challengeId + "/accept"},
                    authHeader);
    else {
      sendToLichess("challenge/" + challengeId + "/decline", "reason",
                    "variant");
      cpr::Response decl =
          cpr::Post(cpr::Url{"https://lichess.org/api/challenge/" +
                             challengeId + "/decline"},
                    authHeader, cpr::Payload{{"reason", "variant"}});
      return true;
    }
    libchess::Position startpos{
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"};
    game fromChallenge(
        challengeId,
        std::string{std::string_view{doc["challenge"]["variant"]["name"]}},
        startpos);
    std::cout << "id: " << fromChallenge.gameId
              << " variant: " << fromChallenge.variant
              << " position: " << fromChallenge.gamePos.get_fen() << '\n';
  } else if (eventType == "gameStart") {
    std::string gameId{std::string_view{doc["game"]["id"]}};
    std::cout << "game with id " << gameId << "\n";
    // futVec.push_back(std::async(streamGameUtility, gameId));
    // runningGameThreads.push_back(std::thread(wrapperStreamgame, gameId));
    runningGamesMap[gameId] = std::thread(wrapperStreamgame, gameId);
  } else if (eventType == "gameFinish") {
    std::string id{doc["game"]["id"].get_string().take_value()};
    std::cout << "now joining thread of game with id " << id << '\n';
    runningGamesMap[id].join();
    std::cout << "thread joined, now deleting from map\n";
    runningGamesMap.erase(id);
  }
  std::cout << runningGamesMap.size() << " games are currently running\n";
  return true;
}

bool wrapperCallback(std::string data, game &thisGame,
                     /* simdjson::ondemand::parser &gameParser,
                      simdjson::ondemand::document &state, */
                     std::mutex &gamemtx, int &currentDepth) {
  std::cout << "input to wrapperCallback: " << data << '\n';
  if (data.length() < 5 || data.find('{') == std::string::npos) {
    std::cout << "exiting wrapperCallback because only keep alive\n";
    return true;
  }
  simdjson::ondemand::parser gameParser;
  data.reserve(data.length() + simdjson::SIMDJSON_PADDING);
  simdjson::ondemand::document state = gameParser.iterate(data);
  std::string respType{state["type"].get_string().take_value()};
  std::cout << respType << " arrived in wrapper callback\n";
  std::string moves = "";
  std::cout << "now locking\n";
  std::lock_guard<std::mutex> lckg(gamemtx);
  std::cout << "locked the mutex with a lock guard\n";
  simdjson::ondemand::object stat;
  std::string stateString;
  if (respType == "chatLine") {
    return true;
  } else if (respType == "gameFull") {
    std::cout << "trying to get gameid\n";
    std::string id{state["id"].get_string().take_value()};
    std::cout << "trying to fill thisGame.gameId\n";
    thisGame.gameId = id;
    std::cout << "filled gameId of thisGame\n";
    std::string variant{state["variant"]["key"].get_string().take_value()};
    thisGame.variant = variant;
    try {
      std::string_view svSide = state["white"]["id"].get_string();
      if (svSide == g_myId) {
        thisGame.mySide = libchess::Side::White;
        std::cout << "I am white\n";
      } else {
        thisGame.mySide = libchess::Side::Black;
        std::cout << "I am black\n";
      }
    } catch (simdjson::simdjson_error &e) {
      std::cout << "error: " << e.what() << '\n';
      thisGame.mySide = libchess::Side::Black;
    }
    thisGame.gamePos = libchess::Position{
        std::string{state["initialFen"].get_string().take_value()}};
    moves = state["state"]["moves"].get_string().take_value();
    stat = state["state"];
  } else if (respType == "gameState") {
    std::cout << "respType == gameState is true\n";
    moves = std::string{state["moves"].get_string().take_value()};
    // stat = state.get_object().take_value();
    // stateString = std::string{state.get_raw_json_string().raw()};
  }
  bool statOrState = respType == "gameState";
  stateString.resize(stateString.size() + simdjson::SIMDJSON_PADDING);
  size_t poss, movePos, moveEnd;
  poss = movePos = moveEnd = 0;
  libchess::Position &pos = thisGame.gamePos;
  std::cout << pos << std::endl;
  bool previousMoveExists = (pos.history().size() > 0);
  std::string previousMoveStr;
  libchess::Move previousMove;
  size_t availableTime = 0;
  std::string side = pos.turn() == libchess::Side::White ? "w" : "b";
  std::string fieldstr = side + "time";
  std::cout << "fieldstr no is " << fieldstr << '\n';
  std::string status;
  size_t timeLeft;
  if (statOrState) {
    timeLeft = state[fieldstr].get_int64().take_value();
    availableTime +=
        std::min(timeLeft / 900, timeLeft / 2 * pos.history().size() / 2);
    fieldstr = side + "inc";
    std::cout << "fieldstr now is " << fieldstr << '\n';
    availableTime += state[fieldstr].get_uint64().take_value();
    availableTime = std::min(timeLeft - 2000, availableTime);
    status = std::string{state["status"].get_string().take_value()};
  } else {
    timeLeft = stat[fieldstr].get_int64().take_value();
    availableTime +=
        std::min(timeLeft / 900, timeLeft / 2 * pos.history().size() / 2);
    fieldstr = side + "inc";
    std::cout << "fieldstr now is " << fieldstr << '\n';
    availableTime += stat[fieldstr].get_uint64().take_value();
    availableTime = std::min(timeLeft - 2000, availableTime);
    status = std::string{stat["status"].get_string().take_value()};
  }
  std::cout << "availableTime is " << availableTime << '\n';

  if (status != "started") {
    std::cout
        << "exiting wrapperCallback because game probably over, status is "
        << status << "\n";
    return false;
  }
  if (previousMoveExists) {
    previousMove = pos.history().back().move;
    previousMoveStr = static_cast<std::string>(previousMove);
    std::cout << "previousMoveSTr= " << previousMoveStr << std::endl;
    while ((movePos = moves.find(previousMoveStr)) != std::string::npos &&

           moves.size() > 0) {
      if (movePos != std::string::npos) {
        moveEnd = moves.find(' ', movePos);
        if (moveEnd < movePos) {
          std::cout << previousMoveStr << " is last move in " << moves << '\n';
          moves.clear();
        } else if (moveEnd == std::string::npos) {
          moves.clear();
        } else {
          moves.erase(0, moveEnd + 1);
        }
      } else {
        std::cout << "some error probably occurred, cant find previousMove in "
                     "move string from response\n";
      }
    }
  }
  std::string singleMoveStr;
  libchess::Move singleMove;
  while ((poss = moves.find(' ')) != std::string::npos) {
    singleMoveStr = moves.substr(0, poss);
    std::cout << "current move: " << singleMoveStr << std::endl;
    std::cout << pos << std::endl;
    singleMove = pos.parse_move(singleMoveStr);
    std::cout << "singleMoveStr legal: " << pos.is_legal(singleMove) << '\n';
    if (pos.history().size() > 0)
      std::cout << static_cast<std::string>(pos.history().back().move)
                << std::endl;
    pos.makemove(singleMove);
    moves.erase(0, poss + 1);
  }
  if (moves.length() > 1) {
    std::cout << "current move: " << moves << std::endl;
    pos.makemove(std::string(moves));
    moves.clear();
  }
  if (pos.turn() != thisGame.mySide) {
    std::cout << "exiting wrapperCallback because not my turn\n";
    return true;
  }
  if (status != "started") {
    std::cout << "exiting wrapperCallback because game probaly over, status is "
              << state["status"].get_string().take_value() << "\n";
    return false;
  }
  std::cout << pos << '\n';
  auto start = std::chrono::steady_clock::now();
  previousMove = calculateMove(pos, currentDepth);
  auto end = std::chrono::steady_clock::now();
  size_t mstime =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  std::cout
      << "calculated moves until depth " << currentDepth << " in "
      << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
             .count()
      << " ns or "
      << std::chrono::duration_cast<std::chrono::microseconds>(end - start)
             .count()
      << " µs or " << mstime << " ms or "
      << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
      << " sec\n";
  mstime = std::max(mstime, mstime / pos.legal_moves().size() * 30);
  size_t lowerLimit = timeLeft < 2000 ? 0 : timeLeft - 2000;
  availableTime = std::min(availableTime, lowerLimit);
  if (mstime == 0 || availableTime / mstime > 20) {
    ++currentDepth;
  } else if (static_cast<float>(mstime)/availableTime > 1.5 ||
             (mstime - (availableTime - (timeLeft / 30)) - 1000) > timeLeft) {
    --currentDepth;
  }
  if (!pos.is_legal(previousMove)) {
    std::cout << "exiting wrapperCallback because returned move not legal\n";
    return false;
  }
  pos.makemove(previousMove);
  cpr::Response moveResp =
      cpr::Post(cpr::Url{"https://lichess.org/api/bot/game/" + thisGame.gameId +
                         "/move/" + static_cast<std::string>(previousMove)},
                authHeader);
  std::cout << "ending wrapperCallback after making my move\n";
  return true;
}

void wrapperStreamgame(std::string gameId) {
  std::cout << "start of wrapperStreamgame\n";
  simdjson::ondemand::parser gameParser;
  simdjson::ondemand::document state;
  std::mutex accessToGlobals;
  game thisGame;
  int currentDepth = 2;
  std::cout << "now beginning to stream game with id " << gameId << '\n';
  cpr::Response r = cpr::Get(
      cpr::Url("https://lichess.org/api/bot/game/stream/" + gameId), authHeader,
      cpr::WriteCallback([&thisGame, &gameParser, &state, &accessToGlobals,
                          &currentDepth](std::string data, intptr_t) {
        return wrapperCallback(data, thisGame, /* gameParser, state,*/
                               accessToGlobals, currentDepth);
      }));
  std::cout << "exiting wrapperStreamgame, game is over\n";
}
