//
// NeuralNet.cpp
// by Jonathan Kreuzer
//
// NeuralNetwork - Provides a generic neural net class for building and evaluating multi-layer neural networks.
// NeuralNetBase - Extendable Base class to easily add and use multiple neural networks. Contains a NeuralNetwork.
// NetworkLayer - Describes the structure of a single layer.
// NetworkTransform - consists of weights and biases for a layer, and can be transform the Input values to the Output values.

#include <string.h>
#include <sstream>  

#include "NeuralNet.h"

template class NeuralNetwork<nnInt_t>;

void NeuralNetBase::SetFilenames(std::string name)
{
	neuralNetFile = std::string("NN/") + name + std::string("Weights.txt");
	neuralNetFileBin = std::string("NN/") + name + std::string("Weights.bin");
	trainingPositionFile = std::string("NN/") + name + std::string("Data.dat");
	trainingLabelFile = std::string("NN/") + name + std::string("Labels.dat");
}

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
bool NeuralNetwork<T>::LoadText( const char* filename )
{
	FILE* fp = fopen(filename, "rb");

	if (fp)
	{
		std::vector<float> loadedWeights;
		char line[2048]; // how long is max line?
		while ( fgets(line, sizeof(line), fp) )
		{
			// TODO : size checking
			if (strstr(line, "Layer")) {
				continue;
			}
			int len = (int)strlen(line);
			for (int i = 0; i < len; i++) 
			{						
				bool isNumber = isdigit(line[i]) || (line[i] == '-' && isdigit(line[i + 1]));
				if (isNumber)
				{ 
					loadedWeights.push_back( ReadFloat(line, i) );
				}
			}
		}
		
		fclose(fp);

		// Copy the weights into the neural net
		uint32_t netWeightCount = WeightCount();
		if ( loadedWeights.size() == netWeightCount)
		{
			for (int i = 0; i < (int)loadedWeights.size(); i++)
			{
				if (IsBias(i))
				{
					Weights[i] = FloatToFixed(loadedWeights[i]);
				}
				else
				{
					// Note : Layer might be input -> # outputs instead of #inputs -> output
					Weights[LoadWeightIdx(i)] = FloatToFixed(loadedWeights[i]);
					// assert(abs(Weights[LoadWeightIdx(i)] < (10 *256) ));
				}
			}
			return true;
		}
	}
	return false;
}

// Use the network weights to compute on output value from the input values
template<typename T>
T NeuralNetwork<T>::Sum( T Values[] )
{
	// First layer inputs are probably sparsely set, so do it input->output
	if (Layers[0].layoutType == LT_INPUT_TO_OUTPUTS)
		Transforms[0].TransformInputToOutput(GetLayerOutputValues(-1, Values), GetLayerOutputValues(0, Values));
	else
		Transforms[0].Transform(GetLayerOutputValues(-1, Values), GetLayerOutputValues(0, Values));

	return SumHiddenLayers(Values);
}

template<typename T>
T NeuralNetwork<T>::SumHiddenLayers(T Values[])
{
	// Transform the output for the first layer through the hidden layers
	T* Outputs = GetLayerOutputValues(0, Values);
	T* Inputs = nullptr;
	for (uint32_t i = 1; i < layerCount; i++)
	{
		Inputs = Outputs;
		Outputs = GetLayerOutputValues(i, Values);
		Transforms[i].Transform(Inputs, Outputs);
	}

	// Returning one value (though we could do multiple outputs if we wanted to)
	return Outputs[0];
}

template<typename T>
std::string NeuralNetwork<T>::PrintWeights()
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
