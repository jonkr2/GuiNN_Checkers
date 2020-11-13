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
const uint32_t S[34] = {
	(1 << 0), (1 << 1), (1 << 2), (1 << 3), (1 << 4), (1 << 5), (1 << 6), (1 << 7), (1 << 8), (1 << 9), (1 << 10), (1 << 11), (1 << 12), (1 << 13), (1 << 14), (1 << 15),
	(1 << 16), (1 << 17), (1 << 18), (1 << 19), (1 << 20), (1 << 21), (1 << 22), (1 << 23), (1 << 24), (1 << 25), (1 << 26), (1 << 27), (1 << 28), (1 << 29), (1 << 30), (1 << 31),
	0, 0 // invalid no bits set
};

// Mirrored squares
const uint32_t FlipS[34] = {
	(1 << 31), (1 << 30), (1 << 29), (1 << 28), (1 << 27), (1 << 26), (1 << 25), (1 << 24), (1 << 23), (1 << 22), (1 << 21), (1 << 20), (1 << 19), (1 << 18), (1 << 17), (1 << 16),
	(1 << 15), (1 << 14), (1 << 13), (1 << 12), (1 << 11), (1 << 10), (1 << 9), (1 << 8), (1 << 7), (1 << 6), (1 << 5), (1 << 4), (1 << 3), (1 << 2), (1 << 1), (1 << 0),
	0, 0
};

// Convert from 64 to 32 board

const int32_t BoardLoc[66] =
{ INV, 28, INV, 29, INV, 30, INV, 31,	24, INV, 25, INV, 26, INV, 27, INV, INV, 20, INV, 21, INV, 22, INV, 23,	16, INV, 17, INV, 18, INV, 19, INV,
	INV, 12, INV, 13, INV, 14, INV, 15,	8, INV, 9, INV, 10, INV, 11, INV, INV, 4, INV, 5, INV, 6, INV, 7,	0, INV, 1, INV, 2, INV, 3, INV };


int FlipSqX(int x);
int StandardSquare(int sq);

// 16 bytes, bitboards
struct SCheckerBitboards
{
	// data
	uint32_t P[2];	// Black & White Pieces
	uint32_t K;		// Kings (of either color)
	uint32_t empty;	// Empty squares

	// functions
	int inline CheckerMoves(const eColor c, uint32_t checkers) const;
	uint32_t GetJumpers(const eColor c) const;
	uint32_t inline GetMovers(const eColor c) const;
	uint32_t GetCheckers() const { return (P[WHITE] | P[BLACK]) & ~K; };
};

struct SBoard 
{
	void Clear();
	void SetFlags();
	uint64_t CalcHashKey();

	int EvaluateBoard( int ahead, uint64_t& databaseNodes ) const;
	int FinishingEval() const;

	std::string ToFen();
	int FromFen( char *sFEN );

	static SBoard StartPosition();
	SBoard Flip();

	int DoMove( const struct SMove &Move);
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
	inline int TotalPieces() const
	{
		return numPieces[BLACK] + numPieces[WHITE];
	}

	// Data
	SCheckerBitboards Bitboards;
	uint64_t HashKey;
	eColor SideToMove;
	int8_t numPieces[2];
	uint8_t reversibleMoves;
};