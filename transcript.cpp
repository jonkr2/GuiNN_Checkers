//
// Transcript.cpp
// by Jonathan Kreuzer
//
// Stores the everything needed to replay the game (start board and moves)
// It supports converting to or from a pdn string.
//

#include <string.h>
#include <stdio.h>
#include <sstream> 
#include <fstream>

#include "defines.h"
#include "transcript.h"

Transcript g_Transcript;

// Convert transcript to PDN string and return that string
std::string Transcript::ToString()
{
	std::stringstream text;
	text << "[Event \"" << g_VersionName << " game\"]\015\012";

	const char* boardStr = startBoard.ToString().c_str();
	if (strcmp(boardStr, "B:W24,23,22,21,28,27,26,25,32,31,30,29:B4,3,2,1,8,7,6,5,12,11,10,9.") != 0)
	{
		text << "[SetUp \"1\"]\015\012";
		text << "[FEN \"" << boardStr << "\"]\015\012";
	}

	for (int i = 0; i < numMoves && moves[i] != NO_MOVE; i++ )
	{
		if ((i % 2) == 0) text << ((i/2) + 1) << ". ";
		text << GetMoveString(moves[i]) << " ";
		if (((i+1) % 12) == 0) text << "\015\012";
	}
	text << "*";

	return text.str();
}

void ReadResult( const char buffer[], float* gameResult)
{
	if (memcmp(buffer, "0-1", 3) == 0) {
		if (gameResult) *gameResult = 0.0f;
	}
	if (memcmp(buffer, "1-0", 3) == 0) {
		if (gameResult) *gameResult = 1.0f;
	}
	if (memcmp(buffer, "1/2-1/2", 7) == 0) {
		if (gameResult) *gameResult = 0.5f;
	}
}

// Convert param sPDN to a transcript
int Transcript::FromString(const char* sPDN, float* gameResult )
{
	int i = 0;
	int nEnd = (int)strlen(sPDN);
	int src = 0, dst = 0;
	char sFEN[512];
	Board tempBoard;

	Init(Board::StartPosition());
	tempBoard = startBoard;

	while (i < nEnd)
	{
		// If we read a *, we're done with this game
		if (sPDN[i] == '*') {
			break;
		}

		// If this has a start position, read it
		if (memcmp(&sPDN[i], "[FEN \"", 6) == 0)
		{
			i += 6;
			int x = 0;
			while (sPDN[i] != '"' && i < nEnd) sFEN[x++] = sPDN[i++];
			sFEN[x++] = 0;

			startBoard.FromString(sFEN);
			tempBoard = startBoard;
		}

		// Skip brackets
		if (sPDN[i] == '[') 
		{
			// If this is a result, store the result
			if (memcmp(&sPDN[i], "[Result ", 8) == 0)
			{
				ReadResult(&sPDN[i + 9], gameResult);
			}
			while (sPDN[i] != ']' && i < nEnd) i++;
		}

		// Skip comments
		if (sPDN[i] == '{') while (sPDN[i] != '}' && i < nEnd) i++;

		// Read digits and convert to moves
		if (sPDN[i] >= '0' && sPDN[i] <= '9' && sPDN[i + 1] == '.') i++;
		if (sPDN[i] >= '0' && sPDN[i] <= '9')
		{
			int sq = sPDN[i] - '0';
			i++;
			if (sPDN[i] >= '0' && sPDN[i] <= '9') sq = sq * 10 + sPDN[i] - '0';
			src = FlipSqX(sq - 1);
		}
		if ((sPDN[i] == '-' || sPDN[i] == 'x')
			&& sPDN[i + 1] >= '0' && sPDN[i + 1] <= '9')
		{
			i++;
			int sq = sPDN[i] - '0';
			i++;
			if (sPDN[i] >= '0' && sPDN[i] <= '9') sq = sq * 10 + sPDN[i] - '0';
			dst = FlipSqX(sq - 1);

			MakeMovePDN(src, dst, tempBoard);
		}
		i++;
	}
	return 1;
}

// Make a single move on this transcript from src to dst
int Transcript::MakeMovePDN(int src, int dst, Board& board)
{
	MoveList moves;
	moves.FindMoves(board);

	for (int i = 0; i < moves.numMoves; i++)
	{
		Move move = moves.moves[i];
		if (move.Src() == src
			&& (move.Dst() == dst || Move::GetFinalDst(move) == dst))
		{
			board.DoMove(move);
			AddMove(move);

			return 1;
		}
	}
	return 0;
}

std::string Transcript::GetMoveString(const Move& move)
{
	int src = StandardSquare(move.Src());
	char cap = (move.JumpLen() > 0) ? 'x' : '-';
	int dst = StandardSquare(Move::GetFinalDst(move));
	return std::to_string(src) + cap + std::to_string(dst);
}

bool Transcript::Save(const char* filepath)
{
	std::ofstream file(filepath);
	if (file.good())
	{
		file << ToString();
		file.close();
		return true;
	}
	return false;
}

bool Transcript::Load(const char* filepath)
{
	std::ifstream file(filepath);
	if (file.good())
	{
		std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); // read the contents of file
		file.close(); // close the file

		FromString(content.c_str());
		return true;
	}
	return false;
}

// ------------------
// Replay Game from Game Move History up to numMoves
// ------------------
void Transcript::ReplayGame(Board& board, uint64_t boardHashHistory[] )
{
	board = startBoard;

	int i = 0;
	while (moves[i].data != 0 && i < numMoves) {
		boardHashHistory[i] = board.hashKey;
		board.DoMove(moves[i]);
		i++;
	}
	numMoves = i;
}