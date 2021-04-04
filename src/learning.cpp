//
// learning.cpp
// by Jonathan Kreuzer
//
// Reads match pdn files from a directory (eg. CheckerBoard matches) and append thems to a training pdn file.
// Reads the training pdn files, converting to positions.
// Converts the positions to the neural net inputs and a target value, to be trained, eg. by TensorFlow
//
#include <fstream>

#include "NeuralNet/NeuralNet.h"
#include "engine.h"
#include "learning.h"

void NeuralNetLearner::WriteNetStructure(const char* filePath, const NeuralNetwork<nnInt_t>& network )
{
	FILE* fp = fopen( filePath, "wt");
	if (fp)
	{
		fprintf(fp, "%d\n", network.InputCount());
		fprintf(fp, "%d\n", network.LayerCount());
		for (int l = 0; l < network.LayerCount(); l++)
		{
			fprintf(fp, "%d\n", network.GetLayer(l)->outputCount );
		}

		fclose(fp);
	}
}

void NeuralNetLearner::WriteNetInputsBinary(FILE* fp, nnInt_t InputValues[], int32_t inputCount)
{
	const int kMaxBufferSize = 4096;
	assert(inputCount < kMaxBufferSize);

	// Build inputs into 1-byte per input buffer. They are just 0 or 1 right now, but would have to do conversion if that changes.
	uint8_t writeBuf[kMaxBufferSize];
	for (int32_t i = 0; i < inputCount; i++)
	{
		writeBuf[i] = (uint8_t)InputValues[i];
	}

	// Write the buffer to the file
	fwrite(&writeBuf, sizeof(uint8_t), inputCount, fp);
}

void NeuralNetLearner::ConvertGamesToPositions( std::vector<std::string> pdnFilenames, std::vector<TrainingPosition>& positionSet )
{
	Transcript transcript;
	char buffer[4192];
	char line[1024];
	for (auto& filename : pdnFilenames)
	{
		// Load the pdn file
		FILE* fp = fopen(filename.c_str(), "rt");
		if (fp)
		{
			while (!feof(fp))
			{
				// Read the next game
				buffer[0] = 0;
				while (fgets(line, sizeof(line), fp))
				{
					strcat(buffer, line);
					// Read until a line beginning with "*"
					if (strstr(line, "*")) {
						break;
					}
				}

				// Read game transcript from buffer
				float gameResult = 0.5f;
				transcript.FromString(buffer, &gameResult);

				// Replay the game exporting positions
				Board replayBoard = transcript.startBoard;
				int i = 0;
				while (transcript.moves[i].data != 0 && i < transcript.numMoves)
				{
					replayBoard.DoMove(transcript.moves[i]);
					i++;

					// skip positions with jumps possible, qsearch will search them and tough to eval
					if (replayBoard.Bitboards.GetJumpers(replayBoard.sideToMove)) { continue; }

					// endgame database should take care of these positions
					if (replayBoard.numPieces[WHITE] + replayBoard.numPieces[BLACK] <= 4) { continue; }

					// TODO : which positions should we export? 
					// every position probably not smart... at least don't export the even endgame ones moving pieces around
					positionSet.push_back(TrainingPosition(replayBoard, gameResult));
				}
			}
			fclose(fp);
		}
		
	}
}

void NeuralNetLearner::ExportTrainingSet(std::vector<TrainingPosition>& positionSet )
{
	// Export training data for each net, for all positions in the set the net is active for
	for (auto net : engine.evalNets )
	{
		WriteNetStructure( net->structureFile.c_str(), net->network);

		FILE* posFile = fopen(net->trainingPositionFile.c_str(), "wb");
		if (posFile)
		{
			nnInt_t inputValues[1024];
			const int inputCount = net->network.InputCount();

			for (auto& pos : positionSet)
			{
				if (net->IsActive(pos.board)) 
				{
					net->ConvertToInputValues(pos.board, inputValues);
					WriteNetInputsBinary(posFile, inputValues, inputCount);

					// Export flipped board to take advantage of symettry
					net->ConvertToInputValues( pos.board.Flip(), inputValues);
					WriteNetInputsBinary(posFile, inputValues, inputCount);
				}
			}

			fclose(posFile);
		}

		FILE* labelFile = fopen(net->trainingLabelFile.c_str(), "wb");
		if (labelFile)
		{
			for (auto& pos : positionSet)
			{
				if (net->IsActive(pos.board))
				{
					fwrite(&pos.targetVal, sizeof(float), 1, labelFile);
					
					float targetVal = 1.0f - pos.targetVal; // export flipped target value, to match flipped board
					fwrite(&targetVal, sizeof(float), 1, labelFile);
				}
			}

			fclose(labelFile);
		}
	}
}

void NeuralNetLearner::CreateTrainingSet()
{
	std::vector<TrainingPosition> positionSet;

	ConvertGamesToPositions(
		{/* "../Training/match1.pdn",*/
		  //"../Training/match2.pdn",
		  "../Training/match3.pdn",
		  "../Training/match4.pdn",
		  "../Training/match5.pdn",
		  "../Training/match6.pdn",
		  "../Training/match7.pdn",
		  "../Training/match8.pdn",
		  "../Training/match9.pdn",
		  "../Training/match10.pdn",
		  //"../Training/match11.pdn",
		},
		positionSet
	);
	ExportTrainingSet( positionSet );
}

int NeuralNetLearner::ImportLatestMatches(MatchResults& results)
{
	std::string matchDir = "C:/Users/Jon/Documents/Martin Fierz/CheckerBoard/games/matches/";
	std::vector<std::string> filenums = { "", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]", "[10]" };
	std::string outputFilename = "../Training/match11.pdn";
	
	int importedFileCount = 0;
	int wins = 0, losses = 0, draws = 0;

	std::ofstream outputFile(outputFilename, std::ios_base::app);
	if (!outputFile.good()) return 0;

	for (auto filenum : filenums)
	{
		std::string filepath = matchDir + "match" + filenum + ".pdn";
		std::ifstream file(filepath);
		if (file.good())
		{
			importedFileCount++;

			std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); // read the contents of file
			outputFile << content; 	// append contents to outputFile
			file.close(); // close the file
			remove(filepath.c_str()); // then delete the file
		}

		filepath = matchDir + "stats" + filenum + ".txt";
		std::ifstream statFile(filepath);
		if (statFile.good())
		{
			std::string content((std::istreambuf_iterator<char>(statFile)), std::istreambuf_iterator<char>());
			const char* wldStart = strstr(content.c_str(), "W-L-D:");
			int32_t win = 0, loss = 0, draw = 0;
			if (wldStart && sscanf(wldStart, "W-L-D:%d-%d-%d", &win, &loss, &draw))
			{
				results.wins += win;
				results.losses += loss;
				results.draws += draw;
			}

			statFile.close(); // close the file
			remove(filepath.c_str()); // then delete the file
		}
	}
	return importedFileCount;
}