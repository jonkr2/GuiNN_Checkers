//
// evaluate.cpp
// by Jonathan Kreuzer
// 
// Computes an eval score for a Board.

#include "engine.h"
#include "kr_db.h"

// return eval relative to board.sideToMove
int Board::EvaluateBoard(int ply, SearchThreadData& search, const EvalNetInfo& netInfo, int depth) const
{
	// Game is over?        
	if ((numPieces[WHITE] == 0 && sideToMove == WHITE) || (numPieces[BLACK] == 0 && sideToMove == BLACK)) {
		return -WinScore(ply);
	}

	// Exact database value?
	if (engine.dbInfo.type == dbType::EXACT_VALUES && engine.dbInfo.InDatabase(*this))
	{
		int value = QueryEdsDatabase(*this, ply);

		if (value != INVALID_DB_VALUE)
		{
			// If we get an exact score from the database we want to return right away
			search.displayInfo.databaseNodes++;
			return (sideToMove == WHITE) ? value : -value;
		}
	}

	int eval = 0;

	if (engine.dbInfo.type == dbType::KR_WIN_LOSS_DRAW && engine.dbInfo.InDatabase(*this)) {
		EGDB_BITBOARD bb;

		gui_to_kr(Bitboards, bb);
		int result = engine.dbInfo.kr_wld->lookup(engine.dbInfo.kr_wld, &bb, gui_to_kr_color(sideToMove), depth <= 3);
		if (result == EGDB_WIN) {
			search.displayInfo.databaseNodes++;
			eval = dbWinEval(sideToMove == BLACK ? BLACKWIN : WHITEWIN);
			return(to_rel_score(eval, sideToMove));
		}
		else if (result == EGDB_LOSS) {
			search.displayInfo.databaseNodes++;
			eval = dbWinEval(sideToMove == BLACK ? WHITEWIN : BLACKWIN);
			return(to_rel_score(eval, sideToMove));
		}
		else if (result == EGDB_DRAW) {
			search.displayInfo.databaseNodes++;
			return(0);
		}
	}

	// Probe the W/L/D bitbase
	if (engine.dbInfo.type == dbType::WIN_LOSS_DRAW && engine.dbInfo.InDatabase(*this))
	{
		int Result = QueryGuiDatabase(*this);

		// Use a heuristic eval to help finish off won games
		if (Result <= 2) {
			search.displayInfo.databaseNodes++;
			if (Result == dbResult::WHITEWIN) eval = dbWinEval(Result);
			if (Result == dbResult::BLACKWIN) eval = dbWinEval(Result);
			if (Result == dbResult::DRAW) return 0;
		}
	}
	else if (Bitboards.GetCheckers() == 0)
	{
		eval = AllKingsEval();
	}
	else
	{
		// NEURAL NET EVAL
		assert(netInfo.firstLayerValues && netInfo.netIdx >= 0);
		eval = engine.evalNets[netInfo.netIdx]->GetSumIncremental(netInfo.firstLayerValues, search.nnValues);
		eval = -SoftClamp(eval / 3, 400, 800); // move it into a better range with rest of evaluation

		// surely winning material advantage?
		if (numPieces[WHITE] >= numPieces[BLACK] + 2)
			eval += (numPieces[BLACK] == 1) ? 150 : 50;
		if (numPieces[BLACK] >= numPieces[WHITE] + 2)
			eval -= (numPieces[WHITE] == 1) ? 150 : 50;
	}

	// return sideToMove relative eval
	return(to_rel_score(eval, sideToMove));
}

// For positions with all kings to help finish the game. 
// (TODO? Could train a small net to handle this using some distance to win metric in targetVal.)
// (TODO? generalize to use for any lop-sided position that should be easy win.)
int Board::AllKingsEval() const
{
	const uint32_t WK = Bitboards.K & Bitboards.P[WHITE];
	const uint32_t BK = Bitboards.K & Bitboards.P[BLACK];
	const int KingCount[2] = { BitCount(BK), BitCount(WK) };

	int eval = 0;
	eval += (KingCount[WHITE] - KingCount[BLACK]) * 25;
	eval += (numPieces[WHITE] - numPieces[BLACK]) * 75;
	eval += (BitCount(WK & SINGLE_EDGE) - BitCount(BK & SINGLE_EDGE)) * -10;// encourage pushing kings to edges (except 2 corner)
	eval += (BitCount(WK & CENTER_8) - BitCount(BK & CENTER_8)) * 4; // even better to be in center
	if (TotalPieces() < 12 && Bitboards.GetCheckers() == 0)
	{
		eval += (numPieces[WHITE] - numPieces[BLACK]) * (240 - TotalPieces() * 20); // encourage trading down
	}
	if (numPieces[WHITE] > numPieces[BLACK])
	{
		if (KingCount[WHITE] == numPieces[WHITE]) eval += 60; // An extra king is usually a win I think?
		if (numPieces[WHITE] >= numPieces[BLACK] + 2) eval += (numPieces[BLACK] == 1) ? 100 : 50; 	// surely winning material advantage?
		if (BK & DOUBLE_CORNER) eval -= 12; // encourage losing side to use double corner
	}
	if (numPieces[BLACK] > numPieces[WHITE])
	{
		if (KingCount[BLACK] == numPieces[BLACK]) eval -= 60;
		if (numPieces[BLACK] >= numPieces[WHITE] + 2) eval -= (numPieces[WHITE] == 1) ? 100 : 50;
		if (WK & DOUBLE_CORNER) eval += 12;
	}

	return eval;
}

int Board::dbWinEval(int dbresult) const
{
	assert(dbresult == WHITEWIN || dbresult == BLACKWIN);
	const int dbwin_offset = 400;
	const uint32_t wm = ~Bitboards.K & Bitboards.P[WHITE];
	const uint32_t bm = ~Bitboards.K & Bitboards.P[BLACK];
	const uint32_t WK = Bitboards.K & Bitboards.P[WHITE];
	const uint32_t BK = Bitboards.K & Bitboards.P[BLACK];
	const int KingCount[2] = { BitCount(BK), BitCount(WK) };

	int eval = 0;
	eval += (KingCount[WHITE] - KingCount[BLACK]) * 25;
	eval += (numPieces[WHITE] - numPieces[BLACK]) * 75;

	// encourage the winning side to trade down
	eval += (numPieces[WHITE] - numPieces[BLACK]) * (240 - TotalPieces() * 20);

	// encourage pushing kings to edges (except 2 corner)
	eval += (BitCount(WK & SINGLE_EDGE) - BitCount(BK & SINGLE_EDGE)) * -10;

	// encourage kings in the center
	eval += (BitCount(WK & CENTER_8) - BitCount(BK & CENTER_8)) * 4;

	// encourage advancement of men on the winning side.
	if (dbresult == WHITEWIN) {
		eval += dbwin_offset;
		if (wm)
			eval += white_tempo(wm);

		// if equal number of pieces, encourage advancement of men on the losing side.
		if (numPieces[WHITE] == numPieces[BLACK])
			eval += black_tempo(bm);
		if (BK & DOUBLE_CORNER)
			eval -= 12; // encourage losing side to use double corner
	}
	else if (dbresult == BLACKWIN) {
		eval -= dbwin_offset;
		if (bm)
			eval -= black_tempo(bm);
		if (numPieces[WHITE] == numPieces[BLACK])
			eval -= white_tempo(wm);
		if (WK & DOUBLE_CORNER)
			eval += 12;
	}

	return eval;
}

