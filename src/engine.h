#pragma once

#include <algorithm>
#include <time.h>

#include "board.h"
#include "movegen.h"
#include "transcript.h"
#include "transpositionTable.h"
#include "endgameDatabase.h"
#include "openingBook.h"
#include "search.h"
#include "history.h"
#include "checkersNN.h"

struct SEngine
{
	eColor	computerColor = WHITE;
	bool	bThinking = false;
	bool	bStopThinking = false;
	bool    bUseHashTable = true;
	uint8_t ttAge = 0;

	TranspositionTable TTable;
	HistoryTable historyTable;

	void Init(char* status_str);
	void NewGame(const SBoard& startBoard);
	void MoveNow();
	void StartThinking();
	const char* GetInfoString();

	SearchLimits searchLimits;
	COpeningBook* openingBook;
	SDatabaseInfo dbInfo;
	std::vector<CheckersNet*> evalNets;

	SBoard board; // current game board
	Transcript transcript;
	uint64_t boardHashHistory[MAX_GAMEMOVES];
};

// we have one global engine
extern SEngine engine;