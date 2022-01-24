#include "playGames.h"

std::string sviewval_to_str(
    simdjson::simdjson_result<simdjson::fallback::ondemand::value> view) {
  return std::string{std::string_view{view}};
}

std::random_device rd;
std::default_random_engine rng(rd());
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

bool streamGame(game specificGame, std::queue<std::string> &q,
                std::condition_variable &condvar, std::mutex &mut) {
  stdoutmtx.lock();
  std::cout << "start of streamGame\n";
  stdoutmtx.unlock();
  std::string id = specificGame.gameId;
  std::string variant = specificGame.variant;
  libchess::Position pos = specificGame.gamePos;
  libchess::Side myColor = specificGame.mySide;
  stdoutmtx.lock();
  std::cout << "extracted from specificGame\n";
  stdoutmtx.unlock();
  simdjson::ondemand::parser specificParser;
  stdoutmtx.lock();
  std::cout << "defined parser\n";
  stdoutmtx.unlock();
  std::string currentData;
  currentData.reserve(1000);
  currentData = "moinsen";
  simdjson::ondemand::document state = specificParser.iterate(currentData);
  stdoutmtx.lock();
  std::cout << "defined document for game state\n";
  std::cout << "locking mut\n";
  std::unique_lock<std::mutex> lck(mut);
  std::cout << "locked mut" << std::endl;

  stdoutmtx.unlock();
  libchess::Move lastMove;
  cpr::Response moveResp;
  bool lastMoveSet = false;
  while (1) {

    stdoutmtx.lock();
    std::cout << "start of while loop\n";
    std::cout << "lastMove: " << lastMove << '\n';
    stdoutmtx.unlock();
    if (q.empty()) {
      std::cout << "now waiting for signal\n";
      condvar.wait_for(lck, std::chrono::seconds(5));
      stdoutmtx.lock();
      std::cout << "received signal, can continue\n";
      stdoutmtx.unlock();
    } else {
      stdoutmtx.lock();
      std::cout << "no need to wait, queue has unprocessed data\n";
      stdoutmtx.unlock();
    }
    stdoutmtx.lock();
    std::cout << "after if clause. lastMove is Set: " << lastMoveSet
              << "and set to " << lastMove << "\n";
    std::cout << "length of queue: " << q.size() << '\n';
    if (q.size() > 0) {
      currentData = q.front();
      q.pop();
    } else {
      currentData = "";
    }
    stdoutmtx.unlock();
    std::cout << "currentData retrieved and popped from queue\n";
    std::cout << currentData << "\n";
    // std::string moves{state["moves"].raw_json_token().value()};
    std::string moves = currentData;
    std::cout << "filled moves with string from data: \"" << moves << "\"\n";
    size_t poss, movePos, moveEnd;
    poss = movePos = moveEnd = 0;
    libchess::Move m;
    if (lastMoveSet) {
      std::string lastMoveStr = static_cast<std::string>(lastMove);
      movePos = moves.find(lastMoveStr);
      std::cout << "string before: " << moves << '\n';
      if (movePos != std::string::npos) {
        moveEnd = moves.find_last_of(' ');
        if (moveEnd < movePos) {
          std::cout << "clearing string because no space after " << lastMoveStr
                    << " in " << moves << '\n';
          std::cout << "movePos: " << movePos << ", moveEnd: " << moveEnd
                    << std::endl;
          std::cout << "moves[movePos] = " << moves.substr(movePos)
                    << std::endl;
          std::cout << "moves[moveEnd-2, moveEnd+2]: \""
                    << moves.substr(moveEnd - 2, 5) << "\" \n";
          moves.clear();
        } else {
          std::cout << "shortening string because "
                    << static_cast<std::string>(lastMove)
                    << " not the last move in " << moves << '\n';
          moves.erase(0, moveEnd + 1);
        }
        std::cout << "string after: " << moves << '\n';
        std::cout << "shortening string to remove already done moves\n";
      }
    } else {
      std::cout << "lastMove not set\n";
    }
    std::string singleMove;
    while ((poss = moves.find(' ')) != std::string::npos) {
      singleMove = moves.substr(0, poss);
      std::cout << "current move: " << singleMove << std::endl;
      pos.makemove(singleMove);
      moves.erase(0, poss + 1);
    }
    std::cout << "moves : " << moves << std::endl;
    if (moves.length() > 1) {
      pos.makemove(moves);
      moves.clear();
    }
    if (pos.history().size() > 0) {
      lastMove = pos.history().back().move;
      std::cout << "lastMove now set to" << lastMove << '\n';
      lastMoveSet = true;
    }
    if (pos.turn() != myColor) {
      std::cout << "not my turn, because " << pos.turn()
                << " is now to kove, but I am " << myColor << "\n";
      continue;
    } else {
      std::cout << "I am to move now\n";
    }
    lastMove = calculateMove(pos, 1);
    std::cout << "about to do the following move: " << lastMove << std::endl;
    lastMoveSet = true;
    if (!pos.is_legal(lastMove))
      return true;
    // TODO: in extra funktion auslagern?
    pos.makemove(lastMove);
    moveResp =
        cpr::Post(cpr::Url{"https://lichess.org/api/bot/game/" + id + "/move/" +
                           static_cast<std::string>(lastMove)},
                  authHeader);
    // hier auslagerung unnötig, nur ein functioncall
  }
  return true;
}

bool streamGameUtility(std::string id) {
  std::cout << "start of streamGameUtility\n";
  simdjson::ondemand::parser gameParser;
  simdjson::ondemand::document state;
  std::queue<std::string> sq;
  std::mutex mut;
  std::condition_variable convar;
  std::cout << "now beginning to stream game with id " << id << '\n';
  cpr::Response r = cpr::Get(
      cpr::Url("https://lichess.org/api/bot/game/stream/" + id), authHeader,
      cpr::WriteCallback([&gameParser, &state, &sq, &mut,

                          &convar](std::string data, intptr_t t) {
        fillGameStreamBuffer(data, t, gameParser, state, sq, mut, convar);
        std::cout << "fillGameStreamBuffer returned, returning true to resume "
                     "transfer\n";
        return true;
      }));
  return true;
}

bool fillGameStreamBuffer(std::string data, intptr_t,
                          simdjson::ondemand::parser &gameParser,
                          simdjson::ondemand::document &state,
                          std::queue<std::string> &sq, std::mutex &mut,
                          std::condition_variable &condvar) {

  std::cout << "input to fillGameStreamBuffer: " << data << '\n';
  if (data.length() < 5 || data.find('{') == std::string::npos)
    return true;
  data.reserve(data.length() + simdjson::SIMDJSON_PADDING);
  std::cout << "somthing larger arrived in fillGameStreamBuffer\n";
  state = gameParser.iterate(data);
  std::cout << "variable \"state\" initialized\n";
  std::string respType = sviewval_to_str(state["type"]);
  std::cout << respType << " arrived\n";
  if (respType == "gameFull") {
    std::cout << "gameFull arrived\n";
    std::string id{state["id"].get_string().take_value()};
    std::cout << "id initialized\n";
    std::string variant{state["variant"]["key"].get_string().take_value()};
    std::cout << "variant initialized\n";
    // TODO: check which id is mine
    libchess::Side mySide;
    std::string_view svSide = state["white"]["id"].get_string();
    // std::string l_myId = g_myId;

    if (svSide == g_myId) {
      mySide = libchess::Side::White;
    } else {
      mySide = libchess::Side::Black;
    }
    std::cout << "my side is " << mySide << "\n";
    std::string startfen = sviewval_to_str(state["initialFen"]);
    std::cout << "startfen initialized\n";
    libchess::Position startpos{startfen};
    std::cout << "discovered all but own side\n";
    std::cout << data << "\n<-data\n";
    // std::cout << std::string{std::string_view{state["state"]}} << "\n";
    // std::cout <<  sviewval_to_str(state.find_field_unordered("black")) <<
    // "\n";
    auto gameState = state["state"];
    std::cout << gameState.raw_json_token() << std::endl;
    // std::string tempData{state["state"].raw_json_token().take_value()};
    std::string tempData{gameState["moves"].get_string().take_value()};
    std::cout << tempData << std::endl;
    std::cout << "filled tempData\n";
    game thisGame{id, variant, startpos, mySide};
    std::cout << "created \"game\" variable\n";
    runningGames.push_back(std::async(std::launch::async, streamGame, thisGame,
                                      std::ref(sq), std::ref(condvar),
                                      std::ref(mut)));
    std::cout << "asynchronously started streamGame\n";
    stdoutmtx.lock();
    std::cout << tempData << "\n";
    sq.push(tempData);
    stdoutmtx.unlock();
    std::cout << "pushed to queue, locking mutex\n";
    std::unique_lock<std::mutex> lck(mut);
    std::cout << "acquired unique_lock, now notifying\n";
    condvar.notify_all();
    std::cout << "notified of arrival of new data\n\n\n ";
  } else if (respType == "gameState") {
    std::cout << "received gameState\n";
    std::string tempData{state["moves"].get_string().take_value()};
    stdoutmtx.lock();
    sq.push(tempData);
    stdoutmtx.unlock();
    std::unique_lock<std::mutex> lck(mut);
    condvar.notify_all();
    stdoutmtx.lock();
    std::cout << "notified of arrival of new data\n";
    stdoutmtx.unlock();
  }
  stdoutmtx.lock();
  std::cout << "exiting fillGameStreamBuffer, returning true\n\n\n\n\n";
  stdoutmtx.unlock();
  return true;
}

int evaluate_position(libchess::Position const &pos, const int &depth = 0) {
  if (pos.is_terminal()) {
    return pos.in_check() * -mateScore * (depth + 1);
  }
  if (pos.threefold() || pos.fiftymoves()) {
    // std::cout << "discovered draw, setting eval = 0\n";
    return 0;
  }
  int localEval = 0;
  int myEval = 0;
  int oppEval = 0;
  int myLostPieceVal = 0;
  int oppLostPieceVal = 0;
  int myUnprotectedVal = 0;
  const libchess::Side me = !pos.turn();
  const auto opp = !me;
  libchess::Bitboard myAttack = pos.squares_attacked(me);
  libchess::Bitboard myPieces = pos.occupancy(me);
  libchess::Bitboard oppAttack = pos.squares_attacked(opp);
  libchess::Bitboard oppPieces = pos.occupancy(opp);
  libchess::Bitboard myLost = ~myAttack & oppAttack & myPieces;
  libchess::Bitboard freeEstate = myAttack & ~oppAttack & oppPieces;
  for (int i = 0; i < 5; ++i) {
    const int multiplier = pieceToVal[i];
    const auto mySpecPiece = pos.pieces(me, libchess::Piece(i));
    const auto oppSpecPiece = pos.pieces(opp, libchess::Piece(i));
    myEval += multiplier * mySpecPiece.count();
    oppEval += multiplier * oppSpecPiece.count();
    myLostPieceVal += multiplier * (myLost & mySpecPiece).count();
    oppLostPieceVal += multiplier * (freeEstate & oppSpecPiece).count();
    myUnprotectedVal += multiplier * (~myAttack & mySpecPiece).count();
  }
  localEval += myEval - oppEval;
  localEval += (oppLostPieceVal - myLostPieceVal) / 2;
  localEval -= myUnprotectedVal / 8;
  if (pos.in_check()) {
    localEval -= 50;
  }
  return localEval;
}

int escalating_eval(libchess::Position const &pos, const int depth) {
  if (pos.is_terminal()) {
    return pos.in_check() * -mateScore * (depth + 1);
  }
  if (pos.threefold() || pos.fiftymoves()) {
    return 0;
  }
  int myPieceEval = 0;
  int oppPieceEval = 0;
  const auto me = pos.turn();
  const auto opp = !me;
  const auto myPieces = pos.occupancy(me);
  const auto oppPieces = pos.occupancy(opp);
  for (int i = 0; i < 5; ++i) {
    const int multiplier = pieceToVal[i];
    const auto piecePositions = pos.occupancy(libchess::Piece(i));
    myPieceEval += multiplier * (piecePositions & myPieces).count();
    oppPieceEval += multiplier * (piecePositions & oppPieces).count();
  }
  int totalEval = myPieceEval - oppPieceEval;
  return totalEval;
}

int negamax(libchess::Position pos, const int depth = 1, int alpha = INT32_MIN,
            int beta = INT32_MAX, const int maxDepth = 1) {
  int bestEval = INT32_MIN;
  auto legalMoves = pos.legal_moves();
  if (depth <= 0 || legalMoves.empty()) {
    return escalating_eval(pos, depth);
  }
  if (depth == maxDepth) {
    std::cout << legalMoves.size() << " legmovs in loop at depth " << depth
              << '\n';
  }
  // std::shuffle(legalMoves.begin(), legalMoves.end(), rng);
  const auto moves = legalMoves;
  for (const auto &move : moves) {
    pos.makemove(move);
    bestEval =
        std::max(bestEval, -negamax(pos, depth - 1, -beta, -alpha, maxDepth));
    pos.undomove();
    if (depth == maxDepth) {
      std::cout << " depth: " << depth << ", move: " << move
                << ", bestEval: " << bestEval << '\n';
    }
    alpha = std::max(alpha, bestEval);
    if (alpha >= beta) {
      break;
    }
  }
  return alpha;
}

libchess::Move topLevelNegamax(libchess::Position pos, const int depth = 1,
                               int alpha = INT32_MIN,
                               const int beta = INT32_MAX) {
  const auto moves = pos.legal_moves();
  if (moves.empty()) {
    libchess::Move move;
    return move;
  }
  int bestEval = INT32_MIN;
  libchess::Move bestMove;
  for (const auto &move : moves) {
    pos.makemove(move);
    int eval = -negamax(pos, depth - 1, -beta, -alpha, depth);
    pos.undomove();
    if (eval > bestEval) {
      bestEval = eval;
      bestMove = move;
    }
    alpha = std::max(alpha, eval);
    if (alpha >= beta) {
      break;
    }
  }
  return bestMove;
}

int verySimpleEval(libchess::Position pos) {
  if (pos.is_terminal()) {
    return pos.is_checkmate() * -mateScore;
  }
  if (pos.threefold() || pos.fiftymoves()) {
    return 0;
  }
  int toMoveCount = pos.occupancy(pos.turn()).count();
  int oppCount = pos.occupancy(!pos.turn()).count();
  return toMoveCount - oppCount;
}

int negamaxNoAB(libchess::Position pos, libchess::Move &bestMove, int depth) {
  int bestEval = INT32_MIN;
  // tmp variable to avoid overwriting progress from greater depths
  libchess::Move tmpBestMove;
  const auto moves = pos.legal_moves();
  if (depth <= 0 || moves.empty()) {
    return escalating_eval(
        pos, depth); // evaluate_position(pos); //verySimpleEval(pos);
  }
  libchess::Move tmpCurrentMove;
  for (auto const &move : moves) {
    pos.makemove(move);
    int localEval = -negamaxNoAB(pos, tmpCurrentMove, depth - 1);
    pos.undomove();
    if (localEval > bestEval) {
      bestEval = localEval;
      tmpBestMove = move;
    }
  }
  bestMove = tmpBestMove;
  return bestEval;
}

libchess::Move calculateMove(libchess::Position pos, int depth) {
  libchess::Move lastMove;
  auto legalMoves = pos.legal_moves();
  auto possibleCaptures = pos.legal_captures();

  // depth = 3;
  std::cout << "depth in calculateMove: " << depth << '\n';
  const auto bestMove = topLevelNegamax(pos, depth, INT32_MIN, INT32_MAX);
  // int bestMoveEval = negamaxNoAB(pos, bestMove, depth);
  if (legalMoves.size() == 0) {
    lastMove = pos.parse_move("a1a1");
  } else {
    lastMove = bestMove;
    std::cout << "chose Move " << bestMove << '\n';
  }
  return lastMove;
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
    std::string_view svSide = state["white"]["id"].get_string();
    if (svSide == g_myId) {
      thisGame.mySide = libchess::Side::White;
      std::cout << "I am white\n";
    } else {
      thisGame.mySide = libchess::Side::Black;
      std::cout << "I am black\n";
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
        std::min(timeLeft / 900, timeLeft / 2) * pos.history().size() / 2;
    fieldstr = side + "inc";
    std::cout << "fieldstr now is " << fieldstr << '\n';
    availableTime += state[fieldstr].get_uint64().take_value();
    availableTime = std::min(timeLeft - 2000, availableTime);
    status = std::string{state["status"].get_string().take_value()};
  } else {
    timeLeft = stat[fieldstr].get_int64().take_value();
    availableTime +=
        std::min(timeLeft / 900, timeLeft / 2) * pos.history().size() / 2;
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
  mstime = std::max(mstime, mstime / pos.legal_moves().size() * 25);
  availableTime = std::min(availableTime, timeLeft);
  if (mstime == 0 || availableTime / mstime > 20) {
    ++currentDepth;
  } else if ((mstime - 2000) > timeLeft ||
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
