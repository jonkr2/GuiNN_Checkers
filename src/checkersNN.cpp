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
	network.AddLayer(192, AT_RELU, LT_INPUT_TO_OUTPUTS);
	network.AddLayer(32, AT_RELU);
	network.AddLayer(32, AT_RELU);
	network.AddLayer(1, AT_LINEAR);
	network.Build();
}

bool CheckersNet::IsActive(const SBoard& board) const
{
	if (board.Bitboards.GetCheckers() == 0) return false; // all kings uses hand-crafted "FinishingEval"

	const int numPieces = board.numPieces[WHITE] + board.numPieces[BLACK];

	eGamePhase boardPhase = eGamePhase::EARLY_GAME;
	if (numPieces <= 6) boardPhase = eGamePhase::LATE_END_GAME; 
	else if (numPieces <= 8) boardPhase = eGamePhase::END_GAME;
	else if (board.Bitboards.K) boardPhase = eGamePhase::MID_GAME;

	return (gamePhase == boardPhase);
}

inline int RotateSq(int sq) { return 31 - sq; }

void CheckersNet::ConvertToInputValues(const SBoard& board, nnInt_t Values[])
{
	NetworkTransform<nnInt_t>* transform = network.GetTransform(0);
	nnInt_t* Inputs = Values;
	nnInt_t* Outputs = network.GetLayerOutputValues(0, Values);

	// Clear inputs and set outputs to biases
	memset(Inputs, 0, sizeof(nnInt_t) * transform->inputCount);
	transform->SetOutputsToBias(Outputs);

	// Set the inputs based on the board
	for (eColor c : {BLACK, WHITE} ) // {board.SideToMove, Opp(board.SideToMove)}
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
	if (board.SideToMove == BLACK) {
		transform->AddInput(whiteInputCount + blackInputCount, Inputs, Outputs);
	}

	// Apply activation function
	transform->TransformActivateOnly(Outputs);
}

// Note : Values are passed to allow safe multi-threading
int CheckersNet::GetNNEval(const SBoard& board, nnInt_t Values[])
{
	ConvertToInputValues(board, Values); // Note : this also sets first layer values for each input

	int val = network.SumHiddenLayers(Values);

	// Reduce value, and soft-clamp so values higher than max are greatly reduced
	return -SoftClamp(val / 3, 400, 800);
}