//
// openingBook.cpp
// by Jonathan Kreuzer
//
// The opening book is a hash table of positions and values indexed by Board.HashKey
//

#include <stdio.h>
#include <time.h>
#include <cmath>

#include "engine.h"
#include "guiWindows.h"

COpeningBook* pOpeningBook = nullptr;

// --------------------
//  Opening Book functions
// --------------------
int COpeningBook::GetValue( SBoard &Board )
{
	uint32_t key   = (uint32_t)Board.HashKey;
	uint32_t checksum = (uint32_t)(Board.HashKey>>32);
	uint32_t entryIdx = key % BOOK_HASH_SIZE;
	SBookEntry* pEntry = &m_pHash[entryIdx];

	while (pEntry != NULL && pEntry->value != BOOK_INVALID_VALUE)
	{
		if (pEntry->checksum == checksum && pEntry->key16 == (key >>16) )
		{
			return pEntry->value;
		}
		pEntry = pEntry->pNext;
	}

	return BOOK_INVALID_VALUE;
}

void COpeningBook::AddPosition( uint32_t key, uint32_t checksum, int16_t wValue, bool bQuiet )
{
	char sTemp[ 255 ];
	int entryIdx = key % BOOK_HASH_SIZE;
	SBookEntry *pEntry = &m_pHash[entryIdx];

	while (pEntry->value != BOOK_INVALID_VALUE)
	{
		if (pEntry->checksum == checksum && pEntry->key16 == (key >>16) )
		{
			if (wValue == 0) 
			{
				if (pEntry->value > 0) wValue--;
				if (pEntry->value < 0) wValue++;
			}
			if (!bQuiet) 
			{
				 sprintf( sTemp, "Position Exists. %d\nValue was %d now %d", entryIdx, pEntry->value, pEntry->value + wValue);
	 			 DisplayText ( sTemp );
			}
			pEntry->value += wValue;
			return;
		}
		if (pEntry->pNext == NULL)
		{
			if (m_nListSize >= BOOK_EXTRA_SIZE)
			{
				DisplayText ("Book Full!");
				return;
			}
			pEntry->pNext = &m_pExtra[ m_nListSize ];
			m_nListSize++;
		}
		pEntry = pEntry->pNext;
	}

	// New Position
	m_nPositions++;
	pEntry->checksum = checksum;
	pEntry->key16 = uint16_t(key >>16);
	pEntry->value = wValue;
	if (!bQuiet)
	{
		sprintf ( sTemp, "Position Added. %d\nValue %d", entryIdx, wValue);
		DisplayText ( sTemp );
	}
}

void COpeningBook::AddPosition(SBoard& Board, int16_t value, bool bQuiet)
{
	uint32_t ulKey = (uint32_t)Board.HashKey;
	uint32_t ulCheck = (uint32_t)(Board.HashKey >> 32);
	AddPosition(ulKey, ulCheck, value, bQuiet);
}

// --------------------
//  Remove a position from the book.
//  (if this position is in the list, it stays in memory until the book is saved and loaded.)
// --------------------
void COpeningBook::RemovePosition( SBoard &Board, bool bQuiet )
{
	char sTemp[255];
	uint32_t key   = (uint32_t)Board.HashKey;
	uint32_t checksum = (uint32_t)(Board.HashKey>>32);
	uint32_t entryIdx = key % BOOK_HASH_SIZE;
	SBookEntry *pEntry = &m_pHash[entryIdx];
	SBookEntry *pPrev = nullptr; 

	while (pEntry != nullptr && pEntry->value != BOOK_INVALID_VALUE)
	{
		if (pEntry->checksum == checksum && pEntry->key16 == (key >>16) )
		{
			m_nPositions--;
			if (!bQuiet)
			{
				sprintf ( sTemp, "Position Removed. %d\nValue was %d", entryIdx, pEntry->value);
				DisplayText ( sTemp );
			}
			if (pPrev != nullptr) {
				pPrev->pNext = pEntry->pNext;				
			} else  {
				if (pEntry->pNext != nullptr) { *pEntry = *pEntry->pNext; }
				else pEntry->value = BOOK_INVALID_VALUE;
			}
			return;
		}
		pPrev = pEntry;
		pEntry = pEntry->pNext;
	}

	if (!bQuiet)
	{
		sprintf( sTemp, "Position does not exist. %d\n", entryIdx);
		DisplayText( sTemp );
	}
}

// --------------------
// FILE I/O For Opening Book
// --------------------
int COpeningBook::Load( const char *sFileName )
{
	FILE *fp = fopen( sFileName, "rb");
	SBookEntry TempEntry;
	int i = 0;

	if ( fp == NULL ) return false;
  
	uint32_t key = TempEntry.Load( fp );
	while (!feof( fp ) )
	{
		i++;
		key += (TempEntry.key16 << 16);
		AddPosition(key, TempEntry.checksum, TempEntry.value, true );
		key = TempEntry.Load( fp );
	}

	fclose ( fp );

	char sTemp[ 255 ];
	sprintf( sTemp, "%d Positions Loaded", i);
	DisplayText( sTemp );

	return true;
}

int COpeningBook::Save( const char *sFileName )
{
	int numPositions = 0;

	FILE *fp = fopen (sFileName, "wb");
	if (fp)
	{
		for (int i = 0; i < BOOK_HASH_SIZE; i++)
		{
			SBookEntry* pEntry = &m_pHash[i];
			if (pEntry->value != BOOK_INVALID_VALUE)
			{
				if (pEntry->value < 100)
				{
					pEntry->Save(fp, i);
					numPositions++;
				}

				while (pEntry->pNext != NULL)
				{
					pEntry = pEntry->pNext;
					pEntry->Save(fp, i);
					numPositions++;
				}
			}
		}

		fclose(fp);
	}
	return numPositions;
}

// --------------------
// Find a book move by doing all legal moves, then checking to see if the position is in the opening book.
// --------------------
int COpeningBook::FindMoves( SBoard &Board, SMove OutMoves[], int16_t values[] )
{
	SBoard TempBoard;
	CMoveList Moves;
	Moves.FindMoves(Board.SideToMove, Board.Bitboards);

	int numFound = 0;

	for (int i = 0; i < Moves.numMoves; i++)
	{
		TempBoard = Board;
		TempBoard.DoMove( Moves.Moves[i] );
		int val = GetValue( TempBoard );
		// Add move if it leads to a position in the book 
		if ( val != BOOK_INVALID_VALUE)
		{
			OutMoves[numFound] = Moves.Moves[ i ];
			values[numFound] = val;
			numFound++;
		}
	}

	return numFound;
}

// --------------------
// Try to do a move in the opening book
// --------------------
int COpeningBook::GetMove( SBoard &Board, SMove& bestMove )
{
	int nVal = BOOK_INVALID_VALUE, nMove = -1;
	SMove Moves[ 60 ];
	SMove GoodMoves[ 60 ];
	int16_t Values[60], GoodValues[60];

	int numBookMoves = FindMoves( Board, Moves, Values);

	srand( (unsigned int)time( 0 ) ); // Randomize

	int numGoodMoves = 0;
	int maxValueFound = 0;
	for (int i = 0; i < numBookMoves; i++)
	{
		 if ( (Board.SideToMove == BLACK && Values[i] <= 0) || (Board.SideToMove == WHITE && Values[i] >= 0) ){
			if (abs(Values[i]) > maxValueFound) maxValueFound = abs(Values[i]);
		}
	}

	for (int i = 0; i < numBookMoves; i++)
	{
		if ( (Board.SideToMove == BLACK && Values[i] <= 0) || (Board.SideToMove == WHITE && Values[i] >= 0) )
		{
			if (Values[i] != 0 || maxValueFound == 0)
			{
				GoodValues[numGoodMoves] = Values[i];
				GoodMoves[numGoodMoves] = Moves[i];
				numGoodMoves++;
			}
		}
	}

	if (numGoodMoves == 0) return BOOK_INVALID_VALUE;

	nMove = rand() % numGoodMoves;
	nVal = GoodValues[ nMove ];

	if (abs(nVal) < abs(maxValueFound))
	{
		nMove = rand() % numGoodMoves;
		nVal = GoodValues[ nMove ];
	}
	if (abs(nVal) < abs(maxValueFound)-1)
	{
		nMove = rand() % numGoodMoves;
		nVal = GoodValues[ nMove ];
	}

	if (nMove!=-1) 
		bestMove = GoodMoves[ nMove ];
	else
		bestMove = NO_MOVE;

	return nVal;
}

// --------------------
// Book Learning
// --------------------
void COpeningBook::LearnGame( int numMoves, int adjust )
{
	for (int i = 0; i <= (numMoves-1)*2; i++)
	{
		uint64_t hash64 = engine.boardHashHistory[i];
		uint32_t key   = (uint32_t)hash64;
		uint32_t checksum = (uint32_t)(hash64 >> 32);
		AddPosition(key, checksum, adjust, true );
	}
}
