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
	auto testNet = new CheckersNet("Test", 128, eGamePhase::EARLY);
	// Note : the enum values need to be in order for netIdx = (int)CheckersNet::GetGamePhase(board);
	engine.evalNets.push_back(new CheckersNet("CheckersEarly", 224, eGamePhase::EARLY));
	engine.evalNets.push_back(new CheckersNet("CheckersMid", 224, eGamePhase::MID));
	engine.evalNets.push_back(new CheckersNet("CheckersEnd", 192, eGamePhase::END));
	engine.evalNets.push_back(new CheckersNet("CheckersLateEnd", 192, eGamePhase::LATE_END));

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
		numLoaded = LoadBinaryNets( (std::string("engines/") + engine.binaryNetFile).c_str() );
		if (numLoaded == 0 ) numLoaded = LoadBinaryNets(engine.binaryNetFile.c_str());
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
	BuildInputMap();

	network.SetInputCount(whiteInputCount + blackInputCount + 1); // +1 for side-to-move
	network.AddLayer(firstLayerNeurons, eActivation::RELU, eLayout::SPARSE_INPUTS);
	network.AddLayer(32, eActivation::RELU);
	network.AddLayer(32, eActivation::RELU);
	network.AddLayer(1);
	network.Build();
}

eGamePhase CheckersNet::GetGamePhase(const Board& board)
{
	const int numPieces = board.numPieces[WHITE] + board.numPieces[BLACK];

	if (numPieces <= 8) {
		return (numPieces <= 6) ? eGamePhase::LATE_END : eGamePhase::END;
	}
	return (board.Bitboards.K) ? eGamePhase::MID : eGamePhase::EARLY;
}

bool CheckersNet::IsActive(const Board& board) const
{
	if (board.Bitboards.GetCheckers() == 0) return false; // all kings uses hand-crafted "FinishingEval"
	
	return (GetGamePhase(board) == gamePhase);
}

void CheckersNet::ConvertToInputValues(const Board& board, nnInt_t Values[]) const
{
	const NetworkTransform<nnInt_t>* transform = network.GetTransform(0);
	nnInt_t* Inputs = Values;
	nnInt_t* Outputs = network.GetLayerOutputValues(0, Values);

	// Clear inputs and set outputs to biases
	memset(Inputs, 0, sizeof(nnInt_t) * transform->inputCount);
	transform->SetOutputsToBias(Outputs);

	// Set the inputs based on the board
	for (eColor c : {BLACK, WHITE} )
	{
		uint32_t checkers = board.Bitboards.P[c] & ~board.Bitboards.K;
		uint32_t kings = board.Bitboards.P[c] & board.Bitboards.K;
		uint32_t piece = (c == WHITE) ? WPIECE : BPIECE;
		while (checkers)
		{
			int sq = PopLowSq(checkers);
			transform->AddInput( InputMap[piece][sq], Inputs, Outputs );
		}
		while (kings)
		{
			int sq = PopLowSq(kings);
			transform->AddInput( InputMap[ piece|KING ][sq], Inputs, Outputs);
		}
	}
	if (board.sideToMove == BLACK) {
		transform->AddInput(whiteInputCount + blackInputCount, Inputs, Outputs);
	}
}

void CheckersNet::BuildInputMap()
{
	memset(InputMap, 0, sizeof(InputMap));
	for (ePieceType piece : { BPIECE, WPIECE, BKING, WKING })
	{
		for (int sq = 0; sq < 32; sq++)
		{
			InputMap[piece][sq] = GetInput(piece, sq );
		}
	}
}

// Get which net input corresponds to piece on square
int CheckersNet::GetInput(ePieceType piece, int sq) const
{
	assert(piece == WKING || piece == BKING || piece == WPIECE || piece == BPIECE);
	const eColor c = (piece & 2) ? WHITE : BLACK;
	const uint32_t start = (c == WHITE) ? 0 : whiteInputCount;

	if (piece & KING) return start + 28 + sq;
	// remove back row for white checkers to map 0-28
	return (c == BLACK) ? start + sq : start + sq - 4;
}

// Incrementally update the first-layer non-activated values of the net
void CheckersNet::IncrementalUpdate(const Move& move, const Board& board, nnInt_t firstLayerValues[])
{
	const NetworkTransform<nnInt_t>* transform = network.GetTransform(0);
	int src = move.Src();
	int dst = move.Dst();
	const int jumpLen = move.JumpLen();
	ePieceType movedPiece = board.GetPiece(src);

	// remove the moved piece
	transform->RemoveInput(InputMap[movedPiece][src], firstLayerValues);

	// jump move removes jumped opponent pieces
	if (jumpLen > 0)
	{
		int jumpedSq = board.GetJumpSq(src, dst);
		transform->RemoveInput(InputMap[board.GetPiece(jumpedSq)][jumpedSq], firstLayerValues);
		for (int i = 0; i < jumpLen - 1; i++)
		{
			src = dst;
			dst += JumpAddDir[move.Dir(i)];
			jumpedSq = board.GetJumpSq(src, dst);
			transform->RemoveInput(InputMap[board.GetPiece(jumpedSq)][jumpedSq], firstLayerValues);
		}
	}

	// add the movedPiece in the destination
	if ( movedPiece < KING && (dst <= 3 || dst >= 28) ) movedPiece = ePieceType( movedPiece | KING );
	transform->AddInput(InputMap[movedPiece][dst], firstLayerValues);

	// toggle stm
	if (board.sideToMove == WHITE) transform->AddInput(whiteInputCount + blackInputCount, firstLayerValues);
	else transform->RemoveInput(whiteInputCount + blackInputCount, firstLayerValues);
}
