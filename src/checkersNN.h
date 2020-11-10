#pragma once

#include "neuralNet/NeuralNet.h"

enum class eGamePhase { EARLY_GAME, MID_GAME, END_GAME, LATE_END_GAME };

struct CheckersNet : NeuralNetBase
{
	CheckersNet(const char* name, eGamePhase phase ) : gamePhase(phase) { 
		SetFilenames(name); 
	}

	void InitNetwork() override;
	bool IsActive(const struct SBoard& board) const override;
	int GetNNEval(const struct SBoard& board, nnInt_t Values[]) override;
	void ConvertToInputValues(const struct SBoard& board, nnInt_t InputValues[]) override;

	eGamePhase gamePhase;
};

int InitializeNeuralNets();
int LoadBinaryNets(const char* filename);
void SaveBinaryNets(const char* filename);