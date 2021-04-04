#pragma once

#include <thread>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#include "mathSimd.h"

enum class eActivation { RELU, NONE };
enum class eLayout { DENSE, SPARSE_INPUTS };

const int kFixedShift = 8;
const float kFixedFloatMult = 256.0f;
const int kFixedMax = (1 << 15) - 1;
const int kMaxEvalNetValues = 1280;
const int kMaxValuesInLayer = 256;

typedef int16_t nnInt_t;

template< typename T >
struct NetworkTransform
{
	uint32_t inputCount = 0;
	uint32_t outputCount = 0;

	T* Weights = nullptr;
	uint32_t weightStart = 0;
	uint32_t biasStart = 0;
	int32_t biases32[kMaxValuesInLayer]; // store biases as 32-bit int to memcpy in

	void Init(int inputs, int outputs, T* InWeights, int inWeightStart, eActivation inType, eLayout inLayoutType)
	{
		activationType = inType;
		layoutType = inLayoutType;
		inputCount = inputs;
		outputCount = outputs;
		Weights = InWeights;
		weightStart = inWeightStart;
		biasStart = inWeightStart + inputCount * outputCount;
	}

	void OnWeightsLoaded()
	{
		for (uint32_t o = 0; o < outputCount; o++) {
			biases32[o] = (int32_t)Weights[biasStart + o] << kFixedShift;
		}
	}
	
	// Transform input network to output network using weights and biases
	void Transform(T Inputs[], T Outputs[]) const
	{
		assert(Inputs && Outputs);
		const T* weightsForOutput = &Weights[weightStart];

		// Build outputs in int32s starting with bias
		alignas(64) int32_t OutputsTemp[kMaxValuesInLayer];
		memcpy(OutputsTemp, biases32, outputCount * sizeof(int32_t));

		// Sum the weighted inputs for this output
		const int32_t* end = &OutputsTemp[outputCount];
		for (int32_t* out = OutputsTemp; out < end; out++, weightsForOutput += inputCount)
		{
			*out += SIMD::dotProductInt16(weightsForOutput, Inputs, inputCount);
			//assert((*out >> kFixedShift) < kFixedMax); // note : simd 32->16 saturates so hopefully not big deal if a few values goes over
		}

		if (activationType == eActivation::RELU)
		{
			SIMD::convertVec32to16clamp0(OutputsTemp, Outputs, kFixedShift, outputCount);
		}
		else
		{
			for (uint32_t o = 0; o < outputCount; o++)
				Outputs[o] = ClampInt(OutputsTemp[0] >> kFixedShift, -kFixedMax, kFixedMax);
		}
	}

	void inline SetOutputsToBias(T Outputs[] ) const
	{
		memcpy(Outputs, &Weights[biasStart], sizeof(T) * outputCount);
	}

	// NOTE : Not currently using this function. It adds inputs the 1 inputs
	void TransformInputToOutput(T Inputs[], T Outputs[]) const
	{
		// Set the outputs to start at their biases.
		SetOutputsToBias(Outputs);

		// For each input that is set, add it to all the outputs
		for (uint32_t i = 0; i < inputCount; i++)
		{
			// If this is just 1s and 0s
			if (Inputs[i] > 0 ) {
				AddInput( i, Outputs);
			}
		}

		// If we need this, then have to use 32-bit intermediate 
		// SIMD::convertVec32to16(OutputsTemp, Outputs, kFixedShift, outputCount);

		TransformActivateOnly( Outputs );
	}

	void TransformActivateOnly(T Outputs[]) const
	{
		if (activationType == eActivation::RELU)
		{
			SIMD::clamp0Vec16(Outputs, outputCount); // don't let values go below 0
		}
	}

	inline void AddInput(int i, T Inputs[], T Outputs[]) const
	{
		assert(i >= 0 && i < (int)inputCount);
		Inputs[i] = 1; // Note : only doing 1s and 0s which is quicker, would have to refactor to support other value

		SIMD::addVec16(Outputs, &Weights[weightStart + i * outputCount], outputCount);
	}

	// For incremental updates of first layer values
	inline void AddInput(int i, T Outputs[]) const
	{
		assert(i >= 0 && i < (int)inputCount);
		SIMD::addVec16(Outputs, &Weights[weightStart + i * outputCount], outputCount);
	}

	inline void RemoveInput(int i, T Outputs[]) const
	{
		assert(i >= 0 && i < (int)inputCount);
		SIMD::subVec16( Outputs, &Weights[weightStart + i * outputCount], outputCount );
	}

	uint32_t Size() const { return WeightCount() * sizeof(T); }
	uint32_t WeightCount() const { return inputCount * outputCount + outputCount; }
	uint32_t InputCount() const { return inputCount; }
	uint32_t OutputCount() const  { return outputCount; }
	inline int WeightIdx(int input, int output) const
	{
		if ( layoutType == eLayout::DENSE)
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

		OnWeightsLoaded();
		return count == WeightCount();
	}

	eActivation activationType = eActivation::RELU;
	eLayout layoutType = eLayout::DENSE;
};

class NetworkLayer
{
public:
	void Init(int inOutputCount, int inValueStart, eActivation inActivationType, eLayout inLayoutType )
	{
		outputCount = inOutputCount;
		outputValueStart = inValueStart;
		activationType = inActivationType;
		layoutType = inLayoutType;
	}

	int outputCount = 0;
	eActivation activationType = eActivation::RELU;
	eLayout layoutType = eLayout::DENSE;

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

	void Sum(T InputLayer[], T FinalOutputs[]) const;
	void SumHiddenLayers(T Inputs[], T TempValues[], T FinalOutputs[]) const;

	void SetInputCount(int inInputCount) { inputCount = inInputCount; }
	void AddLayer(int outputCount, eActivation activationType = eActivation::NONE, eLayout layoutType = eLayout::DENSE);
	void Build();

	bool Load( const char* filename );
	bool Save( const char* filename );
	void WriteNet(FILE* fp);
	bool ReadNet(FILE* fp);

	// For loading from tensor flow
	bool LoadText( const char* filename );
	uint32_t LoadWeightsFromArray(const std::vector<float>& loadedWeights, uint32_t startIdx = 0);
	int LoadWeightIdx(uint32_t w) const;
	static bool LoadWeightsFromTextFile(const char* filename, std::vector<float>& loadedWeights);

	std::string PrintWeights() const;

	int Size() const
	{ 
		int size = 0;
		for (uint32_t i = 0; i < layerCount; i++) { size += Transforms[i].Size(); }
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
	void OnWeightsLoaded()
	{
		for (uint32_t i = 0; i < layerCount; i++) { Transforms[i].OnWeightsLoaded(); }
	}

	double GetWeight(int w) const { return Weights[w]; }

	bool IsBias(uint32_t w) const
	{
		for (uint32_t l = 0; l < layerCount; l++) {
			if (w >= Transforms[l].biasStart && w < Transforms[l].biasStart + Transforms[l].outputCount) return true;
		}
		return false;
	}

	inline T* GetLayerOutputValues(int layerIdx, T* Values ) const
	{
		if (layerIdx < 0) return &Values[0];
		return &Values[ Layers[layerIdx].outputValueStart ];
	}

	int LayerCount() const { return layerCount; }
	int InputCount() const { return inputCount; }
	int InputCount( int layerIdx ) const
	{ 
		if (layerIdx <= 0) return inputCount;
		return Layers[layerIdx-1].outputCount;
	}

	const NetworkLayer* GetLayer(int layer) const { return &Layers[layer]; }
	const NetworkTransform<T>* GetTransform(int layer) const { return &Transforms[layer]; }

private:
	static const int kMaxLayers = 4;

	NetworkLayer Layers[kMaxLayers];
	NetworkTransform<T> Transforms[kMaxLayers];
	uint32_t layerCount = 0;
	uint32_t inputCount = 0;

	T *Weights = nullptr;
	int32_t weightCount = 0;
};

struct NeuralNetBase
{
	virtual void InitNetwork() = 0;
	virtual bool IsActive(const struct Board& board) const = 0;
	virtual void ConvertToInputValues(const Board& board, nnInt_t InputValues[]) const = 0;

	virtual int ComputeFirstLayerValues(const Board& board, nnInt_t* Values, nnInt_t* firstLayerValues) const;
	virtual int GetSumIncremental(const nnInt_t firstLayerValues[], nnInt_t Values[]) const;
	virtual int GetSum(const Board& board, nnInt_t* Values) const;
	virtual int GetSum(nnInt_t* Values) const;

	virtual void SetFilenames(std::string name);
	virtual void LoadText();

	NeuralNetwork<nnInt_t> network;
	std::string neuralNetFile;
	std::string neuralNetFileBin;
	std::string trainingPositionFile;
	std::string trainingLabelFile;
	std::string structureFile;
	std::string baseName;

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