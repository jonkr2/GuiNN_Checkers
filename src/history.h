#pragma once

//
// History move sorting
// (TODO : verify that this is an elo gain in checkers... and isn't buggy.)
//
struct HistoryTable
{
	inline int MoveIdx(int src, int dir, const ePieceType piece)
	{
		return pieceType[piece] + dir * 4 + src * 16;
	}

	void inline UpdateScore(int16_t& score, int delta)
	{
		// Adjust this value, while protecting it from overflow/underflow
		//score = ClampInt(score + delta, -HISTORY_MAX+1, HISTORY_MAX);
		score += delta - (score * abs(delta) / HISTORY_MAX);
	}

	inline void AdjustHistory(int src, int dir, ePieceType piece, int prevOppMoveIdx, int delta)
	{
		int historyIdx = MoveIdx(src, dir, piece);

		UpdateScore(history[historyIdx], delta);

		if (prevOppMoveIdx > 0) {
			UpdateScore(counterHistory[prevOppMoveIdx * PIECE_DIR_SRC_SIZE + historyIdx], delta);
		}
	}

	inline int GetScore(int src, int dir, ePieceType piece, int prevOppMoveIdx)
	{
		int historyIdx = MoveIdx(src, dir, piece);

		return history[historyIdx]
			+ counterHistory[prevOppMoveIdx * PIECE_DIR_SRC_SIZE + historyIdx];
	}

	void Clear()
	{
		memset(history, 0, sizeof(history));
		memset(counterHistory, 0, sizeof(counterHistory));
	}

	static constexpr int PIECE_DIR_SRC_SIZE = 4 * 4 * 32; // pieceType, move direction, source square
	int16_t history[PIECE_DIR_SRC_SIZE];
	int16_t counterHistory[PIECE_DIR_SRC_SIZE * PIECE_DIR_SRC_SIZE];
	static constexpr int pieceType[8] = { 0, 0, 1, 0, 0, 2, 3, 0 };
	static constexpr int HISTORY_MAX = (1 << 15);
};

static void HistorySort(HistoryTable& historyTable, CMoveList& moveList, int startIdx, const SBoard& board, int prevOppMoveIdx)
{
	int numMoves = moveList.numMoves;
	int32_t Scores[CMoveList::MAX_MOVES];

	// score the moves
	for (int i = startIdx; i < numMoves; i++)
	{
		const SMove& move = moveList.Moves[i];
		int src = move.Src();
		Scores[i] = historyTable.GetScore(src, move.Dir(), board.GetPiece(src), prevOppMoveIdx);
	}

	// insertion sort
	for (int d = startIdx + 1; d < numMoves; d++)
	{
		const SMove tempMove = moveList.Moves[d];
		int tempScore = Scores[d];
		int i;
		for (i = d; i > startIdx && tempScore > Scores[i - 1]; i--)
		{
			moveList.Moves[i] = moveList.Moves[i - 1];
			Scores[i] = Scores[i - 1];
		}
		moveList.Moves[i] = tempMove;
		Scores[i] = tempScore;
	}
}

static void UpdateHistory(HistoryTable& historyTable, const SMove& bestMove, int depth, const SBoard& board, int ply, int prevOppMoveIdx, SMove searchedMoves[], int numSearched)
{
	g_stack[ply].killerMove = bestMove;

	int adjust = std::min(depth * std::max(depth - 1, 1), 16 * 16);
	int src = bestMove.Src();
	historyTable.AdjustHistory(src, bestMove.Dir(), board.GetPiece(src), prevOppMoveIdx, adjust);

	if (numSearched > 1 && depth > 2)
	{
		for (int i = 0; i < numSearched; i++)
		{
			if (searchedMoves[i] != bestMove) {
				src = searchedMoves[i].Src();
				int dst = searchedMoves[i].Dst();
				historyTable.AdjustHistory(src, searchedMoves[i].Dir(), board.GetPiece(src), prevOppMoveIdx, -(adjust - 1));
			}
		}
	}
}
