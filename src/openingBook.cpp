//
// openingBook.cpp
// by Jonathan Kreuzer
//
// The opening book is a hash table of positions and values indexed by Board.hashKey
//

#include <stdio.h>
#include <cmath>

#include "engine.h"
#include "guiWindows.h"

COpeningBook* pOpeningBook = nullptr;

// --------------------
//  Opening Book functions
// --------------------
int COpeningBook::GetValue( Board &Board )
{
	uint32_t key   = (uint32_t)Board.hashKey;
	uint32_t checksum = (uint32_t)(Board.hashKey>>32);
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
	 			 DisplayText( sTemp );
			}
			pEntry->value += wValue;
			return;
		}
		if (pEntry->pNext == NULL)
		{
			if (m_nListSize >= BOOK_EXTRA_SIZE)
			{
				DisplayText( "Book Full!" );
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

void COpeningBook::AddPosition(Board& Board, int16_t value, bool bQuiet)
{
	uint32_t ulKey = (uint32_t)Board.hashKey;
	uint32_t ulCheck = (uint32_t)(Board.hashKey >> 32);
	AddPosition(ulKey, ulCheck, value, bQuiet);
}

// --------------------
//  Remove a position from the book.
//  (if this position is in the list, it stays in memory until the book is saved and loaded.)
// --------------------
void COpeningBook::RemovePosition( Board &Board, bool bQuiet )
{
	char sTemp[255];
	uint32_t key   = (uint32_t)Board.hashKey;
	uint32_t checksum = (uint32_t)(Board.hashKey>>32);
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
	snprintf( sTemp, sizeof(sTemp), "%d Positions Loaded", i);
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
int COpeningBook::FindMoves( Board &board, Move OutMoves[], int16_t values[] )
{
	Board tempBoard;
	MoveList moves;
	moves.FindMoves(board);

	int numFound = 0;

	for (int i = 0; i < moves.numMoves; i++)
	{
		tempBoard = board;
		tempBoard.DoMove( moves.moves[i] );
		int val = GetValue(tempBoard);
		// Add move if it leads to a position in the book 
		if ( val != BOOK_INVALID_VALUE)
		{
			OutMoves[numFound] = moves.moves[ i ];
			values[numFound] = val;
			numFound++;
		}
	}

	return numFound;
}

// --------------------
// Try to do a move in the opening book
// --------------------
int COpeningBook::GetMove( Board &Board, Move& bestMove )
{
	int nVal = BOOK_INVALID_VALUE, nMove = -1;
	Move moves[ 60 ];
	Move GoodMoves[ 60 ];
	int16_t Values[60], GoodValues[60];

	int numBookMoves = FindMoves( Board, moves, Values);

	int numGoodMoves = 0;
	int maxValueFound = 0;
	for (int i = 0; i < numBookMoves; i++)
	{
		 if ( (Board.sideToMove == BLACK && Values[i] <= 0) || (Board.sideToMove == WHITE && Values[i] >= 0) ){
			if (abs(Values[i]) > maxValueFound) maxValueFound = abs(Values[i]);
		}
	}

	for (int i = 0; i < numBookMoves; i++)
	{
		if ( (Board.sideToMove == BLACK && Values[i] <= 0) || (Board.sideToMove == WHITE && Values[i] >= 0) )
		{
			if (Values[i] != 0 || maxValueFound == 0)
			{
				GoodValues[numGoodMoves] = Values[i];
				GoodMoves[numGoodMoves] = moves[i];
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
