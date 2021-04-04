// 
// MoveGen.cpp
// by Jonathan Kreuzer
//
// Contains functions the use the bitboards in a Board to generate a list of valid moves.
// In checkers if there are any jumps you have to jump, so will try to find jumps first.
//
#include <cstdint>
#include <assert.h>

#include "board.h"
#include "moveGen.h"

// Create lookup tables used for finding the lowest 1 bit, highest 1 bit, and the number of 1 bits in a 16-bit integer
// Newer processors will use hardward instructions instead of these tables.
unsigned char aBitCount[65536];
unsigned char aLowBit[65536];
unsigned char aHighBit[65536];

void InitBitTables()
{
	for (int moves = 0; moves < 65536; moves++) 
	{
		int nBits = 0;
		int nLow = 255, nHigh = 255;
		for (int i = 0; i < 16; i++) {
			if ( moves & S[i] )
			{
				nBits++;
				if (nLow == 255) nLow = i;
				nHigh = i;
			}
		}
		aLowBit[ moves ] = nLow;
		aHighBit[ moves ] = nHigh;
		aBitCount[ moves ] = nBits;
	}
}

// Return a bitboard of pieces of color param that have a possible non-jumping move. 
// We start with the empty sq bitboard, shift for each possible move directions
// and or all the directions together to get the movable pieces bitboard.
uint32_t CheckerBitboards::GetMovers( const eColor color ) const
{
	assert(color == WHITE || color == BLACK);
	uint32_t movers = 0;
	if (color == WHITE)
	{
		movers = (empty << 4);
		const uint32_t WK = P[WHITE] & K;		  // Kings
		movers |= ((empty & MASK_L3) << 3);
		movers |= ((empty & MASK_L5) << 5);
		movers &= P[WHITE];
		if (WK) {
			movers |= (empty >> 4) & WK;
			movers |= ((empty & MASK_R3) >> 3) & WK;
			movers |= ((empty & MASK_R5) >> 5) & WK;
		}
	}
	else // BLACK
	{
		movers = (empty >> 4);
		const uint32_t BK = P[BLACK] & K;
		movers |= ((empty & MASK_R3) >> 3);
		movers |= ((empty & MASK_R5) >> 5);
		movers &= P[BLACK];
		if (BK) {
			movers |= (empty << 4) & BK;
			movers |= ((empty & MASK_L3) << 3) & BK;
			movers |= ((empty & MASK_L5) << 5) & BK;
		}
	}
	return movers;
}

// Return a list of pieces of color param that have a possible jump move
// To do this we start with a bitboard of empty squares and shift it to check each direction for a black piece, 
// then shift the passing bits to find the white pieces with a jump. Moveable pieces from the different direction checks are or'd together.
uint32_t CheckerBitboards::GetJumpers( const eColor c ) const
{
	uint32_t jumpers = 0;
	if (c == WHITE)
	{
		const uint32_t WK = P[WHITE] & K; // WK = White Kings bitboard
		uint32_t Temp = (empty << 4) & P[BLACK];
		jumpers |= (((Temp & MASK_L3) << 3) | ((Temp & MASK_L5) << 5));
		Temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & P[BLACK];
		jumpers |= (Temp << 4);
		jumpers &= P[WHITE];
		if (WK) {
			Temp = (empty >> 4) & P[BLACK];
			jumpers |= (((Temp & MASK_R3) >> 3) | ((Temp & MASK_R5) >> 5)) & WK;
			Temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & P[BLACK];
			jumpers |= (Temp >> 4) & WK;
		}
	}
	else // BLACK
	{
		const uint32_t BK = P[BLACK] & K;
		uint32_t Temp = (empty >> 4) & P[WHITE];
		jumpers |= (((Temp & MASK_R3) >> 3) | ((Temp & MASK_R5) >> 5));
		Temp = (((empty & MASK_R3) >> 3) | ((empty & MASK_R5) >> 5)) & P[WHITE];
		jumpers |= (Temp >> 4);
		jumpers &= P[BLACK];
		if (BK) {
			Temp = (empty << 4) & P[WHITE];
			jumpers |= (((Temp & MASK_L3) << 3) | ((Temp & MASK_L5) << 5)) & BK;
			Temp = (((empty & MASK_L3) << 3) | ((empty & MASK_L5) << 5)) & P[WHITE];
			jumpers |= (Temp << 4) & BK;
		}
	}
	return jumpers;
}

// -------------------------------------------------
// Find the moves available on board, and store them in Movelist
// -------------------------------------------------
void MoveList::FindMoves(Board& board)
{
	uint32_t jumpers = board.Bitboards.GetJumpers( board.sideToMove );

	if (jumpers) // If there are any jump moves possible
		FindJumps(board.sideToMove, board.Bitboards, jumpers); // We fill this movelist with the jump moves
	else
		FindNonJumps(board.sideToMove, board.Bitboards, board.Bitboards.GetMovers(board.sideToMove)); // Otherwise we fill the movelist with non-jump moves
}

// -------------------------------------------------
// These two functions only add non-jumps
// -------------------------------------------------
void MoveList::FindNonJumps( const eColor color, const CheckerBitboards& Bb, uint32_t Movers )
{
	Clear();

	while (Movers) 
	{
		uint32_t sq = (color == WHITE) ? PopLowSq(Movers) : PopHighSq(Movers);  // Pop a piece from the list of of movable pieces

		AddNormalMove(Bb, sq, ForwardLeft[color]); // Check the 2 forward directions and add valid moves
		AddNormalMove(Bb, sq, ForwardRight[color]);

		if (SqBit(sq) & Bb.K) { // For Kings only
			AddNormalMove(Bb, sq, BackwardLeft[color]); // Check the 2 backward directions and add valid moves
			AddNormalMove(Bb, sq, BackwardRight[color]);
		}
	}
}

// -------------------------------------------------
// These two functions only add jumps
// -------------------------------------------------
void MoveList::FindJumps(const eColor color, CheckerBitboards& B, uint32_t Movers)
{
	Clear();
	while (Movers)
	{
		uint32_t sq = (color == WHITE ) ? PopLowSq(Movers) : PopHighSq(Movers);  // Pop a piece from the list of of movable pieces

		uint32_t oldEmpty = B.empty;
		B.empty |= SqBit(sq); // Temporarly mark the piece square as empty to allow cyclic jump moves

		CheckJumpDir(color, B, sq, ForwardLeft[color]); // Check the two forward directions
		CheckJumpDir(color, B, sq, ForwardRight[color]);
		if (B.K & SqBit(sq)) // For Kings only
		{
			CheckJumpDir(color, B, sq, BackwardRight[color]); // Check the two backward directions
			CheckJumpDir(color, B, sq, BackwardLeft[color]);
		}
		B.empty = oldEmpty;
	}
	numMoves = numJumps;
}

// Check for a jump
void inline MoveList::CheckJumpDir( const eColor color, CheckerBitboards&B, int square, const int DIR_INDEX )
{
	int jumpSquare = nextSq[square+DIR_INDEX];
	if ( S[jumpSquare] & B.P[Opp(color)]) // If there is an opponent's piece in this direction
	{
		int	dstSq = nextSq[jumpSquare+DIR_INDEX];
		if ( B.empty & S[dstSq] ) // And the next square in this direction is empty
		{
			// Then a jump is possible, call FindSqJumps to detect double/triple/etc. jumps
			StartJumpMove( square, dstSq );
			FindSqJumps( color, B, dstSq, 0, jumpSquare, S[square]&B.K );
		} 
	}
}

// -------------
//  If a jump move was possible, we must make the jump then check to see if the same piece can jump again.
//  There might be multiple paths a piece can take on a double jump, these functions store each possible path as a move.
// -------------
int inline MoveList::AddSqDir(const eColor color, CheckerBitboards& B, int square, const bool isKing, int pathIdx, const int DIR_INDEX )
{
	int jumpSquare = nextSq[square+DIR_INDEX];
	if ( S[jumpSquare] & B.P[Opp(color)])
	{
		int	dstSq = nextSq[jumpSquare+DIR_INDEX];
		if ( B.empty & S[dstSq] )
		{
			// jump is possible in this direction, see if the piece can jump more times
			m_JumpMove.SetJumpDir( pathIdx, (DIR_INDEX>>5) );
			FindSqJumps( color, B, dstSq, pathIdx +1, jumpSquare, isKing );
			return 1;
		}
	}
	return 0;
}

void MoveList::FindSqJumps( const eColor color, CheckerBitboards& C, int square, int pathNum, int jumpSquare, const bool isKing )
{
	// Remove the jumped piece (until the end of this function), so we can't jump it again
	int oldPieces = C.P[Opp(color)];
	C.P[Opp(color)] ^= S[jumpSquare];

	// Now see if a piece on this square has more jumps
	int numMoves = AddSqDir(color, C, square, isKing, pathNum, ForwardRight[color] )
				 + AddSqDir(color, C, square, isKing, pathNum, ForwardLeft[color] );

	if (isKing)
	{
		// If this piece is a king, it can also jump backwards	
		numMoves += AddSqDir(color, C, square, isKing, pathNum, BackwardLeft[color] )
				  + AddSqDir(color, C, square, isKing, pathNum, BackwardRight[color] );
	}

	// if this is a leaf store the move
	if (numMoves == 0) 
		AddJump( m_JumpMove, pathNum ); 

	// Put back the jumped piece
	C.P[Opp(color)] = oldPieces;
}

// For PDN support
int Move::GetFinalDst(const Move& Move)
{
	int sq = Move.Dst();
	for (int i = 0; i < Move.JumpLen() - 1; i++)
	{
		sq += JumpAddDir[Move.Dir(i)];
	}
	return sq;
}
