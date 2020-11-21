#pragma once
#include "board.h"

//------------------------
// Endgame Database
//------------------------
enum class dbType { WIN_LOSS_DRAW, EXACT_VALUES };

enum dbResult {
	DRAW = 0,
	BLACKWIN = 1,
	WHITEWIN = 2,
	NO_RESULT = 3
};
const int INVALID_DB_VALUE = -100000;

struct SDatabaseInfo
{
	int numPieces;
	int numWhite;
	int numBlack;
	dbType type;
	bool loaded = false;

	inline bool InDatabase(const Board& board)
	{
		return (loaded && board.numPieces[WHITE] <= numWhite && board.numPieces[BLACK] <= numBlack);
	}
};

void InitializeGuiDatabases(SDatabaseInfo& dbInfo);
int QueryGuiDatabase(const Board& Board);

void InitializeEdsDatabases(SDatabaseInfo& dbInfo);
int QueryEdsDatabase(const Board& Board, int ahead);
