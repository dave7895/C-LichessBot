#include "playGames.h"

std::string sviewval_to_str(
    simdjson::simdjson_result<simdjson::fallback::ondemand::value> view) {
  return std::string{std::string_view{view}};
}

std::map<int, int> pieceToVal{{0, 1}, {1, 3}, {2, 3}, {3, 5}, {4, 9}, {5, 25}};

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

int get_best_move(libchess::Position pos,
                  std::vector<libchess::Move> possibleMoves,
                  libchess::Move &bestCapture, int depth = 1, int argCoeff = 1,
                  int maxDepth = 1) {
  libchess::Move bestMove;
  int bestEval = INT32_MIN;
  int localEval;
  libchess::Move localMove;
  int coeff = 1;
  if (possibleMoves.empty()) {
    return pos.in_check() * 10000 * argCoeff * depth;
  }
  if (depth == 1) {
    for (auto move : possibleMoves) {
      // std::cout << "evaluating move " << move << " at depth " << depth <<
      // '\n';
      localEval = 0;
      if (!pos.is_legal(move)) {
        localEval -= 20000;
      }
      pos.makemove(move);
      /*if (move.is_capturing()) {
        localEval += move.captured() + 1;
      }*/
      int myEval = 0;
      int oppEval = 0;
      int myLostPieceVal = 0;
      int oppLostPieceVal = 0;
      libchess::Bitboard myAttack = pos.squares_attacked(!pos.turn());
      libchess::Bitboard myPieces = pos.occupancy(!pos.turn());
      libchess::Bitboard oppAttack = pos.squares_attacked(pos.turn());
      libchess::Bitboard oppPieces = pos.occupancy(pos.turn());
      libchess::Bitboard myLost = ~myAttack & oppAttack & myPieces;
      libchess::Bitboard freeEstate = myAttack & ~oppAttack & oppPieces;
      int multiplier;
      libchess::Bitboard mySpecPiece, oppSpecPiece;
      for (int i = 0; i < 5; ++i) {
        multiplier = pieceToVal[i];
        mySpecPiece = pos.pieces(!pos.turn(), libchess::Piece(i));
        oppSpecPiece = pos.pieces(pos.turn(), libchess::Piece(i));
        myEval += multiplier * mySpecPiece.count();
        oppEval += multiplier * oppSpecPiece.count();
        myLostPieceVal += multiplier * (myLost & mySpecPiece).count();
        oppLostPieceVal += multiplier * (freeEstate & oppSpecPiece).count();
      }
      localEval += myEval - oppEval;
      localEval += (oppLostPieceVal - myLostPieceVal) / 2;
      if (pos.in_check())
        localEval += 1;
      /* std::cout << "eval after checking for capture value: " << localEval <<
       '\n';
      if (pos.square_attacked(move.to(), pos.turn())) {
        localEval -= move.piece() + 1;
      } else if (move.is_promoting()) {
        localEval += move.promotion();
      }
      // std::cout << "eval after checking if piece is threatened: " <<
      // localEval << '\n';*/

      if (pos.legal_moves().empty()) {
        /*std::cout << "     no legal moves found." << pos << "\n";
        std::cout << pos.in_check() << '\n';*/
        if (pos.in_check()) {
          // std::cout << "found checkmate with move " << move << " \n";
          localEval = 10000;
        } else {
          localEval = 0;
        }
        /*std::cout << "eval after checking if piece is threatened: " <<
           localEval
                  << '\n';*/
        if (pos.threefold() || pos.fiftymoves()) {
          std::cout << "discovered draw, setting eval = 0\n";
          localEval = 0;
        }
      }
      pos.undomove();
      if (localEval > bestEval) {
        // std::cout << "replacing bestEval: " << localEval << ", " << move <<
        // '\n';
        bestEval = localEval;
        bestMove = move;
      }
    }

    coeff = argCoeff;
  } else {
    auto legalMoves = pos.legal_moves();
    if (depth == maxDepth) {
      std::cout << legalMoves.size() << "legmovs in loop at depth " << depth
                << '\n';
    }
    for (auto move : possibleMoves) {
      localEval = 0;
      pos.makemove(move);
      legalMoves = pos.legal_moves();
      localEval = -get_best_move(pos, legalMoves, localMove, depth - 1,
                                 -argCoeff, maxDepth);
      pos.undomove();
      /*std::cout << "current eval: " << localEval << ", " << move << " at depth
      "
                << depth << '\n';
      std::cout << "current bestEval: " << bestEval << ", " << bestMove
                << " at depth " << depth << '\n';*/
      if (depth == maxDepth) {
        std::cout << " depth: " << depth << ", move: " << move
                  << ", eval: " << localEval << '\n';
      }
      if (localEval > bestEval) {
        /*std::cout << "replacing bestEval: " << localEval << ", " << move
                  << " at depth " << depth << '\n';*/
        bestEval = localEval;
        bestMove = move;
      }
    }
  }
  if (possibleMoves.size() == 0) {
    /*std::cout << "     no legal moves found." << pos << "\n";
    std::cout << (pos.in_check()) << '\n';*/
    if (pos.in_check()) {
      bestMove = pos.history().back().move;
      // std::cout << "found checkmate with move " << bestMove << " \n";
      bestEval = 10000;
    } else {
      bestEval = 0;
    }
  }
  /*std::cout << "returning bestEval: " << coeff * bestEval << ", " << bestMove
            << " from depth " << depth << '\n';*/
  bestCapture = bestMove;
  return coeff * bestEval;
}

libchess::Move calculateMove(libchess::Position pos, int depth) {
  libchess::Move lastMove;
  auto legalMoves = pos.legal_moves();
  auto possibleCaptures = pos.legal_captures();
  /*auto freeEstate = pos.squares_attacked(pos.turn());
  std::cout << freeEstate << '\n';
  freeEstate &= ~pos.squares_attacked(!pos.turn());
  libchess::Bitboard attacked = pos.squares_attacked(!pos.turn());
  std::cout << "attacked squares: \n" << attacked << '\n';
  std::cout << "not attacked squares: " << ~attacked.value() << '\n';
  std::cout << freeEstate << '\n';
  freeEstate &= pos.occupancy(!pos.turn());
  std::cout << freeEstate << '\n';
  std::cout << "legalMoves: " << legalMoves.size()
            << " possibleCaptures: " << possibleCaptures.size() << "\n";
  libchess::Piece p;
  auto possibleFreeCaps = pos.legal_moves();
  possibleFreeCaps.clear();
  if (freeEstate) {
    for (auto sq : freeEstate) {
      if (!sq)
        continue;
      std::cout << "found square with unprotected: " << sq << '\n';
      auto att = pos.attackers(sq, pos.turn());
      for (auto frSq : att) {
        if (!frSq) {
          continue;
        }
        libchess::Piece piece = pos.piece_on(frSq);
        libchess::Move m = pos.parse_move(std::string{frSq} + std::string{sq});
        if (pos.is_legal(m))
          possibleFreeCaps.push_back(m);
      }
    }
  } else {
    std::cout << "no freeEstate available\n";
  }
//sort captures by capturing and
  std::sort(possibleCaptures.begin(), possibleCaptures.end(),
            [](libchess::Move m1, libchess::Move m2) {
              return m1.captured() > m2.captured();
            });
  std::sort(possibleCaptures.begin(), possibleCaptures.end(),
            [](libchess::Move m1, libchess::Move m2) {
              return m1.piece() > m2.piece();
            });

            std::sort(possibleCaptures.begin(), possibleCaptures.end(),
                      [](libchess::Move m1, libchess::Move m2) {
                        return m1.captured() > m2.captured();
                      });
            std::sort(possibleCaptures.begin(), possibleCaptures.end(),
                      [](libchess::Move m1, libchess::Move m2) {
                        return m1.piece() > m2.piece();
                      });*/
  /*if (!possibleFreeCaps.empty()) {
    std::cout << "possible Free Caps available: " << possibleFreeCaps.size()
              << '\n';
    lastMove = possibleFreeCaps[0];
  } else */
  libchess::Move bestMove;
  // depth = 2;
  int firstFactor = 2 * (depth % 2) - 1;
  int bestMoveEval =
      get_best_move(pos, legalMoves, bestMove, depth, firstFactor, depth);
  if (legalMoves.size() == 0) {
    lastMove = pos.parse_move("a1a1");
  } else {
    lastMove = bestMove;
    std::cout << "chose Move " << bestMove << "with eval " << bestMoveEval
              << '\n';
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
  if (statOrState) {
    size_t timeLeft = state[fieldstr].get_int64().take_value();
    availableTime += timeLeft / 900 * pos.history().size()/2;
    fieldstr = side + "inc";
    std::cout << "fieldstr now is " << fieldstr << '\n';
    availableTime += state[fieldstr].get_uint64().take_value();
    availableTime = std::min(timeLeft - 2000, availableTime);
    status = std::string{state["status"].get_string().take_value()};
  } else {
    size_t timeLeft = stat[fieldstr].get_int64().take_value();
    availableTime += timeLeft / 90 * pos.history().size();
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
  if (mstime == 0 || availableTime / mstime > 20) {
    ++currentDepth;
  } else if ((mstime - 1000) > availableTime) {
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
