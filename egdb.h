#pragma once

/* Prevent name mangling of exported dll publics. */
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/* Dont define EGDB_EXPORTS when using this library.
 * Only define it if you are actually compiling the library source code.
 */
#ifdef EGDB_EXPORTS
#define EGDB_API EXTERNC __declspec(dllexport)
#else
#define EGDB_API EXTERNC __declspec(dllimport)
#endif


/* Color definitions. */
enum EGDB_COLOR {
	EGDB_BLACK = 0,
	EGDB_WHITE = 1
};

/* Values returned by handle->lookup(). */
enum LOOKUP_VALUE {
	EGDB_SUBDB_UNAVAILABLE = -2,/* this slice is not being used */
	EGDB_NOT_IN_CACHE = -1,		/* conditional lookup and position not in cache. */
	EGDB_UNKNOWN = 0,			/* value not in the database. */
	EGDB_WIN = 1,
	EGDB_LOSS = 2,
	EGDB_DRAW = 3,
	EGDB_DRAW_OR_LOSS = 4,
	EGDB_WIN_OR_DRAW = 5,
};

enum MTC_VALUE {
	MTC_UNKNOWN = 0,
	MTC_LESS_THAN_THRESHOLD = 1,
	MTC_THRESHOLD = 10,
};

typedef enum {
	EGDB_KINGSROW_WLD = 0,			/* obsolete. */
	EGDB_KINGSROW_MTC,				/* obsolete. */
	EGDB_CAKE_WLD,
	EGDB_CHINOOK_WLD,
	EGDB_KINGSROW32_WLD,
	EGDB_KINGSROW32_MTC,
	EGDB_CHINOOK_ITALIAN_WLD,
	EGDB_KINGSROW32_ITALIAN_WLD,
	EGDB_KINGSROW32_ITALIAN_MTC,
	EGDB_KINGSROW32_WLD_TUN,
	EGDB_KINGSROW32_ITALIAN_WLD_TUN,
	EGDB_KINGSROW_DTW,
	EGDB_KINGSROW_ITALIAN_DTW,
} EGDB_TYPE;

typedef enum {
	EGDB_NORMAL = 0,
	EGDB_ROW_REVERSED
} EGDB_BITBOARD_TYPE;

/* for database lookup stats. */
typedef struct {
	unsigned int lru_cache_hits;
	unsigned int lru_cache_loads;
	unsigned int autoload_hits;
	unsigned int db_requests;				/* total egdb requests. */
	unsigned int db_returns;				/* total egdb w/l/d returns. */
	unsigned int db_not_present_requests;	/* requests for positions not in the db */
} EGDB_STATS;

/* This is Kingsrow's definition of a checkers position.
 * The bitboards use bit0 for square 1, bit 1 for square 2, ...,
 * For Italian checkers, the rows are reversed, so that bit 0 represents
 * Italian square 4, bit 1 represents square 3, bit 2 represents square 2, etc.
 */
typedef struct {
	unsigned int black;
	unsigned int white;
	unsigned int king;
} EGDB_NORMAL_BITBOARD;

/* This is Cake's definition of a board position.
 * The bitboards use bit 3 for square 1, bit 2, for square 2, ...
 * This is repeated on each row of squares, thus bit7 for square 5, 
 * bit6 for square 6, bit 5 for square 7, ...
 * For Italian checkers, the rows are reversed, so that bit 0 represents
 * Italian square 1, bit 1 represents square 2, bit 2 represents square 3, etc.
 */
typedef struct {
	unsigned int black_man;
	unsigned int black_king;
	unsigned int white_man;
	unsigned int white_king;
} EGDB_ROW_REVERSED_BITBOARD;

typedef union {
	EGDB_NORMAL_BITBOARD normal;
	EGDB_ROW_REVERSED_BITBOARD row_reversed;
} EGDB_BITBOARD;

/* The driver handle type */
typedef struct egdb_driver {
	int (__cdecl *lookup)(struct egdb_driver *handle, EGDB_BITBOARD *position, int color, int cl);
	void (__cdecl *reset_stats)(struct egdb_driver *handle);
	EGDB_STATS *(__cdecl *get_stats)(struct egdb_driver *handle);
	int (__cdecl *verify)(struct egdb_driver *handle);
	int (__cdecl *close)(struct egdb_driver *handle);
	void *internal_data;
} EGDB_DRIVER;


/* Open an endgame database driver. */
EGDB_API EGDB_DRIVER *__cdecl egdb_open(EGDB_BITBOARD_TYPE bitboard_type,
								int pieces, 
								int cache_mb,
								const char *directory,
								void (__cdecl *msg_fn)(char *));

/*
 * Identify which type of database is present, and the maximum number of pieces
 * for which it has data.
 * The return value of the function is 0 if a database is found, and its
 * database type and piece info are written using the pointer arguments.
 * The return value is non-zero if no database is found.  In this case the values
 * for egdb_type and max_pieces are undefined on return.
 */
EGDB_API int __cdecl egdb_identify(const char *directory, EGDB_TYPE *egdb_type, int *max_pieces);

EGDB_API extern unsigned int egdb_version;

