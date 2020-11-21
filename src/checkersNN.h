#pragma once

#include "neuralNet/NeuralNet.h"

enum class eGamePhase { EARLY_GAME, MID_GAME, END_GAME, LATE_END_GAME };

struct CheckersNet : NeuralNetBase
{
	CheckersNet(const char* name, eGamePhase phase ) : gamePhase(phase) { 
		SetFilenames(name); 
	}

	void InitNetwork() override;
	bool IsActive(const struct Board& board) const override;
	void ConvertToInputValues(const struct Board& board, nnInt_t InputValues[]) override;

	void IncrementalUpdate(const Move& move, const Board& board, nnInt_t firstLayerValues[]);

	static eGamePhase GetGamePhase(const Board& board);

private:
	inline int GetInput(int sq, ePieceType piece) const;

	eGamePhase gamePhase;
};

int InitializeNeuralNets();
int LoadBinaryNets(const char* filename);
void SaveBinaryNets(const char* filename);