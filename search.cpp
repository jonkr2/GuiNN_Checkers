//
// Search.cpp
// by Jonathan Kreuzer
//
// Alpha-Beta search and related functionality.
// TODO : lazy SMP support
// 

#include <stdlib.h>
#include <algorithm>
#include "cb_interface.h"
#include "engine.h"
#include "guiWindows.h"
#include "kr_db.h"

const int kPruneVerifyDepthReduction = 4;
const int kPruneEvalMargin = 28;

const int kLmrMoveCount = 3;

// -------------------------------------------------
// Repetition Testing
// -------------------------------------------------
// Check if there was a previous board with the same hashkey (can assume with almost certainty that it was same board)
bool inline Repetition(const uint64_t hashKey, uint64_t boardHashHistory[], int start, int end)
{
	int i;
	if (start > 0) i = start; else i = 0;

	// (only checks every other move, because side to move must be the same)
	if ((i & 1) != (end & 1)) i++;

	end -= 2;
	for (; i < end; i += 2)
		if (boardHashHistory[i] == hashKey) return true;

	return false;
}

// Principal Variation
std::string SPrincipalVariation::ToString()
{
	std::string ret;
	for ( int i = 0; i < count; i++	)
		ret += Transcript::GetMoveString(moves[i]) + "  ";

	return ret;
}

// Do a move on the board, and also update the first layer neural network values
int DoMove( const Move& move, SearchThreadData& search, int ply )
{
	SearchStackEntry* const stack = search.stack;
	EvalNetInfo& netInfo = stack[ply].netInfo;
	Board& board = stack[ply].board;

	int ret = board.DoMove(move);

	netInfo.netIdx = (int)CheckersNet::GetGamePhase(board);
	if ( netInfo.netIdx >= 0 )
	{
		if (netInfo.netIdx == stack[ply-1].netInfo.netIdx)
		{
			// incrementally update first layer net values
			memcpy( netInfo.firstLayerValues, stack[ply-1].netInfo.firstLayerValues, netInfo.valueCount * sizeof(nnInt_t));
			engine.evalNets[netInfo.netIdx]->IncrementalUpdate(move, stack[ply-1].board, netInfo.firstLayerValues);

			// Verify our values are an exact match
		/*	nnInt_t firstLayerValues[kMaxValuesInLayer];
			int numValues = engine.evalNets[netInfo.netIdx]->ComputeFirstLayerValues(board, search.nnValues, firstLayerValues);
			for (int i = 0; i < numValues; i++)
			{
				assert(firstLayerValues[i] == netInfo.firstLayerValues[i]);
			}*/
		} else {
			// fully recompute first layer net values
			engine.evalNets[netInfo.netIdx]->ComputeFirstLayerValues(board, search.nnValues, netInfo.firstLayerValues);
		}
	}
	return ret;
}

// -------------------------------------------------
// Quiescence Search... Search all jumps, if there are no jumps, stop the search
// -------------------------------------------------
int QuiesceBoard(SearchThreadData& search, int ply, int alpha, int beta )
{
	SearchStackEntry* const stack = search.stack;
	MoveList& moveList = stack[ply].moveList;
	Board& inBoard = stack[ply-1].board;
	stack[ply].pv.Clear();

	const uint32_t jumpers = inBoard.Bitboards.GetJumpers( inBoard.sideToMove );

	// No more jump moves, return evaluation
	if ( jumpers == 0 || (ply+1) >= MAX_SEARCHDEPTH )
	{
		search.displayInfo.selectiveDepth = std::max(search.displayInfo.selectiveDepth, ply-1 );

		return inBoard.EvaluateBoard( ply-1, search, stack[ply-1].netInfo, 0 );
	}

	// There are jump moves, so we keep searching. 
	// (Note : jumps are mandatory, so we can't stand-pat with boardEval >= beta like in chess.)
	moveList.FindJumps(inBoard.sideToMove, inBoard.Bitboards, jumpers);

	assert(moveList.numJumps);
	for (int i = 0; i < moveList.numJumps; i++)
	{
		const Move& move = moveList.moves[i];

		stack[ply].board = inBoard;

		DoMove(move, search, ply);
		search.displayInfo.nodes++;

		// Recursive Call
        int value = -QuiesceBoard( search, ply+1, -beta, -alpha);

		// Keep Track of Best Move and Alpha-Beta Prune
		if (value >= beta) { return beta; }
		if (value > alpha ) { 
			stack[ply].pv.Set(move, stack[ply + 1].pv);
			alpha = value; 
		}
	}

	return alpha;
}
// -------------------------------------------------
//  returns true if time has run out or the interface has asked the engine to stop the search
// -------------------------------------------------
inline bool CheckTimeUp(SearchThreadData& search)
{
	// was the search asked to stop?
	if (checkerBoard.bActive && *checkerBoard.pbPlayNow) return true;
	if (engine.bStopThinking) return true;

	search.displayInfo.nodesLastUpdate = search.displayInfo.nodes;

	// If time has run out, we allow running up to 2*Time if g_bEndHard == FALSE and we are still searching a depth
	float elapsedTime = TimeSince(search.displayInfo.startTimeMs);
	if (elapsedTime > engine.searchLimits.maxSeconds)
		if (elapsedTime > (2 * engine.searchLimits.maxSeconds * engine.searchLimits.panicExtraMult) || engine.searchLimits.bEndHard || search.displayInfo.searchingMove == 0 || abs(search.displayInfo.eval) > 1500)
			return true;

	if (TimeSince(search.displayInfo.lastDisplayTimeMs) > .4f) {
		search.displayInfo.lastDisplayTimeMs = GetCurrentTimeMs();
		RunningDisplay(search.displayInfo, NO_MOVE, 1);
	}
	return false;
}

// -------------------------------------------------
// FirstPlyMoveUpdate
// -------------------------------------------------
inline void FirstPlyMoveUpdate(SearchThreadData& search, int newEval, Move bestmove, int movesSearched )
{
	search.displayInfo.searchingMove = movesSearched;

	// On dramatic changes (such as fail lows) make sure to finish the iteration
	if (abs(newEval) < 3000)
	{
		if (abs(newEval - search.displayInfo.eval) >= 26) engine.searchLimits.panicExtraMult = 1.8f;
		if (abs(newEval - search.displayInfo.eval) >= 50) engine.searchLimits.panicExtraMult = 2.5f;
		if (movesSearched > 0) { search.displayInfo.eval = newEval; }
	}
	if (search.displayInfo.depth > 11 && !engine.bStopThinking) {
		RunningDisplay(search.displayInfo, bestmove, 1);
	}
}

// -------------------------------------------------
//  Alpha Beta Search with Transposition(Hash) Table
// -------------------------------------------------
int ABSearch( SearchThreadData& search, int32_t ply, int32_t depth, int32_t alpha, int32_t beta, bool isPV, Move &bestmove)
{
	assert(ply >= 1);
	assert(beta > alpha);
	// Check to see if move time has run out every 20000 nodes
	if (search.displayInfo.nodes > search.displayInfo.nodesLastUpdate + 20000 )
	{
		if (CheckTimeUp(search)) return TIMEOUT;
	}

	// Internal Iterative Deepening : set predictedBestmove if there's no bestmove already
	Move predictedBestmove = bestmove;
	if (predictedBestmove == NO_MOVE && depth > 4 - isPV ) {
		int val = ABSearch( search, ply, std::max(depth-4,1), alpha, beta, false, predictedBestmove);
		if (val == TIMEOUT) return TIMEOUT;
	}

	// Accessors for the stack variables
	SearchStackEntry* const stack = search.stack;
	MoveList& moveList = stack[ply].moveList;
	Board& board = stack[ply].board;
	Move& killerMove = stack[ply].killerMove;
	const int prevOppMoveIdx = stack[ply - 1].historyMoveIdx;
	const int prevOwnMoveIdx = ply >= 2 ? stack[ply - 2].historyMoveIdx : 0;

	stack[ply].pv.Clear();
	const int startAlpha = alpha;

	// Find possible moves (and set a couple variables)
	Board &board_in = stack[ply - 1].board;
	const eColor color_in = board_in.sideToMove;
	moveList.FindMoves(board_in);

	if (moveList.numMoves == 0) { 
		return -WinScore( ply - 1 ); // If you can't move, you've already lost the game
	}
	alpha = std::max(alpha, -WinScore(ply));
	if (alpha >= beta) return alpha;
	if (alpha >= WinScore(ply)) return alpha; // have a guaranteed faster win already, so don't waste time searching

	/* Check for egdb cutoff at interior nodes. */
	if (ply > 2 && engine.dbInfo.type == dbType::KR_WIN_LOSS_DRAW && engine.dbInfo.InDatabase(board_in)) {
		int egdb_score;
		EGDB_BITBOARD bb;

		gui_to_kr(board_in.Bitboards, bb);
		int result = engine.dbInfo.kr_wld->lookup(engine.dbInfo.kr_wld, &bb, gui_to_kr_color(color_in), depth <= 3);
		if (result == EGDB_WIN) {
			search.displayInfo.databaseNodes++;
			egdb_score = board_in.dbWinEval(color_in == WHITE ? WHITEWIN : BLACKWIN);
			egdb_score = to_rel_score(egdb_score, color_in);

			// This might be aggressive, as it might be possible for the score to temporarily go down by
			// searching deeper. But it's a db win, and the endgame heuristic score will usually only go up.
			if (egdb_score >= beta)
				return(beta);
		}
		else if (result == EGDB_DRAW) {
			search.displayInfo.databaseNodes++;
			return(0);
		}
	}

	// Use killer as predictedBest if we don't have it from some other method
	if (predictedBestmove == NO_MOVE && moveList.numJumps == 0 && killerMove != NO_MOVE) {
		predictedBestmove = killerMove;
	}

	// If predicted best exists, swap it to front
	int sortOnMoveIdx = 0;
	if (predictedBestmove != NO_MOVE &&
		moveList.SwapToFront(predictedBestmove))
	{
		sortOnMoveIdx = 1;	
	}

	// Run through moveList, searching each move
	Move searchedMoves[MoveList::MAX_MOVES];
	int movesSearched = 0;

	for (int i = 0; i < moveList.numMoves; i++)
	{
		if (i == sortOnMoveIdx) 
		{
			// Sort the remaining moves
			if (moveList.numJumps > 1 + sortOnMoveIdx) { moveList.SortJumps(i, board.Bitboards); }
			else if (moveList.numJumps == 0) { HistorySort(search.historyTable, moveList, i,  board, prevOppMoveIdx, prevOwnMoveIdx); }
		}
		
		const Move move = moveList.moves[i];

		if (ply == 1) 
		{
			const int newEval = (color_in == WHITE) ? alpha : -alpha; // eval from red's POV
			FirstPlyMoveUpdate( search, newEval, bestmove, movesSearched );
		}
			 
		// Play the move (after resetting board to the one from previous ply)
        board = stack[ply-1].board;
		const ePieceType movedPiece = board.GetPiece( move.Src() );
        const int unreversible = DoMove(move, search, ply);
	
		search.displayInfo.nodes++;

		search.boardHashHistory[ engine.transcript.numMoves + ply] = board.hashKey;
		searchedMoves[movesSearched++] = move;
		stack[ply].historyMoveIdx = search.historyTable.MoveIdx(move.Src(), move.Dir(0), movedPiece );
		stack[ply + 1].pv.Clear();

        // If the game is over after making this move, return a gameover value now
		if (board.Bitboards.P[WHITE] == 0 || board.Bitboards.P[BLACK] == 0)
		{
			bestmove = move;
			stack[ply].pv.Set(move);
            return WinScore( ply );
        }

		int value = INVALID_VAL;
		int nextDepth = depth - 1;

		// EXTENSIONS
		if (moveList.numJumps > 0 &&
			(isPV || (ply > 1 && stack[ply - 1].moveList.numJumps > 0)))
			nextDepth++; // Any jump "recapture" or jump on PV
		else if (nextDepth == 0 && board.Bitboards.GetJumpers(color_in))
			nextDepth++; // On leaf when there is a jump threatened
	
		if (ply > 1 && board.reversibleMoves > 78 && board.Bitboards.GetJumpers(board.sideToMove) == 0) 
		{ 
			value = 0;  // draw by 40-move rule value.. Not sure actual rules checkerboard calls draws this way sometimes though
		}
		else if (nextDepth >= 1 && ply > 1 && Repetition(board.hashKey, search.boardHashHistory, engine.transcript.numMoves + ply - board.reversibleMoves, engine.transcript.numMoves + ply))
		{
			value = 0; 	// If this is the repetition of a position that has occured already in the search, return a draw score
		}
		else if ( (nextDepth <= 0 || ply >= MAX_SEARCHDEPTH) )
		{
			// quiesce (play jumps) then evaluate the position
			value = -QuiesceBoard( search, ply+1, -beta, -alpha );
	    }
        else 
		{	

			// If this isn't the max depth continue to look ahead 
			Move nextBestmove = NO_MOVE;
			int boardEval = INVALID_VAL;

		   	// TRANSPOSITION TABLE : Look up the this Position and check for value or bestMove
			TEntry* ttEntry = nullptr;
			if (engine.bUseHashTable)
			{
				ttEntry = engine.TTable.GetEntry( board, engine.ttAge );
				ttEntry->Read( board.hashKey, alpha, beta, nextBestmove, value, boardEval, nextDepth, ply);
				if (isPV && ply < 4) value = INVALID_VAL; // don't tt prune at beginning of PV, might miss repetition
			}
			if (value != INVALID_VAL && value <= alpha) { continue; }

			if (value == INVALID_VAL && nextDepth > 2 && ply >= 3)
			{
				// DATABASE : Stop searching if we know the exact value from the database
				if (engine.dbInfo.InDatabase( board ) )
				{
					if (boardEval == INVALID_VAL) { boardEval = -board.EvaluateBoard(ply, search, stack[ply].netInfo, nextDepth); }
					if (boardEval == 0 || (engine.dbInfo.type == dbType::EXACT_VALUES && abs(boardEval) > MIN_WIN_SCORE) )
						value = boardEval;
				}

				// PRUNING : Prune this move if the eval is enough above beta and shallower verification search passes
				if (!isPV && value == INVALID_VAL && beta > -1500 && (!engine.dbInfo.loaded || board.TotalPieces() > engine.dbInfo.numPieces))
				{
					if (boardEval == INVALID_VAL) {
						boardEval = -board.EvaluateBoard(ply, search, stack[ply].netInfo, nextDepth);
						if (ttEntry) { ttEntry->m_boardEval = boardEval; }
					}

					if (boardEval >= beta + kPruneEvalMargin)
					{
						const int verifyDepth = std::max(nextDepth - kPruneVerifyDepthReduction, 1);
						int verifyValue = -ABSearch( search, ply + 1, verifyDepth, -(beta + kPruneEvalMargin + 1), -(beta + kPruneEvalMargin), false, nextBestmove);
						if (verifyValue == -TIMEOUT) return TIMEOUT;
						if (verifyValue > beta + kPruneEvalMargin) value = verifyValue;
					}
				}
			}

            // If value wasn't set from the Transposition Table or Pruning, search this position
            if (value == INVALID_VAL)
			{
				// 1-ply LMR when not on pv
				const bool doLMR = !isPV && moveList.numJumps == 0 && movesSearched >= kLmrMoveCount;

				// Principal Variation Search - if this isn't first move on PV, use a search window width of 1
				if (!isPV || movesSearched > 1)
				{
					const int pvsDepth = nextDepth - doLMR;
					value = -ABSearch( search, ply + 1, pvsDepth, -(alpha + 1), -alpha, false, nextBestmove);
					if (value == -TIMEOUT) return TIMEOUT;
				}

				// full-width and depth search
				if (value == INVALID_VAL || (value > alpha && (value < beta || doLMR) ) )
				{
					value = -ABSearch( search, ply + 1, nextDepth, -beta, -alpha, isPV, nextBestmove);
					if (value == -TIMEOUT) return TIMEOUT;
				}

				// Store for the search info for this position in the tranposition table entry
				if (ttEntry) {
					ttEntry->Write(board.hashKey, alpha, beta, nextBestmove, value, boardEval, nextDepth, ply, engine.ttAge);
				}
            }
		}

		if (ply == 1 && abs(value) < MIN_WIN_SCORE)
		{
			// Penalize moves at root that repeat positions, so hopefully the computer will always make progress if possible... 
			if ( Repetition( board.hashKey, search.boardHashHistory, 0, engine.transcript.numMoves+1) ) value = (value>>1);
			else if (unreversible > 0 && value > alpha ) { value++; } // encourage moves that make progress...
		}

        // Keep Track of Best Move and Alpha-Beta Prune
		if (value > alpha)
		{
			bestmove = move;
			alpha = value;

			if (alpha >= beta) 
			{
				if (moveList.numJumps == 0) {
					stack[ply].killerMove = bestmove;
					UpdateHistory(search.historyTable, bestmove, depth, board, ply, prevOppMoveIdx, prevOwnMoveIdx, searchedMoves, movesSearched);
				}
				return beta;
			}

			if (isPV) {
				stack[ply].pv.Set(bestmove, stack[ply + 1].pv); // Save the pv for display/debugging
				if (ply == 1) {search.displayInfo.pv = stack[ply].pv;}
			}
		}
	} // end move loop

	if (moveList.numJumps == 0 && bestmove != NO_MOVE) {
		stack[ply].killerMove = bestmove;
		UpdateHistory(search.historyTable, bestmove, depth, board, ply, prevOppMoveIdx, prevOwnMoveIdx, searchedMoves, movesSearched );
	}

	return alpha;
}

// -------------------------------------------------
// The computer calculates a move then updates g_Board.
// returns the search eval relative to the side to move
// -------------------------------------------------
BestMoveInfo ComputerMove( Board &InBoard, SearchThreadData& search )
{
	int LastEval = 0;
	Move bestmove = NO_MOVE;
	Move doMove = NO_MOVE;
	engine.ttAge = (engine.ttAge + 1) & 63; // fit into 6 bits to store in tt
	
	MoveList& moveList = search.stack[1].moveList;

	moveList.FindMoves(InBoard);

	if (moveList.numMoves == 0) return BestMoveInfo();	// return if game is over

	bestmove = moveList.moves[ 0 ];

	// Init SearchThreadData for start of search
	search.displayInfo.Reset();
	search.displayInfo.startTimeMs = GetCurrentTimeMs();
	search.displayInfo.numMoves = moveList.numMoves;
	search.ClearStack();
	search.stack[0].netInfo.netIdx = -1; // Set to invalid net to force initial computation
	memcpy(search.boardHashHistory, engine.boardHashHistory, sizeof(search.boardHashHistory));
	search.displayInfo.eval = BOOK_INVALID_VALUE;
	if (checkerBoard.useOpeningBook != CB_BOOK_NONE)
		search.displayInfo.eval = engine.openingBook->GetMove( InBoard, bestmove );

	if ( bestmove != NO_MOVE) {
		doMove = bestmove;
	}
	if (search.displayInfo.eval == BOOK_INVALID_VALUE)
	{
		// Make sure the repetition tester has all the values needed.
		if (!checkerBoard.bActive) {
			engine.transcript.ReplayGame(InBoard, search.boardHashHistory );
		}

		// Initialize search depth
		int depth = (engine.searchLimits.maxDepth < 4) ? engine.searchLimits.maxDepth : 2;
		int Eval = 0;

		// Iteratively deepen until until the computer runs out of time, or reaches g_MaxDepth ply
		for ( ; depth <= engine.searchLimits.maxDepth && Eval != TIMEOUT; depth += 2 )
		{
			search.displayInfo.depth = depth;
			engine.searchLimits.panicExtraMult = 1.0f; // multiplied max time by this, increased when search fails low
                                 
			search.stack[0].board = InBoard;
			search.stack[0].board.hashKey = InBoard.CalcHashKey();

			// search with an aspiration window, will expand window and re-search if value is outside of it
			int windowDelta = (depth < 8 ) ? 4000 : (20 + abs(Eval) / 8);
			while (true)
			{
				const int alpha = LastEval - windowDelta;
				const int beta = LastEval + windowDelta;

				Eval = ABSearch(search, 1, depth, alpha, beta, true, bestmove);
				if (bestmove != NO_MOVE) doMove = bestmove;

				if (Eval != TIMEOUT)
				{
					LastEval = Eval;
					search.displayInfo.eval = (InBoard.sideToMove == BLACK) ? -LastEval : LastEval; // keep eval from reds point of view
					float elapsedTime = TimeSince( search.displayInfo.startTimeMs );

					// Check if there is only one legal move, if so don't keep searching
					if (moveList.numMoves == 1 && engine.searchLimits.maxDepth > 6) { Eval = TIMEOUT; }
					if (elapsedTime > engine.searchLimits.maxSeconds * .7f  // probably won't get any useful info before timeup
						|| (abs(Eval) > WinScore(depth) )) // found a win, can stop searching now) 
					{
						Eval = TIMEOUT;
						search.displayInfo.searchingMove++;
					}
				}
				else
				{
					// Replace TIMEOUT with eval from previously completed search
					if (abs(search.displayInfo.eval) < 3000) { LastEval = (InBoard.sideToMove == BLACK) ? -search.displayInfo.eval : search.displayInfo.eval; }
					break;
				}

				// search value was inside aspiration window, so we are done
				if (LastEval > alpha && LastEval < beta) { break; }

				// otherwise widen the aspiration window and re-search
				windowDelta *= 2;
				if (windowDelta > 300) { windowDelta = 4000; }
			}
		}
	}

	if (checkerBoard.bActive && doMove == NO_MOVE) {
		doMove = moveList.moves[0];
	}

	if (!engine.bStopThinking) {
		RunningDisplay(search.displayInfo, doMove, 0);
	}

	return BestMoveInfo(doMove, LastEval);
}
