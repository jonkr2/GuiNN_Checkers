#pragma once

#include <algorithm>

#include "board.h"
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
	int useOpeningBook = 1;
	char db_path[260] = "db_dtw";
	int enable_wld = 1;
};
extern CheckerboardInterface checkerBoard;