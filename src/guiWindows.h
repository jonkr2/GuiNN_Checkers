#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// GUI function definitions
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
INT_PTR CALLBACK InfoProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
void DrawBitmap(HDC hdc, HBITMAP bitmap, int x, int y, int nSize);
void HighlightSquare(HDC hdc, int Square, int sqSize, unsigned long color, int border);
void DisplayText(const char* text);
void ShowErrorPopup(const char* text);
void DrawBoard(const struct SBoard& board);
void RunningDisplay(const SMove& bestmove, int bSearching);
void ReplayGame(struct Transcript& transcript, struct SBoard& Board);
void UpdateMenuChecks();
void DoGameMove(SMove doMove);
void SetComputerColor(eColor Color);
void ThinkingMenuActive(int bOn);