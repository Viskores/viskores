//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/cont/testing/Testing.h>

#include <string>

namespace {

const char polydataAscii[] =
"# vtk DataFile Version 3.0\n"
"Cube example\n"
"ASCII\n"
"DATASET POLYDATA\n"
"POINTS 8 float\n"
"0.0 0.0 0.0    1.0 0.0 0.0    1.0 1.0 0.0    0.0 1.0 0.0    0.0 0.0 1.0\n"
"1.0 0.0 1.0    1.0 1.0 1.0    0.0 1.0 1.0\n"
"POLYGONS 6 30\n"
"4 0 1 2 3    4 4 5 6 7    4 0 1 5 4    4 2 3 7 6    4 0 4 7 3    4 1 2 6 5\n"
"CELL_DATA 6\n"
"SCALARS cell_scalars int 1\n"
"LOOKUP_TABLE default\n"
"0  1  2  3  4  5\n"
"NORMALS cell_normals float\n"
"0 0 -1    0 0 1    0 -1 0    0 1 0    -1 0 0    1 0 0\n"
"FIELD FieldData 2\n"
"cellIds 1 6 int\n"
"0  1  2  3  4  5\n"
"faceAttributes 2 6 float\n"
"0.0  1.0  1.0  2.0  2.0  3.0  3.0  4.0  4.0  5.0  5.0  6.0\n"
"POINT_DATA 8\n"
"SCALARS sample_scalars float 1\n"
"LOOKUP_TABLE my_table\n"
"0.0  1.0  2.0  3.0  4.0  5.0  6.0  7.0\n"
"LOOKUP_TABLE my_table 8\n"
"0.0 0.0 0.0 1.0    1.0 0.0 0.0 1.0    0.0 1.0 0.0 1.0    1.0 1.0 0.0 1.0\n"
"0.0 0.0 1.0 1.0    1.0 0.0 1.0 1.0    0.0 1.0 1.0 1.0    1.0 1.0 1.0 1.0\n";

const char polydataBin[] =
"# vtk DataFile Version 4.0\n"
"Cube example\n"
"BINARY\n"
"DATASET POLYDATA\n"
"POINTS 8 float\n"
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\n"
"POLYGONS 6 30\n"
"\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03"
"\x00\x00\x00\x04\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x07"
"\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x05\x00\x00\x00\x04"
"\x00\x00\x00\x04\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x07\x00\x00\x00\x06"
"\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x07\x00\x00\x00\x03"
"\x00\x00\x00\x04\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x06\x00\x00\x00\x05"
"\n"
"CELL_DATA 6\n"
"SCALARS cell_scalars int\n"
"LOOKUP_TABLE default\n"
"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04"
"\x00\x00\x00\x05\n"
"NORMALS cell_normals float\n"
"\x00\x00\x00\x00\x00\x00\x00\x00\xbf\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\xbf\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\xbf\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\n"
"FIELD FieldData 2\n"
"cellIds 1 6 int\n"
"\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x04"
"\x00\x00\x00\x05\n"
"faceAttributes 2 6 float\n"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00\x40\x00\x00\x00"
"\x40\x40\x00\x00\x40\x40\x00\x00\x40\x80\x00\x00\x40\x80\x00\x00\x40\xa0\x00\x00"
"\x40\xa0\x00\x00\x40\xc0\x00\x00\n"
"POINT_DATA 8\n"
"SCALARS sample_scalars float\n"
"LOOKUP_TABLE lookup_table\n"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00\x40\x40\x00\x00\x40\x80\x00\x00"
"\x40\xa0\x00\x00\x40\xc0\x00\x00\x40\xe0\x00\x00\n"
"LOOKUP_TABLE lookup_table 8\n"
"\x00\x00\x00\xff\xff\x00\x00\xff\x00\xff\x00\xff\xff\xff\x00\xff\x00\x00\xff\xff"
"\xff\x00\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\n";

const char structuredPointsAscii[] =
"# vtk DataFile Version 3.0\n"
"Volume example\n"
"ASCII\n"
"DATASET STRUCTURED_POINTS\n"
"DIMENSIONS 3 4 6\n"
"SPACING 1 1 1\n"
"ORIGIN 0 0 0\n"
"POINT_DATA 72\n"
"SCALARS volume_scalars char 1\n"
"LOOKUP_TABLE default\n"
"  0   0   0   0   0   0   0   0   0   0   0   0\n"
"  0   5  10  15  20  25  25  20  15  10   5   0\n"
"  0  10  20  30  40  50  50  40  30  20  10   0\n"
"  0  10  20  30  40  50  50  40  30  20  10   0\n"
"  0   5  10  15  20  25  25  20  15  10   5   0\n"
"  0   0   0   0   0   0   0   0   0   0   0   0\n";

const char structuredPointsVisItAscii[] =
"# vtk DataFile Version 3.0\n"
"Volume example\n"
"ASCII\n"
"DATASET STRUCTURED_POINTS\n"
"FIELD FieldData 3\n"
"Nek_SpectralElementData 1 4 int\n"
"8 8 8 68826 \n"
"avtOriginalBounds 1 6 double\n"
"-2 2 -2 2 -2 2 \n"
"FakeData 2 4 int\n"
"81 80 89 68826 \n"
"-81 80 65 6226 \n"
"SPACING 1 1 1\n"
"ORIGIN 0 0 0\n"
"CELL_DATA 27\n"
"VECTORS grad float\n"
"-1 -1 0 0 0 -1 0 0 -1 \n"
"1 1 0 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n"
"0 0 -1 0 0 -1 0 0 -1 \n";

const char structuredPointsBin[] =
"# vtk DataFile Version 4.0\n"
"Volume example\n"
"BINARY\n"
"DATASET STRUCTURED_POINTS\n"
"DIMENSIONS 3 4 6\n"
"SPACING 1 1 1\n"
"ORIGIN 0 0 0\n"
"POINT_DATA 72\n"
"SCALARS volume_scalars char\n"
"LOOKUP_TABLE default\n"
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x05\x0a\x0f\x14\x19\x19\x14"
"\x0f\x0a\x05\x00\x00\x0a\x14\x1e\x28\x32\x32\x28\x1e\x14\x0a\x00\x00\x0a\x14\x1e"
"\x28\x32\x32\x28\x1e\x14\x0a\x00\x00\x05\x0a\x0f\x14\x19\x19\x14\x0f\x0a\x05\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\n";

const char unsturctureGridAscii[] =
"# vtk DataFile Version 3.0\n"
"Unstructured Grid Example\n"
"ASCII\n"
"DATASET UNSTRUCTURED_GRID\n"
"POINTS 26 float\n"
"0 0 0   1 0 0   2 0 0    0 1 0   1 1 0   2 1 0        0 0 1   1 0 1   2 0 1\n"
"0 1 1   1 1 1   2 1 1    0.5 1.7 0.5   1.5 1.7 0.5    0 1 2   1 1 2   2 1 2\n"
"0 1 3   1 1 3   2 1 3    0 1 4   1 1 4   2 1 4        0 1 5   1 1 5   2 1 5\n"
"CELLS 12 64\n"
"8 0 1 4 3 6 7 10 9   6 1 7 2 4 10 5   6 2 7 8 5 10 11       5 4 3 9 10 12\n"
"4 5 4 10 13          4 5 10 11 13     6 17 14 18 15 19 16   4 21 22 19 18\n"
"3 20 21 17           3 21 18 17       2 25 24               1 23\n"
"CELL_TYPES 12\n"
"12  13  13  14  10  10  6  9  5  5  3  1\n"
"POINT_DATA 26\n"
"SCALARS scalars float 1\n"
"LOOKUP_TABLE default\n"
"0   1   2   3   4   5   6   7   8   9   10  11  12\n"
"13  14  15  16  17  18  19  20  21  22  23  24  25\n"
"VECTORS vectors float\n"
"1 0 0   1 1 0   0 2 0   1 0 0   1 1 0   0 2 0   1 0 0   1 1 0   0 2 0\n"
"1 0 0   1 1 0   0 2 0   0 1 0   0 1 0           0 0 1   0 0 1   0 0 1\n"
"0 0 1   0 0 1   0 0 1   0 0 1   0 0 1   0 0 1   0 0 1   0 0 1   0 0 1\n";

const char unsturctureGridVisItAscii[] =
"# vtk DataFile Version 3.0\n"
"Unstructured Grid Example\n"
"ASCII\n"
"DATASET UNSTRUCTURED_GRID\n"
"FIELD FieldData 1\n"
"Nek_SpectralElementData 1 4 int\n"
"8 8 8 68826 \n"
"POINTS 26 float\n"
"0 0 0   1 0 0   2 0 0    0 1 0   1 1 0   2 1 0        0 0 1   1 0 1   2 0 1\n"
"0 1 1   1 1 1   2 1 1    0.5 1.7 0.5   1.5 1.7 0.5    0 1 2   1 1 2   2 1 2\n"
"0 1 3   1 1 3   2 1 3    0 1 4   1 1 4   2 1 4        0 1 5   1 1 5   2 1 5\n"
"CELLS 12 64\n"
"8 0 1 4 3 6 7 10 9   6 1 7 2 4 10 5   6 2 7 8 5 10 11       5 4 3 9 10 12\n"
"4 5 4 10 13          4 5 10 11 13     6 17 14 18 15 19 16   4 21 22 19 18\n"
"3 20 21 17           3 21 18 17       2 25 24               1 23\n"
"CELL_TYPES 12\n"
"12  13  13  14  10  10  6  9  5  5  3  1\n"
"POINT_DATA 26\n"
"SCALARS scalars float 1\n"
"LOOKUP_TABLE default\n"
"0   1   2   3   4   5   6   7   8   9   10  11  12\n"
"13  14  15  16  17  18  19  20  21  22  23  24  25\n"
"VECTORS vectors float\n"
"1 0 0   1 1 0   0 2 0   1 0 0   1 1 0   0 2 0   1 0 0   1 1 0   0 2 0\n"
"1 0 0   1 1 0   0 2 0   0 1 0   0 1 0           0 0 1   0 0 1   4 5 6\n"
"9 0 1   8 0 1   7 0 1   6 0 1   5 0 1   4 0 1   3 0 1   2 0 1   1 0 1\n";

const char unsturctureGridBin[] =
"# vtk DataFile Version 4.0\n"
"Unstructured Grid Example\n"
"BINARY\n"
"DATASET UNSTRUCTURED_GRID\n"
"POINTS 26 float\n"
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x40\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00"
"\x3f\x80\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00\x3f\x80\x00\x00"
"\x3f\x80\x00\x00\x3f\x00\x00\x00\x3f\xd9\x99\x9a\x3f\x00\x00\x00\x3f\xc0\x00\x00"
"\x3f\xd9\x99\x9a\x3f\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00"
"\x3f\x80\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00\x40\x00\x00\x00\x3f\x80\x00\x00"
"\x40\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x40\x40\x00\x00\x3f\x80\x00\x00"
"\x3f\x80\x00\x00\x40\x40\x00\x00\x40\x00\x00\x00\x3f\x80\x00\x00\x40\x40\x00\x00"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x40\x80\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00"
"\x40\x80\x00\x00\x40\x00\x00\x00\x3f\x80\x00\x00\x40\x80\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x40\xa0\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x40\xa0\x00\x00"
"\x40\x00\x00\x00\x3f\x80\x00\x00\x40\xa0\x00\x00\n"
"CELLS 12 64\n"
"\x00\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x04\x00\x00\x00\x03"
"\x00\x00\x00\x06\x00\x00\x00\x07\x00\x00\x00\x0a\x00\x00\x00\x09\x00\x00\x00\x06"
"\x00\x00\x00\x01\x00\x00\x00\x07\x00\x00\x00\x02\x00\x00\x00\x04\x00\x00\x00\x0a"
"\x00\x00\x00\x05\x00\x00\x00\x06\x00\x00\x00\x02\x00\x00\x00\x07\x00\x00\x00\x08"
"\x00\x00\x00\x05\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x05\x00\x00\x00\x04"
"\x00\x00\x00\x03\x00\x00\x00\x09\x00\x00\x00\x0a\x00\x00\x00\x0c\x00\x00\x00\x04"
"\x00\x00\x00\x05\x00\x00\x00\x04\x00\x00\x00\x0a\x00\x00\x00\x0d\x00\x00\x00\x04"
"\x00\x00\x00\x05\x00\x00\x00\x0a\x00\x00\x00\x0b\x00\x00\x00\x0d\x00\x00\x00\x06"
"\x00\x00\x00\x11\x00\x00\x00\x0e\x00\x00\x00\x12\x00\x00\x00\x0f\x00\x00\x00\x13"
"\x00\x00\x00\x10\x00\x00\x00\x04\x00\x00\x00\x15\x00\x00\x00\x16\x00\x00\x00\x13"
"\x00\x00\x00\x12\x00\x00\x00\x03\x00\x00\x00\x14\x00\x00\x00\x15\x00\x00\x00\x11"
"\x00\x00\x00\x03\x00\x00\x00\x15\x00\x00\x00\x12\x00\x00\x00\x11\x00\x00\x00\x02"
"\x00\x00\x00\x19\x00\x00\x00\x18\x00\x00\x00\x01\x00\x00\x00\x17\n"
"CELL_TYPES 12\n"
"\x00\x00\x00\x0c\x00\x00\x00\x0d\x00\x00\x00\x0d\x00\x00\x00\x0e\x00\x00\x00\x0a"
"\x00\x00\x00\x0a\x00\x00\x00\x06\x00\x00\x00\x09\x00\x00\x00\x05\x00\x00\x00\x05"
"\x00\x00\x00\x03\x00\x00\x00\x01\n"
"POINT_DATA 26\n"
"SCALARS scalars float\n"
"LOOKUP_TABLE default\n"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x40\x00\x00\x00\x40\x40\x00\x00\x40\x80\x00\x00"
"\x40\xa0\x00\x00\x40\xc0\x00\x00\x40\xe0\x00\x00\x41\x00\x00\x00\x41\x10\x00\x00"
"\x41\x20\x00\x00\x41\x30\x00\x00\x41\x40\x00\x00\x41\x50\x00\x00\x41\x60\x00\x00"
"\x41\x70\x00\x00\x41\x80\x00\x00\x41\x88\x00\x00\x41\x90\x00\x00\x41\x98\x00\x00"
"\x41\xa0\x00\x00\x41\xa8\x00\x00\x41\xb0\x00\x00\x41\xb8\x00\x00\x41\xc0\x00\x00"
"\x41\xc8\x00\x00\n"
"VECTORS vectors float\n"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x40\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x40\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x40\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x3f\x80\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x3f\x80\x00\x00\n";

inline void createFile(const char *buffer, std::size_t size, const char *fname)
{
  std::ofstream fstr(fname, std::ios_base::out | std::ios_base::binary);
  fstr.write(buffer, static_cast<std::streamsize>(size - 1));
  fstr.close();
}

inline vtkm::cont::DataSet readVTKDataSet(const char *fname)
{
  vtkm::cont::DataSet ds;
  vtkm::io::reader::VTKDataSetReader reader(fname);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  return ds;
}

const char *testFileName = "vtkm-io-reader-test.vtk";

enum Format
{
  FORMAT_ASCII,
  FORMAT_BINARY
};

} // anonymous namespace


void TestReadingPolyData(Format format)
{
  (format == FORMAT_ASCII) ?
    createFile(polydataAscii, sizeof(polydataAscii), testFileName) :
    createFile(polydataBin, sizeof(polydataBin), testFileName);

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 3,
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == 8,
                   "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == 6,
                   "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetSingleType<> >(),
                   "Incorrect cellset type");
}

void TestReadingStructuredPoints(Format format)
{
  (format == FORMAT_ASCII) ?
    createFile(structuredPointsAscii, sizeof(structuredPointsAscii), testFileName) :
    createFile(structuredPointsBin, sizeof(structuredPointsBin), testFileName);

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 1,
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == 72,
                   "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == 30,
                   "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3> >(),
                   "Incorrect cellset type");
}

void TestReadingStructuredPointsVisIt(Format format)
{
  if (format == FORMAT_ASCII)
  {
    createFile(structuredPointsVisItAscii, sizeof(structuredPointsVisItAscii), testFileName);

    vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 1,
                     "Incorrect number of fields");
    VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == 64,
                     "Incorrect number of points");
    VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == 27,
                     "Incorrect number of cells");
    VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetStructured<3> >(),
                     "Incorrect cellset type");
  }
}


void TestReadingUnstructuredGrid(Format format)
{
  (format == FORMAT_ASCII) ?
    createFile(unsturctureGridAscii, sizeof(unsturctureGridAscii), testFileName) :
    createFile(unsturctureGridBin, sizeof(unsturctureGridBin), testFileName);

  vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2,
                   "Incorrect number of fields");
  VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == 26,
                   "Incorrect number of points");
  VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == 15,
                   "Incorrect number of cells");
  VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetExplicit<> >(),
                   "Incorrect cellset type");
}

void TestReadingUnstructuredGridVisIt(Format format)
{
  if (format == FORMAT_ASCII)
  {
    createFile(unsturctureGridVisItAscii, sizeof(unsturctureGridVisItAscii), testFileName);

    vtkm::cont::DataSet ds = readVTKDataSet(testFileName);

    VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2,
                     "Incorrect number of fields");
    VTKM_TEST_ASSERT(ds.GetCoordinateSystem().GetData().GetNumberOfValues() == 26,
                     "Incorrect number of points");
    VTKM_TEST_ASSERT(ds.GetCellSet().GetNumberOfCells() == 15,
                     "Incorrect number of cells");
    VTKM_TEST_ASSERT(ds.GetCellSet().IsType<vtkm::cont::CellSetExplicit<> >(),
                     "Incorrect cellset type");
  }
}

void TestReadingVTKDataSet()
{
  std::cout << "Test reading VTK Polydata file in ASCII" << std::endl;
  TestReadingPolyData(FORMAT_ASCII);
  std::cout << "Test reading VTK Polydata file in BINARY" << std::endl;
  TestReadingPolyData(FORMAT_BINARY);
  std::cout << "Test reading VTK StructuredPoints file in ASCII" << std::endl;
  TestReadingStructuredPoints(FORMAT_ASCII);

  std::cout << "Test reading VTK StructuredPoints file in BINARY" << std::endl;
  TestReadingStructuredPoints(FORMAT_BINARY);
  std::cout << "Test reading VTK UnstructuredGrid file in ASCII" << std::endl;
  TestReadingUnstructuredGrid(FORMAT_ASCII);
  std::cout << "Test reading VTK UnstructuredGrid file in BINARY" << std::endl;
  TestReadingUnstructuredGrid(FORMAT_BINARY);

  std::cout << "Test reading VTK/VisIt StructuredPoints file in ASCII" << std::endl;
  TestReadingStructuredPointsVisIt(FORMAT_ASCII);
  std::cout << "Test reading VTK/VisIt UnstructuredGrid file in ASCII" << std::endl;
  TestReadingUnstructuredGridVisIt(FORMAT_ASCII);
}

int UnitTestVTKDataSetReader(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestReadingVTKDataSet);
}
