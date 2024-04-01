#!/bin/bash
set -ex

if [ ! -d large_files ]
then
    mkdir large_files
fi

if [ ! -d build ]
then
    mkdir build
fi

cd build && cmake .. && make -j16 lifelong && cd ..

OUTPUT_FOLDER="offline_eval/"

# How many threads used to precompute heuristics. By default, it will use all.
# export OMP_NUM_THREADS=1
ARGS="--planTimeLimit 1 --fileStoragePath large_files/"

# random:random
./build/lifelong --inputFile example_problems/random.domain/random_100.json $ARGS -o ${OUTPUT_FOLDER}test_random_100.json --simulationTime 500
./build/lifelong --inputFile example_problems/random.domain/random_200.json $ARGS -o ${OUTPUT_FOLDER}test_random_200.json --simulationTime 500 
./build/lifelong --inputFile example_problems/random.domain/random_400.json $ARGS -o ${OUTPUT_FOLDER}test_random_400.json --simulationTime 1000 
./build/lifelong --inputFile example_problems/random.domain/random_600.json $ARGS -o ${OUTPUT_FOLDER}test_random_600.json --simulationTime 1000 
./build/lifelong --inputFile example_problems/random.domain/random_800.json $ARGS -o ${OUTPUT_FOLDER}test_random_800.json --simulationTime 2000 

# warehouse:warehouse_small
./build/lifelong --inputFile example_problems/warehouse.domain/warehouse_small_200.json $ARGS -o ${OUTPUT_FOLDER}test_warehouse_small_200.json --simulationTime 5000
./build/lifelong --inputFile example_problems/warehouse.domain/warehouse_small_400.json $ARGS -o ${OUTPUT_FOLDER}test_warehouse_small_400.json --simulationTime 5000 
./build/lifelong --inputFile example_problems/warehouse.domain/warehouse_small_600.json $ARGS -o ${OUTPUT_FOLDER}test_warehouse_small_600.json --simulationTime 5000 
./build/lifelong --inputFile example_problems/warehouse.domain/warehouse_small_800.json $ARGS -o ${OUTPUT_FOLDER}test_warehouse_small_800.json --simulationTime 5000 
./build/lifelong --inputFile example_problems/warehouse.domain/warehouse_small_1000.json $ARGS -o ${OUTPUT_FOLDER}test_warehouse_small_1000.json --simulationTime 5000 

# warehouse:warehouse_large
./build/lifelong --inputFile example_problems/warehouse.domain/warehouse_large_8000.json $ARGS -o ${OUTPUT_FOLDER}test_warehouse.json --simulationTime 5000 

# warehouse:sortation
./build/lifelong --inputFile example_problems/warehouse.domain/sortation_large_10000.json $ARGS -o ${OUTPUT_FOLDER}test_sortation.json --simulationTime 5000 

# game:brc202_d
./build/lifelong --inputFile example_problems/game.domain/brc202d_4000.json $ARGS  -o ${OUTPUT_FOLDER}test_brc202d.json --simulationTime 5000 

# city:paris
./build/lifelong --inputFile example_problems/city.domain/paris_1000.json $ARGS -o ${OUTPUT_FOLDER}test_paris_1000.json --simulationTime 2000 
./build/lifelong --inputFile example_problems/city.domain/paris_3000.json $ARGS -o ${OUTPUT_FOLDER}test_paris_3000.json --simulationTime 4000 