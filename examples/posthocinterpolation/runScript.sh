#!/bin/bash

NUM_NODES="1"
INTERVAL="" #write frequency
INPUT="" #path to input basis flows
FILENAME="basisflows"
START=$INTERVAL
END="" #end time
OUTPUT="output"
NUMSEEDS="" #number of seeds
XMIN="" #data set bounds
XMAX=""
YMIN=""
YMAX=""
ZMIN=""
ZMAX=""
INPUT_SEEDS="1" #use input seed file (1) or generate random seeds in the domain (0)
SEED_FILE="" #path to seed file X Y Z T0 TN  .. T0 -- seed start time TN -- seed end time. Implementation not available yet


rm -rf output
mkdir output
./PostHocInterpolation $INPUT $FILENAME $START $END $INTERVAL $OUTPUT $NUMSEEDS $XMIN $XMAX $YMIN $YMAX $ZMIN $ZMAX $INPUT_SEEDS $SEED_FILE $NUM_NODES
