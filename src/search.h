#pragma once

extern clock_t starttime;

struct BestMoveInfo
{
	SMove move = NO_MOVE;
	int eval = 0;
};

BestMoveInfo ComputerMove(SBoard& InBoard);

//
// Search Stack
//
struct SStackEntry
{
	CMoveList moveList;
	SBoard board;
	SMove killerMove;
	int historyMoveIdx;
};

//
// Search Limits
//
struct SearchLimits
{
	float maxSeconds = 2.0f;				// The number of seconds the computer is allowed to search
	double newIterationMaxTime;				// Threshold for starting another iteration of iterative deepening
	float panicExtraMult;
	bool bEndHard = false;					// Set to true to stop search after fMaxSeconds no matter what.
	int maxDepth = EXPERT_DEPTH;
};

//
// Search Info
//
struct SSearchInfo
{
	uint64_t nodes;
	uint64_t nodesLastUpdate;
	uint64_t databaseNodes;
	int32_t depth;
	int32_t selectiveDepth;
	int searchingMove;
	int eval;

	void Reset() {
		memset(this, 0, sizeof(*this));
	}
};

// Some globals (if we add multi-threaded search would want to change that)
extern SStackEntry g_stack[MAX_SEARCHDEPTH + 1];
extern SSearchInfo g_searchInfo;

bool Repetition(const uint64_t HashKey, int start, int end);