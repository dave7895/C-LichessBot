#include "engine.h"
std::random_device rd;
std::default_random_engine rng(rd());
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
  // value of my pieces
  int myPieceEval = 0;
  // value of opponents pieces
  int oppPieceEval = 0;
  // value of pieces that are mine, but under attack and unprotected
  int myLostPieceVal = 0;
  // value of pieces that can be taken without direct repercussions
  int oppLostPieceVal = 0;
  // value of pieces that are neither protected nor attacked
  int myUnprotectedSafeVal = 0;
  int oppUnprotectedSafeVal = 0;
  const auto me = pos.turn();
  const auto opp = !me;
  const auto myPieces = pos.occupancy(me);
  const auto oppPieces = pos.occupancy(opp);
  const auto myAttack = pos.squares_attacked(me);
  const auto oppAttack = pos.squares_attacked(opp);
  const auto myUnprotectedUnderAttack = ~myAttack & oppAttack;
  const auto oppUnprotectedUnderAttack = myAttack & ~oppAttack;
  const auto myUnprotectedButSafe = ~myAttack & ~oppAttack;
  const auto oppUnprotectedButSafe = myUnprotectedButSafe;
  for (const auto i : libchess::pieces) {
    const int multiplier = pieceToVal[i];
    // const auto piecePositions = pos.occupancy(i);
    const auto myPiecePositions = pos.pieces(me, i);
    const auto oppPiecePositions = pos.pieces(opp, i);
    myPieceEval += multiplier * pos.pieces(me, i).count();
    oppPieceEval += multiplier * pos.pieces(opp, i).count();
    myLostPieceVal +=
        multiplier * (myPiecePositions & myUnprotectedUnderAttack).count();
    oppLostPieceVal +=
        multiplier * (oppPiecePositions & oppUnprotectedUnderAttack).count();
    if (i < 5) {
      myUnprotectedSafeVal +=
          multiplier * (myPiecePositions & myUnprotectedButSafe).count();
      oppUnprotectedSafeVal +=
          multiplier * (oppPiecePositions & oppUnprotectedButSafe).count();
    }
  }
  int myTotalEval = myPieceEval;
  int oppTotalEval = oppPieceEval;
  myTotalEval -= pos.in_check() * 50;
  // a piece that is unprotected has only a quarter the value of a protected
  // piece
  oppTotalEval += (myLostPieceVal / 2); // * 3) / 4;
  myTotalEval += (oppLostPieceVal / 2); // * 3) / 4;
  // pieces that are neither protected nor attacked are worth slightly less, due
  // to the potential danger
  myTotalEval -= myUnprotectedSafeVal / 16;
  oppTotalEval -= oppUnprotectedSafeVal / 16;
  return myTotalEval - oppTotalEval;
}

int negamax(libchess::Position &pos, int depth = 1, int alpha = -INT32_MAX,
            int beta = INT32_MAX, const int maxDepth = 1) {
  int bestEval = INT32_MIN;
  const auto moves = pos.legal_moves();
  if (depth <= 0 || moves.empty()) {
    return escalating_eval(pos, depth);
  }
  if (pos.threefold() || pos.fiftymoves()) {
    return 0;
  }
  if (pos.in_check()){
    depth++;
  }

  if (depth == maxDepth) {
    std::cout << moves.size() << " legmovs in loop at depth " << depth << '\n';
  }
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

libchess::Move topLevelNegamax(libchess::Position &pos, const int depth,
                               const bool alphabeta, int alpha,
                               const int beta) {
  const bool info = true;
  auto legalMoves = pos.legal_moves();
  if (legalMoves.empty()) {
    libchess::Move move;
    return move;
  }
  // std::shuffle(legalMoves.begin(), legalMoves.end(), rng);
  const auto moves = legalMoves;
  int bestEval = -INT32_MAX;
  libchess::Move bestMove;
  info &&std::cout << "evaluating " << moves.size() << " moves at depth "
                   << depth << '\n';
  if (alphabeta) {
    for (const auto &move : moves) {
      pos.makemove(move);
      info &&std::cout << "evaluating move " << move;
      int eval = -negamax(pos, depth - 1, -beta, -alpha, depth);
      info &&std::cout << ", eval is " << eval << '\n';
      pos.undomove();
      if (eval > bestEval) {
        bestEval = eval;
        bestMove = move;
      }
      // alpha = std::max(alpha, eval);
      if (alpha >= beta) {
        break;
      }
    }
  } else {
    for (const auto &move : moves) {
      pos.makemove(move);
      info &&std::cout << "evaluating move " << move;
      int eval = -negamaxNoAB(pos, depth - 1);
      info &&std::cout << ", eval is " << eval << '\n';
      pos.undomove();
      if (eval > bestEval) {
        bestEval = eval;
        bestMove = move;
      }
    }
  }
  return bestMove;
}

int simpleEval(libchess::Position pos) {
  if (pos.is_terminal()) {
    return pos.is_checkmate() * -mateScore;
  }
  if (pos.threefold() || pos.fiftymoves()) {
    return 0;
  }
  int totalEval = 0;
  for (const auto piece : libchess::pieces) {
    const int multiplier = pieceToVal[piece];
    totalEval += multiplier * pos.pieces(pos.turn(), piece).count();
    totalEval -= multiplier * pos.pieces(!pos.turn(), piece).count();
  }
  return totalEval;
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

int negamaxNoAB(libchess::Position pos, int depth) {
  int bestEval = INT32_MIN;
  // tmp variable to avoid overwriting progress from greater depths
  const auto moves = pos.legal_moves();
  if (depth <= 0 || moves.empty()) {
    return escalating_eval(pos, depth); // simpleEval(pos);
  }
  for (auto const &move : moves) {
    pos.makemove(move);
    int localEval = -negamaxNoAB(pos, depth - 1);
    pos.undomove();
    if (localEval > bestEval) {
      bestEval = localEval;
    }
  }
  return bestEval;
}

libchess::Move calculateMove(libchess::Position pos, int depth) {
  libchess::Move lastMove;
  auto legalMoves = pos.legal_moves();
  auto possibleCaptures = pos.legal_captures();

  // depth = 3;
  std::cout << "depth in calculateMove: " << depth << '\n';
  const auto bestMove = topLevelNegamax(pos, depth, true);
  // int bestMoveEval = negamaxNoAB(pos, bestMove, depth);
  if (legalMoves.size() == 0) {
    lastMove = pos.parse_move("a1a1");
  } else {
    lastMove = bestMove;
    std::cout << "chose Move " << bestMove << '\n';
  }
  return lastMove;
}
