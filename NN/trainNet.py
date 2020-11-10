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
        dataFilename = netName + "Data.dat";
        labelFilename = netName + "Labels.dat"

        # TODO : structure file
        numInputs = (32 + 28) * 2 + 1
        epochsParam = 30 # good values for this do depend somewhat on number of training positions
        batchSizeParam = 15000
        # layers, neurons, epochs?
 
        # Load input data for each position
        print("Loading Positions : " + dataFilename )
        f = open(dataFilename, 'rb')
        chessData = np.fromfile(f, dtype=np.int8) 
        chessData = chessData.reshape(-1, numInputs)
        f.close()

        # Load target labels for each position
        print("Loading Labels : " + labelFilename)
        f = open(labelFilename, 'rb')
        chessLabels = np.fromfile(f, dtype=np.float32) 
        f.close()

        print ("positionLen:", len(chessData))
        print ("labelLen:", len(chessLabels))

        # Create the neural net model
        model = keras.Sequential([	
                keras.layers.Dense(192, activation="relu"),
                keras.layers.Dense(32, activation="relu"),
                keras.layers.Dense(32, activation="relu"),
                keras.layers.Dense(1, activation="sigmoid"), # use sigmoid for our 0-1 training labels
                ])

        lr_schedule = keras.optimizers.schedules.ExponentialDecay(initial_learning_rate=.01,decay_steps=5000,decay_rate=0.96)
        opt = keras.optimizers.Adam( learning_rate = lr_schedule  )

        #not sure what loss function should be or if should use sigmoid activation
        model.compile(optimizer=opt, loss="mean_squared_error")

        model.fit(chessData, chessLabels, batch_size= batchSizeParam, epochs= epochsParam )

        # print some predictions for a quick sanity check
        prediction = model.predict( chessData[0:50] )
        for x in range(0, 50):
                print( str(x+1) + " prediction = " + str(prediction[x]) + "  target = " + str(chessLabels[x]) )

        # Save the trained weights
        weightFilename =  netName + "Weights.txt";
        saveWeightsText( weightFilename )
        
        print ("Saved " + weightFilename)
        print ("------------------------------")



