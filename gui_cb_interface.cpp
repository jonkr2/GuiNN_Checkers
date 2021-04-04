//
// This is the interface between GuiCheckers and CheckerBoard.
// You can get CheckerBoard at http://www.fierz.ch/
//
#include "egdb.h"
#include "engine.h"
#include "cb_interface.h"
#include "checkersGui.h"
#include "kr_db.h"
#include "registry.h"

int ConvertFromCB[16] = { 0, 0, 0, 0, 0, 2, 1, 0, 0, 6, 5, 0, 0, 0, 0, 0 };
int ConvertToCB[16] = { 0, 6, 5, 0, 0, 10, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


// Set the search time limits for the next move search based on the parameters received
void SetSearchTimeLimits( double maxTime, int info, int moreInfo )
{
	bool incremental;
	double remaining, increment, desired;

	if (incremental = get_incremental_times(info, moreInfo, &increment, &remaining)) 
	{
		const double ratio = 1.6;

		// Using incremental time.
		engine.searchLimits.bEndHard = true;	// Dont allow stretching of fMaxSeconds during the search.
		if (remaining < increment) {
			desired = remaining / ratio;
			engine.searchLimits.maxSeconds = (float)remaining;
			engine.searchLimits.newIterationMaxTime = 0.5 * engine.searchLimits.maxSeconds / ratio;
		}
		else {
			desired = increment + remaining / 9;
			engine.searchLimits.maxSeconds = (float)std::min(ratio * desired, remaining);
			engine.searchLimits.newIterationMaxTime = 0.5 * engine.searchLimits.maxSeconds / ratio;
		}

		// Allow a few msec for overhead.
		if (engine.searchLimits.maxSeconds > .01)
			engine.searchLimits.maxSeconds -= .003f;
	}
	else
	{
		// Using fixed time per move. These params result in an average search time of maxtime.
		engine.searchLimits.maxSeconds = (float)maxTime * .985f;
		if (info & CB_EXACT_TIME) {
			engine.searchLimits.bEndHard = true;
			engine.searchLimits.newIterationMaxTime = engine.searchLimits.maxSeconds;
		}
		else {
			engine.searchLimits.bEndHard = false;
			engine.searchLimits.newIterationMaxTime = 0.6 * maxTime;
		}
	}
}

// Returns a game result based on the previous search (and maybe the ones before that)
int GetAdjucationValue( const BestMoveInfo& searchMove, int& numDrawScoreMoves )
{
	// I don't think this ever happens
	if (searchMove.eval == INVALID_VAL || searchMove.eval == BOOK_INVALID_VALUE || searchMove.eval == TIMEOUT)
		return CB_UNKNOWN;

	const int WinEval = 400;
	const int DrawEval = (engine.transcript.numMoves > 150) ? 45 : 11;

	int retVal = CB_UNKNOWN;
	if (searchMove.eval > WinEval)
		retVal = CB_WIN;
	if (searchMove.eval < -WinEval)
		retVal = CB_LOSS;

	if (engine.transcript.numMoves > 90 && abs(searchMove.eval) < DrawEval) { numDrawScoreMoves++; }
	else { numDrawScoreMoves = 0; }

	if (numDrawScoreMoves >= 8)
		retVal = CB_DRAW;

	if (engine.dbInfo.InDatabase(engine.board) && abs(searchMove.eval) < 2)
		retVal = CB_DRAW;

	return retVal;
}

//
// Find the move we want to play and return it to CheckerBoard
//
int WINAPI getmove
(
	int board[8][8],
	int color,
	double maxtime,
	char str[1024],
	int* playnow,
	int info,
	int moreinfo,
	struct CBmove* move
)
{
	static Board oldBoard;
	static int numDrawScoreMoves = 0;

	if (info & CB_RESET_MOVES) {
		engine.board = Board::StartPosition();
		engine.transcript.Init( engine.board );
	}
	checkerBoard.infoString = str;
	checkerBoard.pbPlayNow = playnow;

	for (int i = 0; i < 64; i++)
		engine.board.SetPiece(Board64to32[i], ConvertFromCB[board[i % 8][7 - i / 8]]);

	/* Convert CheckerBoard color to Gui Checkers color. */
	engine.board.sideToMove = (color == CB_WHITE) ? WHITE : BLACK;
	engine.board.SetFlags();

	if (!checkerBoard.bActive)
	{
		// engine initialization
		checkerBoard.bActive = true;
		engine.Init(str);
		engine.transcript.Init(engine.board);
	}

	init_egdb(str);

	// Work around for a bug in checkerboard not sending reset
	static int lastTotalPieces = 0;
	if (engine.board.TotalPieces() > lastTotalPieces)
	{
		/*
			int Adjust = 0;
			if (Eval < -50) Adjust = -1;
			if (Eval > 50)  Adjust = 1;
			if (g_numMoves > 150 && abs(Eval) < 90) Adjust = 0;
			pBook->LearnGame ( 15, Adjust );
			pBook->Save ( "opening.gbk" );
		*/

		numDrawScoreMoves = 0;
		engine.NewGame(engine.board, true);
	}

	// We get only get a new board, so add the board to repetition history
	uint64_t key = engine.board.CalcHashKey();
	if (engine.transcript.numMoves >= 1 && engine.boardHashHistory[engine.transcript.numMoves - 1] == key)
		--engine.transcript.numMoves;		// Mode Autoplay
	else if (engine.transcript.numMoves >= 2 && engine.boardHashHistory[engine.transcript.numMoves - 2] == key)
		engine.transcript.numMoves -= 2;	// Repeatedly hitting "Play" in analysis mode
	else {
		engine.boardHashHistory[engine.transcript.numMoves] = key;

		// Need to look at old board to try to find if opponent's move was reversible
		if (engine.board.TotalPieces() == oldBoard.TotalPieces() && engine.board.Bitboards.GetCheckers() == oldBoard.Bitboards.GetCheckers()) {
			engine.board.reversibleMoves++;
		}
		else
			engine.board.reversibleMoves = 0;
	}

	lastTotalPieces = engine.board.TotalPieces();

	SetSearchTimeLimits( maxtime, info, moreinfo);

	engine.computerColor = engine.board.sideToMove;
	BestMoveInfo bestMove = ComputerMove(engine.board, engine.searchThreadData);

	if (bestMove.move != NO_MOVE)
	{
		engine.board.DoMove(bestMove.move);
		engine.transcript.AddMove(bestMove.move);
		engine.boardHashHistory[engine.transcript.numMoves] = engine.board.CalcHashKey();
	}

	// Update the board param so CheckerBoard knows what we played
	for (int i = 0; i < 64; i++) {
		int square32 = Board64to32[i];
		if (square32 >= 0) {
			board[i % 8][7 - i / 8] = ConvertToCB[engine.board.GetPiece(square32)];
		}
	}

	int retVal = GetAdjucationValue( bestMove, numDrawScoreMoves );

	// Increment moves again since the next board coming in will be after the opponent has played his move
	// I think we don't know the move unless we find it by testing board position?
	oldBoard = engine.board;
	engine.transcript.numMoves++;

	return retVal;
}

void static_eval(char *fen, char *reply)
{
	Board board;
	std::string evalstr;

	board.FromString(fen);
	GUI.get_eval_string(board, evalstr);
	strcpy(reply, evalstr.c_str());
}

int WINAPI enginecommand(char str[256], char reply[1024])
{
	const int REPLY_MAX = 1024;
	char command[256], param1[256], param2[256];
	char* stopstring;

	command[0] = 0;
	param1[0] = 0;
	param2[0] = 0;
	sscanf(str, "%s %s %s", command, param1, param2);

	if (strcmp(command, "name") == 0) {
		snprintf(reply, REPLY_MAX, g_VersionName);
		return 1;
	}

	if (strcmp(command, "about") == 0) {
		snprintf(reply, REPLY_MAX, "%s\nby Jonathan Kreuzer\n\n%s ", g_VersionName, engine.GetInfoString().c_str() );
		return 1;
	}

	if (strcmp(command, "staticevaluation") == 0) {
		if (!checkerBoard.bActive) {
			strcpy(reply, "Engine not initialized yet. Do a search first.\n");
			return(1);
		}
		static_eval(param1, reply);
		return(1);
	}

	if (strcmp(param1, "check_wld_dir") == 0) {
		check_wld_dir(param2, reply);
		return(1);
	}

	if (strcmp(command, "set") == 0) {
		int val;

		if (strcmp(param1, "hashsize") == 0) {

			int numMBs = strtol(param2, &stopstring, 10);
			if (numMBs < 1)
				return 0;
			numMBs = ClampInt(numMBs, 1, 4096);
			while (!engine.TTable.SetSizeMB(numMBs)) {
				numMBs /= 2;
				snprintf(reply, REPLY_MAX, "allocation failed, downsizing to %dmb", numMBs);
				engine.TTable.SetSizeMB(numMBs);
			}

			save_hashsize(numMBs);
			return 1;
			return 1;
		}

		if (strcmp(param1, "dbpath") == 0) {
			char* p = strstr(str, "dbpath");	/* Cannot use param2 here because it does not allow spaces in the path. */
			while (!isspace(*p))				/* Skip 'dbpath' and following space. */
				++p;
			while (isspace(*p))
				++p;
			if (strcmp(p, checkerBoard.db_path)) {
				checkerBoard.request_egdb_init = true;
				strcpy(checkerBoard.db_path, p);
				save_dbpath(checkerBoard.db_path);
			}

			sprintf(reply, "dbpath set to %s", checkerBoard.db_path);
			return(1);
		}

		if (strcmp(param1, "enable_wld") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != checkerBoard.enable_wld) {
				checkerBoard.request_egdb_init = true;
				checkerBoard.enable_wld = val;
				save_enable_wld(checkerBoard.enable_wld);
			}

			snprintf(reply, REPLY_MAX, "enable_wld set to %d", checkerBoard.enable_wld);
			return(1);
		}

		if (strcmp(param1, "book") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != checkerBoard.useOpeningBook) {
				checkerBoard.useOpeningBook = val;
				save_book_setting(checkerBoard.useOpeningBook);
			}

			snprintf(reply, REPLY_MAX, "book set to %d", checkerBoard.useOpeningBook);
			return(1);
		}
		if (strcmp(param1, "max_dbpieces") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != checkerBoard.max_dbpieces) {
				checkerBoard.request_egdb_init = true;
				checkerBoard.max_dbpieces = val;
				save_max_dbpieces(checkerBoard.max_dbpieces);
			}

			sprintf(reply, "max_dbpieces set to %d", checkerBoard.max_dbpieces);
			return(1);
		}

		if (strcmp(param1, "dbmbytes") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != checkerBoard.wld_cache_mb) {
				checkerBoard.request_egdb_init = true;
				checkerBoard.wld_cache_mb = val;
				save_dbmbytes(checkerBoard.wld_cache_mb);
			}

			sprintf(reply, "dbmbytes set to %d", checkerBoard.wld_cache_mb);
			return(1);
		}
	}

	if (strcmp(command, "get") == 0) {
		if (strcmp(param1, "hashsize") == 0) {
			get_hashsize(&engine.TTable.sizeMb);
			snprintf(reply, REPLY_MAX, "%d", engine.TTable.sizeMb);
			return 1;
		}

		if (strcmp(param1, "protocolversion") == 0) {
			snprintf(reply, REPLY_MAX, "2");
			return 1;
		}

		if (strcmp(param1, "gametype") == 0) {
			snprintf(reply, REPLY_MAX, "%d", GT_ENGLISH);
			return 1;
		}

		if (strcmp(param1, "dbpath") == 0) {
			get_dbpath(checkerBoard.db_path, sizeof(checkerBoard.db_path));
			snprintf(reply, REPLY_MAX, checkerBoard.db_path);
			return(1);
		}

		if (strcmp(param1, "enable_wld") == 0) {
			get_enable_wld(&checkerBoard.enable_wld);
			snprintf(reply, REPLY_MAX, "%d", checkerBoard.enable_wld);
			return(1);
		}

		if (strcmp(param1, "book") == 0) {
			get_book_setting(&checkerBoard.useOpeningBook);
			snprintf(reply, REPLY_MAX, "%d", checkerBoard.useOpeningBook);
			return(1);
		}

		if (strcmp(param1, "max_dbpieces") == 0) {
			get_max_dbpieces(&checkerBoard.max_dbpieces);
			sprintf(reply, "%d", checkerBoard.max_dbpieces);
			return(1);
		}

		if (strcmp(param1, "dbmbytes") == 0) {
			get_dbmbytes(&checkerBoard.wld_cache_mb);
			sprintf(reply, "%d",checkerBoard.wld_cache_mb);
			return(1);
		}
	}

	strcpy(reply, "?");
	return 0;
}


