//
// evaluate.cpp
// by Jonathan Kreuzer
// 
// Computes an eval score for a Board.

#include "engine.h"

// return eval relative to board.SideToMove
int SBoard::EvaluateBoard(int ahead, uint64_t& databaseNodes )
{
	// Game is over?        
	if ((numPieces[WHITE] == 0 && SideToMove == WHITE) || (numPieces[BLACK] == 0 && SideToMove == BLACK)) {
		return -WinScore( ahead );
	}

	// Exact database value?
	if (engine.dbInfo.type == dbType::EXACT_VALUES && engine.dbInfo.InDatabase(*this))
	{
		int value = QueryEdsDatabase(*this, ahead);

		if (value != INVALID_DB_VALUE)
		{
			// If we get an exact score from the database we want to return right away
			databaseNodes++;
			return (SideToMove == WHITE) ? value : -value;
		}
	}

	int eval = 0;

	// Probe the W/L/D bitbase
	if (engine.dbInfo.type == dbType::WIN_LOSS_DRAW && engine.dbInfo.InDatabase(*this))
	{
		// Use a heuristic eval to help finish off won games
		eval = FinishingEval();

		int Result = QueryGuiDatabase(*this);
		if (Result <= 2) {
			databaseNodes++;
			if (Result == dbResult::WHITEWIN) eval += 400;
			if (Result == dbResult::BLACKWIN) eval -= 400;
			if (Result == dbResult::DRAW) return 0;
		}
	}
	else if (Bitboards.GetCheckers() == 0)
	{
		eval = FinishingEval(); // All kings also uses heuristic eval
	}
	else
	{
		static alignas(64) nnInt_t nnValues[1024]; // TEMP : to thread should have values in threadData struct
		// Look for which neural net is active, and get the board evaluation from that one.
		for (auto net : engine.evalNets )
		{
			if (net->IsActive(*this)) {
				eval = net->GetNNEval(*this, nnValues);
				break;
			}
		}

		// surely winning material advantage?
		if (numPieces[WHITE] >= numPieces[BLACK] + 2)
			eval += (numPieces[BLACK] == 1) ? 150 : 50;
		if (numPieces[BLACK] >= numPieces[WHITE] + 2)
			eval -= (numPieces[WHITE] == 1) ? 150 : 50;
	}

	// return SideToMove relative eval
	return (SideToMove == WHITE) ? eval : -eval;
}

// For 1. won positions in database 2. positions with all kings to help finish the game. 
// (TODO? Could train a small net to handle this using some distance to win metric in targetVal.)
// (TODO? generalize to use for any lop-sided position that should be easy win.)
int SBoard::FinishingEval()
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
		eval += (numPieces[WHITE] - numPieces[BLACK]) * (240 - TotalPieces() * 20);
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