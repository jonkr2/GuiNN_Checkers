//
// Checkers Win/Loss/Draw Database Generation & Probing
// by Jonathan Kreuzer
//
#include <initializer_list>
#include "defines.h"
#include "uncompress.h"
#include "engine.h"

const int SIZE2 = 32/4 * 32 * 2;
const int SIZE3 = SIZE2 * 32;
int SIZE4 = 0;
unsigned char ResultsTwo  [ 4 * SIZE2 ];
unsigned char ResultsThree[ 6 * SIZE3 ];
unsigned char *ResultsFour;
unsigned char *pResults;

int PC2[4*4] = { WKING, BKING, EMPTY, EMPTY,    WKING, BPIECE, EMPTY, EMPTY,
				 WPIECE, BKING, EMPTY, EMPTY,   WPIECE, BPIECE, EMPTY, EMPTY};
int PC3[4*12] = {WKING, WKING, BKING, EMPTY,    WKING, BKING, BKING, EMPTY,
				 WKING, WKING, BPIECE, EMPTY,   WPIECE, BKING, BKING, EMPTY, 
				 WKING, WPIECE, BKING, EMPTY,   WKING, BKING, BPIECE, EMPTY,
				 WPIECE, WPIECE, BKING, EMPTY,  WKING, BPIECE, BPIECE, EMPTY,
				 WKING, WPIECE, BPIECE, EMPTY,  WPIECE, BKING, BPIECE, EMPTY,
				 WPIECE, WPIECE, BPIECE, EMPTY, WPIECE, BPIECE, BPIECE, EMPTY};
int PC4[4*9] = 	{WKING, WKING, BKING, BKING,    WKING, WPIECE, BKING, BKING ,   WKING, WKING, BKING, BPIECE,
				 WKING, WPIECE, BKING, BPIECE,  WPIECE, WPIECE, BKING, BKING,   WKING, WKING, BPIECE, BPIECE,
				 WPIECE, WPIECE, BKING, BPIECE, WKING, WPIECE, BPIECE, BPIECE,  WPIECE, WPIECE, BPIECE, BPIECE };

int FourIndex[9] = { 0, 1, 9, 2, 3, 11, 4, 12, 5};
int ThreeIndex[12]= { 0, 8, 1, 9, 2, 10, 3, 11, 4, 12, 5, 13};
int FlipResult[9] = { 0, 2, 1, 3 };

dbResult GetResult( unsigned char *pResults, int Index )
{	
	return (dbResult)(( pResults[ Index/4 ] >> ((Index&3)*2) ) & 3);
}

void SetResult ( int Index, int Result )
{	
	pResults[ Index/4 ] ^= ((int)GetResult(pResults, Index) << ((Index&3)*2));
	pResults[ Index/4 ] |= (Result << ((Index&3)*2));
}

struct SDatabase 
{
	int nStart, nSizeW, nSizeB;
	int Pieces[4];
	int IndW[32*32], IndB[32*32];

	int inline GetIndex( int nW, int nB, int stm )
	{
		return nStart + stm + 2*IndW[ nW ] + 2*nSizeW*IndB[ nB ];
	}
};

SDatabase FourPc[12];

void Compute2PieceIndices( int pData[], int &nSize, int nP[] )
{
	int nSq1, nSq2;
	int bSame = (nP[0] == nP[1]) ? 1: 0;
	nSize = 0;

	for (nSq1 = 0; nSq1 < 32; nSq1++)
		for (nSq2 = 0; nSq2 < 32; nSq2++)
		{
			if ((nSq2 < nSq1 && bSame) ) pData[ nSq1 + nSq2*32 ] = pData[ nSq2 + nSq1*32 ];
			else if (nSq1 == nSq2) pData[ nSq1 + nSq2*32 ] = 0;
			else if ((nSq1 < 4 && nP[0] == WPIECE) || (nSq2 < 4 && nP[1] == WPIECE) || (nSq1 > 27 && nP[0] == BPIECE) || (nSq2 > 27 && nP[1] == BPIECE)) {
				pData[ nSq1 + nSq2*32 ] = 0;
			} else {
				pData[ nSq1 + nSq2*32 ] = nSize;
				nSize++;
			}
		}
}

int ComputeAllIndices( )
{
	int nStart = 0;
	for (int i = 0; i < 9; i++)
	{
		int n = FourIndex[i];
		if (n > 8) continue;
		memcpy( &FourPc[n].Pieces[0], &PC4[4*i] , 4*sizeof(int) );
		FourPc[n].nStart = nStart;
		Compute2PieceIndices( FourPc[n].IndW, FourPc[n].nSizeW, &FourPc[n].Pieces[0] );
		Compute2PieceIndices( FourPc[n].IndB, FourPc[n].nSizeB, &FourPc[n].Pieces[2] );
		nStart += FourPc[n].nSizeW * FourPc[n].nSizeB * 2;
	}
	SIZE4 = nStart/4+1;
	ResultsFour = (unsigned char*) malloc ( nStart/4 + 4 );
	memset (ResultsFour,  0, SIZE4);
	memset (ResultsThree, 0, 6*SIZE3);
	memset (ResultsTwo,   0, 4*SIZE2);
	return SIZE4;
}

//
int ComputeIndex( int WS[], int BS[], int &nPieces, int stm, int &bFlip)
{
	int Index, i;
	nPieces = 0;
	bFlip = 0;
	if (WS[0] == -1) {nPieces = 1; return 1;}
	if (BS[0] == -1) {nPieces = 1; return 2;}

	int Sqs[4];
	Sqs[ nPieces++ ] = (WS[0]>>4); 
	WS[0] = (WS[0]&15);
	if (WS[1] >= 0) {Sqs[ nPieces++ ]=(WS[1]>>4); WS[1] = (WS[1]&15); }
	Sqs[ nPieces++ ] = (BS[0]>>4);
	BS[0] = (BS[0]&15);
	if (BS[1] >= 0) {Sqs[ nPieces++ ]=(BS[1]>>4); BS[1] = (BS[1]&15); }

	if (nPieces == 2)
	{
		for (i = 0; i < 4; i++)
			if (WS[0] == PC2[0 + i*4] && BS[0] == PC2[0 + i*4+1]) break;
		Index = Sqs[0] + Sqs[1]*32 + stm*1024 + i*(32*32*2);
		return Index;
	}
	if (nPieces == 3)
	{
		for (i = 0; i < 12; i++)
			if ((WS[0] == PC3[0 + i*4] && WS[1] == PC3[1 + i*4] && BS[0] == PC3[2 + i*4]) ||
			   ( WS[0] == PC3[0 + i*4] && BS[0] == PC3[1 + i*4] && BS[1] == PC3[2 + i*4])) break;
		if ( ThreeIndex[i] < 8 ) Index = Sqs[0] + Sqs[1]*32 + Sqs[2]*32*32 + stm*32*32*32;
		else {Index = (31-Sqs[1]) + (31-Sqs[2])*32 + (31-Sqs[0])*32*32 + (stm^1)*32*32*32;
			  bFlip = 1;
		}
		return Index + (ThreeIndex[i]&7) * (32*32*32*2);
	}
	if (nPieces == 4)
	{
		for (i = 0; i < 9; i++)
			if (WS[0] == PC4[0 + i*4] && WS[1] == PC4[1 + i*4] && BS[0] == PC4[2 + i*4] && BS[1] == PC4[3 + i*4]) break;
		if ( FourIndex[i]<8 ) 
			Index = FourPc[ FourIndex[i] ].GetIndex( Sqs[0]+Sqs[1]*32, Sqs[2]+Sqs[3]*32, stm );
		else {Index = FourPc[ FourIndex[i]&7 ].GetIndex( (31-Sqs[2]) + (31-Sqs[3])*32 , (31-Sqs[0]) + (31-Sqs[1])*32, stm^1 ); 
			  bFlip = 1;
		 }
		return Index;
	}
	return 0;
}

//
// Get Index From Board
// Should be 2 of a color at most
//
int GetIndexFromBoard( const Board &board, int &bFlip, int &nPieces )
{
	int WSqs[2], BSqs[2];
	int stm = board.sideToMove;

	WSqs[0] = -1; WSqs[1] = -1;
	BSqs[0] = -1; BSqs[1] = -1;

	uint32_t WPieces = board.Bitboards.P[WHITE];
	while ( WPieces ) 
	{
		uint32_t sq = FindLowBit( WPieces );
		WPieces &= ~S[sq];
		ePieceType Piece = board.GetPiece( sq );
		if (WSqs[0]==-1) { WSqs[0] = Piece + sq*16; }
		else if (board.Bitboards.K & S[sq] ) {WSqs[1] = WSqs[0]; WSqs[0] = Piece + sq*16; }
		else WSqs[1] = Piece + sq*16;
	}
	uint32_t BPieces = board.Bitboards.P[BLACK];
	while ( BPieces ) 
	{
		uint32_t sq = FindLowBit( BPieces );
		BPieces &= ~S[sq];
		ePieceType Piece = board.GetPiece( sq );
		if (BSqs[0]==-1) { BSqs[0] = Piece+ sq*16; }
		else if (board.Bitboards.K & S[sq] ) {BSqs[1] = BSqs[0]; BSqs[0] = Piece + sq*16; }
		else BSqs[1] = Piece + sq*16;
	}
	return ComputeIndex( WSqs, BSqs, nPieces, stm, bFlip);
}

//
// Return a Win/Loss/Draw value for the board
//
int QueryGuiDatabase( const Board& board )
{
	int bFlip, nPieces;
	int Result = dbResult::NO_RESULT;
	int Index = GetIndexFromBoard(board, bFlip, nPieces);

	if (nPieces == 1) return Index;
	if (nPieces == 2) Result = GetResult( ResultsTwo  , Index );
	if (nPieces == 3) Result = GetResult( ResultsThree, Index );
	if (nPieces == 4) Result = GetResult( ResultsFour , Index );
	if (bFlip) return FlipResult[ Result ];
	return Result;
}

//
// Test passed board and store the result
//
int inline TestBoard( Board &board, int nPieces, int np1, int np2, int np3, int RIndex )
{
	MoveList moves;
	Board oldBoard = board;
	int nLosses, Index, bFlip;
	int totalWins = 0;

	for (eColor stm : { BLACK, WHITE } )
	{
		board.sideToMove = stm;
		oldBoard.sideToMove = stm;

		Index = GetIndexFromBoard(board, bFlip, nPieces );
		if (nPieces == 2) pResults = ResultsTwo;
		else if (nPieces == 3) pResults = ResultsThree;  
		else if (nPieces == 4) pResults = ResultsFour;		
		if (nPieces == 3) {
			Index = np1 + np2*32 + np3*32*32 + stm*32*32*32 + (RIndex) * (32*32*32*2);
		}

		if ( GetResult( pResults, Index ) != dbResult::DRAW ) {
			totalWins++;
			continue;
		}
		
		const dbResult Win = (board.sideToMove == WHITE) ? dbResult::WHITEWIN : dbResult::BLACKWIN;
		const dbResult Loss = (board.sideToMove == WHITE) ? dbResult::BLACKWIN : dbResult::WHITEWIN;

		moves.FindMoves(board);
		if (moves.numMoves == 0) { // Can't move, so it's a loss
			SetResult( Index, Loss ); 
			totalWins++;
		} else {
			nLosses = 0;
			// Check the moves. If a move wins this is a win, if they all lose it's a loss, otherwise it's a draw
			for (int i = 0; i < moves.numMoves; i++)
			{
				board.DoMove( moves.moves[i] );
				dbResult Result = (dbResult)QueryGuiDatabase(board);
				board = oldBoard;
				if ( Result == Win ) { 
					SetResult(Index, Win);
					totalWins++;
					break;
				}
				if ( Result == Loss ) nLosses++;
			}
			if ( nLosses == moves.numMoves ) 
			{
				SetResult(Index, Loss);
				totalWins++;
			}
		}
	}
	return totalWins;
}
//
// This function will generate a win/loss/draw database. It's actually not hard to generate such a database. 
// Doing a good job is the tougher part, but since I only want small databases for now, this is at least works.
// NOTE : THIS CODE HASN'T BEEN TESTED RECENTLY
//
void GenDatabase( int Piece1, int Piece2, int Piece3, int Piece4, int RIndex )
{
	Board board;

	int lastTotalWins = -1;
	int totalWins = 0;
	int it = 0;
	int nPieces = -1;
	int bFlip;

	while (totalWins != lastTotalWins) // Keep going until we reach a plateau
	{
		lastTotalWins = totalWins;
		totalWins = 0;

		// Loop through every possible position
		for (int np1 = 0; np1 < 32; np1++)
		  for (int np2 = 0; np2 < 32; np2++)
			for (int np3 = 0; np3 < 32; np3++)
			{
				for (int np4 = 0; np4 < 32; np4++)
				{
					// Skip Illegal Positions
					if (np1 == np2) continue;
					if (Piece3 != EMPTY)
						if (np1 == np3 || np2 == np3) continue;
					if (Piece4 != EMPTY)
						if (np4 == np3 || np4 == np2 || np4 == np1) continue;

					// Setup the board
					board.Clear( );
					board.SetPiece( np4, Piece4 );
					board.SetPiece( np3, Piece3 );
					board.SetPiece( np2, Piece2 );
					board.SetPiece( np1, Piece1 );

					// Skip illegal positions( checker on back row )
					if (board.Bitboards.P[WHITE] & ~board.Bitboards.K & (S[0]|S[1]|S[2]|S[3]) ) continue;
					if (board.Bitboards.P[BLACK] & ~board.Bitboards.K & (S[28]|S[29]|S[30]|S[31]) ) continue;
		
					if (nPieces == -1)
					{
						GetIndexFromBoard(board, bFlip, nPieces );
						if (bFlip) return;
					}
					totalWins += TestBoard(board, nPieces, np1, np2, np3, RIndex );

					if (Piece4 == EMPTY) break;
				}

				if (Piece3 == EMPTY) break;
			}
		it++;
	}
}

void SaveAllDatabases()
{
	FILE *FP = fopen ("2pc.cdb", "wb");
	fwrite (ResultsTwo , 4, SIZE2, FP );
	fclose (FP);
	FP = fopen ("3pc.cdb", "wb");
	fwrite (ResultsThree, 6, SIZE3, FP );
	fclose (FP);
	FP = fopen ("4pc.cdb", "wb");
	fwrite (ResultsFour, 1, SIZE4, FP );
	fclose (FP);
}

void InitializeGuiDatabases( SDatabaseInfo& dbInfo )
{
	dbInfo.type = dbType::WIN_LOSS_DRAW;
	dbInfo.numPieces = 4;
	dbInfo.numBlack = 2;
	dbInfo.numWhite = 2;
	dbInfo.loaded = true;

	ComputeAllIndices( );

	char sAFile[256];
	if (checkerBoard.bActive)
	{
		strcpy( sAFile, "engines\\database.jef");
		FILE *FP = fopen( sAFile, "rb");
		if (FP) fclose(FP);	
		else strcpy( sAFile, "database.jef");
	}
	else strcpy( sAFile, "database.jef");

	if (uncompressFileFromArchive( sAFile, "2pc.cdb", ResultsTwo)  != 1) dbInfo.loaded = false;
	if (uncompressFileFromArchive( sAFile, "3pc.cdb", ResultsThree)!= 1) dbInfo.loaded = false;
	if (uncompressFileFromArchive( sAFile, "4pc.cdb", ResultsFour) != 1) dbInfo.loaded = false;

	/*
	FILE *FP;
	if ( (FP = fopen ("2pc.cdb", "rb")) ) {
		fread (ResultsTwo , 4, SIZE2, FP );
		fclose (FP); 
		} else g_bDatabases = 0;
	if ( (FP = fopen ("3pc.cdb", "rb")) ) {
		fread (ResultsThree, 6, SIZE3, FP );
		fclose (FP);
		} else g_bDatabases = 0;
	if ((FP = fopen ("4pc.cdb", "rb"))) {
		fread (ResultsFour, 1, SIZE4 , FP );
		fclose (FP); 
		} else g_bDatabases = 0;
	*/
}


void close_gui_databases(SDatabaseInfo &dbInfo)
{
	if (ResultsFour != nullptr) {
		free(ResultsFour);
		ResultsFour = nullptr;
	}
	dbInfo.loaded = false;
}


void GenAllDatabases( )
{
	ComputeAllIndices();
	for (int i = 0; i < 4; i++)  GenDatabase( PC2[ i*4 ], PC2[ i*4+1 ], PC2[ i*4+2 ], PC2[ i*4+3 ], i );
	for (int i = 0; i < 12; i++) GenDatabase( PC3[ i*4 ], PC3[ i*4+1 ], PC3[ i*4+2 ], PC3[ i*4+3 ], ThreeIndex[i] );
	for (int i = 0; i < 9; i++)  GenDatabase( PC4[ i*4 ], PC4[ i*4+1 ], PC4[ i*4+2 ], PC4[ i*4+3 ], FourIndex[i] );
	SaveAllDatabases();
}