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
	void ReplayGame(SBoard& board, uint64_t boardHashHistory[]);
	static std::string GetMoveString(const SMove& move);
	bool Save(const char* filepath);
	bool Load(const char* filepath);


	SBoard startBoard;
	SMove moves[MAX_GAMEMOVES];
	int numMoves = 0;
};
