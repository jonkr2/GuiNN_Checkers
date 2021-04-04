#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

void DisplayText(const char* text);
void RunningDisplay(SearchInfo& displayInfo, const Move& bestmove, bool bSearching);

const int NO_SQUARE = 255;
enum class eMoveResult { INVALID_MOVE = 0, VALID_MOVE = 1, DOUBLEJUMP = 2000 };
enum class eMouseButton { LEFT, RIGHT, MIDDLE };

// Windows specific GUI function definitions
class WindowsGUI
{
public:

	// True windows functions
	static bool AnyInstance(HINSTANCE this_inst);
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static INT_PTR CALLBACK LevelDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	int RegisterClass(HINSTANCE this_inst);
	LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void KeyboardCommands(int key);

	int TextFromClipboard(char* sText, int nMaxBytes);
	int TextToClipboard(const char* sText);
	void ThinkingMenuActive(bool bActive);
	void UpdateMenuChecks();
	void AddMenuItem(HMENU menu, uint32_t cmd, const char* text);

	void DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize);
	void HighlightSquare(HDC hdc, int Square, int sqSize, unsigned long color, int border);
	void ShowErrorPopup(const char* text);

	void OnSelectSquare(int square);

	// Game functions
	void DrawBoard(const struct Board& board);
	void DoGameMove(Move doMove);
	void OnNewGame(const Board& startBoard, bool resetTranscript = true);

	void EndBoardSetup();
	void SetBoardSize(int width, int height);
	int GetSquare(int x, int y);
	void SetTranscriptPosition(int newPos);

	// Overload-able functions
	virtual bool Init(HINSTANCE this_inst);
	virtual void ComputerGo();
	virtual void ProcessMenuCommand(WORD cmd, HWND hwnd);

	virtual void SetComputerColor(eColor Color);
	virtual void SetSearchDepth(int newDepth);
	virtual void SetSearchTime(float newTime);

	virtual void CopyPosition();
	virtual void CopyTranscript();
	virtual void PastePosition() {}
	virtual void PasteTranscript() {}
	virtual std::string GetTranscriptFileType() { return ""; }
	virtual void DisplayEvaluation(const Board& board) {}
	virtual void LoadPieceBitmaps(HINSTANCE inst) {}
	virtual void ClearSelection() {}
	virtual void OnClickSquare(int sq, eMouseButton mouseButton) {}
	virtual void DrawSquare(HDC hdc, const Board& board, int square, int xPos, int yPos, int size) {}
	virtual void InitMenuItems(HMENU menu) {}

	// DATA
	bool bBoardFlip = false;
	bool bSetupBoard = false;

	int boardWidth = 8;
	int boardHeight = 8;

	int windowWidth = 720;
	int windowHeight = 710;
	int squareSize = 64;
	int xAdd = 44, yAdd = 20;
	int selectedSquare = NO_SQUARE;

	static const int MAX_BITMAPS = 32;
	HBITMAP PieceBitmaps[MAX_BITMAPS];

	HWND MainWnd = nullptr;
	HWND BottomWnd = nullptr;

	const uint32_t CUSTOM_MENU_CMD_BASE = 50000;
};
