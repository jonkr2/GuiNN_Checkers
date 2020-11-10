#pragma once

#include "defines.h"
#include "board.h"
#include "moveGen.h"

struct Transcript
{
	void AddMove(SMove move)
	{
		assert(numMoves < MAX_GAMEMOVES);
		moves[numMoves++] = move;
		moves[numMoves] = NO_MOVE;
	}
	void Init(SBoard inStartBoard)
	{
		startBoard = inStartBoard;
		numMoves = 0;
		moves[0].data = 0;
	}
	SMove& Back()
	{
		return moves[numMoves];
	}

	std::string ToPDN();
	int FromPDN(const char* sPDN, float* gameResult = nullptr );
	int MakeMovePDN(int src, int dst, SBoard& board);
	static std::string GetMoveString(const SMove& move);

	SBoard startBoard;
	SMove moves[MAX_GAMEMOVES];
	int numMoves = 0;
};
