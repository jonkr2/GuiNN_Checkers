#pragma once

#include <chrono>
#include "NeuralNet/NeuralNet.h"

struct BestMoveInfo
{
	BestMoveInfo() {}
	BestMoveInfo( Move inMove, int inEval ) : move(inMove), eval(inEval) {}

	Move move = NO_MOVE;
	int eval = 0;
};

BestMoveInfo ComputerMove(Board& InBoard, struct SearchThreadData& search);
bool Repetition(const uint64_t hashKey, uint64_t boardHashHistory[], int start, int end);

// Keep track of principal variation moves for display and debugging
struct SPrincipalVariation
{
	Move moves[MAX_SEARCHDEPTH];
	int count;

	void Clear()
	{
		count = 0;
	}
	void Set(const Move& move)
	{
		assert(count + 1 < MAX_SEARCHDEPTH);
		moves[0] = move;
		count = 1;
	}
	void Set(const Move& move, const SPrincipalVariation& pv)
	{
		assert(count + 1 + pv.count < MAX_SEARCHDEPTH);

		moves[0] = move;
		count = 1;
		if (pv.count > 0) {
			memcpy(&moves[count], &pv.moves[0], sizeof(Move) * pv.count);
			count += pv.count;
		}
	}

	std::string ToString();
};

struct EvalNetInfo
{
	// Store first layer un-activated values for eval net incremental updates
	nnInt_t *firstLayerValues = nullptr;
	int valueCount;
	// Need to know which eval net the values are for too
	int netIdx = -1; 
};

//
// Search Stack
//
struct SearchStackEntry
{
	MoveList moveList;
	Board board;
	Move killerMove;
	int historyMoveIdx;
	SPrincipalVariation pv;
	EvalNetInfo netInfo;
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
// Search Info - mainly for display
//
struct SearchInfo
{
	uint64_t startTimeMs;
	uint64_t lastDisplayTimeMs;
	uint64_t nodes;
	uint64_t nodesLastUpdate;
	uint64_t databaseNodes;
	int32_t depth;
	int32_t selectiveDepth;
	int searchingMove;
	int numMoves;
	int eval;
	SPrincipalVariation pv;

	void Reset() {
		memset(this, 0, sizeof(*this));
		pv.Clear();
	}
};

// Store in structure passed to search function for multi-threading support
struct SearchThreadData
{
	SearchStackEntry stack[MAX_SEARCHDEPTH + 1];
	SearchInfo displayInfo;
	uint64_t boardHashHistory[MAX_GAMEMOVES];

	HistoryTable historyTable;
	nnInt_t* nnValues = nullptr;

	~SearchThreadData()
	{
		if (nnValues) { AlignedFreeUtil(nnValues); }
		for (int i = 0; i < MAX_SEARCHDEPTH + 1; i++)
		{
			 AlignedFreeUtil(stack[i].netInfo.firstLayerValues);
		}
	}

	void Alloc( int firstLayerSize )
	{
		nnValues = AlignedAllocUtil<nnInt_t>(kMaxEvalNetValues, 64);
		for (int i = 0; i < MAX_SEARCHDEPTH + 1; i++)
		{
			stack[i].netInfo.valueCount = firstLayerSize;
			stack[i].netInfo.firstLayerValues = AlignedAllocUtil<nnInt_t>(firstLayerSize, 64);
		}
	}

	void ClearStack()
	{
		for (int i = 0; i < MAX_SEARCHDEPTH + 1; i++)
		{
			stack[i].killerMove = NO_MOVE;
			stack[i].moveList.Clear();
			stack[i].pv.Clear();
		}
	}
};

// Time helpers
inline uint64_t GetCurrentTimeMs()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

// Returns time in seconds since startTime
inline float TimeSince(uint64_t startTimeMs)
{
	double elapsed = (GetCurrentTimeMs() - startTimeMs) / 1000.0f;
	return (float)elapsed;
}