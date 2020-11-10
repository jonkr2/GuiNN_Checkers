#pragma once
#include <nmmintrin.h>
#include <intrin.h>
#include <assert.h>

const uint32_t MAX_JUMP_PATH = 8;

struct SMove
{
	uint32_t data; // src(5), dst(5), jumpLen(5), jumpPathDirs(16) 8 * 2 bits, can jump 4 directions, unused(1)

	SMove() {}
	explicit SMove(uint32_t val) { data = val; }

	inline int Src() const { return (data & 31); }
	inline int Dst() const { return (data >> 5) & 31; }

	inline int Dir(const int i = 0) const { return (data >> (i * 2 + 15)) & 3; }
	inline void SetJumpDir(int i, uint32_t sq) {
		int shift = (i * 2 + 15);
		data &= ~(0xFFFFFFFFULL << shift); // TODO : is there a better way to clear this for when multiple jump paths are possible?
		data |= sq << shift;
	}

	inline int JumpLen() const { return (data >> 10) & 15; }
	inline void SetJumpLen(int jumpLen) { data &= ~(15 << 10); data |= (jumpLen << 10); }

	inline bool operator==(const SMove& b) const { return data == b.data; }
	inline bool operator!=(const SMove& b) const { return data != b.data; }

	static int GetFinalDst(const SMove& Move);
};

static const SMove NO_MOVE(0);

// This is a 32x4 ( 32 source squares x 4 move directions ) lookup table that will return the destination square index for the direction
const int nextSq[32 * 4] = {
	4,5,6,7,
	9,10,11,INV,
	12,13,14,15,
	17,18,19,INV,
	20,21,22,23,
	25,26,27,INV,
	28,29,30,31,
	INV,INV,INV,INV,

	INV,4,5,6,
	8,9,10,11,
	INV,12,13,14,
	16,17,18,19,
	INV,20,21,22,
	24,25,26,27,
	INV,28,29,30,
	INV,INV,INV,INV,

	INV,INV,INV,INV,
	0,1,2,3,
	INV,4,5,6,
	8,9,10,11,
	INV,12,13,14,
	16,17,18,19,
	INV,20,21,22,
	24,25,26,27,

	INV,INV,INV,INV,
	1,2,3,INV,
	4,5,6,7,
	9,10,11,INV,
	12,13,14,15,
	17,18,19,INV,
	20,21,22,23,
	25,26,27,INV
};

//
// MoveList
//
class CMoveList
{
public:
	static const int MAX_MOVES = 36;
	static const int MAX_MOVE_PATH = 10;

	// DATA
	int numMoves = 0;
	int numJumps = 0;
	SMove m_JumpMove;
	SMove Moves[MAX_MOVES];

	// FUNCTIONS
	void inline Clear()
	{
		numJumps = 0;
		numMoves = 0;
	}

	void FindMoves(const eColor color, SCheckerBitboards& B);
	void FindJumps(const eColor color, SCheckerBitboards& B, uint32_t Movers);
	void FindNonJumps(const eColor color, const SCheckerBitboards& B, uint32_t Movers);

	int inline FindIndex(const SMove& move) const
	{
		for (int i = 0; i < numMoves; i++)
		{
			if (Moves[i].data == move.data) {
				return i;
			}
		}
		return -1;
	}

	// Sorting
	bool inline SwapToFront(const SMove& move)
	{
		for (int i = 0; i < numMoves; i++)
		{
			if (Moves[i] == move) {
				SMove temp = Moves[0];
				Moves[0] = move;
				Moves[i] = temp;
				return true;
			}
		}
		return false;
	}

	// Sort jumps based on expected material change
	inline void SortJumps(int startIdx, const SCheckerBitboards& B)
	{
		/*int Scores[32];
		for (int i = 0; i < numJumps; i++) {
			Scores[i] = Moves[i].JumpLen() + ((SqBit(Moves[i].Src()) & B.K) == 0);  // TODO : check for jump backs, to be more often helpful in sorting
		}*/
		// Insertion Sort
		for (int d = startIdx + 1; d < numJumps; d++)
		{
			const SMove tempMove = Moves[d];
			int tempScore = Moves[d].JumpLen();
			int i;
			for (i = d; i > 0 && tempScore > Moves[i - 1].JumpLen(); i--)
			{
				Moves[i] = Moves[i - 1];
				//Scores[i] = Scores[i - 1];
			}
			Moves[i] = tempMove;
			//Scores[i] = tempScore;
		}
	}

private:

	int inline AddSqDir(const eColor color, SCheckerBitboards& B, int square, const bool isKing, int pathNum, const int dirFlag);
	void inline CheckJumpDir(const eColor, SCheckerBitboards& B, int square, const int DIR);
	void inline FindSqJumps(const eColor color, SCheckerBitboards& B, int square, int pathNum, int jumpSquare, const bool isKing);

	void inline StartJumpMove(int src, int dst)
	{
		m_JumpMove.data = (src)+(dst << 5);
	}

	void inline AddJump(SMove& Move, int pathNum)
	{
		assert(pathNum < MAX_MOVE_PATH);
		assert(numJumps < MAX_MOVES);

		Move.SetJumpLen(pathNum + 1);
		int jumpLen = Move.JumpLen();
		Moves[numJumps++] = Move;
	}

	void inline AddMove(int src, int dst, int dirFlag)
	{
		assert(numMoves < MAX_MOVES);

		Moves[numMoves++].data = (src)+(dst << 5) + (dirFlag << 15);
		return;
	}

	// Add a move if the destination square is empty
	void inline AddNormalMove(const SCheckerBitboards& C, const int src, const int dirFlag)
	{
		int dst = nextSq[src + dirFlag];
		if (S[dst] & C.empty)
			AddMove(src, dst, dirFlag);
	}
};

void InitBitTables();

// BITBOARD helpers
// Return the number of 1 bits in a 32-bit int
int inline BitCount(uint32_t Moves)
{
#ifdef __GNUC__	
	return __builtin_popcountll(Moves);
#else
	return _mm_popcnt_u32(Moves);

	/*if (Moves == 0) return 0;
	return aBitCount[ (Moves & 65535) ] + aBitCount[ ((Moves>>16) & 65535) ];*/
#endif

}

// Find the index of the "lowest" 1 bit of a 32-bit int
uint32_t inline FindLowBit(uint32_t Moves)
{
	/*
	if ( (Moves & 65535) ) return aLowBit[ (Moves & 65535) ];
	if ( ((Moves>>16) & 65535) ) return aLowBit[ ((Moves>>16) & 65535) ] + 16;
	*/
#ifdef __GNUC__
	// TODO : can we check this some other way, or not call it if zero, (eg. put assert(bb) back )
	if (bb == 0) return 0;
	unsigned long  sq = __builtin_ctzll(bb);
	return sq;
#else
	unsigned long sq;
	if (_BitScanForward(&sq, Moves)) { return sq; }
#endif
	return 0;
}

// Find the "highest" 1 bit of a 32-bit int
uint32_t inline FindHighBit(uint32_t Moves)
{
	/*if ( ((Moves>>16) & 65535) ) return aHighBit[ ((Moves>>16) & 65535) ] + 16;
	if ( (Moves & 65535) ) return aHighBit[ (Moves & 65535) ];*/
#ifdef __GNUC__
	return 63 - __builtin_clzll(bb);
#else
	unsigned long sq;
	if (_BitScanReverse(&sq, Moves)) { return sq; }
#endif
	return 0;
}

uint32_t inline PopLowSq(uint32_t& bb)
{
	uint32_t sq = FindLowBit(bb);
	bb ^= (1 << sq);
	return sq;
}

uint32_t inline PopHighSq(uint32_t& bb)
{
	uint32_t sq = FindHighBit(bb);
	bb ^= (1 << sq);
	return sq;
}


// same thing, can probably get rid of array
constexpr inline uint32_t SqBit(const int sq) { return 1 << sq; }
constexpr inline uint32_t SqBitFlip(const int sq) { return 1 << (31 - sq); }

enum dirFlags {
	DIR_UP_RIGHT = (0 << 5), DIR_UP_LEFT = (1 << 5), DIR_DOWN_LEFT = (2 << 5), DIR_DOWN_RIGHT = (3 << 5)
};
constexpr int JumpAddDir[4] = { 9, 7, -9, -7 };

constexpr dirFlags ForwardRight[2] = { DIR_UP_RIGHT, DIR_DOWN_RIGHT };
constexpr dirFlags ForwardLeft[2] = { DIR_UP_LEFT, DIR_DOWN_LEFT };
constexpr dirFlags BackwardRight[2] = { DIR_DOWN_RIGHT, DIR_UP_RIGHT };
constexpr dirFlags BackwardLeft[2] = { DIR_DOWN_LEFT, DIR_UP_LEFT };


// These masks are used in move generation. 
// They define squares for which a particular shift is possible. 
// eg. L3 = Left 3 = squares where you can move from src to src+3
const uint32_t MASK_L3 = S[1] | S[2] | S[3] | S[9] | S[10] | S[11] | S[17] | S[18] | S[19] | S[25] | S[26] | S[27];
const uint32_t MASK_L5 = S[4] | S[5] | S[6] | S[12] | S[13] | S[14] | S[20] | S[21] | S[22];
const uint32_t MASK_R3 = S[28] | S[29] | S[30] | S[20] | S[21] | S[22] | S[12] | S[13] | S[14] | S[4] | S[5] | S[6];
const uint32_t MASK_R5 = S[25] | S[26] | S[27] | S[17] | S[18] | S[19] | S[9] | S[10] | S[11];


const uint32_t MASK_RANK[8] = { S[0] | S[1] | S[2] | S[3],
							 S[4] | S[5] | S[6] | S[7],
							 S[8] | S[9] | S[10] | S[11],
							 S[12] | S[13] | S[14] | S[15],
							 S[16] | S[17] | S[18] | S[19],
							 S[20] | S[21] | S[22] | S[23],
							 S[24] | S[25] | S[26] | S[27],
							 S[28] | S[29] | S[30] | S[31] };

// Returns rank relative to color c, from 1-8
constexpr uint32_t RankMask(const eColor c, const int rank) { return MASK_RANK[c == BLACK ? rank - 1 : 8 - rank]; }

const uint32_t MASK_ODD_ROW = MASK_RANK[0] | MASK_RANK[2] | MASK_RANK[4] | MASK_RANK[6];
const uint32_t SINGLE_EDGE = S[0] | S[1] | S[2] | S[8] | S[16] | S[12] | S[20] | S[29] | S[30] | S[31] | S[23] | S[15];
const uint32_t CENTER_8 = S[9] | S[10] | S[13] | S[14] | S[17] | S[18] | S[21] | S[22];
const uint32_t DOUBLE_CORNER = S[3] | S[7] | S[24] | S[28];