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

void SBoard::Clear( )
{
	Bitboards.P[BLACK] = 0;
	Bitboards.P[WHITE] = 0;
	Bitboards.K = 0;
	reversibleMoves = 0;
	SetFlags();
}

void SBoard::SetPiece(int sq, int piece)
{
	// Clear square first
	Bitboards.P[WHITE] &= ~S[sq];
	Bitboards.P[BLACK] &= ~S[sq];
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

SBoard SBoard::StartPosition()
{
	SBoard b;
	b.Clear();

	b.Bitboards.P[BLACK] = RankMask(BLACK,1) | RankMask(BLACK, 2) | RankMask(BLACK, 3);
	b.Bitboards.P[WHITE] = RankMask(WHITE,1) | RankMask(WHITE, 2) | RankMask(WHITE, 3);
	b.SideToMove = BLACK;

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
SBoard SBoard::Flip()
{
	SBoard ret;
	ret.SideToMove = Opp(SideToMove);
	ret.Bitboards.K = FlipBb(Bitboards.K);
	ret.Bitboards.P[WHITE] = FlipBb(Bitboards.P[BLACK]);
	ret.Bitboards.P[BLACK] = FlipBb(Bitboards.P[WHITE]);
	ret.SetFlags();
	return ret;
}

uint64_t SBoard::CalcHashKey()
{
	uint64_t CheckSum = 0;

	for (int index = 0; index < 32; index++)
	{
		int nPiece = GetPiece(index);
		if (nPiece != EMPTY) CheckSum ^= TranspositionTable::HashFunction[index][nPiece];
	}
	if (SideToMove == BLACK)
	{
		CheckSum ^= TranspositionTable::HashSTM;
	}
	return CheckSum;
}

void SBoard::SetFlags( )
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
	HashKey = CalcHashKey();
}

// ---------------------
//  Helper Functions for DoMove 
// ---------------------
void inline SBoard::DoSingleJump( int src, int dst, const ePieceType piece, const eColor color )
{
	int jumpedSq = ((dst + src) >> 1 );
	if ( S[jumpedSq] & MASK_ODD_ROW ) jumpedSq += 1; // correct for square number since the jumpedSq sometimes up 1 sometimes down 1 of the average

	ePieceType jumpedPiece = GetPiece( jumpedSq );

	// Update Piece Count
	if ( jumpedPiece == WPIECE || jumpedPiece == WKING ) { numPieces[WHITE]--; }
	else { numPieces[BLACK]--; }

	// Update Hash Key
	HashKey ^= TranspositionTable::HashFunction[src][piece] ^ TranspositionTable::HashFunction[dst][piece];
	HashKey	^= TranspositionTable::HashFunction[jumpedSq][jumpedPiece];

	// Update the bitboards
	uint32_t BitMove = (S[ src ] | S[ dst ]);
	if (color == WHITE) {
		Bitboards.P[WHITE] ^= BitMove;
		Bitboards.P[BLACK] ^= S[jumpedSq];
		Bitboards.K  &= ~S[jumpedSq];
	} else {
		Bitboards.P[BLACK] ^= BitMove;
		Bitboards.P[WHITE] ^= S[jumpedSq];
		Bitboards.K  &= ~S[jumpedSq];
	}
	Bitboards.empty = ~(Bitboards.P[WHITE] | Bitboards.P[BLACK]);
	if (piece & KING) Bitboards.K ^= BitMove;
}

// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
void inline SBoard::CheckKing( const int dst, const ePieceType Piece )
{
	if ( dst <= 3 || dst >= 28 ) 
	{
		HashKey  ^= TranspositionTable::HashFunction[ dst ][Piece] ^ TranspositionTable::HashFunction[ dst ][ Piece | KING ];
		Bitboards.K |= S[ dst ];
	}
}

// ---------------------
//  Execute a Move 
//  Returns 1 if it's a non reversible move (anything but moving a king around.)
// ---------------------

int SBoard::DoMove( const SMove &Move )
{
	const int src = Move.Src();
	int dst = Move.Dst();
	const int jumpLen = Move.JumpLen();
	const ePieceType piece = GetPiece(src);
	const eColor color = SideToMove;

	// Flip SideToMove
	SideToMove = Opp(SideToMove);
	HashKey ^= TranspositionTable::HashSTM; 
		
	if (jumpLen == 0)
	{
		// Perform Non-Jump move
		// Update the bitboards
		uint32_t BitMove = S[ src ] | S[ dst ];
		Bitboards.P[color] ^= BitMove;
		Bitboards.empty ^= BitMove;
		if (piece & KING) { Bitboards.K ^= BitMove; }

		// Update hash values
		HashKey ^= TranspositionTable::HashFunction[ src ][piece] ^ TranspositionTable::HashFunction[ dst ][piece];
		
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

	if ( jumpLen == 1 ) 
	{
		if (piece < KING)
			CheckKing(dst, piece);

		// not a double jump, we are done now
		return 1;
	}

	// Double jump path
	int nextDst = dst;
	for (int i = 0; i < jumpLen-1; i++)
	{
		nextDst += JumpAddDir[ Move.Dir(i) ]; 

		DoSingleJump( dst, nextDst, piece, color );
		dst = nextDst;
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
	int y = sq&3;
	sq ^= y;
	sq += 3-y;
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
void SBoard::ToFen( char *sFEN )
{
	char buffer[80];
	int i;
	if (SideToMove == WHITE) strcpy( sFEN, "W:"); 
		else strcpy( sFEN, "B:");

	strcat( sFEN, "W");
	for (i = 0; i < 32; i++) {
		if ( GetPiece( i ) == WKING) strcat(sFEN, "K");
		if ( GetPiece( i )&WPIECE)  {
			strcat( sFEN, _ltoa (FlipSqX(i)+1, buffer, 10) );
			strcat( sFEN, ","); 
		}
	}
	if (strlen(sFEN) > 3) sFEN [ strlen(sFEN)-1 ] = NULL;

	strcat( sFEN, ":B");
	for (i = 0; i < 32; i++) {
		if ( GetPiece( i ) == BKING) strcat(sFEN, "K");
		if ( GetPiece( i )&BPIECE) {
			strcat( sFEN, _ltoa (FlipSqX(i)+1, buffer, 10) );
			strcat( sFEN, ","); 
		}
	}
	sFEN[ strlen(sFEN)-1 ] = '.';
}

int SBoard::FromFen( char *sFEN )
{
	int nColor = 0, i = 0;
	while (sFEN[i] == ' ') i++;
	if ((sFEN[i] != 'W' && sFEN[i]!='B') || sFEN[i+1]!=':') return 0;
	Clear();

	if (sFEN[i] == 'W') SideToMove = WHITE;
	if (sFEN[i] == 'B') SideToMove = BLACK;

	while (sFEN[i] != 0 && sFEN[i]!= '.' && sFEN[i-1]!= '.')
	{
		int nKing = 0;
		if (sFEN[i] == 'W') nColor = WPIECE;
		if (sFEN[i] == 'B') nColor = BPIECE;
		if (sFEN[i] == 'K') {nKing = 4; i++; }
		if (sFEN[i] >= '0' && sFEN[i] <= '9')
		{
			int sq = sFEN[i]-'0';
			i++;
			if (sFEN[i] >= '0' && sFEN[i] <= '9') sq = sq*10 + sFEN[i]-'0';
			SetPiece(FlipSqX(sq-1), nColor | nKing );
		}
		i++;
	}

	SetFlags( );
	return 1;
}
