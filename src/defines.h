#pragma once

#include <cstdint>
#include <string>

#define USE_AVX // Need to compile without AVX for older CPUs
#define USE_SSE2 // I think all 64-bit PCs have SSE2, so no reason to turn this off

static const char* g_VersionName = "GuiNN Checkers 2.0";

const int MAX_GAMEMOVES = 2048;
const int INV = 33; // invalid square
const int INVALID_VAL = -9999;
const int TIMEOUT = 31000;

enum eColor : uint8_t { 
	BLACK = 0, // Black == Red
	WHITE = 1,
	NO_COLOR = 2 
}; 

// Max depth for each level
const int BEGINNER_DEPTH = 2;
const int NORMAL_DEPTH = 8;
const int EXPERT_DEPTH = 52;
const int MAX_SEARCHDEPTH = 120;

const int MIN_WIN_SCORE = 1800;
inline int WinScore(int ply) { return 2001 - ply; }
inline int ClampInt(int i, int min, int max) { return (i < min) ? min : (i > max) ? max : i; }

// For extern Checkerboard interface
struct SCheckerboardInterface
{
	bool bActive = false;
	int* pbPlayNow = nullptr;
	char* infoString = nullptr;
	int useOpeningBook = 1;
	char db_path[260] = "db_dtw";
	int enable_wld = 1;
};
extern SCheckerboardInterface checkerBoard;