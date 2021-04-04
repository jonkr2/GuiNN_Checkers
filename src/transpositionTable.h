//
// Tranpositiontable.h
// by Jonathan Kreuzer
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

	bool inline IsBoard(uint64_t hashKey) const
	{
		return m_checksum == (hashKey >> 32);
	}
	void inline Read(uint64_t boardHash, short alpha, short beta, Move& bestmove, int& value, int& boardEval, int depth, int ahead)
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

				switch (FailType())
				{
				case TT_EXACT: 
					value = tempVal;
					break;
				case TT_FAIL_LOW: 
					if (tempVal <= alpha) value = tempVal; // Alpha Bound (check to make sure it's usuable)
					break;
				case TT_FAIL_HIGH: 
					if (tempVal >= beta) value = tempVal; // Beta Bound (check to make sure it's usuable)
					break;
				}
			}
			// Take the best move from Transposition Table                                                
			bestmove = m_bestmove;
			boardEval = m_boardEval;
		}
	}

	void inline Write(uint64_t boardHash, short alpha, short beta, Move& bestmove, int searchEval, int boardEval, int depth, int ahead, int ttAge)
	{
		const uint32_t newChecksum = (uint32_t)(boardHash >> 32);
		const bool sameBoard = (newChecksum == m_checksum);

		m_checksum = newChecksum;
		m_searchEval = searchEval;
		m_boardEval = boardEval;
		m_depth = depth;
		if (!sameBoard || bestmove != NO_MOVE) // don't ovewrite a bestmove with no_move
		{
			m_bestmove = bestmove;
		}

		// If this is a game ending value, must adjust it since it depends on the variable ahead
		if (m_searchEval > MIN_WIN_SCORE) m_searchEval += ahead;
		if (m_searchEval < -MIN_WIN_SCORE) m_searchEval -= ahead;
		assert(m_searchEval <= 2001);

		m_ageAndFailtype = ttAge;
		if (searchEval <= alpha) m_ageAndFailtype |= (TT_FAIL_LOW << 6);
		else if (searchEval >= beta)  m_ageAndFailtype |= (TT_FAIL_HIGH << 6);
		else m_ageAndFailtype |= (TT_EXACT << 6);
	}

	// DATA
	uint32_t m_checksum;
	Move m_bestmove;
	int16_t m_searchEval;
	int16_t m_boardEval;
	int8_t m_depth;
	uint8_t m_ageAndFailtype; // Age (6 bits) + FailType (2 bits)

	inline uint8_t FailType() { return m_ageAndFailtype >> 6; }
	inline uint8_t Age() { return (m_ageAndFailtype & 63); }

	int inline GetTestEval()
	{
		if (m_searchEval == INVALID_VAL) { return m_boardEval; }
		bool preferSearchEval = (FailType() == TT_EXACT || (FailType() == TT_FAIL_HIGH && m_searchEval > m_boardEval) || (FailType() == TT_FAIL_LOW && m_searchEval < m_boardEval));
		return preferSearchEval ? m_searchEval : m_boardEval;
	}
};

// Buckets of entries. Choose which entry to overwrite, if they are all filled. Mainly helpful in case of ttable saturation.
constexpr int ENTRIES_PER_BUCKET = 2;
struct TBucket
{
	TEntry entries[ENTRIES_PER_BUCKET];

	// Choose which entry to write to
	TEntry* ChooseEntry(uint64_t hashKey, uint8_t searchAge )
	{
		int bestScore = -1000;
		TEntry* bestEntry = &entries[0];
		for (int i = 0; i < ENTRIES_PER_BUCKET; i++)
		{
			int score;

			// If we find a match or empty entry, use it
			if (entries[i].IsBoard(hashKey) || entries[i].m_checksum == 0)
			{
				bestEntry = &entries[i];
				break;
			}
			
			// Otherwise use the best entry based on depth and failtype
			int ageDiff = searchAge - entries[i].Age();
			if (ageDiff < 0)
				ageDiff += 64;

			score = -entries[i].m_depth;
			score -= (entries[i].FailType() == TEntry::TT_EXACT) ? 10 : 0;
			score += (ageDiff > 1) ? 100 : (ageDiff == 1) ? 4 : 0;
			if (score > bestScore)
			{
				bestScore = score;
				bestEntry = &entries[i];
			}
		}
		return bestEntry;
	}
};

//
// The Transposition table is made up of an array of TTEntries.
// It's indexed as a hash table using board.HashKey
//
struct TranspositionTable
{
	// transposition table data
	TBucket* buckets = nullptr;
	size_t numBuckets = 0;
	int sizeMb = 128;

	static uint64_t HashFunction[NUM_BOARD_SQUARES][NUM_PIECE_TYPES];
	static uint64_t HashSTM;

	void Clear()
	{
		memset(buckets, 0, sizeof(TBucket) * numBuckets);
	}

	TEntry* GetEntry(const Board& board, uint8_t searchAge)
	{
		auto& bucket = buckets[board.hashKey % numBuckets];
		return bucket.ChooseEntry(board.hashKey, searchAge);
	}

	// Allocate the hashtable.
	// returns true if successful
	bool SetSizeMB(int _sizeMb)
	{
		if (buckets)
			AlignedFreeUtil(buckets);

		sizeMb = _sizeMb;
		numBuckets = ((size_t)sizeMb * (1 << 20)) / sizeof(TBucket);
		buckets = AlignedAllocUtil<TBucket>(numBuckets, 64);
		if (buckets != nullptr)
			Clear();

		return (buckets != nullptr);
	}

	// FIXME? This probably isn't a good rand
	static uint64_t Rand15() { return (uint64_t)rand(); }
	static uint64_t Rand64() {
		return Rand15() ^ (Rand15() << 10) ^ (Rand15() << 20) ^ (Rand15() << 30) ^ (Rand15() << 40) ^ (Rand15() << 50);
	}

	static void CreateHashFunction()
	{
		/* Changed to a compile time table because of differences in rand() seeding between
		 * various runtime environments.
		 */
		HashSTM = HashFunction[0][0];
	}
};
