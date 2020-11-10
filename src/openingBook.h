#pragma once

const int32_t BOOK_INVALID_VALUE = -30000;
const uint32_t BOOK_HASH_SIZE = 131072;
const uint32_t BOOK_EXTRA_SIZE = 200000;

// --------------------
// A single entry (position) in the opening book
// --------------------
struct SBookEntry
{
	SBookEntry()
	{
		pNext = nullptr;
		value = BOOK_INVALID_VALUE;
	}
	~SBookEntry()
	{
	}

	void Save(FILE* pFile, uint16_t positionKey)
	{
		fwrite(&checksum, sizeof(checksum), 1, pFile);
		fwrite(&key16, sizeof(key16), 1, pFile);
		fwrite(&value, sizeof(value), 1, pFile);
		fwrite(&positionKey, sizeof(positionKey), 1, pFile);
	}

	uint16_t Load(FILE* pFile)
	{
		uint16_t positionKey = 0;
		fread(&checksum, sizeof(checksum), 1, pFile);
		fread(&key16, sizeof(key16), 1, pFile);
		fread(&value, sizeof(value), 1, pFile);
		fread(&positionKey, sizeof(positionKey), 1, pFile);
		return positionKey;
	}

	SBookEntry* pNext;
	uint32_t checksum;
	uint16_t key16;
	int16_t value;
};

// --------------------
// The opening book 
// --------------------
class COpeningBook
{
public:
	COpeningBook()
	{
		m_pHash = new SBookEntry[BOOK_HASH_SIZE];
		m_pExtra = new SBookEntry[BOOK_EXTRA_SIZE];
		m_nListSize = 0;
		m_nPositions = 0;
	}

	~COpeningBook()
	{
		delete[] m_pHash;
		delete[] m_pExtra;
	}

	// Functions
	void LearnGame(int numMoves, int adjust);
	int  FindMoves(SBoard& board, SMove Moves[], int16_t values[]);
	int  GetMove(SBoard& board, SMove& bestMove);
	void RemovePosition(SBoard& board, bool bQuiet);
	int  GetValue(SBoard& board);
	void AddPosition(SBoard& board, int16_t value, bool bQuiet);
	void AddPosition(uint32_t ulKey, uint32_t ulCheck, int16_t value, bool bQuiet);
	int  Load(const char* sFileName);
	int  Save(const char* sFileName);

	// Data
	SBookEntry* m_pHash;
	SBookEntry* m_pExtra;
	int m_nListSize;
	int m_nPositions;
};