#pragma once

#include <cstdint>
#include <string>

#define USE_AVX2 // Need to compile without AVX for older CPUs
#define USE_SSE2 // I think all 64-bit PCs have SSE2, so no reason to turn this off
// #define NO_POP_COUNT // for very old processors that have no hardware popcount instruction

#ifdef USE_AVX2
static const char* g_VersionName = "GuiNN Checkers 2.06 avx2";
#else
static const char* g_VersionName = "GuiNN Checkers 2.06";
#endif

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
inline int SQ(int x, int y) { return y * 8 + x; }

const int NUM_BOARD_SQUARES = 32;
const int NUM_PIECE_TYPES = 12; // not really 12 but this is what TT was at. Check what is actually needed.

// Aligned malloc and free for MSVC and GCC
template<typename T>
inline T* AlignedAllocUtil(size_t count, size_t align)
{
#ifdef __GNUC__
	return (T*)aligned_alloc(align, count * sizeof(T));
#else
	return (T*)_aligned_malloc(count * sizeof(T), align);
#endif
}

inline void AlignedFreeUtil(void* data)
{
#ifdef __GNUC__
	free(data);
#else
	_aligned_free(data);
#endif
}

typedef int16_t nnInt_t;