#pragma once
#include "board.h"
#include "egdb.h"

//------------------------
// Endgame Database
//------------------------
enum class dbType { WIN_LOSS_DRAW, EXACT_VALUES, KR_WIN_LOSS_DRAW };

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
	EGDB_DRIVER *kr_wld;
	inline bool InDatabase(const Board& board)
	{
		if (!loaded)
			return(false);

		if (board.numPieces[WHITE] > numWhite || board.numPieces[BLACK] > numBlack)
			return(false);

		if (type == dbType::WIN_LOSS_DRAW || type == dbType::EXACT_VALUES)
			return(true);

		if (type == dbType::KR_WIN_LOSS_DRAW && (board.numPieces[WHITE] + board.numPieces[BLACK] <= numPieces)) {
			if (board.Bitboards.GetJumpers(board.sideToMove) || board.Bitboards.GetJumpers(Opp(board.sideToMove)))
				return(false);
			return(true);
		}
		return(false);
	}
};

void InitializeGuiDatabases(SDatabaseInfo& dbInfo);
int QueryGuiDatabase(const Board& Board);
void close_gui_databases(SDatabaseInfo &dbInfo);
void InitializeEdsDatabases(SDatabaseInfo& dbInfo);
int QueryEdsDatabase(const Board& Board, int ahead);
void close_trice_egdb(SDatabaseInfo &dbInfo);
