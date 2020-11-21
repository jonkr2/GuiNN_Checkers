# Train evaluation nets for GuiNN Checkers
# example usage : python trainNet.py CheckersEarly CheckersMid CheckersEnd CheckersLateEnd
#
# TODO : generalize to read a file that contains inputs & net structure

from pprint import pprint
import tensorflow as tf
from tensorflow import keras
import numpy as np
import sys

def saveWeightsText( fileName ):
	# save the weights as a text file
	np.set_printoptions(suppress=True)
	np.set_printoptions(threshold=np.inf)
	f = open(fileName, "w+") 
	layerIdx = 0;
	for layer in model.layers:
		weights = layer.get_weights()
		f.write("\n\n========Layer " + str(layerIdx) + "========\n")

		for wArray in weights:
			f.write(np.array2string(wArray))

		layerIdx = layerIdx + 1
	f.close()

# compute all the neural nets requested
for argNum in range(1, len(sys.argv) ):

        netName = sys.argv[argNum];
        dataFilename = netName + "Data.dat"
        labelFilename = netName + "Labels.dat"
        structureFilename = netName + "Struct.txt"

        # good values for these depend somewhat on number of training positions
        epochsParam = 30 
        batchSizeParam = 15000

        # Load Net Structure
        netStructure = np.loadtxt( structureFilename );
        numInputs = int(netStructure[0])
        numLayers = int(netStructure[1])
        layerSizes = netStructure[2:6]
        print("numInputs : " + str(numInputs) + " layers : " + str(numLayers) )
        print(layerSizes)

        # Load input data for each position
        print("Loading Positions : " + dataFilename )
        f = open(dataFilename, 'rb')
        positionData = np.fromfile(f, dtype=np.int8) 
        positionData = positionData.reshape(-1, numInputs)
        f.close()

        # Load target labels for each position
        print("Loading Labels : " + labelFilename)
        f = open(labelFilename, 'rb')
        positionLabels = np.fromfile(f, dtype=np.float32) 
        f.close()

        print ("dataLen:", format(len(positionData), ",") )
        print ("labelLen:", format(len(positionLabels), ",") )

        # Create the neural net model
        model = keras.Sequential([	
                keras.layers.Dense(layerSizes[0], activation="relu"),
                keras.layers.Dense(layerSizes[1], activation="relu"),
                keras.layers.Dense(layerSizes[2], activation="relu"),
                keras.layers.Dense(layerSizes[3], activation="sigmoid"), # use sigmoid for our 0-1 training labels
                ])

        lr_schedule = keras.optimizers.schedules.ExponentialDecay(initial_learning_rate=.01,decay_steps=5000,decay_rate=0.96)
        opt = keras.optimizers.Adam( learning_rate = lr_schedule  )

        #not sure what loss function should be or if should use sigmoid activation
        model.compile(optimizer=opt, loss="mean_squared_error")

        model.fit(positionData, positionLabels, batch_size= batchSizeParam, epochs= epochsParam )

        # print some predictions for a quick sanity check
        prediction = model.predict( positionData[0:50] )
        for x in range(0, 50):
                print( str(x+1) + " prediction = " + str(prediction[x]) + "  target = " + str(positionLabels[x]) )

        # Save the trained weights
        weightFilename =  netName + "Weights.txt";
        saveWeightsText( weightFilename )
        
        print ("Saved " + weightFilename)
        print ("------------------------------")



