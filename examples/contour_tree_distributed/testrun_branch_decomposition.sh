#!/bin/sh

mkdir -p out
DATA_DIR=${DATA_DIR:-${HOME}/devel/parallel-peak-pruning/Data/2D}

if [ ! -d  $DATA_DIR ]; then
    echo "Error: Directory  $DATA_DIR does not exist!"
    exit 1;
fi;

echo
echo "Starting Timing Runs"
echo
echo "8x9 Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/8x9test.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/8x9test.txt 4
# ./hact_test_branch_decomposition.sh $DATA_DIR/8x9test.txt 8
echo
echo "Vancouver Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vanc.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vanc.txt 4
# ./hact_test_branch_decomposition.sh $DATA_DIR/vanc.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vanc.txt 16
echo
echo "Vancouver SWSW Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSW.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSW.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSW.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSW.txt 16
echo
echo "Vancouver SWNW Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNW.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNW.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNW.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNW.txt 16
echo
echo "Vancouver SWSE Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSE.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSE.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSE.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWSE.txt 16
echo
echo "Vancouver SWNE Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNE.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNE.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNE.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSWNE.txt 16
echo
echo "Vancouver NE Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNE.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNE.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNE.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNE.txt 16
echo
echo "Vancouver NW Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNW.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNW.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNW.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverNW.txt 16
echo
echo "Vancouver SE Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSE.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSE.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSE.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSE.txt 16
echo
echo "Vancouver SW Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSW.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSW.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSW.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/vancouverSW.txt 16
echo
echo "Icefields Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/icefield.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/icefield.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/icefield.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/icefield.txt 16
# ./hact_test_branch_decomposition.sh $DATA_DIR/icefield.txt 32
# ./hact_test_branch_decomposition.sh $DATA_DIR/icefield.txt 64
echo
echo "GTOPO30 Full Tiny Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/gtopo_full_tiny.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/gtopo_full_tiny.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/gtopo_full_tiny.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo_full_tiny.txt 16
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo_full_tiny.txt 32
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo_full_tiny.txt 64
echo
echo "GTOPO30 UK Tile Test Set"
./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 2
./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 4
./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 8
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 16
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 32
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 64
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 128
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 256
# ./hact_test_branch_decomposition.sh $DATA_DIR/gtopo30w020n40.txt 512
echo "Done"
