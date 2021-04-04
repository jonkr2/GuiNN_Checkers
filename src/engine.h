#pragma once

#include <algorithm>

#include "board.h"
#include "cb_interface.h"
#include "movegen.h"
#include "transcript.h"
#include "transpositionTable.h"
#include "endgameDatabase.h"
#include "openingBook.h"
#include "history.h"
#include "search.h"
#include "checkersNN.h"

struct Engine
{
	// FUNCTIONS
	void Init(char* status_str);
	void NewGame(const Board& startBoard, bool resetTranscript);
	void MoveNow();
	void StartThinking();
	std::string GetInfoString();

	// DATA
	eColor	computerColor = WHITE;
	bool	bThinking = false;
	bool	bStopThinking = false;
	bool    bUseHashTable = true;
	uint8_t ttAge = 0;

	TranspositionTable TTable;

	SearchLimits searchLimits;
	COpeningBook* openingBook;
	SDatabaseInfo dbInfo;
	std::vector<CheckersNet*> evalNets;
	std::string binaryNetFile = "Nets206.gnn";

	Board board; // current game board
	Transcript transcript;
	uint64_t boardHashHistory[MAX_GAMEMOVES];

	// If we add multi-threaded search, this would be per-thread
	SearchThreadData searchThreadData;
};

// we have one global engine
extern Engine engine;

// for extern Checkerboard interface
struct CheckerboardInterface
{
	bool bActive = false;
	int* pbPlayNow = nullptr;
	char* infoString = nullptr;
	int useOpeningBook = CB_BOOK_BEST_MOVES;
#ifdef _WINDLL
	char db_path[260] = "db";
#else
	char db_path[260] = "db_dtw";
#endif
	int enable_wld = 1;
	int max_dbpieces = 10;
	int wld_cache_mb = 2048;
	bool did_egdb_init = false;
	bool request_egdb_init = false;
};

/*
 * Convert an internal absolute score to a relative score.
 * Internal absolute scores are + good for white.
 * Relative scores are + good for side-to-move.
 * Externally visible absolute scores are + good for black.
 */
inline int to_rel_score(int score, eColor color) {
	if (color == WHITE)
		return(score);
	else
		return(-score);
}

inline int black_tempo(uint32_t bm)
{
	return(4 * BitCount((MASK_RANK[4] | MASK_RANK[5] | MASK_RANK[6]) & bm) + 
			2 * BitCount((MASK_RANK[2] | MASK_RANK[3] | MASK_RANK[6]) & bm) + 
			BitCount((MASK_RANK[1] | MASK_RANK[3] | MASK_RANK[5]) & bm));
}

inline int white_tempo(uint32_t wm)
{
	return(4 * BitCount((MASK_RANK[3] | MASK_RANK[2] | MASK_RANK[1]) & wm) + 
			2 * BitCount((MASK_RANK[5] | MASK_RANK[4] | MASK_RANK[1]) & wm) + 
			BitCount((MASK_RANK[6] | MASK_RANK[4] | MASK_RANK[2]) & wm));
}

extern CheckerboardInterface checkerBoard;