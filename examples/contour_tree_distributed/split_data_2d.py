#!/usr/bin/env python3
import numpy as np
import math
import os
import sys

# Read a 2D text file from disk into a NumPy array
def read_file(fn):
    data = np.fromfile(fn, dtype=np.int, sep=" ")
    data = data[2:].reshape(tuple(data[0:2]))
    return data

# Save a block from a 2D NumPy array to disk
def save_piece(fn, array, offset, n_blocks, block_index, size):
    with open(fn, 'w') as f:
        f.write('#GLOBAL_EXTENTS ' + ' '.join(map(str, array.shape)) + '\n')
        f.write('#OFFSET ' + ' '.join(map(str, offset))+'\n')
        f.write('#BLOCKS_PER_DIM ' + ' '.join(map(str, n_blocks))+'\n')
        f.write('#BLOCK_INDEX ' + ' '.join(map(str, block_index))+'\n')
        f.write(' '.join(map(str, size)) + '\n')
        np.savetxt(f, array[offset[0]:offset[0]+size[0],offset[1]:offset[1]+size[1]], fmt='%.16g')

# Compute split points for splitting into n blocks
def split_points(shape, nblocks):
    dx = float(shape-1) / nblocks
    return [ math.floor(i*dx) for i in range(nblocks)] + [ shape - 1 ]

if len(sys.argv) < 2:
    print("Error: Usage split_data_2d.py <filename> [<n_blocks_per_axis>|<n_blocks_x> <n_blocks_y>]", file=sys.stderr)
    sys.exit(1)

# Parse parameters
in_filename = sys.argv[1]
n_blocks = (2, 2)
if len(sys.argv) > 2:
    if len(sys.argv) >= 4:
        n_blocks = (int(sys.argv[2]), int(sys.argv[3]))
    else:
        n_blocks = (int(sys.argv[2]), int(sys.argv[2]))

name, ext = os.path.splitext(in_filename)
out_filename_pattern = name + '_part_%d_of_' + str(n_blocks[0]*n_blocks[1]) + ext

# Read data
data = read_file(in_filename)

# Compute split points
split_points_x = split_points(data.shape[0], n_blocks[0])
split_points_y = split_points(data.shape[1], n_blocks[1])

# Save blocks
block_no = 0
for block_index_x, (x_start, x_stop) in enumerate(zip(split_points_x, split_points_x[1:])):
    for block_index_y, (y_start, y_stop) in enumerate(zip(split_points_y, split_points_y[1:])):
        n_x = x_stop - x_start + 1
        n_y = y_stop - y_start + 1
        save_piece(out_filename_pattern % block_no, data, (x_start, y_start), n_blocks, (block_index_x, block_index_y), (n_x, n_y))
#         print("Wrote block %d, origin %d %d, size %d %d" % (block_no, x_start, y_start, n_x, n_y))
        block_no += 1
