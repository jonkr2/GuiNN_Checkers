//
// NeuralNet.cpp
// by Jonathan Kreuzer
//
// NeuralNetwork - Provides a generic neural net class for building and evaluating multi-layer neural networks.
// NeuralNetBase - Extendable Base class to easily add and use multiple neural networks. Contains a NeuralNetwork.
// NetworkLayer - Describes the structure of a single layer.
// NetworkTransform - consists of weights and biases for a layer, used to transform the Input values to the Output values.

#include <string.h>
#include <sstream>  

#include "NeuralNet.h"

template class NeuralNetwork<nnInt_t>;

std::string neuralNetDir("NN/");

// -------------------
// NeuralNetBase
// -------------------
void NeuralNetBase::SetFilenames(std::string name)
{
	baseName = name;
	
	neuralNetFile = neuralNetDir + name + "Weights.txt";
	neuralNetFileBin = neuralNetDir + name + "Weights.bin";
	trainingPositionFile = neuralNetDir + name + "Data.dat";
	trainingLabelFile = neuralNetDir + name + "Labels.dat";
	structureFile = neuralNetDir + name + "Struct.txt";
}

void NeuralNetBase::LoadText()
{
	isLoaded = network.LoadText(neuralNetFile.c_str());
}

int NeuralNetBase::ComputeFirstLayerValues(const Board& board, nnInt_t* Values, nnInt_t* firstLayerValues) const
{
	ConvertToInputValues(board, Values); // Note : this also sets first-layer values for each input

	// Save the converted board values into first layer values for later
	const NetworkTransform<nnInt_t>* transform = network.GetTransform(0);
	nnInt_t* Outputs = network.GetLayerOutputValues(0, Values);
	memcpy(firstLayerValues, Outputs, sizeof(nnInt_t) * transform->outputCount);

	return transform->outputCount;
}

// param firstLayerValues = the non-activated output values from the first layer
int NeuralNetBase::GetSumIncremental(const nnInt_t firstLayerValues[], nnInt_t Values[]) const
{
	nnInt_t* Outputs = network.GetLayerOutputValues(0, Values);
	memcpy(Outputs, firstLayerValues, sizeof(nnInt_t) * network.GetLayer(0)->outputCount);

	return GetSum(Values);
}

// param board = board to convert to inputs. 
// Note : Values are passed to allow safe multi-threading
int NeuralNetBase::GetSum(const Board& board, nnInt_t* Values) const
{
	ConvertToInputValues(board, Values); // Note : this also sets first-layer values for each input

	return GetSum(Values);
}

int NeuralNetBase::GetSum(nnInt_t* Inputs) const
{
	// Activation function on first layer
	Inputs = network.GetLayerOutputValues(0, Inputs);
	network.GetTransform(0)->TransformActivateOnly(Inputs);

	// Sum all the hidden layers
	alignas(64) nnInt_t TempValues[kMaxEvalNetValues];
	nnInt_t FinalOutput;
	network.SumHiddenLayers(Inputs,TempValues,&FinalOutput);
	return FinalOutput;
}

// -------------------
// NeuralNetwork
// -------------------
template<typename T>
bool NeuralNetwork<T>::Save( const char *filename )
{
	FILE* fp = fopen(filename, "wb");
	if (fp)
	{
		WriteNet(fp);
		fclose(fp);

		return true;
	}
	return false;
}

template<typename T>
bool NeuralNetwork<T>::Load( const char* filename )
{
	std::string filepath = "NN/";
	filepath += filename;
	FILE* fp = fopen(filepath.c_str(), "rb");
	if (fp)
	{
		ReadNet(fp);
		return true;
	}
	return false;
}

template<typename T>
void NeuralNetwork<T>::WriteNet(FILE* fp)
{
	// Write some structure info
	fwrite(&layerCount, sizeof(layerCount), 1, fp);
	fwrite(&inputCount, sizeof(inputCount), 1, fp);

	// Save the weights from each layer
	for (uint32_t i = 0; i < layerCount; i++)
		Transforms[i].Save(fp);
}

template<typename T>
bool NeuralNetwork<T>::ReadNet(FILE* fp)
{
	// Make sure the structure matches
	size_t bytesRead = 0;
	uint32_t testLayerCount = 0, testInputCount = 0;
	bytesRead += fread(&testLayerCount, sizeof(testLayerCount), 1, fp);
	bytesRead += fread(&testInputCount, sizeof(testInputCount), 1, fp);

	if (testLayerCount != layerCount || testInputCount != inputCount) return false;

	// Load the layers
	for (uint32_t i = 0; i < layerCount; i++)
		if (!Transforms[i].Load(fp)) return false;

	return true;
}

float ReadFloat(char line[], int& i)
{
	const char* numberStr = &line[i];

	// advance i to next space
	while (line[i] != ' ' && line[i] != '[' && line[i] != ']' && line[i] != 0 && line[i] != '\n') i++;

	// convert numberStr to float
	return (float)atof(numberStr);
}

int32_t FloatToFixed(float v)
{
	return (int32_t)ClampInt((int32_t)(v * kFixedFloatMult), -kFixedMax, kFixedMax);
}

template<typename T>
int NeuralNetwork<T>::LoadWeightIdx(uint32_t w) const
{
	for (uint32_t l = 0; l < layerCount; l++) {
		if (w >= Transforms[l].weightStart && w < Transforms[l].weightStart + Transforms[l].WeightCount()) {
			int n = w - Transforms[l].weightStart;
			int x = n % Transforms[l].outputCount;
			int y = n / Transforms[l].outputCount;

			return Transforms[l].WeightIdx(y, x);
		}
	}
	return w;
}

template<typename T>
bool NeuralNetwork<T>::LoadWeightsFromTextFile(const char* filename, std::vector<float>& loadedWeights )
{
	FILE* fp = fopen(filename, "rb");
	if (fp)
	{
		char line[2048];
		int layerWeightCount = 0;
		while (fgets(line, sizeof(line), fp))
		{
			if (strstr(line, "Layer")) {
				int backstreet = 0;
				layerWeightCount = 0;
				continue;
			}
			int len = (int)strlen(line);
			for (int i = 0; i < len; i++)
			{
				bool isNumber = isdigit(line[i]) || (line[i] == '-' && isdigit(line[i + 1]));
				if (isNumber)
				{
					loadedWeights.push_back(ReadFloat(line, i));
					layerWeightCount++;
				}
			}
		}

		fclose(fp);
		return true;
	}
	return false;
}

template<typename T>
bool NeuralNetwork<T>::LoadText( const char* filename )
{
	std::vector<float> loadedWeights;
	if ( LoadWeightsFromTextFile(filename, loadedWeights) )
	{
		return LoadWeightsFromArray(loadedWeights, 0) > 0;
	}
	return false;
}

template<typename T>
uint32_t NeuralNetwork<T>::LoadWeightsFromArray( const std::vector<float>& loadedWeights, uint32_t startIdx )
{
	// Copy the weights into the neural net
	const uint32_t netWeightCount = WeightCount();
	if (startIdx + netWeightCount <= loadedWeights.size())
	{
		for (uint32_t i = 0; i < netWeightCount; i++)
		{
			if (IsBias(i))
			{
				Weights[i] = FloatToFixed(loadedWeights[startIdx+i]);
			}
			else
			{
				// Note : Layer might be input -> # outputs instead of #inputs -> output
				Weights[LoadWeightIdx(i)] = FloatToFixed(loadedWeights[startIdx+i]);
				// assert(abs(Weights[LoadWeightIdx(i)] < (10 *256) ));
			}
		}
		OnWeightsLoaded();
		return netWeightCount;
	}
	return 0;
}

// Use the network weights to compute on output value from the input values
template<typename T>
void NeuralNetwork<T>::Sum( T Inputs[], T FinalOutputs[] ) const
{
	T alignas(64) TempValues[kMaxEvalNetValues];

	// First layer inputs are probably sparsely set, so do it input->output
	T* Outputs = (layerCount>1) ? GetLayerOutputValues(0, TempValues) : FinalOutputs;
	if (Layers[0].layoutType == eLayout::SPARSE_INPUTS)
		Transforms[0].TransformInputToOutput(Inputs, Outputs);
	else
		Transforms[0].Transform(Inputs, Outputs);

	SumHiddenLayers( Outputs, TempValues, FinalOutputs );
}

template<typename T>
void NeuralNetwork<T>::SumHiddenLayers(T Inputs[], T TempValues[], T FinalOutputs[]) const
{
	// Transform the output for the first layer through the hidden layers
	for (uint32_t i = 1; i < layerCount; i++)
	{
		T* Outputs = (i < layerCount-1) ? GetLayerOutputValues(i, TempValues) : FinalOutputs;
		Transforms[i].Transform(Inputs, Outputs);
		Inputs = Outputs;
	}
}

template<typename T>
void NeuralNetwork<T>::AddLayer(int outputCount, eActivation activationType, eLayout layoutType)
{
	int32_t outputValueStart = (layerCount > 0) ? Layers[layerCount - 1].outputValueStart : 0;
	outputValueStart += InputCount(layerCount);

	// Enforce 64-byte alignment (16 * 2 bytes per value)
	if ((outputValueStart % 32)) outputValueStart += 32 - (outputValueStart % 32);
	assert(outputValueStart % 32 == 0);
	assert(outputCount <= kMaxValuesInLayer);

	// Initialize the layer from the params
	Layers[layerCount].Init(outputCount, outputValueStart, activationType, layoutType);
	if (layoutType == eLayout::SPARSE_INPUTS) assert((outputCount % 16) == 0);
	layerCount++;
}

template<typename T>
void NeuralNetwork<T>::Build()
{
	// How many weights do we need
	weightCount = 0;
	int prevOutputCount = inputCount;
	for (uint32_t l = 0; l < layerCount; l++)
	{
		weightCount += Layers[l].outputCount * prevOutputCount + Layers[l].outputCount; // weights(dense) + biases
		prevOutputCount = Layers[l].outputCount;
	}

	// Free any allocated weights
	if (Weights) {
		delete[] Weights;
	}

	// Allocate the weights
	const uint32_t maxWeightCount = (weightCount + layerCount * 31);
	Weights = AlignedAllocUtil<T>(maxWeightCount, 64);
	memset(Weights, 0, maxWeightCount * sizeof(T));

	// Initialize the layers
	int weightStart = 0;
	for (uint32_t l = 0; l < layerCount; l++)
	{
		int inputs = l > 0 ? Layers[l - 1].outputCount : inputCount;
		Transforms[l].Init(inputs, Layers[l].outputCount, Weights, weightStart, Layers[l].activationType, Layers[l].layoutType);
		weightStart += Transforms[l].WeightCount();
		if ((weightStart % 16)) weightStart += 16 - (weightStart % 16); // enforce 32-byte alignment
	}

	assert(ValueMax() < kMaxEvalNetValues);
}

template<typename T>
std::string NeuralNetwork<T>::PrintWeights() const
{
	std::stringstream ss;
	ss << "layerCount : " << layerCount << "  weightCount : " << WeightCount() << "  size : " << Size() << "\r\n\r\n";
	for (uint32_t l = 0; l < layerCount; l++)
	{
		ss << "Transform " << l+1 << "  ( " << InputCount(l) << " inputs " << InputCount(l+1) << " outputs )\r\n";
		ss << "(weights)\r\n";
		for (uint32_t i = 0; i < Transforms[l].inputCount; i++)
		{
			ss << "[";
			for (uint32_t o = 0; o < Transforms[l].outputCount; o++)
			{
				ss << Weights[ Transforms[l].WeightIdx(i, o) ];
				if ( o < Transforms[l].outputCount -1 ) ss << ", ";
			}
			ss << "]\r\n";
		}
		// Print biases
		ss << "(biases)\r\n[";
		for (uint32_t o = 0; o < Transforms[l].outputCount; o++)
		{
			ss << Weights[Transforms[l].biasStart + o];
			if ( o < Transforms[l].outputCount - 1 ) ss << ", ";
		}
		ss << "]";

		ss << "\r\n\r\n";
	}
	return ss.str();
}
