// Board.cpp
// by Jonathan Kreuzer
// copyright 2005, 2009, 2020
//
// Checkers Board 
// this is bitboard representation with bit numbers arranged in board layout
//
//   28  29  30  31
// 24  25  26  27
//   20  21  22  23
// 16  17  18  19
//   12  13  14  15
// 08  09  10  11
//   04  05  06  07
// 00  01  02  03
//

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "engine.h"

void Board::Clear( )
{
	Bitboards.P[BLACK] = 0;
	Bitboards.P[WHITE] = 0;
	Bitboards.K = 0;
	reversibleMoves = 0;
	SetFlags();
}

void Board::SetPiece(int sq, int piece)
{
	// Clear square first
	Bitboards.P[WHITE] &= ~S[sq];
	Bitboards.P[BLACK] &= ~S[sq];
	Bitboards.K &= ~S[sq];

	// Set the piece
	if (piece & WPIECE)
		Bitboards.P[WHITE] |= S[sq];
	if (piece & BPIECE)
		Bitboards.P[BLACK] |= S[sq];
	if (piece & KING)
		Bitboards.K |= S[sq];
}

Board Board::StartPosition()
{
	Board b;
	b.Clear();

	b.Bitboards.P[BLACK] = RankMask(BLACK,1) | RankMask(BLACK, 2) | RankMask(BLACK, 3);
	b.Bitboards.P[WHITE] = RankMask(WHITE,1) | RankMask(WHITE, 2) | RankMask(WHITE, 3);
	b.sideToMove = BLACK;

	b.SetFlags();
	return b;
}

static inline uint32_t FlipBb( uint32_t bb )
{
	uint32_t ret = 0;
	for (int i = 0; i < 32; i++)
		ret |= ((bb >> i) & 1) << (31 - i);
	return ret;
}

// Rotate the board 180 and swap the colors
Board Board::Flip()
{
	Board ret;
	ret.sideToMove = Opp(sideToMove);
	ret.Bitboards.K = FlipBb(Bitboards.K);
	ret.Bitboards.P[WHITE] = FlipBb(Bitboards.P[BLACK]);
	ret.Bitboards.P[BLACK] = FlipBb(Bitboards.P[WHITE]);
	ret.SetFlags();
	return ret;
}

uint64_t Board::CalcHashKey()
{
	uint64_t CheckSum = 0;

	for (int index = 0; index < 32; index++)
	{
		ePieceType piece = GetPiece(index);
		if (piece != EMPTY) CheckSum ^= TranspositionTable::HashFunction[index][piece];
	}
	if (sideToMove == BLACK)
	{
		CheckSum ^= TranspositionTable::HashSTM;
	}
	return CheckSum;
}

void Board::SetFlags( )
{
	numPieces[WHITE] = 0; 
	numPieces[BLACK] = 0; 
	Bitboards.empty = ~(Bitboards.P[WHITE] | Bitboards.P[BLACK]);

	for (int i = 0; i < 32; i++) 
	{
		ePieceType piece = GetPiece( i );
		if ( piece == WKING || piece == WPIECE ) 
		{
			numPieces[WHITE]++;
		}
		if (piece == BKING || piece == BPIECE )
		{
			numPieces[BLACK]++;
		}
	}
	hashKey = CalcHashKey();
}

// ---------------------
//  Helper Functions for DoMove 
// ---------------------
void inline Board::DoSingleJump( int src, int dst, const ePieceType piece, const eColor color )
{
	int jumpedSq = GetJumpSq(src, dst);

	// Update Piece Count
	numPieces[Opp(color)]--;

	// Update Hash Key
	const ePieceType jumpedPiece = GetPiece(jumpedSq);
	hashKey ^= TranspositionTable::HashFunction[src][piece] ^ TranspositionTable::HashFunction[dst][piece];
	hashKey	^= TranspositionTable::HashFunction[jumpedSq][jumpedPiece];

	// Update the bitboards
	uint32_t BitMove = SqBit( src ) | SqBit( dst );

	Bitboards.P[color] ^= BitMove;
	Bitboards.P[Opp(color)] ^= SqBit(jumpedSq);
	Bitboards.K  &= ~SqBit(jumpedSq);
	Bitboards.empty ^= BitMove ^ SqBit(jumpedSq);
	if (piece & KING) Bitboards.K ^= BitMove;
}

// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
void inline Board::CheckKing( const int dst, const ePieceType piece )
{
	if ( dst <= 3 || dst >= 28 ) 
	{
		hashKey  ^= TranspositionTable::HashFunction[ dst ][piece] ^ TranspositionTable::HashFunction[ dst ][piece | KING ];
		Bitboards.K |= S[ dst ];
	}
}

// ---------------------
//  Execute a Move 
//  Returns 1 if it's a non reversible move (anything but moving a king around.)
// ---------------------
int Board::DoMove( const Move &Move )
{
	const int src = Move.Src();
	int dst = Move.Dst();
	const int jumpLen = Move.JumpLen();
	const ePieceType piece = GetPiece(src);
	const eColor color = sideToMove;

	// Flip sideToMove
	sideToMove = Opp(sideToMove);
	hashKey ^= TranspositionTable::HashSTM; 
		
	if (jumpLen == 0)
	{
		// Perform Non-Jump move
		// Update the bitboards
		uint32_t BitMove = S[ src ] | S[ dst ];
		Bitboards.P[color] ^= BitMove;
		Bitboards.empty ^= BitMove;
		if (piece & KING) { Bitboards.K ^= BitMove; }

		// Update hash values
		hashKey ^= TranspositionTable::HashFunction[ src ][piece] ^ TranspositionTable::HashFunction[ dst ][piece];
		
		if (piece < KING) {
			CheckKing(dst, piece);
			reversibleMoves = 0;
			return 1;
		}

		reversibleMoves++;
		return 0;
	}

	// Perform Jump Move
	DoSingleJump( src, dst, piece, color );
	reversibleMoves = 0;

	if ( jumpLen > 1 ) 
	{
		// Double jump path
		int nextDst = dst;
		for (int i = 0; i < jumpLen - 1; i++)
		{
			nextDst += JumpAddDir[Move.Dir(i)];

			DoSingleJump(dst, nextDst, piece, color);
			dst = nextDst;
		}
	}

	if (piece < KING)
		CheckKing(dst, piece);

    return 1;
}

// ------------------
// Flip square horizontally because the internal board is flipped.
// ------------------
int FlipSqX(int sq )
{	
	int x = sq&3;
	sq ^= x;
	sq += 3-x;
	return sq;
}

int StandardSquare( int sq )
{
	// Convert internal bitboard bit index to standard checkers board square
	return FlipSqX(sq) + 1;
}

// ------------------
// Position Copy & Paste Functions
// ------------------
std::string Board::ToString()
{	
	std::string ret = (sideToMove == WHITE) ? "W:" : "B:";

	ret += "W";
	int pc = 0;
	for (int i = 0; i < 32; i++) {
		if ((GetPiece(i) & WPIECE) && pc > 0) { ret += ","; }
		if (GetPiece(i) == WKING) { ret += "K"; }
		if (GetPiece(i) & WPIECE) {
			ret += std::to_string(StandardSquare(i));
			pc++;
		}
	}

	ret += ":B";
	pc = 0;
	for (int i = 0; i < 32; i++) {
		if ((GetPiece(i) & BPIECE) && pc > 0) { ret += ","; }
		if (GetPiece(i) == BKING) { ret += "K"; }
		if (GetPiece(i) & BPIECE) { 
			ret += std::to_string(StandardSquare(i));
			pc++;
		}
	}
	ret += '.';

	return ret;
}

int Board::FromString( char *text )
{
	int nColor = 0, i = 0;
	while (text[i] == ' ') i++;
	if ((text[i] != 'W' && text[i]!='B') || text[i+1]!=':') return 0;
	Clear();

	if (text[i] == 'W') sideToMove = WHITE;
	if (text[i] == 'B') sideToMove = BLACK;

	while (text[i] != 0 && text[i]!= '.' && text[i-1]!= '.')
	{
		int nKing = 0;
		if (text[i] == 'W') nColor = WPIECE;
		if (text[i] == 'B') nColor = BPIECE;
		if (text[i] == 'K') {nKing = 4; i++; }
		if (text[i] >= '0' && text[i] <= '9')
		{
			int sq = text[i]-'0';
			i++;
			if (text[i] >= '0' && text[i] <= '9') sq = sq*10 + text[i]-'0';
			SetPiece(FlipSqX(sq-1), nColor | nKing );
		}
		i++;
	}

	SetFlags( );
	return 1;
}
