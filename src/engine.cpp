// GuiNN Checkers 2.0
// by Jonathan Kreuzer
// with contributions by Ed Trice and Ed Gilbert
// copyright 2005, 2020
// http://www.3dkingdoms.com/checkers.htm
//
// NOTICES & CONDITIONS
//
// Commercial use of this code, in whole or in any part, is prohibited.
// Non-commercial use is welcome, but you should clearly mention this code, and where it came from.
// Also the source should remain publicly available.
// www.3dkingdoms.com/checkers.htm
//
// These conditions apply to the code itself only.
// You are of course welcome to read this code and learn from it without restrictions

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <conio.h>
#include <memory.h>
#include <assert.h>
#include <cstdint>
#include <initializer_list>
#include <algorithm>

#include "defines.h"
#include "resource.h"
#include "cb_interface.h"
#include "registry.h"
#include "engine.h"
#include "learning.h"
#include "guiWindows.h"

// The Checkers Engine
SEngine engine;

// For external Checkerboard interface
SCheckerboardInterface checkerBoard;

// Transposition table hash function values
uint64_t TranspositionTable::HashFunction[32][12];
uint64_t TranspositionTable::HashSTM;

// -----------------------
// Display search information
// -----------------------
const char* GetNodeCount(uint64_t count, int nBuf)
{
	static char buffer[4][100];

	if (count < 1000)
		sprintf(buffer[nBuf], "%d n", (int)count);
	else if (count < 1000000)
		sprintf(buffer[nBuf], "%.2f kn", (float)count / 1000.0f);
	else
		sprintf(buffer[nBuf], "%.2f Mn", (float)count / 1000000.0f);

	return buffer[nBuf];
}

void RunningDisplay( const SMove& bestMove, int bSearching)
{
	int game_over_distance = 300;
	char sTemp[4096];
	static int LastEval = 0;

	static SMove LastBest = NO_MOVE;
	if (bestMove != NO_MOVE) {
		LastBest = bestMove;
	}

	int j = 0;
	if (!checkerBoard.bActive) {
		j += sprintf(sTemp + j, "Red: %d   White: %d                           ", engine.board.numPieces[BLACK], engine.board.numPieces[WHITE]);
		if (engine.searchLimits.maxDepth == BEGINNER_DEPTH)
			j += sprintf(sTemp + j, "Level: Beginner  %ds  ", (int)engine.searchLimits.maxSeconds);
		else if (engine.searchLimits.maxDepth == EXPERT_DEPTH)
			j += sprintf(sTemp + j, "Level: Expert  %ds  ", (int)engine.searchLimits.maxSeconds);
		else if (engine.searchLimits.maxDepth == NORMAL_DEPTH)
			j += sprintf(sTemp + j, "Level: Advanced  %ds  ", (int)engine.searchLimits.maxSeconds);
		if (bSearching)
			j += sprintf(sTemp + j, "(searching...)\n");
		else
			j += sprintf(sTemp + j, "\n");
	}

	float seconds = float(clock() - starttime) / CLOCKS_PER_SEC;
	int nps = 0;
	if (seconds > 0.0f)
		nps = int(float(g_searchInfo.nodes) / (1000.0 * seconds));

	if (abs(g_searchInfo.eval) < 3000)
		LastEval = g_searchInfo.eval;

	// Show smiley if winning to better follow fast blitz matches
	bool winning = (-LastEval > 40 && engine.computerColor == BLACK) || (-LastEval < -40 && engine.computerColor == WHITE);
	winning = winning && g_searchInfo.depth > 6; // single move can create false moods because of low search depth
	const char* sMood = (winning && abs(LastEval) > 200) ? ":-D  " : winning ? ":-)  " : "";

	j += sprintf(sTemp + j,
		"%sEval: %d  Depth: %d/%d (%d/%d)  ",
		sMood,
		-LastEval,
		g_searchInfo.depth,
		g_searchInfo.selectiveDepth,
		g_searchInfo.searchingMove,
		g_stack[1].moveList.numMoves );

	// Also announce distance to win when the databases are returning high scores
	if (abs(2000 - abs(LastEval)) <= game_over_distance)
	{
		if (LastEval < 0) {
			j += sprintf(sTemp + j, "(Red wins in %d)", abs(2001 - abs(LastEval)));
		} else {
			j += sprintf(sTemp + j, "(White wins in %d)", abs(2001 - LastEval));
		}
	}

	if (!checkerBoard.bActive)
		j += sprintf(sTemp + j, "\n");

	j += sprintf(sTemp + j,
		"Move: %s   Time: %.2fs   Speed %d KN/s   Nodes: %s (db: %s)   ",
		Transcript::GetMoveString(LastBest).c_str(),
		seconds,
		nps,
		GetNodeCount(g_searchInfo.nodes, 0),
		GetNodeCount(g_searchInfo.databaseNodes, 1));

	DisplayText(sTemp);
}

void SEngine::NewGame( const SBoard& startBoard )
{
	transcript.Init( startBoard );
	board = startBoard;
	historyTable.Clear();
}

std::string SEngine::GetInfoString()
{
	// Endgame Database
	std::string displayStr;
	if (dbInfo.loaded == false)
	{
		displayStr = "No Database Loaded\n";
	} else {
		displayStr = "Database : " + std::to_string(dbInfo.numPieces) + "-man ",
		displayStr += dbInfo.type == dbType::WIN_LOSS_DRAW ? "Win/Loss/Draw\n" : "Perfect Play\n";
	}

	// Opening Book
	if (openingBook) {
		displayStr += "Opening Book : " + std::to_string(openingBook->m_nPositions) + " positions\n";
	} else {
		displayStr += "Opening Book : not loaded.\n";
	}

	// Neural Nets
	int numLoadedNets = 0;
	for (auto net : evalNets) { numLoadedNets += (net->isLoaded) ? 1 : 0; }
	displayStr += "Neural Nets : " + std::to_string(numLoadedNets) + "\n";

	return displayStr;
}

// TODO : convert to std::thread
HANDLE hEngineReady, hAction;
HANDLE hThread;

DWORD WINAPI ThinkingThread(void* /* param */)
{
	hEngineReady = CreateEvent(NULL, TRUE, FALSE, NULL);

	// Think of Move
	while (true)
	{
		SetEvent(hEngineReady);
		WaitForSingleObject(hAction, INFINITE);
		ResetEvent(hEngineReady);

		winGUI.SetComputerColor((eColor)engine.board.SideToMove);
		BestMoveInfo moveInfo = ComputerMove(engine.board);
		winGUI.DoGameMove(moveInfo.move);
		winGUI.ThinkingMenuActive(false);
		engine.bThinking = false;
	}

	CloseHandle(hThread);
}

//
// ENGINE INITILIZATION
//
// status_str is NULL if not called from CheckerBoard
//
void SEngine::Init(char* status_str)
{
	InitBitTables();
	InitializeNeuralNets();
	TranspositionTable::CreateHashFunction();
	openingBook = new COpeningBook;
	historyTable.Clear();

	if (checkerBoard.bActive) {
		get_hashsize(&TTable.sizeMb);
		get_dbpath(checkerBoard.db_path, sizeof(checkerBoard.db_path));
		get_enable_wld(&checkerBoard.enable_wld);
		get_book_setting(&checkerBoard.useOpeningBook);
	}

	if (!TTable.SetSizeMB(TTable.sizeMb))
		TTable.SetSizeMB(64);

	if (!checkerBoard.bActive || !openingBook->Load("engines\\opening.gbk"))
		openingBook->Load("opening.gbk");

	// Load Databases
	if (!dbInfo.loaded && checkerBoard.enable_wld)
	{
		if (status_str)
			sprintf(status_str, "Initializing endgame db...");

		InitializeEdsDatabases(dbInfo);

		if (!dbInfo.loaded) {
			InitializeGuiDatabases(dbInfo);
		}

		if (status_str)
			sprintf(status_str, "Initialized endgame db.");
	}

	board.StartPosition();
	transcript.Init(board);

	if (!checkerBoard.bActive)
	{
		// Create a thinking thread so we don't lock up the GUI
		static DWORD ThreadId;
		hAction = CreateEvent(NULL, FALSE, FALSE, NULL);
		hThread = CreateThread(NULL, 0, ThinkingThread, 0, 0, &ThreadId);	
		if (hThread == NULL) {
			winGUI.ShowErrorPopup("Cannot Create Thread");
		}
	}
}

void SEngine::MoveNow()
{
	if (bThinking) {
		bStopThinking = true;
		WaitForSingleObject(hEngineReady, 500);
		RunningDisplay(NO_MOVE, 0);
	}
}

void SEngine::StartThinking()
{
	WaitForSingleObject(hEngineReady, 1000);
	engine.bStopThinking = false;
	engine.bThinking = true;
	engine.searchLimits.newIterationMaxTime = engine.searchLimits.maxSeconds * .60;
	SetEvent(hAction);
}