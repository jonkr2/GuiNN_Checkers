#pragma once

#include "guiWindows.h"

class CheckersGUI : public WindowsGUI
{
public:
	bool Init(HINSTANCE this_inst) override;

	void PastePosition() override;
	void PasteTranscript() override;
	std::string GetTranscriptFileType() override;
	void get_eval_string(const Board& board, std::string &evalstr);
	void DisplayEvaluation(const Board& board) override;
	void ClearSelection() override;

	void OnClickSquare(int sq, eMouseButton mouseButton) override;

	void LoadPieceBitmaps(HINSTANCE inst) override;
	void DrawSquare(HDC hdc, const Board& board, int square, int xPos, int yPos, int size) override;
	void InitMenuItems(HMENU menu) override;
	void ProcessMenuCommand(WORD cmd, HWND hwnd) override;

private:
	void SetupAddPiece(int square64, eColor color);
	eMoveResult SquareMove(Board& board, int src, int dst, eColor Color);

protected:
	int nDouble = 0;
	char scratchBuffer[32768];	// For transcript copying/pasting/loading/saving
};

extern CheckersGUI GUI;
