#pragma once

#include "neuralNet/NeuralNet.h"

enum class eGamePhase { EARLY, MID, END, LATE_END };

struct CheckersNet : NeuralNetBase
{
	CheckersNet(const char* name, int inFirstLayerNeurons, eGamePhase inGamePhase)
	{ 
		firstLayerNeurons = inFirstLayerNeurons;
		gamePhase = inGamePhase;
		SetFilenames(name); 
		memset(InputMap, 0, sizeof(InputMap));
	}

	void InitNetwork() override;
	bool IsActive(const struct Board& board) const override;
	void ConvertToInputValues(const struct Board& board, nnInt_t InputValues[]) const override;

	void IncrementalUpdate(const Move& move, const Board& board, nnInt_t firstLayerValues[]);

	static eGamePhase GetGamePhase(const Board& board);

private:
	void BuildInputMap();
	inline int GetInput(ePieceType piece, int sq) const;

	eGamePhase gamePhase;
	int firstLayerNeurons;
	int InputMap[NUM_PIECE_TYPES][NUM_BOARD_SQUARES];
};

int InitializeNeuralNets();
int LoadBinaryNets(const char* filename);
void SaveBinaryNets(const char* filename);