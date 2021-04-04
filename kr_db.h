#pragma once

#include "engine.h"


/*
 * Convert a GUI_checkers bitboard to a format used by the egdb driver.
 * The driver supports either "normal" or row-reversed bitboard formats.
 * GUI_checkers uses row-reversed format, so we opened the driver to use that type.
 */
inline void gui_to_kr(const CheckerBitboards &gui, EGDB_BITBOARD &kr)
{
	kr.row_reversed.black_man = gui.P[BLACK] & ~gui.K;
	kr.row_reversed.black_king = gui.P[BLACK] & gui.K;
	kr.row_reversed.white_man = gui.P[WHITE] & ~gui.K;
	kr.row_reversed.white_king = gui.P[WHITE] & gui.K;
}

inline int gui_to_kr_color(eColor color)
{
	return(color == BLACK ? EGDB_BLACK : EGDB_WHITE);
}


void init_egdb(char msg[1024]);
void check_wld_dir(const char *dir, char *reply);
