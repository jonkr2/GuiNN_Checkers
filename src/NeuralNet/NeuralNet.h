#pragma once

#include <thread>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "mathSimd.h"

enum eActivationType { AT_RELU, AT_LINEAR };
enum eLayoutType { LT_INPUTS_TO_OUTPUT, LT_INPUT_TO_OUTPUTS };

const int kFixedShift = 8;
const float kFixedFloatMult = 256.0f;
const int kFixedMax = (1 << 15) - 1;
const int kMaxEvalNetValues = 1280;

// TODO : where to put this?
// Aligned malloc and free for MSVC and GCC
inline void* AlignedAllocUtil(size_t memSize, size_t align)
{
#ifdef __GNUC__
	return aligned_alloc(align, memSize);
#else
	return _aligned_malloc(memSize, align);
#endif
}

inline void AlignedFreeUtil(void* data)
{
#ifdef __GNUC__
	free(data);
#else
	_aligned_free(data);
#endif
}

typedef int16_t nnInt_t;

template< typename T >
struct NetworkTransform
{
	uint32_t inputCount = 0;
	uint32_t outputCount = 0;

	T* Weights = nullptr;
	uint32_t weightStart = 0;
	uint32_t biasStart = 0;

	void Init(int inputs, int outputs, T* InWeights, int inWeightStart, eActivationType inType, eLayoutType inLayoutType)
	{
		activationType = inType;
		layoutType = inLayoutType;
		inputCount = inputs;
		outputCount = outputs;
		Weights = InWeights;
		weightStart = inWeightStart;
		biasStart = inWeightStart + inputCount * outputCount;

		InitRandom();
	}
	
	// Transform input network to output network using weights and biases
	void Transform(T Inputs[], T Outputs[])
	{
		assert(Inputs && Outputs);

		T* weightsForOutput = &Weights[weightStart];
		for (uint32_t o = 0; o < outputCount; o++, weightsForOutput += inputCount)
		{
			// Sum the weighted inputs for this output (build val in int32 starting with bias)
			int32_t val = (int32_t)Weights[biasStart + o] << kFixedShift;
					
			//val += dotProductSimdInt32(weightsForOutput, Inputs, inputCount);
			val += dotProductSimdInt16(weightsForOutput, Inputs, inputCount);

			if (activationType == AT_RELU) {
				Outputs[o] = ClampInt(val >> kFixedShift, 0, kFixedMax);
				// assert((val >> kFixedShift) < kFixedMax);
			} else {
				Outputs[o] = ClampInt(val >> kFixedShift, -kFixedMax, kFixedMax);
			}
		}
	}

	void inline SetOutputsToBias(T Outputs[] )
	{
		memcpy(Outputs, &Weights[biasStart], sizeof(T) * outputCount);
	}

	// If input is 0, can skip it... but would have to sum by input instead of by output. Which makes sense for incremental updates and rook endgames.
	void TransformInputToOutput(T Inputs[], T Outputs[])
	{
		assert(false); 
		/*
		// Set the outputs to start at their biases.
		SetOutputsToBias(Outputs);

		// For each input that is set, add it to all the outputs
		for (int i = 0; i < inputCount; i++)
		{
			// If this is just 1s and 0s
			if ( Input[i] > 0 ) {
				addVecSimd16(Output, &Weights[WeightIdx(i, 0)], outputCount);
			}
		}

		// this shift could be in the activate
		for (int o = 0; o < outputCount; o++ )
		{
			Outputs[o] = (int32_t)Output[o] >> kFixedShift;
		}

		TransformActivateOnly( Outputs );
		*/
	}

	void TransformActivateOnly(T Outputs[])
	{
		if (activationType == AT_RELU)
		{
			for (uint32_t o = 0; o < outputCount; o++)
			{
				Outputs[o] = std::max( Outputs[o], (T)0 );
			}
		}
	}

	void AddInput(int i, T Inputs[], T Outputs[])
	{
		Inputs[i] = 1; // TODO : decide input type here?

		//addVecSimd32(Outputs, &Weights[WeightIdx(i, 0)], outputCount);
		addVecSimd16(Outputs, &Weights[weightStart + i * outputCount], outputCount);
	}

	// For incremental updates.. Untested look at this again
	void RemoveInput(int i, T Inputs[], T Outputs[])
	{
		// Turn off old input... Do we have to worry about precision issues building up?
		// Also do we need to verify input is on in this function?
		Inputs[i] = 0;

		subVecSimd(Outputs, &Weights[weightStart + i * outputCount], outputCount);
	}

	uint32_t Size() const { return WeightCount() * sizeof(T); }
	uint32_t WeightCount() const { return inputCount * outputCount + outputCount; }
	uint32_t InputCount() const { return inputCount; }
	uint32_t OutputCount() const  { return outputCount; }
	inline int WeightIdx(int input, int output) const
	{
		if ( layoutType == LT_INPUTS_TO_OUTPUT)
			return weightStart + inputCount * output + input;
		else 
			return weightStart + output + input * outputCount;
	}
	inline int BiasIdx(uint32_t o) const { return biasStart + o;  }

	void Save(FILE* fp)
	{
		// Save Weights + Biases
		const uint32_t count = WeightCount();
		fwrite(&count, sizeof(uint32_t), 1, fp );
		fwrite(&Weights[weightStart], sizeof(T), WeightCount(), fp);
	}

	bool Load(FILE* fp)
	{
		size_t bytesRead = 0;
		uint32_t count = 0;
		bytesRead += fread(&count, sizeof(uint32_t), 1, fp);
		bytesRead += fread(&Weights[weightStart], sizeof(T), count, fp);

		return count == WeightCount();
	}

	void InitRandom()
	{
		/*for (uint32_t i = weightStart; i < weightStart + inputCount * outputCount; i++)
		{
			uint32_t r = RandGen.Rand32() % 2048;
			double rd = (r - 1024.0) / 1024.0;
			rd *= sqrt(6.0f) / sqrt((float)(inputCount + outputCount));
			Weights[i] = (T)rd;
		}

		for (uint32_t i = biasStart; i < biasStart + outputCount; i++)
		{
			Weights[i] = (T)0;
		}*/
	}

	eActivationType activationType = AT_RELU;
	eLayoutType layoutType = LT_INPUTS_TO_OUTPUT;
};

class NetworkLayer
{
public:
	void Init(int inOutputCount, int inValueStart, eActivationType inActivationType, eLayoutType inLayoutType )
	{
		outputCount = inOutputCount;
		outputValueStart = inValueStart;
		activationType = inActivationType;
		layoutType = inLayoutType;
	}

	int outputCount = 0;
	eActivationType activationType = AT_RELU;
	eLayoutType layoutType = LT_INPUTS_TO_OUTPUT;

	// Value array is in NeuralNetwork, this is the start index
	int outputValueStart = 0;
};

template< typename T >
class NeuralNetwork
{
public:
	~NeuralNetwork()
	{
		if (Weights)
		{
			AlignedFreeUtil(Weights);
			Weights = nullptr;
		}
	}
	T Sum(T InputLayer[]);
	T SumHiddenLayers(T InputLayer[]);
	// T SumIncremental(T IncValues[], T Values[]);

	bool Load( const char* filename );
	bool Save( const char* filename );
	void WriteNet(FILE* fp);
	bool ReadNet(FILE* fp);

	// For loading from tensor flow
	bool LoadText( const char* filename );

	int Size() const
	{ 
		int size = 0;
		for (uint32_t i = 0; i < layerCount; i++) size += Transforms[i].Size();
		return size;
	}
	int WeightCount() const
	{
		return weightCount;
	}
	int ValueMax() const
	{
		if (layerCount == 0) return 0;
		return Layers[layerCount - 1].outputValueStart + Layers[layerCount - 1].outputCount;
	}

	double GetWeight(int w) { return Weights[w]; }
	std::string PrintWeights();
	void ResetWeights() 
	{
		// TODO : save type of init in case it matters
		for (uint32_t l = 0; l < layerCount-1; l++)
			Transforms[l].InitRandom();
	}
	bool IsBias(uint32_t w) 
	{
		for (uint32_t l = 0; l < layerCount; l++) {
			if (w >= Transforms[l].biasStart && w < Transforms[l].biasStart + Transforms[l].outputCount) return true;
		}
		return false;
	}
	int LoadWeightIdx(uint32_t w)
	{
		for (uint32_t l = 0; l < layerCount; l++) {
			if (w >= Transforms[l].weightStart && w < Transforms[l].weightStart + Transforms[l].WeightCount()) {
				int n = w - Transforms[l].weightStart;
				int x = n % Transforms[l].outputCount;
				int y = n / Transforms[l].outputCount;
				
				return Transforms[l].WeightIdx(y,x);
			}
		}
		return w;
	}

	T* GetLayerOutputValues(int layerIdx, T* Values )
	{
		if (layerIdx < 0) return &Values[0];
		return &Values[ Layers[layerIdx].outputValueStart ];
	}

	int LayerCount() { return layerCount; }
	int InputCount() { return inputCount; }
	int InputCount( int layerIdx ) 
	{ 
		if (layerIdx <= 0) return inputCount;
		return Layers[layerIdx-1].outputCount;
	}

	NetworkLayer* GetLayer(int layer) { return &Layers[layer]; }
	NetworkTransform<T>* GetTransform(int layer) { return &Transforms[layer]; }

	void SetInputCount(int inInputCount)
	{
		inputCount = inInputCount;
	}

	void AddLayer(int outputCount, eActivationType activationType, eLayoutType layoutType = LT_INPUTS_TO_OUTPUT )
	{
		// 3k4/R5p1/1K3p2/6r1/p7/8/8/8 w - - 8 1
		int32_t outputValueStart = (layerCount > 0) ? Layers[layerCount - 1].outputValueStart : 0;
		outputValueStart += InputCount(layerCount);

		// Enforce 64-byte alignment (16 * 2 bytes per value)
		if ((outputValueStart % 32)) outputValueStart += 32 - (outputValueStart % 32);
		assert(outputValueStart % 32 == 0);

		// Initialize the layer from the params
		Layers[layerCount].Init(outputCount, outputValueStart, activationType, layoutType );
		if (layoutType == LT_INPUT_TO_OUTPUTS) assert((outputCount % 16) == 0);
		layerCount++;
	}

	void Build()
	{
		// How many weights do we need
		weightCount = 0;
		int prevOutputCount = inputCount;
		for (uint32_t l = 0; l < layerCount; l++)
		{
			weightCount += Layers[l].outputCount * prevOutputCount + Layers[l].outputCount; // weights + biases
			prevOutputCount = Layers[l].outputCount;
		}

		// Free any allocated weights
		if (Weights) {
			delete[] Weights;
		}

		// Allocate the weights
		const uint32_t maxWeightCount = (weightCount + layerCount * 31);
		Weights = (T*)AlignedAllocUtil(maxWeightCount * sizeof(T), 64);
		memset(Weights, 0, maxWeightCount * sizeof(T) );

		// Initialize the layers
		int weightStart = 0;
		for (uint32_t l = 0; l < layerCount; l++)
		{
			int inputs = l > 0 ? Layers[l-1].outputCount : inputCount;
			Transforms[l].Init(inputs, Layers[l].outputCount, Weights, weightStart, Layers[l].activationType, Layers[l].layoutType );
			weightStart += Transforms[l].WeightCount();
			if ((weightStart % 16)) weightStart += 16 - (weightStart % 16); // enforce 32-byte alignment
		}
	}

public:
	static const int kMaxLayers = 4;
	int maxWeights = 0;

private:
	NetworkLayer Layers[kMaxLayers];
	NetworkTransform<T> Transforms[kMaxLayers];
	uint32_t layerCount = 0;
	uint32_t inputCount = 0;

	T *Weights = nullptr;
	int32_t weightCount = 0;
};


// needs to learn as float or double, but can convert to an int type for actual use
int InitializeNeuralNets();

struct NeuralNetBase
{
	virtual void InitNetwork() = 0;
	virtual bool IsActive(const struct SBoard& board) const = 0;
	virtual int GetNNEval(const SBoard& board, nnInt_t Values[]) = 0;
	virtual void ConvertToInputValues(const SBoard& board, nnInt_t InputValues[]) = 0;
	virtual void SetFilenames(std::string name);

	NeuralNetwork<nnInt_t> network;
	std::string neuralNetFile;
	std::string neuralNetFileBin;
	std::string trainingPositionFile;
	std::string trainingLabelFile;

	int whiteInputCount = 0;
	int blackInputCount = 0;
	bool isLoaded = false;
};

// Don't let value get too high
inline int SoftClamp(int value, const int softMax, const int hardMax)
{
	const int absValue = abs(value);
	if (absValue <= softMax) return value;

	const int adjustedAbsValue = softMax + ((absValue - softMax) >> 3);
	const int softClampedValue = (value > 0) ? adjustedAbsValue : -adjustedAbsValue;
	return ClampInt(softClampedValue, -hardMax, hardMax);
}