#pragma once

#include <cstdint>
#include "defines.h"


inline eColor Opp(eColor c) { return (eColor)(c ^ 1); }

enum ePieceType {
	EMPTY = 0,
	BPIECE = 1,
	WPIECE = 2,
	KING = 4,
	BKING = 5,
	WKING = 6,
	INVALID = 8
};

// S[] contains 32-bit bitboards with a single bit for each of the 32 squares (plus 2 invalid squares with no bits set)
constexpr uint32_t S[34] = {
	(1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6), (1 << 7), (1 << 8), (1 << 9), (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
	(1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23), (1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), (1 << 31),
	0, 0 // invalid no bits set
};

// Convert from 64 to 32 board
constexpr int32_t Board64to32[66] = {
	INV, 28, INV, 29, INV, 30, INV, 31,	24, INV, 25, INV, 26, INV, 27, INV, INV, 20, INV, 21, INV, 22, INV, 23,	16, INV, 17, INV, 18, INV, 19, INV,
	INV, 12, INV, 13, INV, 14, INV, 15,	8, INV, 9, INV, 10, INV, 11, INV, INV, 4, INV, 5, INV, 6, INV, 7,	0, INV, 1, INV, 2, INV, 3, INV 
};

constexpr uint32_t MASK_RANK[8] = {
	S[0] | S[1] | S[2] | S[3],
	S[4] | S[5] | S[6] | S[7],
	S[8] | S[9] | S[10] | S[11],
	S[12] | S[13] | S[14] | S[15],
	S[16] | S[17] | S[18] | S[19],
	S[20] | S[21] | S[22] | S[23],
	S[24] | S[25] | S[26] | S[27],
	S[28] | S[29] | S[30] | S[31] 
};

// Returns rank relative to color c, from 1-8
constexpr uint32_t RankMask(const eColor c, const int rank) { return MASK_RANK[c == BLACK ? rank - 1 : 8 - rank]; }

constexpr uint32_t MASK_ODD_ROW = MASK_RANK[0] | MASK_RANK[2] | MASK_RANK[4] | MASK_RANK[6];

int FlipSqX(int x);
int StandardSquare(int sq);

// 16 bytes, bitboards
struct CheckerBitboards
{
	// data
	uint32_t P[2];	// Black & White Pieces
	uint32_t K;		// Kings (of either color)
	uint32_t empty;	// Empty squares

	// functions
	uint32_t inline GetJumpers(const eColor c) const;
	uint32_t inline GetMovers(const eColor c) const;
	uint32_t inline GetCheckers() const { return (P[WHITE] | P[BLACK]) & ~K; };
};

struct Board 
{
	void Clear();
	void SetFlags();
	uint64_t CalcHashKey();

	int EvaluateBoard( int ply, struct SearchThreadData& search, const struct EvalNetInfo& netInfo, int depth) const;
	int AllKingsEval() const;
	int dbWinEval(int dbresult) const;

	std::string ToString();
	int FromString( char *text );

	static Board StartPosition();
	Board Flip();

	int DoMove( const struct Move &Move);
	void DoSingleJump( int src, int dst, const ePieceType piece, const eColor color);
	void CheckKing( int dst, ePieceType piece);

	void SetPiece(int sq, int piece);
	inline ePieceType GetPiece(int sq) const
	{
		if (S[sq] & Bitboards.P[BLACK])
		{
			return (S[sq] & Bitboards.K) ? BKING : BPIECE;
		}
		if (S[sq] & Bitboards.P[WHITE])
		{
			return (S[sq] & Bitboards.K) ? WKING : WPIECE;
		}

		return (sq >= 32) ? INVALID : EMPTY;
	}
	inline int GetJumpSq(int src, int dst) const
	{
		int jumpedSq = ((dst + src) >> 1);
		if (S[jumpedSq] & MASK_ODD_ROW) jumpedSq += 1; // correct for square number since the jumpedSq sometimes up 1 sometimes down 1 of the average
		return jumpedSq;

	}
	inline int TotalPieces() const
	{
		return numPieces[BLACK] + numPieces[WHITE];
	}

	// Data
	CheckerBitboards Bitboards;
	uint64_t hashKey;
	eColor sideToMove;
	uint8_t reversibleMoves;
	int8_t numPieces[2];
};