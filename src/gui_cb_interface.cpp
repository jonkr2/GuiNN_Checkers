//
// This is the interface between GuiCheckers and CheckerBoard.
// You can get CheckerBoard at http://www.fierz.ch/
//
#include "engine.h"
#include "cb_interface.h"
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
	const int WinEval = 10000; // NOTE : Super high value to turn off adjucation.. need to verify it can win all these first
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
	static SBoard oldBoard;
	static int numDrawScoreMoves = 0;

	if (info & CB_RESET_MOVES) {
		engine.board = SBoard::StartPosition();
		engine.transcript.Init( engine.board );
	}
	checkerBoard.infoString = str;
	checkerBoard.pbPlayNow = playnow;

	for (int i = 0; i < 64; i++)
		engine.board.SetPiece(BoardLoc[i], ConvertFromCB[board[i % 8][7 - i / 8]]);

	/* Convert CheckerBoard color to Gui Checkers color. */
	engine.board.SideToMove = (color == CB_WHITE) ? WHITE : BLACK;
	engine.board.SetFlags();

	if (!checkerBoard.bActive)
	{
		// engine initialization
		checkerBoard.bActive = true;
		engine.Init(str);
		engine.transcript.Init(engine.board);
	}

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
		engine.transcript.Init( engine.board );
		engine.historyTable.Clear();
	}

	// We get only get a new board, so add the board to repetition history
	engine.boardHashHistory[engine.transcript.numMoves] = engine.board.CalcHashKey();
	// Need to look at old board to try to find if opponent's move was reversible
	if (engine.board.TotalPieces() == oldBoard.TotalPieces() && engine.board.Bitboards.GetCheckers() == oldBoard.Bitboards.GetCheckers()) {
		engine.board.reversibleMoves++;
	}

	lastTotalPieces = engine.board.TotalPieces();

	SetSearchTimeLimits( maxtime, info, moreinfo);

	engine.computerColor = engine.board.SideToMove;
	BestMoveInfo bestMove = ComputerMove(engine.board);

	if (bestMove.move != NO_MOVE)
	{
		engine.board.DoMove(bestMove.move);
		engine.transcript.AddMove(bestMove.move);
		engine.boardHashHistory[engine.transcript.numMoves] = engine.board.CalcHashKey();
	}

	// Update the board param so CheckerBoard knows what we played
	for (int i = 0; i < 64; i++) {
		if (BoardLoc[i] >= 0) {
			board[i % 8][7 - i / 8] = ConvertToCB[engine.board.GetPiece(BoardLoc[i])];
		}
	}

	int retVal = GetAdjucationValue( bestMove, numDrawScoreMoves );

	// Increment moves again since the next board coming in will be after the opponent has played his move
	// I think we don't know the move unless we find it by testing board position?
	oldBoard = engine.board;
	engine.transcript.numMoves++;

	return retVal;
}

int WINAPI enginecommand(char str[256], char reply[1024])
{
	char command[256], param1[256], param2[256];
	char* stopstring;

	command[0] = 0;
	param1[0] = 0;
	param2[0] = 0;
	sscanf(str, "%s %s %s", command, param1, param2);

	if (strcmp(command, "name") == 0) {
		sprintf(reply, g_VersionName);
		return 1;
	}

	if (strcmp(command, "about") == 0) {
		sprintf(reply, "%s\nby Jonathan Kreuzer\n\n%s ", g_VersionName, engine.GetInfoString());
		return 1;
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
				sprintf(reply, "allocation failed, downsizing to %dmb", numMBs);
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
				strcpy(checkerBoard.db_path, p);
				save_dbpath(checkerBoard.db_path);
			}

			sprintf(reply, "dbpath set to %s", checkerBoard.db_path);
			return(1);
		}

		if (strcmp(param1, "enable_wld") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != checkerBoard.enable_wld) {
				checkerBoard.enable_wld = val;
				save_enable_wld(checkerBoard.enable_wld);
				if (!engine.dbInfo.loaded && checkerBoard.enable_wld) {
					sprintf(reply, "Initializing endgame db...");
					InitializeEdsDatabases(engine.dbInfo);
				}
			}

			sprintf(reply, "enable_wld set to %d", checkerBoard.enable_wld);
			return(1);
		}

		if (strcmp(param1, "book") == 0) {
			val = strtol(param2, &stopstring, 10);
			if (val != checkerBoard.useOpeningBook) {
				checkerBoard.useOpeningBook = val;
				save_book_setting(checkerBoard.useOpeningBook);
			}

			sprintf(reply, "book set to %d", checkerBoard.useOpeningBook);
			return(1);
		}
	}

	if (strcmp(command, "get") == 0) {
		if (strcmp(param1, "hashsize") == 0) {
			get_hashsize(&engine.TTable.sizeMb);
			sprintf(reply, "%d", engine.TTable.sizeMb);
			return 1;
		}

		if (strcmp(param1, "protocolversion") == 0) {
			sprintf(reply, "2");
			return 1;
		}

		if (strcmp(param1, "gametype") == 0) {
			sprintf(reply, "%d", GT_ENGLISH);
			return 1;
		}

		if (strcmp(param1, "dbpath") == 0) {
			get_dbpath(checkerBoard.db_path, sizeof(checkerBoard.db_path));
			sprintf(reply, checkerBoard.db_path);
			return(1);
		}

		if (strcmp(param1, "enable_wld") == 0) {
			get_enable_wld(&checkerBoard.enable_wld);
			sprintf(reply, "%d", checkerBoard.enable_wld);
			return(1);
		}

		if (strcmp(param1, "book") == 0) {
			get_book_setting(&checkerBoard.useOpeningBook);
			sprintf(reply, "%d", checkerBoard.useOpeningBook);
			return(1);
		}
	}

	strcpy(reply, "?");
	return 0;
}
