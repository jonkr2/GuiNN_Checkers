#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

void DisplayText(const char* text);
void RunningDisplay(const SMove& bestmove, int bSearching);

const int NO_SQUARE = 255;
enum eMoveResult { INVALID_MOVE = 0, VALID_MOVE = 1, DOUBLEJUMP = 2000 };

// Windows specific GUI function definitions
class WindowsGUI
{
public:
	static int AnyInstance(HINSTANCE this_inst);
	int RegisterClass(HINSTANCE this_inst);
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static INT_PTR CALLBACK LevelDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void ProcessCommand(WORD cmd, HWND hwnd);
	void KeyboardCommands(int key);

	void DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize);
	void HighlightSquare(HDC hdc, int Square, int sqSize, unsigned long color, int border);
	void ShowErrorPopup(const char* text);
	void DrawBoard(const struct SBoard& board);
	void UpdateMenuChecks();
	eMoveResult SquareMove(SBoard& board, int x, int y, int xloc, int yloc, eColor Color);
	void DoGameMove(SMove doMove);
	void SetComputerColor(eColor Color);
	void ThinkingMenuActive(int bOn);
	void SetupAddPiece(int x, int y, eColor color);
	void EndBoardSetup();
	int GetSquare(int& x, int& y);
	void DisplayEvaluation(const SBoard& board);

	int TextFromClipboard(char* sText, int nMaxBytes);
	int TextToClipboard(const char* sText);
	void CopyFen();
	void CopyPDN();
	void PasteFen();
	void PastePDN();

	// DATA
	bool bBoardFlip = false;
	int nDouble = 0;
	bool bSetupBoard = false;

	int xAdd = 44, yAdd = 20, nSqSize = 64;
	HBITMAP PieceBitmaps[32];
	int nSelSquare = NO_SQUARE;

	HWND MainWnd = nullptr;
	HWND BottomWnd = nullptr;
};
extern WindowsGUI winGUI;