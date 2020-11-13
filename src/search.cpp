//
// Search.cpp
// by Jonathan Kreuzer
//
// Alpha-Beta search and related functionality.
// (note : still needs clean-up and general improvement. And doesn't have multi-threading support.)
// 

#include <stdlib.h>
#include <time.h>
#include <algorithm>

#include "engine.h"
#include "guiWindows.h"

// Search Globals
SStackEntry g_stack[MAX_SEARCHDEPTH + 1];
SSearchInfo g_searchInfo;

clock_t starttime = 0;
clock_t endtime = 0;
clock_t lastDisplayTime = 0;

// -------------------------------------------------
// Repetition Testing
// -------------------------------------------------
// Check if there was a previous board with the same hashkey (can assume with almost certainty that it was same board)
bool inline Repetition(const uint64_t HashKey, int start, int end)
{
	int i;
	if (start > 0) i = start; else i = 0;

	// (only checks every other move, because side to move must be the same)
	if ((i & 1) != (end & 1)) i++;

	end -= 2;
	for (; i < end; i += 2)
		if (engine.boardHashHistory[i] == HashKey) return true;

	return false;
}

// -------------------------------------------------
// Quiescence Search... Search all jumps, if there are no jumps, stop the search
// -------------------------------------------------
int QuiesceBoard( int ply, int alpha, int beta )
{
	CMoveList& moveList = g_stack[ply].moveList;
	SBoard& inBoard = g_stack[ply-1].board;

	const uint32_t jumpers = inBoard.Bitboards.GetJumpers( inBoard.SideToMove );

	// No more jump moves, return evaluation
	if ( jumpers == 0 || (ply+1) >= MAX_SEARCHDEPTH )
	{
		g_searchInfo.selectiveDepth = std::max(g_searchInfo.selectiveDepth, ply-1 );

		return inBoard.EvaluateBoard( ply-1, g_searchInfo.databaseNodes);
	}

	// There are jump moves, so we keep searching.
	moveList.FindJumps(inBoard.SideToMove, inBoard.Bitboards, jumpers);

	assert(moveList.numJumps);
	for (int i = 0; i < moveList.numJumps; i++)
	{
		g_stack[ply].board = inBoard;
		g_stack[ply].board.DoMove( moveList.Moves[i] );
		g_searchInfo.nodes++;

		// Recursive Call
        int value = -QuiesceBoard( ply+1, -beta, -alpha);

		// Keep Track of Best Move and Alpha-Beta Prune
		if (value >= beta) { return beta; }
		if (value > alpha ) { alpha = value; }
	}

	return alpha;
}
// -------------------------------------------------
//  returns true if time has run out or the interface has asked the engine to stop the search
// -------------------------------------------------
bool inline CheckTimeUp()
{
	// was the search asked to stop?
	if (checkerBoard.bActive && *checkerBoard.pbPlayNow) return true;
	if (engine.bStopThinking) return true;
	endtime = clock();
	g_searchInfo.nodesLastUpdate = g_searchInfo.nodes;
	// If time has run out, we allow running up to 2*Time if g_bEndHard == FALSE and we are still searching a depth
	if ((endtime - starttime) > (CLOCKS_PER_SEC * engine.searchLimits.maxSeconds))
		if ((endtime - starttime) > (2 * CLOCKS_PER_SEC * engine.searchLimits.maxSeconds * engine.searchLimits.panicExtraMult) || engine.searchLimits.bEndHard || g_searchInfo.searchingMove == 0 || abs(g_searchInfo.eval) > 1500)
			return true;

	if ((endtime - lastDisplayTime) > (CLOCKS_PER_SEC * .4f)) {
		lastDisplayTime = endtime;
		RunningDisplay(NO_MOVE, 1);
	}
	return false;
}

// -------------------------------------------------
//  Alpha Beta Search with Transposition(Hash) Table
// -------------------------------------------------
int ABSearch( int32_t ply, int32_t depth, int32_t alpha, int32_t beta, bool isPV, SMove &bestmove)
{
	assert(ply >= 1);
	assert(beta > alpha);
	// Check to see if move time has run out every 20000 nodes
	if (g_searchInfo.nodes > g_searchInfo.nodesLastUpdate + 20000 )
	{
		if (CheckTimeUp()) return TIMEOUT;
	}

	// Internal Iterative Deepening : set predictedBestmove if there's no bestmove already
	SMove predictedBestmove = bestmove;
	if (predictedBestmove == NO_MOVE && depth > 4 - isPV ) {
		int val = ABSearch( ply, std::max(depth-4,1), alpha, beta, false, predictedBestmove);
		if (val == TIMEOUT) return TIMEOUT;
		if (predictedBestmove != NO_MOVE) bestmove = predictedBestmove;
	}

	// Accessors for the stack variables
	CMoveList& moveList = g_stack[ply].moveList;
	SBoard& board = g_stack[ply].board;
	SMove& killerMove = g_stack[ply].killerMove;
	const int prevOppMoveIdx = g_stack[ply - 1].historyMoveIdx;

	// Find possible moves (and set a couple variables)
	const eColor color = g_stack[ply - 1].board.SideToMove;
	moveList.FindMoves( color, g_stack[ply-1].board.Bitboards );

	const int startAlpha = alpha;
	if (moveList.numMoves == 0) { 
		return -WinScore( ply - 1 ); // If you can't move, you've already lost the game
	}
	alpha = std::max(alpha, -WinScore(ply));
	if (alpha >= beta) return beta;
	if (alpha >= WinScore(ply)) return alpha; // have a guaranteed faster win already, so don't waste time searching

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
	SMove searchedMoves[CMoveList::MAX_MOVES];
	int movesSearched = 0;

	for (int i = 0; i < moveList.numMoves; i++)
	{
		if (i == sortOnMoveIdx) {
			// Sort the remaining moves
			if (moveList.numJumps > 1 + sortOnMoveIdx) { moveList.SortJumps(i, board.Bitboards); }
			else if (moveList.numJumps == 0) { HistorySort(engine.historyTable, moveList, i,  board, prevOppMoveIdx); }
		}
		
		const SMove move = moveList.Moves[i];

		if (ply == 1) 
		{
			g_searchInfo.searchingMove = movesSearched;

			// On dramatic changes (such as fail lows) make sure to finish the iteration
			const int newEval = (color == WHITE) ? alpha : -alpha; // eval from red's POV
			if (abs(newEval) < 3000)
			{
				if (abs(newEval - g_searchInfo.eval) >= 26) engine.searchLimits.panicExtraMult = 1.8f;
				if (abs(newEval - g_searchInfo.eval) >= 50) engine.searchLimits.panicExtraMult = 2.5f;
				if (movesSearched > 0) { g_searchInfo.eval = newEval; }
			}
			if (g_searchInfo.depth > 11 && !engine.bStopThinking) {
				RunningDisplay(bestmove, 1);
			}
		}
			 
		// Play the move (after resetting board to the one from previous ply)
        board = g_stack[ply-1].board;
		const ePieceType movedPiece = board.GetPiece(move.Src());
        const int unreversible = board.DoMove( move );
		g_searchInfo.nodes++;

		engine.boardHashHistory[ engine.transcript.numMoves + ply] = board.HashKey;
		searchedMoves[movesSearched++] = move;
		g_stack[ply].historyMoveIdx = engine.historyTable.MoveIdx(move.Src(), move.Dir(0), movedPiece );

        // If the game is over after making this move, return a gameover value now
		if (board.Bitboards.P[WHITE] == 0 || board.Bitboards.P[BLACK] == 0)
		{
			bestmove = move;
            return WinScore( ply );
        }

		int value = INVALID_VAL;
		int nextDepth = depth - 1;

		// EXTENSIONS
		if (moveList.numJumps > 0 &&
			(isPV || (ply > 1 && g_stack[ply - 1].moveList.numJumps > 0)))
			nextDepth++; // Any jump "recapture" or jump on PV
		else if (nextDepth == 0 && board.Bitboards.GetJumpers(color))
			nextDepth++; // On leaf when there is a jump threatened
	
		if (ply > 1 && board.reversibleMoves > 78 && board.Bitboards.GetJumpers(board.SideToMove) == 0) 
		{ 
			value = 0;  // draw by 40-move rule value.. Not sure actual rules checkerboard calls draws this way sometimes though
		}
		else if (nextDepth >= 1 && ply > 1 && Repetition(board.HashKey, engine.transcript.numMoves - 24, engine.transcript.numMoves + ply))
		{
			value = 0; 	// If this is the repetition of a position that has occured already in the search, return a draw score
		}
		else if ( (nextDepth <= 0 || ply >= MAX_SEARCHDEPTH) )
		{
			// quiesce (play jumps) then evaluate the position
			value = -QuiesceBoard( ply+1, -beta, -alpha );
	    }
        else 
		{	
			// If this isn't the max depth continue to look ahead 
			SMove nextBestmove = NO_MOVE;

		   	// TRANSPOSITION TABLE : Look up the this Position and check for value or bestMove
			TEntry* ttEntry = nullptr;
			int boardEval = INVALID_VAL;
			if (engine.bUseHashTable)
			{
				ttEntry = engine.TTable.GetEntry( board );
				ttEntry->Read( board.HashKey, alpha, beta, nextBestmove, value, boardEval, nextDepth, ply);
			}
			if (isPV && ply < 4) value = INVALID_VAL; // don't tt prune at beginning of PV, might miss repetition

			if (value == INVALID_VAL && nextDepth > 2 && ply >= 3)
			{
				// DATABASE : Stop searching if we know the exact value from the database
				if (engine.dbInfo.InDatabase( board ) )
				{
					if (boardEval == INVALID_VAL) { boardEval = -board.EvaluateBoard(ply, g_searchInfo.databaseNodes); }
					if (boardEval == 0 || (engine.dbInfo.type == dbType::EXACT_VALUES && abs(boardEval) > MIN_WIN_SCORE) )
						value = boardEval;
				}

				// PRUNING : Prune this move if the eval is enough above beta and shallower verification search passes
				const bool bForced = (ply > 1 && g_stack[ply - 1].moveList.numMoves == 1 && moveList.numMoves == 1);
				if (!bForced && !isPV && value == INVALID_VAL && beta > -1500)
				{
					const int evalMargin = 28;
					const int verifyDepth = std::max(nextDepth - 4, 1);
					if (boardEval == INVALID_VAL) {
						boardEval = -board.EvaluateBoard(ply, g_searchInfo.databaseNodes);
						if (ttEntry) { ttEntry->m_boardEval = boardEval; }
					}

					if (boardEval >= beta + evalMargin )
					{
						value = -ABSearch( ply + 1, verifyDepth, -(beta + evalMargin + 1), -(beta + evalMargin), false, nextBestmove);
						if (value == -TIMEOUT) return TIMEOUT;
						if (value <= beta + evalMargin) value = INVALID_VAL;
					}
				}
			}

            // If value wasn't set from the Transposition Table or Pruning, search this position
            if (value == INVALID_VAL)
			{
				// 1-ply LMR when not on pv
				const bool doLMR = !isPV && moveList.numJumps == 0 && movesSearched > 2;

				// Principal Variation Search - if this isn't first move on PV, use a search window width of 1
				if (!isPV || movesSearched > 1)
				{
					const int pvsDepth = nextDepth - doLMR;
					value = -ABSearch( ply + 1, pvsDepth, -(alpha + 1), -alpha, false, nextBestmove);
					if (value == -TIMEOUT) return TIMEOUT;
				}

				// full-width and depth search
				if (value == INVALID_VAL || (value > alpha && (value < beta || doLMR) ) )
				{
					value = -ABSearch( ply + 1, nextDepth, -beta, -alpha, isPV && movesSearched <= 1, nextBestmove);
					if (value == -TIMEOUT) return TIMEOUT;
				}

				// Store for the search info for this position in the tranposition table entry
				if (ttEntry) {
					ttEntry->Write(board.HashKey, alpha, beta, nextBestmove, value, boardEval, nextDepth, ply, engine.ttAge);
				}
            }
		}

		if (ply == 1 && abs(value) < MIN_WIN_SCORE)
		{
			// Penalize moves at root that repeat positions, so hopefully the computer will always make progress if possible... 
			if ( Repetition( board.HashKey, 0, engine.transcript.numMoves+1) ) value = (value>>1);
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
					UpdateHistory(engine.historyTable, bestmove, depth, board, ply, prevOppMoveIdx, searchedMoves, movesSearched);
				}

				return beta;
			}
		}
	} // end move loop

	if (moveList.numJumps == 0 && bestmove != NO_MOVE) {
		UpdateHistory(engine.historyTable, bestmove, depth, board, ply, prevOppMoveIdx, searchedMoves, movesSearched );
	}

	return alpha;
}

// -------------------------------------------------
// The computer calculates a move then updates g_Board.
// returns the search eval relative to the side to move
// -------------------------------------------------
BestMoveInfo ComputerMove( SBoard &InBoard )
{
	int LastEval = 0;
	SMove bestmove = NO_MOVE;
	SMove doMove = NO_MOVE;
	engine.ttAge = (engine.ttAge + 1) % 63; // fit into 6 bits to store in tt
	CMoveList& moveList = g_stack[1].moveList;

	moveList.FindMoves(InBoard.SideToMove, InBoard.Bitboards);

	if (moveList.numMoves == 0) return BestMoveInfo();	// return if game is over

	srand( (unsigned int)time(0) );
	bestmove = moveList.Moves[ 0 ];
	starttime = clock();
	endtime = starttime;

	g_searchInfo.Reset();

	for (int i = 0; i < MAX_SEARCHDEPTH + 1; i++)
	{
		g_stack[i].killerMove = NO_MOVE;
		g_stack[i].moveList.Clear();
	}

	g_searchInfo.eval = engine.openingBook->GetMove( InBoard, bestmove );
	if ( bestmove != NO_MOVE) {
		doMove = bestmove;
	}
	if (g_searchInfo.eval == BOOK_INVALID_VALUE)
	{
		// Make sure the repetition tester has all the values needed.
		if (!checkerBoard.bActive) {
			engine.transcript.ReplayGame(InBoard, engine.boardHashHistory );
		}

		// Initialize search depth
		int depth = (engine.searchLimits.maxDepth < 4) ? engine.searchLimits.maxDepth : 2;
		int Eval = 0;

		// Iteratively deepen until until the computer runs out of time, or reaches g_MaxDepth ply
		for ( ; depth <= engine.searchLimits.maxDepth && Eval != TIMEOUT; depth += 2 )
		{
			g_searchInfo.depth = depth;
			engine.searchLimits.panicExtraMult = 1.0f; // multiplied max time by this, increased when search fails low
                                 
			 g_stack[0].board = InBoard;
			 g_stack[0].board.HashKey = InBoard.CalcHashKey();

			 // search with an aspiration window, will expand window and re-search if value is outside of it
			 int windowDelta = (g_searchInfo.depth < 8 ) ? 4000 : (24 + abs(Eval) / 8);
			 while (true)
			 {
				 const int alpha = LastEval - windowDelta;
				 const int beta = LastEval + windowDelta;

				 Eval = ABSearch( 1, depth, alpha, beta, true, bestmove);
				 if (bestmove != NO_MOVE) doMove = bestmove;

				 if (Eval != TIMEOUT)
				 {
					 LastEval = Eval;
					 g_searchInfo.eval = (InBoard.SideToMove == BLACK) ? -LastEval : LastEval; // keep eval from reds point of view
					 float elapsedTime = (float)(clock() - starttime) / (float)CLOCKS_PER_SEC;

					 // Check if there is only one legal move, if so don't keep searching
					 if (moveList.numMoves == 1 && engine.searchLimits.maxDepth > 6) { Eval = TIMEOUT; }
					 if (elapsedTime > engine.searchLimits.maxSeconds * .7f  // probably won't get any useful info before timeup
						 || (abs(Eval) > WinScore(depth) )) // found a win, can stop searching now) 
					 {
						 Eval = TIMEOUT;
						 g_searchInfo.searchingMove++;
					 }
				 }
				 else
				 {
					 if (abs(g_searchInfo.eval) < 3000) LastEval = g_searchInfo.eval;

					 break;
				 }

				 // search value was inside aspiration window, so we are done
				 if (LastEval > alpha && LastEval < beta ) break;

				 // otherwise widen the aspiration window and re-search
				 windowDelta *= 2;
				 if (windowDelta > 300) windowDelta = 4000;
			 }
		}
	}

	if (checkerBoard.bActive && doMove == NO_MOVE) doMove = moveList.Moves[ 0 ];

	BestMoveInfo ret;
	ret.move = doMove;
	ret.eval = LastEval;
	return ret;
}
