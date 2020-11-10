//
// Tranpositiontable.h
//
// TEntry - single entry in the tranposition table, storing the usual info (searchEval, depth, best-move, etc.)
// TranspositionTable - a table of entries and related functionality
//
#pragma once

#include <stdlib.h> 
#include <cstring>
#include <string>

#include "defines.h"

struct TEntry
{
	enum eFailType { TT_EXACT, TT_FAIL_LOW, TT_FAIL_HIGH };

	bool inline IsBoard(const SBoard& board) const
	{
		return m_checksum == (board.HashKey >> 32);
	}
	void inline Read(uint64_t boardHash, short alpha, short beta, SMove& bestmove, int& value, int& boardEval, int depth, int ahead)
	{
		if (m_checksum == (boardHash >> 32)) // To be almost totally sure these are really the same position.  
		{
			// Get the Value if the search was deep enough, and bounds usable
			if (m_depth >= depth)
			{
				int tempVal = m_searchEval;
				// This is a game ending value, must adjust it since it depends on the variable ahead
				if (m_searchEval > MIN_WIN_SCORE) {
					tempVal = m_searchEval - ahead;
				}
				if (m_searchEval < -MIN_WIN_SCORE) {
					tempVal = m_searchEval + ahead;
				}

				switch ( FailType() )
				{
					case TT_EXACT: value = tempVal;  // Exact value
						break;
					case TT_FAIL_LOW: if (tempVal <= alpha) value = tempVal; // Alpha Bound (check to make sure it's usuable)
						break;
					case TT_FAIL_HIGH: if (tempVal >= beta) value = tempVal; // Beta Bound (check to make sure it's usuable)
						break;
				}
			}
			// Take the best move from Transposition Table                                                
			bestmove = m_bestmove;
			boardEval = m_boardEval;
		}
	}

	void inline Write(uint64_t boardHash, short alpha, short beta, SMove& bestmove, int searchEval, int boardEval, int depth, int ahead, int ttAge)
	{
		if ((m_ageAndFailtype&63) == ttAge && m_depth > depth && m_depth > 14) return; // Don't write over deeper entries from same search

		m_checksum = (uint32_t)(boardHash >> 32);
		m_searchEval = searchEval;
		m_boardEval = boardEval;
		// If this is a game ending value, must adjust it since it depends on the variable ahead
		if (m_searchEval > MIN_WIN_SCORE) m_searchEval += ahead;
		if (m_searchEval < -MIN_WIN_SCORE) m_searchEval -= ahead;
		assert(m_searchEval <= 2001);
		m_depth = depth;
		m_bestmove = bestmove;
		m_ageAndFailtype = ttAge;
		if (searchEval <= alpha) m_ageAndFailtype |= (TT_FAIL_LOW<<6);
		else if (searchEval >= beta)  m_ageAndFailtype |= (TT_FAIL_HIGH<<6);
		else m_ageAndFailtype |= (TT_EXACT<<6);
	}

	// DATA
	uint32_t m_checksum;
	SMove m_bestmove;
	int16_t m_searchEval;
	int16_t m_boardEval;
	int8_t m_depth;
	uint8_t m_ageAndFailtype; // Age (6 bits) + FailType (2 bits)

	inline uint8_t FailType() { return m_ageAndFailtype >> 6; }
};

//
// The Transposition table is made up of an array of TTEntries.
// It's indexed as a hash table using board.HashKey
//
struct TranspositionTable
{
	// transposition table data
	TEntry* entries = nullptr;
	size_t numEntries = 0;
	int sizeMb = 128;

	static uint64_t HashFunction[32][12], HashSTM;

	void Clear()
	{
		memset(entries, 0, sizeof(TEntry) * numEntries);
	}

	TEntry* GetEntry(const SBoard& board)
	{
		return &entries[board.HashKey % numEntries];
	}

	// Allocate the hashtable.
	// returns true if successful
	bool SetSizeMB(int _sizeMb)
	{
		if (entries)
			free(entries);

		sizeMb = _sizeMb;
		numEntries = ((size_t)sizeMb * (1 << 20)) / sizeof(TEntry);
		entries = (TEntry*)malloc(sizeof(TEntry) * numEntries);

		return (entries);
	}

	static uint64_t Rand15() { return (uint64_t)rand(); }
	static uint64_t Rand64() {
		return Rand15() + (Rand15() << 10) + (Rand15() << 20) +	(Rand15() << 30) + (Rand15() << 40) + (Rand15() << 50);
	}

	static void CreateHashFunction()
	{
		// Note : the hash function is also used for opening book, so if it change the opening book won't find positions
		for (int i = 0; i < 32; i++)
			for (int x = 0; x < 9; x++)
			{
				HashFunction[i][x] = Rand64();
			}
		HashSTM = Rand64();
	}
};
