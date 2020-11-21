//
// CheckersNN.cpp
// by Jonathan Kreuzer
//
// Neural nets for checkers evaluation.
// We're using a 4 different relatively small nets for each for its own game stage.
// For learning the exporter automatically exports the labeled training position to the proper net.
//

#include <string.h>
#include <sstream>  

#include "neuralNet/NeuralNet.h"
#include "engine.h"

int InitializeNeuralNets()
{
	// Note : the enum values need to be in order for netIdx = (int)CheckersNet::GetGamePhase(board);
	engine.evalNets.push_back(new CheckersNet("CheckersEarly", eGamePhase::EARLY_GAME));
	engine.evalNets.push_back(new CheckersNet("CheckersMid", eGamePhase::MID_GAME));
	engine.evalNets.push_back(new CheckersNet("CheckersEnd", eGamePhase::END_GAME));
	engine.evalNets.push_back(new CheckersNet("CheckersLateEnd", eGamePhase::LATE_END_GAME));

	// Init the structure and load the networks
	int numLoaded = 0;
	for (auto net : engine.evalNets )
	{
		net->InitNetwork();
		net->isLoaded = net->network.LoadText(net->neuralNetFile.c_str());
		numLoaded += net->isLoaded ? 1 : 0;
	}

	// If nothing loaded, load nets from binary data instead. The released version won't have the development text nets.
	if (numLoaded == 0) {
		numLoaded = LoadBinaryNets("engines/Nets.gnn");
		if (numLoaded == 0 ) numLoaded = LoadBinaryNets("Nets.gnn");
	}

	return numLoaded;
}

// Load and Save nets from a single binary file
int LoadBinaryNets(const char* filename)
{
	int validNets = 0;
	FILE* fp = fopen(filename, "rb");
	if (fp)
	{
		for (auto net : engine.evalNets)
		{
			net->isLoaded = net->network.ReadNet(fp);
			validNets += net->isLoaded ? 1 : 0;
		}
		fclose(fp);
	}
	return validNets;
}

void SaveBinaryNets( const char *filename )
{
	FILE* fp = fopen(filename, "wb");
	if (fp)
	{
		for (auto net : engine.evalNets)
		{
			net->network.WriteNet(fp);
		}
		fclose(fp);
	}
}

void CheckersNet::InitNetwork()
{
	whiteInputCount = 32 + 28; // 32 king squares, 28 checkers square
	blackInputCount = 32 + 28;

	network.SetInputCount(whiteInputCount + blackInputCount + 1); // +1 for side-to-move
	network.AddLayer(192, eActivation::RELU, eLayout::INPUT_TO_OUTPUTS);
	network.AddLayer(32, eActivation::RELU);
	network.AddLayer(32, eActivation::RELU);
	network.AddLayer(1, eActivation::LINEAR);
	network.Build();
}

eGamePhase CheckersNet::GetGamePhase(const Board& board)
{
	const int numPieces = board.numPieces[WHITE] + board.numPieces[BLACK];

	eGamePhase boardPhase = eGamePhase::EARLY_GAME;
	if (numPieces <= 6) boardPhase = eGamePhase::LATE_END_GAME;
	else if (numPieces <= 8) boardPhase = eGamePhase::END_GAME;
	else if (board.Bitboards.K) boardPhase = eGamePhase::MID_GAME;
	return boardPhase;
}

bool CheckersNet::IsActive(const Board& board) const
{
	if (board.Bitboards.GetCheckers() == 0) return false; // all kings uses hand-crafted "FinishingEval"
	
	return (GetGamePhase(board) == gamePhase);
}

inline int RotateSq(int sq) { return 31 - sq; }

void CheckersNet::ConvertToInputValues(const Board& board, nnInt_t Values[])
{
	NetworkTransform<nnInt_t>* transform = network.GetTransform(0);
	nnInt_t* Inputs = Values;
	nnInt_t* Outputs = network.GetLayerOutputValues(0, Values);

	// Clear inputs and set outputs to biases
	memset(Inputs, 0, sizeof(nnInt_t) * transform->inputCount);
	transform->SetOutputsToBias(Outputs);

	// Set the inputs based on the board
	for (eColor c : {BLACK, WHITE} ) // {board.sideToMove, Opp(board.sideToMove)}
	{
		uint32_t checkers = board.Bitboards.P[c] & ~board.Bitboards.K;
		uint32_t kings = board.Bitboards.P[c] & board.Bitboards.K;
		uint32_t start = (c == WHITE) ? 0 : whiteInputCount;
		while (checkers)
		{
			int sq = PopLowSq(checkers);
			if (c == BLACK) sq = RotateSq(sq);
			transform->AddInput( start + sq - 4, Inputs, Outputs );
		}
		while (kings)
		{
			int sq = PopLowSq(kings);
			if (c == BLACK) sq = RotateSq(sq);
			transform->AddInput(start + 28 + sq, Inputs, Outputs);
		}
	}
	if (board.sideToMove == BLACK) {
		transform->AddInput(whiteInputCount + blackInputCount, Inputs, Outputs);
	}
}

// Get which net input corresponds to piece on square
int CheckersNet::GetInput(int sq, ePieceType piece) const
{
	assert(piece == WKING || piece == BKING || piece == WPIECE || piece == BPIECE);
	const eColor c = (piece & 2) ? WHITE : BLACK;
	const uint32_t start = (c == WHITE) ? 0 : whiteInputCount;
	if (c == BLACK) sq = RotateSq(sq);

	if (piece & KING) return start + 28 + sq;
	return start + sq- 4;
}

// Incrementally update the first-layer non-activated values of the net
void CheckersNet::IncrementalUpdate(const Move& move, const Board& board, nnInt_t firstLayerValues[])
{
	NetworkTransform<nnInt_t>* transform = network.GetTransform(0);
	int src = move.Src();
	int dst = move.Dst();
	const int jumpLen = move.JumpLen();
	ePieceType movedPiece = board.GetPiece(src);

	// remove the moved piece
	transform->RemoveInput(GetInput(src, movedPiece), firstLayerValues);

	// jump move removes jumped opponent pieces
	if (jumpLen > 0)
	{
		int jumpedSq = board.GetJumpSq(src, dst);
		transform->RemoveInput(GetInput(jumpedSq, board.GetPiece(jumpedSq)), firstLayerValues);
		for (int i = 0; i < jumpLen - 1; i++)
		{
			src = dst;
			dst += JumpAddDir[move.Dir(i)];
			jumpedSq = board.GetJumpSq(src, dst);
			transform->RemoveInput(GetInput(jumpedSq, board.GetPiece(jumpedSq)), firstLayerValues);
		}
	}

	// add the movedPiece in the destination
	if ( movedPiece < KING && (dst <= 3 || dst >= 28) ) movedPiece = ePieceType( movedPiece | KING );
	transform->AddInput(GetInput(dst, movedPiece), firstLayerValues);

	// toggle stm
	if (board.sideToMove == WHITE) transform->AddInput(whiteInputCount + blackInputCount, firstLayerValues);
	else transform->RemoveInput(whiteInputCount + blackInputCount, firstLayerValues);
}
