//
// Windows GUI for GuiNN checkers. 
// by Jonathan Kreuzer
//
// This file also contains WinMain, which is the main function loop under windows.
//

#define NOMINMAX
#include <windows.h>

#include "defines.h"
#include "resource.h"
#include "engine.h"
#include "guiWindows.h"
#include "checkersGui.h"

// ------------------------------------
// Initialize the GUI and the Engine
// ------------------------------------
bool WindowsGUI::AnyInstance( HINSTANCE this_inst )
{
	return GUI.Init( this_inst );
}

bool WindowsGUI::Init( HINSTANCE this_inst )
{
	// create main window
	MainWnd = CreateWindow("GuiCheckersClass",
		g_VersionName,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT, CW_USEDEFAULT,
		windowWidth, windowHeight,
		NULL, NULL, this_inst, NULL);

	if (MainWnd == NULL)
		return false;

	BottomWnd = CreateDialog(this_inst, "BotWnd", MainWnd, WindowsGUI::InfoProc);

	LoadPieceBitmaps(this_inst);

	const int bottomWindowHeight = 180;
	MoveWindow(BottomWnd, 0, 550, windowWidth, bottomWindowHeight, TRUE);
	ShowWindow(MainWnd, SW_SHOW);
	InitMenuItems( GetMenu(MainWnd) );
	ThinkingMenuActive(false);

	DrawBoard(engine.board);

	engine.Init(nullptr);

	OnNewGame(Board::StartPosition());
	DisplayText(engine.GetInfoString().c_str());

	return true;
}

// ------------------------------------
//  Register window class for the application if this is first instance
// ------------------------------------
int WindowsGUI::RegisterClass(HINSTANCE this_inst)
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
		GUI.RegisterClass(this_inst);

	if (GUI.AnyInstance(this_inst) == false)
		return(FALSE);

	// Main message loop
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return((int)msg.wParam);
}

// Size board only on init right now, but if we allow dynamic would want more handling here
void WindowsGUI::SetBoardSize(int width, int height)
{
	boardWidth = width;
	boardHeight = height;
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
	else if (GUI.BottomWnd)
	{
		SetDlgItemText(GUI.BottomWnd, 150, sText);
	}
}

void WindowsGUI::ShowErrorPopup(const char* text)
{
	MessageBox(MainWnd, text, "Error", MB_OK);
}

void WindowsGUI::UpdateMenuChecks()
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

void WindowsGUI::AddMenuItem(HMENU menu, uint32_t cmd, const char* text)
{
	AppendMenu(menu, MF_BYPOSITION | MF_STRING, CUSTOM_MENU_CMD_BASE + cmd, text);
}

// -------------------------------
// WINDOWS CLIPBOARD FUNCTIONS
// -------------------------------
int WindowsGUI::TextToClipboard(const char* sText)
{
	DisplayText(sText);

	char* bufferPtr;
	static HGLOBAL clipTranscript;
	size_t nLen = strlen(sText);
	if (OpenClipboard(MainWnd) == TRUE) {
		EmptyClipboard();
		clipTranscript = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, nLen + 10);
		bufferPtr = (char*)GlobalLock( clipTranscript );
		memcpy(bufferPtr, sText, nLen + 1);
		GlobalUnlock(clipTranscript);
		SetClipboardData(CF_TEXT, clipTranscript);
		CloseClipboard();
		return 1;
	}

	DisplayText("Cannot Open Clipboard");
	return 0;
}

int WindowsGUI::TextFromClipboard(char* sText, int maxBytes)
{
	if (!IsClipboardFormatAvailable(CF_TEXT)) {
		DisplayText("No Clipboard Data");
		return 0;
	}

	OpenClipboard(GUI.MainWnd);

	HANDLE hData = GetClipboardData(CF_TEXT);
	LPVOID pData = GlobalLock(hData);

	int nSize = (int)strlen((char*)pData);
	nSize = std::min(nSize, maxBytes);

	memcpy(sText, (LPSTR)pData, nSize);
	sText[nSize] = NULL;

	GlobalUnlock(hData);
	CloseClipboard();

	return 1;
}

// Gray out certain items
void WindowsGUI::ThinkingMenuActive( bool bActive )
{
	int SwitchItems[10] =
	{
		ID_GAME_NEW,
		ID_FILE_LOADGAME,
		ID_EDIT_SETUPBOARD,
		ID_EDIT_PASTE_TRANSCRIPT,
		ID_EDIT_PASTE_POS,
		ID_GAME_CLEAR_HASH,
		ID_GAME_COMPUTEROFF,
		ID_OPTIONS_COMPUTERBLACK,
		ID_OPTIONS_COMPUTERWHITE,
		ID_GAME_HASHING
	};
	UINT newEnabled = (bActive) ? MF_GRAYED : MF_ENABLED;

	for (int i = 0; i < 8; i++) {
		EnableMenuItem(GetMenu(GUI.MainWnd), SwitchItems[i], newEnabled);
	}

	EnableMenuItem(GetMenu(GUI.MainWnd), ID_GAME_MOVENOW, (bActive) ? MF_ENABLED : MF_GRAYED);
}

void WindowsGUI::OnNewGame(const Board& startBoard, bool resetTranscript )
{
	engine.NewGame(startBoard, resetTranscript);

	SetComputerColor(WHITE);
	ClearSelection();
	DrawBoard(engine.board);
}

// Perform a move on the game board
void WindowsGUI::DoGameMove(Move doMove)
{
	if (TimeSince( engine.searchThreadData.displayInfo.startTimeMs) < 0.25f ) {
		Sleep(200); // pause a bit if move is really quick
	}

	if (doMove != NO_MOVE)
	{
		engine.board.DoMove(doMove); // TODO : for double jumps pause in middle
		engine.transcript.AddMove(doMove);
	}

	DrawBoard(engine.board);
}

void WindowsGUI::OnSelectSquare(int square)
{
	selectedSquare = square;
	if (selectedSquare != NO_SQUARE) {
		DrawBoard(engine.board);
	}

	HDC hdc = GetDC(MainWnd);
	HighlightSquare(hdc, selectedSquare, squareSize, 0xFFFFFF, 1);
	ReleaseDC(MainWnd, hdc);
}

//---------------------------------------------------------
// Draw a frame around a square clicked on by user
//----------------------------------------------------------
void WindowsGUI::HighlightSquare(HDC hdc, int Square, int sqSize, unsigned long color, int border)
{
	HBRUSH brush = CreateSolidBrush(color);

	const int numSquares = boardWidth * boardHeight;
	if (Square >= 0 && Square <= numSquares)
	{
		int x = Square % 8, y = Square / 8;
		if (bBoardFlip) {
			x = boardWidth - 1 - x;
			y = boardHeight -1 - y;
		}

		RECT rect;
		rect.left = x * sqSize + xAdd;
		rect.right = (x + 1) * sqSize + xAdd;
		rect.top = y * sqSize + yAdd;
		rect.bottom = (y + 1) * sqSize + yAdd;

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
void WindowsGUI::DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize)
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
	} else {
		size.x = nSize;
		size.y = nSize;
	}

	DPtoLP(hdc, &origin, 1);
	DPtoLP(memorydc, &size, 1);

	if (nSize == bitmapbuff.bmWidth) {
		BitBlt(hdc, origin.x, origin.y, size.x, size.y, memorydc, 0, 0, SRCCOPY);
	} else {
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
void WindowsGUI::DrawBoard(const Board& board)
{
	const HDC hdc = GetDC(MainWnd);
	const int numSquares = boardWidth * boardHeight;

	for (int sq = 0; sq < numSquares; sq++)
	{
		int srcSq = (bBoardFlip) ? numSquares - 1 - sq : sq;
		int x = srcSq % boardWidth;
		int y = srcSq / boardWidth;

		DrawSquare(hdc, board, sq, x * squareSize + xAdd, y * squareSize + yAdd, squareSize);
	}

	if (selectedSquare != NO_SQUARE)
		HighlightSquare( hdc, selectedSquare, squareSize, 0xFFFFFF, 1);

	ReleaseDC(MainWnd, hdc);
}

// --------------------
// GetFileName - get a file name using common dialog
// --------------------
static BOOL GetFileName(HWND hwnd, BOOL save, char* fname, const char* filterList)
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
	} else {
		rc = GetOpenFileName(&of);
	}

	return(rc);
}

void WindowsGUI::EndBoardSetup()
{
	if (bSetupBoard)
	{
		engine.board.SetFlags();
		bSetupBoard = FALSE;
		engine.transcript.Init(engine.board);
		DisplayText("");
	}
}

void WindowsGUI::ComputerGo()
{
	EndBoardSetup();

	if (engine.bThinking) {
		engine.MoveNow();
	} else {
		ThinkingMenuActive(true);
		engine.StartThinking();
	}
}

// Convert from windows(x,y) to a square on the board. If not on board, return NO_SQUARE
int WindowsGUI::GetSquare(int x, int y)
{
	// Calculate the square the user clicked on
	x = (int)floorf( (x - xAdd) / (float)squareSize );
	y = (int)floorf( (y - yAdd) / (float)squareSize );

	// Make sure it's valid, preferrably this function would only be called with valid input
	if (x < 0 || x >= boardWidth) return NO_SQUARE;
	if (y < 0 || y >= boardWidth) return NO_SQUARE;

	if (bBoardFlip) {
		x = boardWidth - 1 - x;
		y = boardHeight - 1 - y;
	}

	return x + y * boardWidth;
}

void WindowsGUI::SetComputerColor(eColor Color)
{
	engine.computerColor = Color;
	UpdateMenuChecks();
}

void WindowsGUI::SetSearchDepth(int newDepth)
{
	engine.searchLimits.maxDepth = newDepth;
	UpdateMenuChecks();
}

void WindowsGUI::SetSearchTime(float newTime)
{
	engine.searchLimits.maxSeconds = newTime;
	UpdateMenuChecks();
}

void WindowsGUI::CopyPosition()
{
	TextToClipboard(engine.board.ToString().c_str());
}

void WindowsGUI::CopyTranscript()
{
	TextToClipboard(engine.transcript.ToString().c_str());
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

void WindowsGUI::KeyboardCommands(int key)
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
		GUI.PastePosition();
	if (key == 'C')
		GUI.CopyPosition();
	if (key == 'V' && (GetKeyState(VK_CONTROL) < 0))
		GUI.PasteTranscript();
	if (key == 'C' && (GetKeyState(VK_CONTROL) < 0))
		GUI.CopyTranscript();

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
		DisplayEvaluation( engine.board );
	}
	if (key == 'T')
	{
		// show TT entry
		TEntry* entry = engine.TTable.GetEntry(engine.board, engine.ttAge);
		if (entry && entry->IsBoard(engine.board.hashKey)) {
			DisplayText(GetTTEntryString(entry, engine.board.sideToMove).c_str());
		} else {
			DisplayText("No TT Entry for board");
		}
	}
}

// ===============================================
// Process messages to the Bottom Window
// ===============================================
void WindowsGUI::SetTranscriptPosition( int newPos )
{
	engine.MoveNow(); // Stop thinking

	engine.transcript.numMoves = std::max( newPos, 0 );
	engine.transcript.ReplayGame(engine.board, engine.boardHashHistory);

	DrawBoard(engine.board);
	SetFocus(MainWnd);
}


INT_PTR CALLBACK WindowsGUI::InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) 
	{
	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_TAKEBACK:
			GUI.SetTranscriptPosition(engine.transcript.numMoves - 2);
			break;

		case IDC_PREV:
			GUI.SetTranscriptPosition(engine.transcript.numMoves - 1);
			break;

		case IDC_NEXT:
			GUI.SetTranscriptPosition(engine.transcript.numMoves + 1);
			break;

		case IDC_START:
			GUI.SetTranscriptPosition(0);
			break;

		case IDC_END:
			GUI.SetTranscriptPosition( MAX_GAMEMOVES );
			break;

		case IDC_GO:
			GUI.EndBoardSetup();
			GUI.ComputerGo();
			SetFocus(GUI.MainWnd);
			break;
		}
	}

	return(0);
}

// ===============================================
//  Level Select Dialog Procedure
// ===============================================
INT_PTR CALLBACK WindowsGUI::LevelDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM /*lparam*/)
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

// ===============================================
// Process a command message
// ===============================================
void WindowsGUI::ProcessMenuCommand(WORD cmd, HWND hwnd)
{
	switch (cmd)
	{
	case ID_GAME_FLIPBOARD:
		bBoardFlip = !bBoardFlip;
		DrawBoard(engine.board);
		break;

	case ID_GAME_NEW:
		OnNewGame(Board::StartPosition());
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
		GUI.UpdateMenuChecks();
		break;

	case ID_GAME_CLEAR_HASH:
		engine.TTable.Clear();
		break;

	case ID_OPTIONS_BEGINNER: SetSearchDepth(BEGINNER_DEPTH); break;
	case ID_OPTIONS_NORMAL:   SetSearchDepth(NORMAL_DEPTH); break;
	case ID_OPTIONS_EXPERT:   SetSearchDepth(EXPERT_DEPTH); break;

	case ID_OPTIONS_1SECOND:   SetSearchTime(1.0f); break;
	case ID_OPTIONS_2SECONDS:  SetSearchTime(2.0f); break;
	case ID_OPTIONS_5SECONDS:  SetSearchTime(5.0f); break;
	case ID_OPTIONS_10SECONDS: SetSearchTime(10.0f); break;
	case ID_OPTIONS_30SECONDS: SetSearchTime(30.0f); break;

	case ID_OPTIONS_CUSTOMLEVEL:
		DialogBox((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), "LevelWnd", hwnd, LevelDlgProc);
		break;

	case ID_FILE_SAVEGAME:
	{
		char filepath[2048];
		if (GetFileName(hwnd, 1, filepath, GetTranscriptFileType().c_str()))
			engine.transcript.Save(filepath);
		break;
	}

	case ID_FILE_LOADGAME:
	{
		char filepath[2048];
		if (GetFileName(hwnd, 0, filepath, GetTranscriptFileType().c_str())) {
			if (engine.transcript.Load(filepath)) {
				OnNewGame(engine.transcript.startBoard, false);
			}
		}
		break;
	}

	case ID_FILE_EXIT:
		PostMessage(hwnd, WM_DESTROY, 0, 0);
		break;

	case ID_EDIT_SETUPBOARD:
		if (bSetupBoard) {
			GUI.EndBoardSetup();
		} else {
			bSetupBoard = true;
			DisplayText("BOARD SETUP MODE.       (Click GO to end) \n(shift+click to erase pieces) \n(alt+click on a piece to set that color to move)");
		}
		break;

	case ID_EDIT_COPY_POS:
		CopyPosition();
		break;

	case ID_EDIT_PASTE_POS:
		PastePosition();
		break;

	case ID_EDIT_COPY_TRANSCRIPT:
		CopyTranscript();
		break;

	case ID_EDIT_PASTE_TRANSCRIPT:
		PasteTranscript();
		break;

	case ID_GAME_MOVENOW:
		engine.MoveNow();
		break;
	}
}

// ===============================================
// Process messages to the MAIN Window
// ===============================================
LRESULT CALLBACK WindowsGUI::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return GUI.ProcessMessage(hwnd, msg, wparam, lparam);
}

LRESULT WindowsGUI::ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	HDC hdc;
	int square;

	switch (msg)
	{
	case WM_COMMAND:
		ProcessMenuCommand(LOWORD(wparam), hwnd);
		break;

	case WM_KEYDOWN:
		KeyboardCommands((int)wparam);
		break;

		// Process Mouse Clicks on the board
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
		SetFocus(hwnd);

		if (engine.bThinking)
			return TRUE;  // Don't move pieces when computer is thinking

		// Did the user click a square on the board?
		square = GetSquare((int)LOWORD(lparam), (int)HIWORD(lparam));

		if (square != NO_SQUARE && msg == WM_LBUTTONDOWN)
		{
			OnClickSquare(square, eMouseButton::LEFT );
		}

		return TRUE;

	case WM_RBUTTONDOWN:
		
		square = GetSquare((int)LOWORD(lparam), (int)HIWORD(lparam));

		if (square != NO_SQUARE) {
			OnClickSquare(square, eMouseButton::RIGHT);
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
