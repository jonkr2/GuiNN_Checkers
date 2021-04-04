#pragma once

#include "defines.h"
#include "board.h"
#include "moveGen.h"

struct Transcript
{
	void AddMove(Move move)
	{
		assert(numMoves < MAX_GAMEMOVES);
		moves[numMoves++] = move;
		moves[numMoves] = NO_MOVE;
	}
	void Init(Board inStartBoard)
	{
		startBoard = inStartBoard;
		numMoves = 0;
		moves[0].data = 0;
	}
	Move& Back()
	{
		return moves[numMoves];
	}

	std::string ToString();
	int FromString(const char* sPDN, float* gameResult = nullptr );
	int MakeMovePDN(int src, int dst, Board& board);
	void ReplayGame(Board& board, uint64_t boardHashHistory[]);
	static std::string GetMoveString(const Move& move);
	bool Save(const char* filepath);
	bool Load(const char* filepath);


	Board startBoard;
	Move moves[MAX_GAMEMOVES];
	int numMoves = 0;
};
