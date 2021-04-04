//
// Game-Specific GUI class for 8x8 Checkers
//

#include "engine.h"
#include "checkersGui.h"
#include "kr_db.h"
#include "learning.h"

CheckersGUI GUI;

bool CheckersGUI::Init(HINSTANCE this_inst)
{
	windowWidth = 720;
	windowHeight = 710;

	squareSize = 64;
	boardWidth = 8;
	boardHeight = 8;

	return WindowsGUI::Init(this_inst);
}

void CheckersGUI::LoadPieceBitmaps(HINSTANCE inst)
{
	char* bitmapNames[10] = { nullptr, "Rcheck", "Wcheck", nullptr, nullptr, "RKing", "Wking", "Wsquare", "Bsquare", nullptr };

	for (int i = 0; i < 10; i++) {
		if (bitmapNames[i] != nullptr)
			GUI.PieceBitmaps[i] = ((HBITMAP)LoadImage(inst, bitmapNames[i], IMAGE_BITMAP, 0, 0, 0));
	}
}

// MENUS
enum { MENU_IMPORT_MATCHES, MENU_EXPORT_TRAINING, MENU_SAVE_BINARY_NETS };

void CheckersGUI::InitMenuItems( HMENU menu )
{
	// Create Dev Menu
	HMENU subMenu = GetSubMenu(menu, 4);
	AddMenuItem(subMenu, MENU_IMPORT_MATCHES, "Import Matches");
	AddMenuItem(subMenu, MENU_EXPORT_TRAINING, "Export Training Sets");
	AddMenuItem(subMenu, MENU_SAVE_BINARY_NETS, "Save Binary Nets");
}

void CheckersGUI::ProcessMenuCommand(WORD cmd, HWND hwnd)
{
	WORD customCmd = cmd - CUSTOM_MENU_CMD_BASE;
	char tempBuf[256] = { 0 };
	switch (customCmd)
	{
	case MENU_SAVE_BINARY_NETS:
		SaveBinaryNets( engine.binaryNetFile.c_str() );
		snprintf(tempBuf, sizeof(tempBuf), "Saved Binary Nets to \"%s\"", engine.binaryNetFile.c_str());
		DisplayText(tempBuf);
		break;

	case MENU_EXPORT_TRAINING:
		DisplayText("Exporting training Sets...");
		NeuralNetLearner::CreateTrainingSet();
		DisplayText("Export Training Sets Done!");
		break;

	case MENU_IMPORT_MATCHES:
	{
		DisplayText("Import Latest Matches");
		MatchResults results;
		int numFiles = NeuralNetLearner::ImportLatestMatches(results);
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "Imported %d files.\nW-L-D : %d-%d-%d  (%.1f%%)\n%.1f elo\n", numFiles, results.wins, results.losses, results.draws, results.Percent() * 100.0f, results.EloDiff());
		DisplayText(buffer);
		break;
	}
		default: break;
	}

	WindowsGUI::ProcessMenuCommand(cmd, hwnd);
}

void CheckersGUI::DrawSquare(HDC hdc, const Board& board, int square, int xPos, int yPos, int size)
{
	int square32 = Board64to32[square];
	ePieceType piece = board.GetPiece(square32);
	if (piece != EMPTY && piece != INVALID)
	{
		DrawBitmap(hdc, PieceBitmaps[piece], xPos, yPos, size);
	}
	else
	{
		// empty square
		if (((square % 2 == 1) && (square / boardWidth) % 2 == 1) || ((square % 2 == 0) && (square / boardWidth) % 2 == 0))
			DrawBitmap(hdc, PieceBitmaps[7], xPos, yPos, size);
		else
			DrawBitmap(hdc, PieceBitmaps[8], xPos, yPos, size);
	}
}

void CheckersGUI::SetupAddPiece(int square64, eColor color)
{
	int square = Board64to32[square64];
	if (square == INV)
		return;

	if (GetKeyState(VK_SHIFT) < 0) {
		engine.board.SetPiece(square, EMPTY);
		DrawBoard(engine.board);
		return;
	}

	if (GetKeyState(VK_MENU) < 0) {
		engine.board.sideToMove = (engine.board.GetPiece(square) & BPIECE) ? BLACK : WHITE;
		return;
	}

	ePieceType checker = (color == WHITE) ? WPIECE : BPIECE;
	ePieceType king = (color == WHITE) ? WKING : BKING;
	ePieceType oldPiece = engine.board.GetPiece(square);
	ePieceType newPiece = (oldPiece == checker) ? king : (oldPiece == king) ? EMPTY : (RankMask(color, 7) & S[square]) ? king : checker;
	engine.board.SetPiece(square, newPiece);

	DrawBoard(engine.board);
}

void CheckersGUI::get_eval_string(const Board& board, std::string &evalstr)
{
	EvalNetInfo &netInfo = engine.searchThreadData.stack->netInfo; 
	netInfo.netIdx = (int)CheckersNet::GetGamePhase(board);
	engine.evalNets[netInfo.netIdx]->ComputeFirstLayerValues(board, engine.searchThreadData.nnValues, netInfo.firstLayerValues);
	int eval = board.EvaluateBoard(0, engine.searchThreadData, netInfo, 100);
	if (board.sideToMove == WHITE)
		eval = -eval;			/* Make it + strong for black. */

	std::string databaseBuffer;
	if (abs(eval) != 2001 && engine.dbInfo.InDatabase(board))
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

		if (engine.dbInfo.type == dbType::KR_WIN_LOSS_DRAW && engine.dbInfo.InDatabase(board)) {
			EGDB_BITBOARD bb;

			gui_to_kr(board.Bitboards, bb);
			int result = engine.dbInfo.kr_wld->lookup(engine.dbInfo.kr_wld, &bb, gui_to_kr_color(board.sideToMove), 100);
			if (result == EGDB_WIN) {
				if (board.sideToMove == WHITE)
					databaseBuffer = "white win";
				else
					databaseBuffer = "black win";
			}
			else if (result == EGDB_LOSS) {
				if (board.sideToMove == BLACK)
					databaseBuffer = "white win";
				else
					databaseBuffer = "black win";
			}
			else if (result == EGDB_DRAW) {
				databaseBuffer = "db draw";
			}
		}
		if (engine.dbInfo.type == dbType::EXACT_VALUES) {
			int result = QueryEdsDatabase(board, 0);
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

	snprintf(scratchBuffer, sizeof(scratchBuffer),
		"eval: %d\nnumWhite: %d   numBlack: %d\n%s %s\n",
		eval,
		board.numPieces[WHITE],
		board.numPieces[BLACK],
		databaseBuffer.c_str(),
		Repetition(board.hashKey, engine.boardHashHistory, 0, engine.transcript.numMoves) ? "(Repetition)" : "");
	evalstr = scratchBuffer;
}

void CheckersGUI::DisplayEvaluation(const Board& board)
{
	std::string evalstr;
	get_eval_string(board, evalstr);
	DisplayText(evalstr.c_str());
}

void CheckersGUI::PastePosition()
{
	if (TextFromClipboard(scratchBuffer, 512))
	{
		DisplayText(scratchBuffer);
		if (engine.board.FromString(scratchBuffer))
		{
			OnNewGame(engine.board);
		}
	}
}

void ReadTranscriptFromBuffer(const char* buffer)
{
	GUI.OnNewGame( Board::StartPosition() );
	engine.transcript.FromString(buffer);

	engine.transcript.ReplayGame(engine.board, engine.boardHashHistory);

	GUI.DrawBoard(engine.board);
	DisplayText(buffer);
}

void CheckersGUI::PasteTranscript()
{
	if (TextFromClipboard(scratchBuffer, 16380))
	{
		ReadTranscriptFromBuffer(scratchBuffer);
	}
}

std::string CheckersGUI::GetTranscriptFileType()
{
	return std::string("Checkers Game (*.pdn)\0*.pdn\0\0");
}

// Mouse Input
void CheckersGUI::OnClickSquare(int square, eMouseButton mouseButton )
{
	if (!bSetupBoard)
	{
		// In regular mode we select our pieces and move them
		if (mouseButton == eMouseButton::LEFT)
		{
			ePieceType piece = engine.board.GetPiece(Board64to32[square]);
			if (piece != EMPTY)
			{
				if (nDouble != 0)
					return;  // Can't switch to a different piece when double jumping

				if (((piece & BPIECE) && engine.board.sideToMove == BLACK) ||
					((piece & WPIECE) && engine.board.sideToMove == WHITE))
				{
					// this is a valid selection, update the GUI
					OnSelectSquare(square);
				}
			}
			else if (selectedSquare != NO_SQUARE)
			{
				eMoveResult MoveResult = SquareMove(engine.board, selectedSquare, square, engine.board.sideToMove);
			
				// Did the user click on a valid destination square?			
				if (MoveResult == eMoveResult::VALID_MOVE) {
					selectedSquare = NO_SQUARE;
					DrawBoard(engine.board);

					if (engine.computerColor == engine.board.sideToMove)
						ComputerGo();
				}
				else if (MoveResult == eMoveResult::DOUBLEJUMP) {
					selectedSquare = square;
					DrawBoard(engine.board);
				}
			}
		}
	}

	// In board setup mode, this changes the piece on square
	if (bSetupBoard)
	{
		if (mouseButton == eMouseButton::LEFT)
		{
			SetupAddPiece(square, BLACK);
		}
		if (mouseButton == eMouseButton::RIGHT)
		{
			SetupAddPiece(square, WHITE);
		}
	}
}

void CheckersGUI::ClearSelection()
{
	nDouble = 0;
	selectedSquare = NO_SQUARE;
}

//  Check Possiblity/Execute Move from one selected square to another
//  returns INVALID_MOVE if the move is not possible
//          VALID_MOVE if is a valid and complete move
//          DOUBLEJUMP if the move is an uncompleted jump
eMoveResult CheckersGUI::SquareMove(Board& board, int src, int dst, eColor Color)
{
	MoveList MoveList;
	MoveList.FindMoves(board);
	src = Board64to32[src];
	dst = Board64to32[dst];

	for (int i = 0; i < MoveList.numMoves; i++)
	{
		const Move& move = MoveList.moves[i];
		if (move.Src() == src && move.Dst() == dst)
		{
			// Check if the src & dst match a move from the generated movelist
			if (nDouble > 0)
			{
				// Build double jump move for game transcript
				Move& jumpMove = engine.transcript.Back();
				for (int j = 0; j < 4; j++) {
					if (dst - src == JumpAddDir[j]) {
						jumpMove.SetJumpDir(nDouble - 1, j);
					}
				}
				jumpMove.SetJumpLen(nDouble + 1);
			}
			else
			{
				engine.transcript.AddMove(move);
			}

			if (move.JumpLen() > 1)
			{
				// perform the first part of the move on board, will have to move again
				board.DoSingleJump(src, dst, board.GetPiece(src), board.sideToMove);
				nDouble++;
				return eMoveResult::DOUBLEJUMP;
			}

			board.DoMove(move);

			nDouble = 0;
			return eMoveResult::VALID_MOVE;
		}
	}

	return eMoveResult::INVALID_MOVE;
}
