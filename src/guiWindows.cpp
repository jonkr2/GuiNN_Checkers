//
// Windows GUI for GuiNN checkers. 
// by Jonathan Kreuzer
//
// This file also contains WinMain, which is the main function loop under windows.
//

#define NOMINMAX
#include <windows.h>
#include <time.h>

#include "defines.h"
#include "resource.h"
#include "engine.h"
#include "search.h"
#include "endgameDatabase.h"
#include "openingBook.h"
#include "guiWindows.h"
#include "learning.h"
#include "board.h"

const int NO_SQUARE = 255;

enum eMoveResult { INVALID_MOVE = 0, VALID_MOVE = 1, DOUBLEJUMP = 2000 };

// GUI Data - todo : cleanup
bool BoardFlip = false;
int g_xAdd = 44, g_yAdd = 20, g_nSqSize = 64;
HBITMAP PieceBitmaps[32];
int g_nSelSquare = NO_SQUARE;
char g_buffer[32768];	// For PDN copying/pasting/loading/saving

int g_nDouble = 0;
bool g_bSetupBoard = false;

HWND MainWnd = nullptr;
HWND BottomWnd = nullptr;

void OnNewGame(const SBoard& startBoard);

// ------------------------------------
// Initialize the GUI and the Engine
// ------------------------------------
static int AnyInstance(HINSTANCE this_inst)
{
	const int width = 720;
	const int height = 710;

	// create main window
	MainWnd = CreateWindow("GuiCheckersClass",
		g_VersionName,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT,
		width, height,
		NULL, NULL, this_inst, NULL);

	if (MainWnd == NULL)
		return FALSE;

	BottomWnd = CreateDialog(this_inst, "BotWnd", MainWnd, InfoProc);

	// Load Piece Bitmaps
	char* psBitmapNames[10] = { nullptr, "Rcheck", "Wcheck", nullptr, nullptr, "RKing", "Wking", "Wsquare", "Bsquare", nullptr };
	for (int i = 0; i < 9; i++) {
		if (psBitmapNames[i] != nullptr)
			PieceBitmaps[i] = (HBITMAP)LoadImage(this_inst, psBitmapNames[i], IMAGE_BITMAP, 0, 0, 0);
	}

	MoveWindow(BottomWnd, 0, 550, width, 160, TRUE);
	ShowWindow(MainWnd, SW_SHOW);
	ThinkingMenuActive(false);

	DrawBoard(engine.board);

	engine.Init( nullptr );

	OnNewGame(SBoard::StartPosition());
	DisplayText( engine.GetInfoString() );

	return TRUE;
}

// ------------------------------------
//  Register window class for the application if this is first instance
// ------------------------------------
int RegisterClass(HINSTANCE this_inst)
{
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = this_inst;
	wc.hIcon = LoadIcon(this_inst, "chIcon");
	wc.hIconSm = LoadIcon(this_inst, "chIcon");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
	wc.lpszMenuName = "CheckMenu";
	wc.lpszClassName = "GuiCheckersClass";

	BOOL rc = RegisterClassEx(&wc);

	return rc;
}

// ------------------------------------
//  WinMain - initialization, message loop
// ------------------------------------
int WINAPI WinMain(HINSTANCE this_inst, HINSTANCE prev_inst, LPSTR cmdline, int cmdshow)
{
	if (!prev_inst)
		RegisterClass(this_inst);

	if (AnyInstance(this_inst) == FALSE)
		return(FALSE);

	// Main message loop
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return((int)msg.wParam);
}

// ------------------------------------
//  Some display helpers
// ------------------------------------
void DisplayText(const char* sText)
{
	if (checkerBoard.bActive)
	{
		if (checkerBoard.infoString) {
			strcpy(checkerBoard.infoString, sText);
		}
	}
	else if (BottomWnd)
	{
		SetDlgItemText(BottomWnd, 150, sText);
	}
}

void ShowErrorPopup(const char* text)
{
	MessageBox(MainWnd, "Cannot Create Thread", "Error", MB_OK);
}

void UpdateMenuChecks()
{
	HMENU menu = GetMenu(MainWnd);
	CheckMenuItem(menu, ID_OPTIONS_BEGINNER, (engine.searchLimits.maxDepth == BEGINNER_DEPTH) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_NORMAL, (engine.searchLimits.maxDepth == NORMAL_DEPTH) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_EXPERT, (engine.searchLimits.maxDepth == EXPERT_DEPTH) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(menu, ID_GAME_HASHING, (engine.bUseHashTable) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(menu, ID_OPTIONS_COMPUTERWHITE, (engine.computerColor == WHITE) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_COMPUTERBLACK, (engine.computerColor == BLACK) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_GAME_COMPUTEROFF, (engine.computerColor == NO_COLOR) ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(menu, ID_OPTIONS_1SECOND, (engine.searchLimits.maxSeconds == 1.0f) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_2SECONDS, (engine.searchLimits.maxSeconds == 2.0f) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_5SECONDS, (engine.searchLimits.maxSeconds == 5.0f) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_10SECONDS, (engine.searchLimits.maxSeconds == 10.0f) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(menu, ID_OPTIONS_30SECONDS, (engine.searchLimits.maxSeconds == 30.0f) ? MF_CHECKED : MF_UNCHECKED);
}

// -------------------------------
// WINDOWS CLIPBOARD FUNCTIONS
// -------------------------------
int TextToClipboard(const char* sText)
{
	DisplayText(sText);

	char* bufferPtr;
	static HGLOBAL clipTranscript;
	size_t nLen = strlen(sText);
	if (OpenClipboard(MainWnd) == TRUE) {
		EmptyClipboard();
		clipTranscript = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, nLen + 10);
		bufferPtr = (char*)GlobalLock(clipTranscript);
		memcpy(bufferPtr, sText, nLen + 1);
		GlobalUnlock(clipTranscript);
		SetClipboardData(CF_TEXT, clipTranscript);
		CloseClipboard();
		return 1;
	}

	DisplayText("Cannot Open Clipboard");
	return 0;
}

int TextFromClipboard(char* sText, int nMaxBytes)
{
	if (!IsClipboardFormatAvailable(CF_TEXT)) {
		DisplayText("No Clipboard Data");
		return 0;
	}

	OpenClipboard(MainWnd);

	HANDLE hData = GetClipboardData(CF_TEXT);
	LPVOID pData = GlobalLock(hData);

	int nSize = (int)strlen((char*)pData);
	if (nSize > nMaxBytes)
		nSize = nMaxBytes;
	memcpy(sText, (LPSTR)pData, nSize);
	sText[nSize] = NULL;

	GlobalUnlock(hData);
	CloseClipboard();

	return 1;
}

// Gray out certain items
void ThinkingMenuActive(int bOn)
{
	int SwitchItems[10] =
	{
		ID_GAME_NEW,
		ID_FILE_LOADGAME,
		ID_EDIT_SETUPBOARD,
		ID_EDIT_PASTE_PDN,
		ID_EDIT_PASTE_POS,
		ID_GAME_CLEAR_HASH,
		ID_GAME_COMPUTEROFF,
		ID_OPTIONS_COMPUTERBLACK,
		ID_OPTIONS_COMPUTERWHITE,
		ID_GAME_HASHING
	};
	UINT newEnabled = (bOn) ? MF_GRAYED : MF_ENABLED;

	for (int i = 0; i < 8; i++) {
		EnableMenuItem(GetMenu(MainWnd), SwitchItems[i], newEnabled);
	}

	EnableMenuItem(GetMenu(MainWnd), ID_GAME_MOVENOW, (bOn) ? MF_ENABLED : MF_GRAYED);
}

void SetComputerColor(eColor Color)
{
	engine.computerColor = Color;
	UpdateMenuChecks();
}

void OnNewGame(const SBoard& startBoard)
{
	engine.NewGame(startBoard);

	SetComputerColor(WHITE);

	g_nSelSquare = NO_SQUARE;
	g_nDouble = 0;

	DisplayText("The Game Begins...");
	DrawBoard(engine.board);
}

// Perform a move on the game board
void DoGameMove(SMove doMove)
{
	if ((clock() - starttime) < (CLOCKS_PER_SEC / 4)) {
		Sleep(200); // pause a bit if move is really quick
	}

	if (doMove != NO_MOVE)
	{
		engine.board.DoMove(doMove); // TODO : for double jumps pause in middle
		engine.transcript.AddMove(doMove);
	}

	if (!engine.bStopThinking) {
		RunningDisplay(doMove, 0);
	}

	DrawBoard(engine.board);
}

// ---------------------------------------------
//  Check Possiblity/Execute Move from one selected square to another
//  returns INVALID_MOVE if the move is not possible, VALID_MOVE if it is or DOUBLEJUMP if the move is an uncompleted jump
// ---------------------------------------------
eMoveResult SquareMove(SBoard& board, int x, int y, int xloc, int yloc, eColor Color)
{
	CMoveList MoveList;
	MoveList.FindMoves(Color, board.Bitboards);

	int dst = BoardLoc[xloc + yloc * 8];
	int src = BoardLoc[x + y * 8];

	for (int i = 0; i < MoveList.numMoves; i++)
	{
		const SMove& move = MoveList.Moves[i];
		if (move.Src() == src && move.Dst() == dst)
		{
			// Check if the src & dst match a move from the generated movelist
			if (g_nDouble > 0)
			{
				// Build double jump move for game transcript
				SMove& jumpMove = engine.transcript.Back();
				for (int j = 0; j < 4; j++) {
					if (dst - src == JumpAddDir[j]) {
						jumpMove.SetJumpDir(g_nDouble - 1, j);
					}
				}
				jumpMove.SetJumpLen(g_nDouble + 1);
			}
			else 
			{
				engine.transcript.AddMove(move);
			}

			if (move.JumpLen() > 1)
			{
				// perform the first part of the move on board, will have to move again
				board.DoSingleJump(src, dst, board.GetPiece(src), board.SideToMove );
				g_nDouble++;
				return DOUBLEJUMP;
			}

			board.DoMove(move);

			g_nDouble = 0;
			return VALID_MOVE;
		}
	}

	return INVALID_MOVE;
}

//---------------------------------------------------------
// Draw a frame around a square clicked on by user
//----------------------------------------------------------
void HighlightSquare(HDC hdc, int Square, int sqSize, unsigned long color, int border)
{
	HBRUSH brush = CreateSolidBrush(color);

	if (Square >= 0 && Square <= 63)
	{
		int x = Square % 8, y = Square / 8;
		if (BoardFlip == 1) {
			x = 7 - x;
			y = 7 - y;
		}

		RECT rect;
		rect.left = x * sqSize + g_xAdd;
		rect.right = (x + 1) * sqSize + g_xAdd;
		rect.top = y * sqSize + g_yAdd;
		rect.bottom = (y + 1) * sqSize + g_yAdd;

		FrameRect(hdc, &rect, brush);

		if (border == 2) {
			rect.left += 1;
			rect.right -= 1;
			rect.top += 1;
			rect.bottom -= 1;
			FrameRect(hdc, &rect, brush);
		}
	}

	DeleteObject(brush);
}

//---------------------------------------------------------------
// Function to Draw a single Bitmap
// --------------------------------------------------------------
void DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize)
{
	BITMAP bitmapbuff;
	HDC memorydc;
	POINT origin, size;

	memorydc = CreateCompatibleDC(hdc);
	SelectObject(memorydc, bitmap);
	SetMapMode(memorydc, GetMapMode(hdc));

	int result = GetObject(bitmap, sizeof(BITMAP), &bitmapbuff);

	origin.x = x;
	origin.y = y;

	if (nSize == 0) {
		size.x = bitmapbuff.bmWidth;
		size.y = bitmapbuff.bmHeight;
	}
	else {
		size.x = nSize;
		size.y = nSize;
	}

	DPtoLP(hdc, &origin, 1);
	DPtoLP(memorydc, &size, 1);

	if (nSize == bitmapbuff.bmWidth) {
		BitBlt(hdc, origin.x, origin.y, size.x, size.y, memorydc, 0, 0, SRCCOPY);
	}
	else {
		SetStretchBltMode(hdc, COLORONCOLOR);	//STRETCH_HALFTONE
		SetBrushOrgEx(hdc, 0, 0, NULL);
		StretchBlt(hdc,
			origin.x, origin.y,
			size.x, size.y,
			memorydc,
			0, 0,
			bitmapbuff.bmWidth, bitmapbuff.bmHeight,
			SRCCOPY);
	}

	DeleteDC(memorydc);
}

// ------------------
// Draw Board
// ------------------
void DrawBoard(const SBoard& board)
{
	HDC hdc = GetDC(MainWnd);
	int start = 0, add = 1, add2 = 1, x = 0, y = 0, mul = 1;
	int nSize = 64;

	if (BoardFlip == 1) {
		start = 63;
		add = -1;
		add2 = 0;
	}

	for (int i = start; i >= 0 && i <= 63; i += add)
	{
		// piece
		if (board.GetPiece(BoardLoc[i]) != EMPTY && board.GetPiece(BoardLoc[i]) != INVALID) {
			DrawBitmap(hdc,
				PieceBitmaps[board.GetPiece(BoardLoc[i])],
				x * nSize * mul + g_xAdd,
				y * nSize * mul + g_yAdd,
				nSize);
		}

		// empty square
		else if (((i % 2 == 1) && (i / 8) % 2 == 1) || ((i % 2 == 0) && (i / 8) % 2 == 0))
			DrawBitmap(hdc, PieceBitmaps[7], x * nSize * mul + g_xAdd, y * nSize * mul + g_yAdd, nSize);
		else
			DrawBitmap(hdc, PieceBitmaps[8], x * nSize * mul + g_xAdd, y * nSize * mul + g_yAdd, nSize);

		x++;
		if (x == 8) {
			x = 0;
			y++;
		}
	}

	if (g_nSelSquare != NO_SQUARE)
		HighlightSquare(hdc, g_nSelSquare, g_nSqSize, 0xFFFFFF, 1);

	ReleaseDC(MainWnd, hdc);
}

// ------------------
// Replay Game from Game Move History up to nMove
// ------------------
void ReplayGame(Transcript& transcript, SBoard& Board)
{
	Board = transcript.startBoard;

	int i = 0;
	while (transcript.moves[i].data != 0 && i < transcript.numMoves) {
		engine.boardHashHistory[i] = Board.HashKey;
		Board.DoMove(transcript.moves[i]);
		i++;
	}

	transcript.numMoves = i;
	g_nSelSquare = NO_SQUARE;
	g_nDouble = 0;
}

// --------------------
// GetFileName - get a file name using common dialog
// --------------------
static unsigned char sSaveGame[] = "Checkers Game (*.pdn)\0*.pdn\0\0";

static BOOL GetFileName(HWND hwnd, BOOL save, char* fname, unsigned char* filterList)
{
	OPENFILENAME of;
	int rc;

	memset(&of, 0, sizeof(OPENFILENAME));
	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hwnd;
	of.lpstrFilter = (LPSTR)filterList;
	of.lpstrDefExt = "";
	of.nFilterIndex = 1L;
	of.lpstrFile = fname;
	of.nMaxFile = _MAX_PATH;
	of.lpstrTitle = NULL;
	of.Flags = OFN_HIDEREADONLY;
	if (save) {
		rc = GetSaveFileName(&of);
	}
	else {
		rc = GetOpenFileName(&of);
	}

	return(rc);
}

// ------------------
// File I/O
// ------------------
void SaveGame(char* sFilename)
{
	FILE* pFile = fopen(sFilename, "wb");
	if (pFile == NULL)
		return;

	const char* pdnText = engine.transcript.ToPDN().c_str();
	fwrite(pdnText, 1, strlen(pdnText), pFile);

	fclose(pFile);
}

void ReadPDNFromBuffer(const char* buffer)
{
	OnNewGame(SBoard::StartPosition());
	engine.transcript.FromPDN(buffer);

	ReplayGame(engine.transcript, engine.board);

	DrawBoard(engine.board);
	DisplayText(buffer);
}

void LoadGame(char* sFilename)
{
	FILE* pFile = fopen(sFilename, "rb");
	if (pFile)
	{
		int x = 0;
		while (x < 32000 && !feof(pFile)) {
			g_buffer[x++] = getc(pFile);
		}

		if (x > 0) { g_buffer[x - 1] = 0; }
		fclose(pFile);

		ReadPDNFromBuffer(g_buffer);
	}
}

//
// GUI HELPER FUNCTIONS
//
void EndSetup()
{
	engine.board.SetFlags();
	g_bSetupBoard = FALSE;
	engine.transcript.Init(engine.board);
	DisplayText("");
}

void ComputerGo()
{
	if (g_bSetupBoard) {
		EndSetup();
	}

	if (engine.bThinking) {
		engine.MoveNow();
	} else {
		ThinkingMenuActive(true);
		engine.StartThinking();
	}
}

// This changes x and y from window coords to board coords (Kinda weird)
int GetSquare(int& x, int& y)
{
	// Calculate the square the user clicked on (0-63)
	x = (x - g_xAdd) / g_nSqSize;
	y = (y - g_yAdd) / g_nSqSize;

	// Make sure it's valid, preferrably this function would only be called with valid input
	x = ClampInt(x, 0, 7);
	y = ClampInt(y, 0, 7);

	if (BoardFlip) {
		x = 7 - x;
		y = 7 - y;
	}

	return x + y * 8;
}

void DisplayEvaluation()
{
	int Eval = -engine.board.EvaluateBoard(0, g_searchInfo.databaseNodes);

	std::string databaseBuffer;
	if (abs(Eval) != 2001 && engine.dbInfo.InDatabase(engine.board))
	{
		if (engine.dbInfo.type == dbType::WIN_LOSS_DRAW)
		{
			int result = QueryGuiDatabase(engine.board);
			switch (result)
			{
			case DRAW:		databaseBuffer = "DRAW"; break;
			case WHITEWIN:	databaseBuffer = "WHITE WIN"; break;
			case BLACKWIN:	databaseBuffer = "BLACK WIN"; break;
			default:        databaseBuffer = "DATABASE ERROR"; break;
			}
		}

		if (engine.dbInfo.type == dbType::EXACT_VALUES) {
			int result = QueryEdsDatabase(engine.board, 0);
			if (result == 0)
				databaseBuffer = "DRAW";
			if (result > MIN_WIN_SCORE) {
				databaseBuffer = "WHITE WINS IN " + std::to_string(abs(2001 - result));
			}
			if (result < -MIN_WIN_SCORE) {
				databaseBuffer = "BLACK WINS IN %d " + std::to_string(abs(-2001 - result));
			}
		}
	}

	if (engine.board.SideToMove == BLACK) Eval = -Eval;

	snprintf(g_buffer, sizeof(g_buffer),
		"Eval: %d\nWhite : %d   Red : %d\n%s %s",
		Eval,
		engine.board.numPieces[WHITE],
		engine.board.numPieces[BLACK],
		databaseBuffer.c_str(),
		Repetition(engine.board.HashKey, 0, engine.transcript.numMoves) ? "(Repetition)" : "");

	DisplayText(g_buffer);
}

void CopyFen()
{
	engine.board.ToFen(g_buffer);
	TextToClipboard(g_buffer);
}

void CopyPDN()
{
	TextToClipboard( engine.transcript.ToPDN().c_str() );
}

void PasteFen()
{
	if (TextFromClipboard(g_buffer, 512))
	{
		DisplayText(g_buffer);
		if (engine.board.FromFen(g_buffer))
		{
			OnNewGame(engine.board);
		}
	}
}

void PastePDN()
{
	if (TextFromClipboard(g_buffer, 16380))
	{
		ReadPDNFromBuffer(g_buffer);
	}
}

void SetSearchDepth(int newDepth)
{
	engine.searchLimits.maxDepth = newDepth;
	UpdateMenuChecks();
}

void SetSearchTime(float newTime)
{
	engine.searchLimits.maxSeconds = newTime;
	UpdateMenuChecks();
}

void SetupAddPiece(int x, int y, eColor color)
{
	int square64 = GetSquare(x, y);

	int square = BoardLoc[square64];
	if (square == INV)
		return;

	if (GetKeyState(VK_SHIFT) < 0) {
		engine.board.SetPiece(square, EMPTY);
		DrawBoard(engine.board);
		return;
	}

	if (GetKeyState(VK_MENU) < 0) {
		engine.board.SideToMove = (engine.board.GetPiece(square) & BPIECE) ? BLACK : WHITE;
		return;
	}

	ePieceType checker = (color == WHITE) ? WPIECE : BPIECE;
	ePieceType king = (color == WHITE) ? WKING : BKING;
	ePieceType oldPiece = engine.board.GetPiece(square);
	ePieceType newPiece = (oldPiece == checker) ? king : (oldPiece == king) ? EMPTY : (RankMask(color, 7) & S[square]) ? king : checker;
	engine.board.SetPiece(square, newPiece);

	DrawBoard(engine.board);
}

std::string GetTTEntryString(TEntry* entry, eColor stm)
{
	int eval = stm == WHITE ? entry->m_searchEval : -entry->m_searchEval;
	int boardEval = stm == WHITE ? entry->m_boardEval : -entry->m_boardEval;
	int failType = entry->FailType();
	if (stm == BLACK && failType == TEntry::TT_FAIL_HIGH) failType = TEntry::TT_FAIL_LOW;
	else if (stm == BLACK && failType == TEntry::TT_FAIL_LOW) failType = TEntry::TT_FAIL_HIGH;
	std::string valueCompare = (failType == TEntry::TT_EXACT) ? "=" : (failType == TEntry::TT_FAIL_HIGH) ? ">=" : "<=";

	std::string ret = "Depth  " + std::to_string(entry->m_depth);
	ret += "\nEval " + valueCompare + " " + std::to_string(eval);
	ret += "\nMove  " + Transcript::GetMoveString( entry->m_bestmove );
	ret += "\nBoardEval  " + std::to_string(boardEval);
	return ret;
}

void KeyboardCommands(int key)
{
	// Opening Book Edit
	if (key == '2')
		engine.openingBook->AddPosition(engine.board, -1, false);
	if (key == '3')
		engine.openingBook->AddPosition(engine.board, 0, false);
	if (key == '4')
		engine.openingBook->AddPosition(engine.board, 1, false);
	if (key == '6')
		engine.openingBook->RemovePosition(engine.board, false);
	if (key == 'S')
	{
		int numPositions = engine.openingBook->Save("opening.gbk");
		std::string displayStr = (std::to_string(numPositions) + " Positions Saved");
		DisplayText(displayStr.c_str());
	}

	// Copy / Paste
	if (key == 'V')
		PasteFen();
	if (key == 'C')
		CopyFen();
	if (key == 'V' && (GetKeyState(VK_CONTROL) < 0))
		PastePDN();
	if (key == 'C' && (GetKeyState(VK_CONTROL) < 0))
		CopyPDN();

	if (key == 'G')
		ComputerGo();
	if (key == 'M')
		engine.MoveNow();

	if (key == 'H')
	{
		engine.TTable.Clear();
		DisplayText("Hash Cleared");
	}
	if (key == 'E')
	{
		DisplayEvaluation();
	}
	if (key == 'T')
	{
		// show TT entry
		TEntry* entry = engine.TTable.GetEntry(engine.board);
		if (entry && entry->IsBoard(engine.board)) {
			DisplayText(GetTTEntryString(entry, engine.board.SideToMove).c_str());
		}
		else {
			DisplayText("No TT Entry for board");
		}
	}
}

// ===============================================
// Process messages to the Bottom Window
// ===============================================

void SetTranscriptPosition( int newPos )
{
	engine.MoveNow(); // Stop thinking

	engine.transcript.numMoves = std::max( newPos, 0 );
	ReplayGame(engine.transcript, engine.board);
	DrawBoard(engine.board);
	SetFocus(MainWnd);
}

INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_TAKEBACK:
			SetTranscriptPosition(engine.transcript.numMoves - 2);
			break;

		case IDC_PREV:
			SetTranscriptPosition(engine.transcript.numMoves - 1);;
			break;

		case IDC_NEXT:
			SetTranscriptPosition(engine.transcript.numMoves+1);
			break;

		case IDC_START:
			SetTranscriptPosition(0);
			break;

		case IDC_END:
			SetTranscriptPosition( 1000 );
			break;

		case IDC_GO:
			if (g_bSetupBoard) {
				EndSetup();
				break;
			}

			ComputerGo();
			SetFocus(MainWnd);
			break;
		}
	}

	return(0);
}

// ==============================================================
//  Level Select Dialog Procedure
// ==============================================================
INT_PTR CALLBACK LevelDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM /*lparam*/)
{
	switch (msg) {
	case WM_INITDIALOG:
		SetDlgItemInt(hwnd, IDC_DEPTH, engine.searchLimits.maxDepth, 1);
		SetDlgItemInt(hwnd, IDC_TIME, (int)engine.searchLimits.maxSeconds, 1);
		return(TRUE);

	case WM_COMMAND:
		if (LOWORD(wparam) == IDOK) {
			engine.searchLimits.maxDepth = GetDlgItemInt(hwnd, IDC_DEPTH, 0, 1);
			engine.searchLimits.maxDepth = ClampInt(engine.searchLimits.maxDepth, 2, 52);
			engine.searchLimits.maxSeconds = (float)GetDlgItemInt(hwnd, IDC_TIME, 0, 1);
		}

		if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL) {
			EndDialog(hwnd, TRUE);
			return(TRUE);
		}
		break;
	}

	return FALSE;
}

//
// Process a command message
//
void ProcessCommand(WORD cmd, HWND hwnd)
{
	switch (cmd)
	{
	case ID_GAME_FLIPBOARD:
		BoardFlip = !BoardFlip;
		DrawBoard(engine.board);
		break;

	case ID_GAME_NEW:
		OnNewGame(SBoard::StartPosition());
		break;

	case ID_GAME_COMPUTERMOVE:
		ComputerGo();
		break;

	case ID_GAME_COMPUTEROFF:
		SetComputerColor(NO_COLOR);
		break;

	case ID_OPTIONS_COMPUTERBLACK:
		SetComputerColor(BLACK);
		break;

	case ID_OPTIONS_COMPUTERWHITE:
		SetComputerColor(WHITE);
		break;

	case ID_GAME_HASHING:
		engine.bUseHashTable = !engine.bUseHashTable;
		UpdateMenuChecks();
		break;

	case ID_GAME_CLEAR_HASH:
		engine.TTable.Clear();
		break;

	case ID_OPTIONS_BEGINNER: SetSearchDepth(BEGINNER_DEPTH); break;
	case ID_OPTIONS_NORMAL: SetSearchDepth(NORMAL_DEPTH); break;
	case ID_OPTIONS_EXPERT: SetSearchDepth(EXPERT_DEPTH); break;

	case ID_OPTIONS_1SECOND: SetSearchTime(1.0f); break;
	case ID_OPTIONS_2SECONDS: SetSearchTime(2.0f); break;
	case ID_OPTIONS_5SECONDS: SetSearchTime(5.0f); break;
	case ID_OPTIONS_10SECONDS: SetSearchTime(10.0f); break;
	case ID_OPTIONS_30SECONDS: SetSearchTime(30.0f); break;

	case ID_OPTIONS_CUSTOMLEVEL:
		DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), "LevelWnd", hwnd, LevelDlgProc);
		break;

	case ID_FILE_SAVEGAME:
	{
		char filename[2048];
		if (GetFileName(hwnd, 1, filename, sSaveGame))
			SaveGame(filename);
		break;
	}

	case ID_FILE_LOADGAME:
	{
		char filename[2048];
		if (GetFileName(hwnd, 0, filename, sSaveGame))
			LoadGame(filename);
		break;
	}

	case ID_FILE_EXIT:
		PostMessage(hwnd, WM_DESTROY, 0, 0);
		break;

	case ID_EDIT_SETUPBOARD:
		if (g_bSetupBoard) {
			EndSetup();
		}
		else {
			g_bSetupBoard = TRUE;
			DisplayText("BOARD SETUP MODE.       (Click GO to end) \n(shift+click to erase pieces) \n(alt+click on a piece to set that color to move)");
		}
		break;

	case ID_EDIT_PASTE_POS:
		PasteFen();
		break;

	case ID_EDIT_COPY_POS:
		CopyFen();
		break;

	case ID_EDIT_COPY_PDN:
		CopyPDN();
		break;

	case ID_GAME_MOVENOW:
		engine.MoveNow();
		break;

	case ID_EDIT_PASTE_PDN:
		PastePDN();
		break;

	case ID_DEVELOPER_SAVE_BINARY_NETS:
		SaveBinaryNets("Nets.gnn");
		DisplayText("Saved Binary Nets to \"Nets.gnn\"");
		break;

	case ID_DEVELOPER_EXPORT_TRAINING_SET:
		DisplayText("Exporting training Sets...");
		CreateTrainingSet();
		DisplayText("Export Training Sets Done!");
		break;

	case ID_DEVELOPER_IMPORTLATESTMATCHES:
	{
		DisplayText("Import Latest Matches");
		MatchResults results;
		int numFiles = ImportLatestMatches(results);
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "Imported %d files.\nW-L-D : %d-%d-%d  (%.1f%%)\n%.1f elo\n", numFiles, results.wins, results.losses, results.draws, results.Percent() * 100.0f, results.EloDiff());
		DisplayText(buffer);
		break;
	}
	}
}


// ===============================================
// Process messages to the MAIN Window
// ===============================================
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	HDC hdc;
	int x, y;
	int nSquare;

	switch (msg)
	{
		// Process Menu Commands
	case WM_COMMAND:
		ProcessCommand(LOWORD(wparam), hwnd);
		break;

	case WM_KEYDOWN:
		KeyboardCommands((int)wparam);
		break;

		// Process Mouse Clicks on the board
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		SetFocus(hwnd);

		x = (int)LOWORD(lparam);
		y = (int)HIWORD(lparam);
		nSquare = GetSquare(x, y);

		if (g_bSetupBoard)
		{
			if (msg == WM_LBUTTONUP)
				return TRUE;

			SetupAddPiece((int)LOWORD(lparam), (int)HIWORD(lparam), BLACK);
			return TRUE;
		}

		if (engine.bThinking)
			return TRUE;  // Don't move pieces when computer is thinking

		// Did the user click on his/her own piece?
		if (nSquare >= 0 && nSquare <= 63 && engine.board.GetPiece(BoardLoc[nSquare]) != EMPTY)
		{
			if (msg == WM_LBUTTONUP)
				return TRUE;

			if (g_nDouble != 0)
				return TRUE;  // Can't switch to a different piece when double jumping

			if (((engine.board.GetPiece(BoardLoc[nSquare]) & BPIECE) && engine.board.SideToMove == BLACK) ||
				((engine.board.GetPiece(BoardLoc[nSquare]) & WPIECE) && engine.board.SideToMove == WHITE))
			{
				g_nSelSquare = nSquare;
				if (g_nSelSquare != NO_SQUARE) {
					DrawBoard(engine.board);
				}

				hdc = GetDC(MainWnd);
				HighlightSquare(hdc, g_nSelSquare, g_nSqSize, 0xFFFFFF, 1);
				ReleaseDC(MainWnd, hdc);
			}
		}
		else if (g_nSelSquare >= 0 && g_nSelSquare <= 63)
		{
			// Did the user click on a valid destination square?
			eMoveResult MoveResult = SquareMove(engine.board, g_nSelSquare % 8, g_nSelSquare / 8, x, y, engine.board.SideToMove);
			if (MoveResult == VALID_MOVE) {
				g_nSelSquare = NO_SQUARE;
				DrawBoard(engine.board);

				if (engine.computerColor == engine.board.SideToMove)
					ComputerGo();
			}
			else if (MoveResult == DOUBLEJUMP) {
				g_nSelSquare = nSquare;
				DrawBoard(engine.board);
			}
		}

		return TRUE;

	case WM_RBUTTONDOWN:

		if (g_bSetupBoard)
		{
			SetupAddPiece((int)LOWORD(lparam), (int)HIWORD(lparam), WHITE);
			return(TRUE);
		}

		return(TRUE);

	case WM_SETCURSOR:
		if (!engine.bThinking)
			SetCursor(LoadCursor(NULL, IDC_ARROW));
		return(TRUE);

	case WM_PAINT:
		PAINTSTRUCT ps;
		hdc = BeginPaint(hwnd, &ps);
		DrawBoard(engine.board);
		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return(DefWindowProc(hwnd, msg, wparam, lparam));
	}

	return 0;
}
