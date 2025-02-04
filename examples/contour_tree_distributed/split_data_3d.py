#!/usr/bin/env python3

##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

import numpy as np
import math
import os
import sys
# For readBOV
from functools import reduce
import operator
try:
    import h5py
    USE_HDF = True
except:
    USE_HDF = False

def read_file(fn):
    """
    Read a 3D plain text file from disk into a NumPy array
    """
    data = np.fromfile(fn, dtype=float, sep=" ")
    data = data[3:].reshape((int(data[2]),int(data[0]),int(data[1])))
    return data

def readBOV(filename):
    """
    Read data from a VisIt BOV file
    """
    with open(filename, 'r') as f:
        header = dict([(lambda x: (x[0].strip().lower(), x[1].strip()))(l.strip().split(':')) for l in f.readlines()])
        if 'data_endian' in header:
            if header['data_endian'].lower() != sys.byteorder:
                print('Unsopported endianess ' + eader['data_endian'].lower())
                return None
        shape = tuple([int(x) for x in header['data_size'].split(' ')])
        count = reduce(operator.mul, shape, 1)
        dtype_map = { 'float': 'float32', 'double': 'float64', 'char': 'uint8' }
        dtype = np.dtype(dtype_map[header['data_format'].lower()])
        dataname = os.path.realpath(os.path.join(os.path.dirname(filename), header['data_file']))
        if 'variable' not in header:
            header['variable'] = 'val'
        return (header['variable'], header['centering'].lower(), np.fromfile(dataname, dtype, count).reshape(tuple(reversed(shape))))
    return None

def save_piece(fn, array, offset, n_blocks, block_index, size):
    """
    Save a block from a 3D NumPy array to disk.

    Python order is slice, row, col
    TXT file order is row, col, slice
    offset and size are in file order

    Args:
        fn (str): filename
        array (np.array) : Array with the full data
        offset (tuple) : Tuple of int offsets
        n_blocks (tuple) : Tuple of ints with the number of blocks per dimension
        block_index (tuple) : Tuple of ints with index of the block
        size (tuple) : Tuple of ints with the size of the block in each dimension
    """
    with open(fn, 'w') as f:
        perm = [1, 2, 0]
        f.write('#GLOBAL_EXTENTS ' + ' '.join(map(str, [array.shape[i] for i in perm])) + '\n')
        f.write('#OFFSET ' + ' '.join(map(str, offset))+'\n')
        f.write('#BLOCKS_PER_DIM ' + ' '.join(map(str, n_blocks))+'\n')
        f.write('#BLOCK_INDEX ' + ' '.join(map(str, block_index))+'\n')
        f.write(' '.join(map(str, size)) + '\n')
        if fn[-5:]=='.bdem':
            array[offset[2]:offset[2]+size[2],offset[0]:offset[0]+size[0],offset[1]:offset[1]+size[1]].astype(np.double).tofile(f)
        else:
            for s in range(offset[2], offset[2]+size[2]):
                np.savetxt(f, array[s, offset[0]:offset[0]+size[0],offset[1]:offset[1]+size[1]], fmt='%.16g')
                f.write('\n')

def split_points(shape, nblocks):
    """
    Compute split points for splitting into n blocks:

    Args:
        shape (int): Length of the axis
        nblocks (int): Number of blocks to split the axis into

    Return:
        List of split points along the axis
    """
    dx = float(shape-1) / nblocks
    return [ math.floor(i*dx) for i in range(nblocks)] + [ shape - 1 ]

def save_hdf(filename, data, **kwargs):
    """
    Save the data to HDF5.
    The axes of the data will be transposed and reorded to match the order of save_piece function.

    Args:
        filename (str) : Name fo the HDF5 file
        data (np.array): 3D array with the data
        kwargs (dict) : Dict with keyword arguments for the h5py create_dataset function
    """
    f = h5py.File(filename, 'w')
    f.create_dataset(name='data', data=np.swapaxes(np.transpose(data), 0, 1), **kwargs)

if __name__ == '__main__':

    if len(sys.argv) < 2:
        print("Error: Usage split_data_3d.py <filename> <outfilepattern> [<n_blocks_per_axis>|<n_blocks_x> <n_blocks_y> <n_blocks_z>]", file=sys.stderr)
        sys.exit(1)

    # Parse parameters
    in_filename = sys.argv[1]

    name, ext = os.path.splitext(in_filename)
    #out_filename_pattern = name + '_split_%d.txt'
    out_filename_pattern = sys.argv[2]

    n_blocks = (2, 2, 2)
    if len(sys.argv) > 3:
        if len(sys.argv) >= 6:
            n_blocks = (int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5]))
        else:
            n_blocks = (int(sys.argv[3]), int(sys.argv[3]), int(sys.argv[3]))

    # Read data
    if ext == '.bov':
        data = readBOV(in_filename)[2]
    else:
        data = read_file(in_filename)

    # export to hdf5 as well
    if USE_HDF:
        save_hdf((out_filename_pattern % 0).replace('.txt', '.h5'), data)

    # Python order is slice, row, col
    # Compute split points
    split_points_s = split_points(data.shape[0], n_blocks[2])
    split_points_r = split_points(data.shape[1], n_blocks[0])
    split_points_c = split_points(data.shape[2], n_blocks[1])

    # Create the file that records the slice values
    slice_filename = name + '_slices.txt'

    # Save blocks
    block_no = 0
    for block_index_s, (s_start, s_stop) in enumerate(zip(split_points_s, split_points_s[1:])):
        for block_index_r, (r_start, r_stop) in enumerate(zip(split_points_r, split_points_r[1:])):
            for block_index_c, (c_start, c_stop) in enumerate(zip(split_points_c, split_points_c[1:])):
                n_s = s_stop - s_start + 1
                n_r = r_stop - r_start + 1
                n_c = c_stop - c_start + 1
                save_piece(out_filename_pattern % block_no, data, (r_start, c_start, s_start), n_blocks, (block_index_r, block_index_c, block_index_s), (n_r, n_c, n_s))
                block_no += 1
