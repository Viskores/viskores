//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <algorithm>

#include <vtkm/worklet/FieldStatistics.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>

//
// Make a simple 2D, 10 cell dataset
//
vtkm::cont::DataSet Make2DUniformStatDataSet0()
{
  vtkm::cont::DataSet dataSet;

  const int dimension = 2;
  const int xVerts = 11;
  const int yVerts = 2;
  const int nVerts = xVerts * yVerts;

  const int xCells = xVerts - 1;
  const int yCells = yVerts - 1;
  const int nCells = xCells * yCells;

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vtkm::Id3(xVerts, yVerts, 1));
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  // Create cell scalar
  vtkm::Float32 data[nVerts] = { 4, 1, 10, 6, 8, 2, 9, 3, 5, 7 };
  dataSet.AddField(vtkm::cont::make_Field(
    "data", vtkm::cont::Field::Association::CELL_SET, "cells", data, nCells, vtkm::CopyFlag::On));

  vtkm::cont::CellSetStructured<dimension> cellSet("cells");

  //Set uniform structure
  cellSet.SetPointDimensions(vtkm::make_Vec(xVerts, yVerts));
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

//
// Make a simple 2D, 1000 point dataset populated with stat distributions
//
vtkm::cont::DataSet Make2DUniformStatDataSet1()
{
  vtkm::cont::DataSet dataSet;

  const int dimension = 2;
  const int xVerts = 20;
  const int yVerts = 50;
  const int nVerts = xVerts * yVerts;

  const int xCells = xVerts - 1;
  const int yCells = yVerts - 1;
  const int nCells = xCells * yCells;

  // Poisson distribution [0:49] mean = 10
  vtkm::Float32 poisson[nVerts] = {
    8,  10, 9,  8,  14, 11, 12, 9,  19, 7,  8,  11, 7,  10, 11, 11, 11, 6,  8,  8,  7,  15, 9,  7,
    8,  10, 9,  10, 10, 12, 7,  6,  14, 10, 14, 10, 7,  11, 13, 9,  13, 11, 10, 10, 12, 12, 7,  12,
    10, 11, 12, 8,  13, 9,  5,  12, 11, 9,  5,  9,  12, 9,  6,  10, 11, 9,  9,  11, 9,  7,  7,  18,
    16, 13, 12, 8,  10, 11, 9,  8,  17, 3,  15, 15, 9,  10, 10, 8,  10, 9,  7,  9,  8,  10, 13, 9,
    7,  11, 7,  10, 13, 10, 11, 9,  10, 7,  10, 6,  12, 6,  9,  7,  6,  12, 12, 9,  12, 12, 11, 6,
    1,  12, 8,  13, 14, 8,  8,  10, 7,  7,  6,  7,  5,  11, 6,  11, 13, 8,  13, 5,  9,  12, 7,  11,
    10, 15, 11, 9,  7,  12, 15, 7,  8,  7,  12, 8,  21, 16, 13, 11, 10, 14, 12, 11, 12, 14, 7,  11,
    7,  12, 16, 8,  10, 8,  9,  7,  8,  7,  13, 13, 11, 15, 7,  7,  6,  11, 7,  12, 12, 13, 14, 11,
    13, 11, 11, 9,  15, 8,  6,  11, 12, 10, 11, 7,  6,  14, 11, 10, 12, 5,  8,  9,  11, 15, 11, 10,
    17, 14, 9,  10, 10, 12, 11, 13, 13, 12, 11, 7,  8,  10, 7,  11, 10, 5,  8,  10, 13, 13, 12, 6,
    10, 7,  13, 8,  11, 7,  10, 7,  8,  7,  14, 16, 9,  11, 8,  11, 9,  15, 11, 10, 10, 12, 7,  7,
    11, 7,  5,  17, 9,  11, 11, 11, 10, 17, 10, 15, 7,  11, 12, 16, 9,  8,  11, 14, 9,  22, 8,  8,
    8,  13, 12, 12, 1,  14, 15, 6,  15, 8,  11, 16, 14, 8,  6,  9,  8,  9,  9,  10, 8,  6,  13, 8,
    6,  12, 11, 12, 13, 8,  6,  6,  5,  6,  10, 9,  11, 12, 14, 12, 10, 11, 10, 10, 8,  13, 8,  11,
    7,  13, 13, 12, 12, 13, 15, 4,  9,  16, 7,  9,  8,  10, 6,  9,  11, 12, 6,  7,  14, 6,  4,  15,
    5,  18, 9,  9,  11, 12, 9,  5,  6,  7,  15, 6,  11, 14, 8,  12, 6,  9,  5,  9,  14, 9,  12, 6,
    9,  14, 11, 12, 12, 13, 15, 9,  8,  7,  13, 12, 7,  13, 6,  9,  10, 10, 10, 9,  11, 5,  9,  13,
    16, 9,  10, 8,  9,  6,  13, 12, 8,  12, 9,  12, 17, 8,  11, 10, 8,  7,  11, 7,  13, 13, 10, 14,
    11, 9,  6,  6,  14, 16, 5,  9,  13, 11, 12, 7,  4,  6,  9,  11, 11, 10, 12, 9,  7,  13, 8,  8,
    12, 5,  10, 7,  11, 11, 10, 10, 14, 6,  8,  8,  3,  12, 16, 11, 11, 7,  6,  12, 11, 5,  9,  12,
    9,  13, 7,  8,  9,  9,  12, 7,  9,  8,  12, 11, 6,  10, 6,  7,  6,  11, 10, 8,  9,  8,  4,  19,
    12, 6,  10, 9,  6,  12, 9,  14, 7,  8,  11, 7,  7,  12, 13, 9,  13, 12, 8,  6,  10, 17, 19, 10,
    10, 13, 5,  11, 8,  10, 8,  16, 12, 6,  6,  7,  10, 9,  12, 8,  5,  10, 7,  18, 9,  12, 10, 4,
    9,  9,  15, 15, 6,  7,  7,  11, 12, 4,  8,  18, 5,  12, 12, 11, 10, 14, 9,  9,  10, 8,  10, 8,
    10, 9,  9,  4,  10, 12, 5,  13, 6,  9,  7,  5,  12, 8,  11, 10, 9,  17, 9,  9,  8,  11, 18, 11,
    10, 9,  4,  13, 10, 15, 5,  10, 9,  7,  7,  8,  10, 6,  6,  19, 10, 16, 7,  7,  9,  10, 10, 13,
    10, 10, 14, 13, 12, 8,  7,  13, 12, 11, 13, 12, 9,  8,  6,  8,  10, 3,  8,  8,  12, 12, 13, 13,
    10, 5,  10, 7,  13, 7,  9,  5,  13, 7,  10, 8,  13, 11, 17, 9,  6,  14, 10, 10, 13, 9,  15, 8,
    15, 9,  12, 11, 12, 8,  3,  9,  8,  10, 12, 8,  14, 13, 12, 11, 12, 9,  18, 10, 13, 7,  4,  4,
    11, 8,  3,  7,  9,  10, 12, 7,  11, 21, 9,  7,  8,  9,  10, 10, 11, 9,  15, 13, 21, 12, 8,  11,
    9,  10, 11, 9,  17, 8,  9,  8,  14, 6,  13, 9,  8,  11, 12, 12, 12, 11, 6,  13, 7,  9,  11, 15,
    17, 17, 11, 10, 7,  8,  11, 8,  6,  9,  13, 7,  9,  6,  5,  10, 7,  16, 16, 9,  7,  6,  14, 8,
    13, 16, 7,  7,  10, 11, 6,  10, 9,  9,  8,  14, 11, 9,  11, 9,  10, 11, 9,  8,  14, 11, 7,  12,
    11, 8,  9,  9,  10, 11, 11, 10, 9,  6,  6,  11, 16, 10, 7,  6,  6,  13, 18, 8,  12, 11, 14, 13,
    8,  8,  10, 17, 17, 6,  6,  10, 18, 5,  8,  11, 6,  6,  14, 10, 9,  6,  11, 6,  13, 12, 10, 6,
    9,  9,  9,  13, 7,  17, 10, 14, 10, 9,  10, 10, 11, 10, 11, 15, 13, 6,  12, 19, 10, 12, 12, 15,
    13, 10, 10, 13, 11, 13, 13, 17, 6,  5,  6,  7,  6,  9,  13, 11, 8,  12, 9,  6,  10, 16, 11, 12,
    5,  12, 14, 13, 13, 16, 11, 6,  12, 12, 15, 8,  7,  11, 8,  5,  10, 8,  9,  11, 9,  12, 10, 5,
    12, 11, 9,  6,  14, 12, 10, 11, 9,  6,  7,  12, 8,  12, 8,  15, 9,  8,  7,  9,  3,  6,  14, 7,
    8,  11, 9,  10, 12, 9,  10, 9,  8,  6,  12, 11, 6,  8,  9,  8,  15, 11, 7,  18, 12, 11, 10, 13,
    11, 11, 10, 7,  9,  8,  8,  11, 11, 13, 6,  12, 13, 16, 11, 11, 5,  12, 14, 15, 9,  14, 15, 6,
    8,  7,  6,  8,  9,  19, 7,  12, 11, 8,  14, 12, 10, 9,  3,  7
  };

  // Normal distribution [0:49] mean = 25 standard deviation = 5.0
  vtkm::Float32 normal[nVerts] = {
    24, 19, 28, 19, 25, 28, 25, 22, 27, 26, 35, 26, 30, 28, 24, 23, 21, 31, 20, 11, 21, 22, 14, 25,
    20, 24, 24, 21, 24, 29, 26, 21, 32, 29, 23, 28, 31, 25, 23, 30, 18, 24, 22, 25, 33, 24, 22, 23,
    21, 17, 20, 28, 30, 18, 20, 32, 25, 24, 32, 15, 27, 24, 27, 19, 30, 27, 17, 24, 29, 23, 22, 19,
    24, 19, 28, 24, 25, 24, 25, 30, 24, 31, 30, 27, 25, 25, 25, 15, 29, 23, 29, 29, 21, 25, 35, 24,
    28, 10, 31, 23, 22, 22, 22, 33, 29, 27, 18, 27, 27, 24, 20, 20, 21, 29, 23, 31, 23, 23, 22, 23,
    30, 27, 28, 31, 16, 29, 25, 19, 33, 28, 25, 24, 15, 27, 37, 29, 15, 19, 14, 19, 24, 23, 30, 29,
    35, 22, 19, 26, 26, 14, 24, 30, 32, 23, 30, 29, 26, 27, 25, 23, 17, 26, 32, 29, 20, 17, 21, 23,
    22, 20, 36, 12, 26, 23, 15, 29, 24, 22, 26, 33, 24, 23, 20, 26, 22, 17, 26, 26, 34, 22, 26, 17,
    23, 18, 29, 27, 21, 29, 28, 29, 24, 25, 28, 19, 18, 21, 23, 23, 27, 25, 24, 25, 24, 25, 21, 25,
    21, 27, 23, 20, 29, 15, 28, 30, 24, 27, 17, 23, 16, 21, 25, 17, 27, 28, 21, 13, 19, 27, 16, 30,
    31, 25, 30, 17, 17, 25, 26, 22, 21, 17, 24, 17, 25, 22, 27, 14, 27, 24, 27, 25, 26, 31, 21, 23,
    30, 30, 22, 19, 23, 22, 23, 25, 24, 25, 24, 28, 26, 30, 18, 25, 30, 37, 27, 34, 28, 34, 25, 10,
    25, 22, 35, 30, 24, 32, 24, 34, 19, 29, 26, 16, 27, 17, 26, 23, 27, 25, 26, 21, 31, 21, 28, 15,
    32, 24, 23, 23, 18, 15, 22, 25, 16, 25, 31, 26, 25, 28, 24, 26, 23, 25, 33, 20, 27, 28, 24, 29,
    32, 20, 24, 20, 19, 32, 24, 6,  24, 21, 26, 18, 15, 30, 19, 26, 22, 30, 35, 23, 22, 30, 20, 22,
    18, 30, 28, 25, 16, 25, 27, 30, 18, 24, 30, 28, 20, 19, 20, 28, 21, 24, 15, 33, 20, 18, 20, 36,
    30, 26, 25, 18, 28, 27, 31, 31, 15, 26, 16, 22, 27, 14, 17, 27, 27, 22, 32, 30, 22, 34, 22, 25,
    20, 22, 26, 29, 28, 33, 18, 23, 20, 20, 27, 24, 28, 21, 25, 27, 25, 19, 19, 25, 19, 32, 29, 27,
    23, 21, 28, 33, 23, 23, 28, 26, 31, 19, 21, 29, 21, 27, 23, 32, 24, 26, 21, 28, 28, 24, 17, 31,
    27, 21, 19, 32, 28, 23, 30, 23, 29, 15, 26, 26, 15, 20, 25, 26, 27, 31, 21, 23, 23, 33, 28, 19,
    23, 22, 22, 25, 27, 17, 23, 17, 25, 28, 26, 30, 32, 31, 19, 25, 25, 19, 23, 29, 27, 23, 34, 22,
    13, 21, 32, 10, 20, 33, 21, 17, 29, 31, 14, 24, 23, 19, 19, 22, 17, 26, 37, 26, 22, 26, 38, 29,
    29, 27, 30, 20, 31, 14, 32, 32, 24, 23, 23, 18, 21, 31, 24, 20, 28, 15, 21, 25, 25, 20, 30, 25,
    22, 21, 21, 25, 24, 25, 18, 23, 28, 30, 20, 27, 27, 19, 10, 32, 24, 20, 29, 26, 25, 20, 25, 29,
    28, 24, 32, 26, 22, 19, 23, 27, 27, 29, 20, 25, 21, 30, 28, 31, 24, 19, 23, 19, 19, 18, 30, 18,
    16, 24, 20, 20, 30, 25, 29, 25, 31, 21, 28, 31, 24, 26, 27, 21, 24, 23, 26, 18, 32, 26, 28, 26,
    24, 26, 29, 30, 22, 20, 24, 28, 25, 29, 20, 21, 22, 15, 30, 27, 33, 26, 22, 32, 30, 31, 20, 19,
    24, 26, 27, 31, 17, 17, 33, 27, 16, 27, 27, 22, 27, 19, 24, 21, 17, 24, 28, 23, 26, 24, 19, 26,
    20, 24, 22, 19, 22, 21, 21, 28, 29, 39, 19, 16, 25, 29, 31, 22, 22, 29, 26, 22, 22, 22, 26, 23,
    23, 23, 30, 25, 25, 25, 27, 29, 18, 33, 21, 12, 22, 29, 12, 20, 35, 22, 34, 28, 18, 29, 21, 20,
    24, 33, 24, 26, 23, 34, 31, 25, 31, 22, 35, 21, 20, 29, 27, 22, 30, 22, 27, 23, 22, 32, 16, 19,
    27, 22, 24, 27, 21, 33, 25, 25, 19, 28, 20, 27, 21, 25, 28, 20, 27, 22, 21, 20, 26, 30, 33, 23,
    20, 24, 17, 23, 28, 35, 14, 23, 22, 28, 28, 26, 25, 18, 20, 28, 28, 22, 13, 24, 22, 20, 30, 26,
    26, 18, 22, 20, 23, 24, 20, 27, 34, 28, 18, 24, 34, 33, 25, 33, 37, 21, 20, 31, 19, 23, 29, 22,
    21, 24, 19, 27, 19, 32, 25, 23, 33, 26, 33, 27, 29, 30, 19, 22, 30, 19, 18, 24, 25, 17, 31, 19,
    31, 26, 22, 23, 28, 28, 25, 24, 19, 19, 27, 28, 23, 21, 29, 26, 31, 22, 22, 25, 16, 29, 21, 22,
    23, 25, 22, 21, 22, 19, 27, 26, 28, 30, 22, 21, 24, 22, 23, 26, 28, 22, 18, 25, 23, 27, 31, 19,
    15, 29, 20, 19, 27, 25, 21, 29, 22, 24, 25, 17, 36, 29, 22, 22, 24, 28, 27, 22, 26, 31, 29, 31,
    18, 25, 23, 16, 37, 27, 21, 31, 25, 24, 20, 23, 28, 33, 24, 21, 26, 20, 18, 31, 20, 24, 23, 19,
    27, 17, 23, 23, 20, 26, 28, 23, 26, 31, 25, 31, 19, 32, 26, 18, 19, 29, 20, 21, 15, 25, 27, 29,
    22, 22, 22, 26, 23, 22, 23, 29, 28, 20, 21, 22, 20, 22, 27, 25, 23, 32, 23, 20, 31, 20, 27, 26,
    34, 20, 22, 36, 21, 29, 25, 20, 21, 22, 29, 29, 25, 22, 24, 22
  };

  //Chi squared distribution [0:49] degrees of freedom = 5.0
  vtkm::Float32 chiSquare[nVerts] = {
    3,  1,  4,  6,  5,  4,  8,  7,  2,  9,  2,  0,  0,  4,  3,  2,  5,  2,  3,  6,  3,  8,  3,  4,
    3,  3,  2,  7,  2,  10, 9,  6,  1,  1,  4,  7,  3,  3,  1,  4,  4,  3,  9,  4,  4,  7,  3,  2,
    4,  7,  3,  3,  2,  10, 1,  6,  2,  2,  3,  8,  3,  3,  6,  9,  4,  1,  4,  3,  16, 7,  0,  1,
    8,  7,  13, 3,  5,  0,  3,  8,  10, 3,  5,  5,  1,  5,  2,  1,  3,  2,  5,  3,  4,  3,  3,  3,
    3,  1,  13, 2,  3,  1,  2,  7,  3,  4,  1,  2,  5,  4,  4,  4,  2,  6,  3,  2,  7,  8,  1,  3,
    4,  1,  2,  0,  1,  6,  1,  8,  8,  1,  1,  4,  2,  1,  4,  3,  5,  4,  6,  4,  2,  3,  8,  8,
    3,  3,  3,  4,  5,  8,  8,  16, 7,  12, 4,  3,  14, 8,  3,  12, 5,  0,  5,  3,  5,  2,  9,  2,
    9,  4,  1,  0,  0,  4,  4,  6,  3,  4,  11, 2,  4,  7,  4,  2,  1,  9,  4,  3,  2,  5,  1,  5,
    3,  8,  2,  8,  1,  8,  0,  4,  1,  3,  2,  1,  2,  3,  2,  1,  8,  5,  4,  1,  9,  9,  1,  3,
    5,  0,  1,  6,  10, 8,  3,  12, 3,  4,  4,  7,  1,  3,  6,  4,  4,  6,  1,  4,  7,  5,  6,  11,
    6,  5,  2,  7,  2,  5,  3,  5,  6,  3,  6,  2,  1,  10, 8,  3,  7,  0,  2,  6,  9,  3,  11, 3,
    2,  5,  1,  4,  6,  10, 9,  1,  4,  3,  7,  12, 3,  10, 0,  2,  11, 2,  1,  0,  4,  1,  2,  16,
    5,  17, 7,  8,  2,  10, 10, 3,  1,  3,  2,  2,  4,  8,  4,  3,  2,  4,  4,  6,  8,  6,  2,  3,
    2,  4,  2,  4,  7,  10, 5,  3,  5,  2,  4,  6,  9,  3,  1,  1,  1,  1,  4,  2,  2,  7,  4,  9,
    2,  3,  5,  6,  2,  5,  1,  6,  5,  7,  8,  3,  7,  2,  2,  8,  6,  2,  10, 2,  1,  4,  5,  1,
    1,  1,  5,  6,  1,  1,  4,  5,  4,  2,  4,  3,  2,  7,  19, 4,  7,  2,  7,  5,  2,  5,  3,  8,
    4,  6,  7,  2,  0,  0,  2,  12, 6,  2,  2,  3,  5,  9,  4,  9,  2,  2,  7,  8,  3,  3,  10, 6,
    3,  2,  1,  6,  2,  4,  6,  3,  5,  8,  2,  3,  6,  14, 0,  3,  6,  5,  2,  7,  0,  3,  8,  5,
    3,  2,  2,  5,  1,  3,  12, 11, 16, 2,  1,  3,  7,  3,  1,  6,  4,  3,  12, 5,  1,  3,  1,  4,
    9,  1,  3,  3,  4,  4,  6,  7,  7,  5,  2,  4,  2,  3,  2,  2,  6,  4,  2,  2,  3,  5,  1,  4,
    9,  1,  0,  7,  6,  4,  3,  3,  7,  3,  3,  6,  2,  7,  9,  3,  1,  16, 5,  4,  3,  6,  3,  2,
    5,  2,  2,  4,  3,  1,  3,  3,  6,  3,  5,  9,  1,  10, 1,  7,  2,  2,  6,  7,  3,  5,  3,  7,
    2,  2,  2,  2,  6,  4,  3,  2,  5,  5,  3,  15, 4,  2,  7,  7,  4,  3,  3,  5,  1,  2,  9,  0,
    5,  7,  12, 2,  4,  8,  5,  7,  8,  3,  2,  2,  18, 1,  7,  2,  2,  1,  3,  3,  3,  7,  1,  9,
    8,  4,  3,  7,  6,  4,  5,  2,  0,  5,  1,  5,  10, 4,  2,  8,  2,  2,  0,  5,  6,  4,  5,  0,
    1,  5,  11, 3,  3,  4,  4,  2,  3,  5,  1,  6,  5,  7,  2,  2,  5,  7,  4,  8,  4,  1,  1,  7,
    2,  3,  9,  6,  13, 1,  5,  4,  6,  2,  4,  11, 2,  5,  5,  1,  4,  1,  4,  7,  1,  5,  8,  3,
    1,  10, 9,  13, 1,  7,  2,  9,  4,  3,  3,  10, 12, 2,  0,  4,  6,  5,  5,  1,  4,  7,  2,  12,
    7,  6,  5,  0,  6,  4,  4,  12, 1,  3,  10, 1,  9,  2,  2,  2,  1,  5,  5,  6,  9,  6,  4,  1,
    11, 6,  9,  3,  2,  7,  1,  7,  4,  3,  0,  3,  1,  12, 17, 2,  1,  6,  4,  4,  2,  1,  5,  5,
    3,  2,  2,  4,  6,  5,  4,  6,  11, 3,  12, 6,  3,  6,  3,  0,  6,  3,  7,  4,  8,  5,  14, 5,
    1,  9,  4,  6,  5,  3,  9,  3,  1,  1,  0,  3,  7,  3,  5,  1,  6,  2,  2,  6,  2,  12, 1,  0,
    6,  3,  3,  5,  4,  7,  2,  2,  15, 7,  3,  10, 4,  2,  6,  3,  4,  8,  3,  1,  5,  5,  5,  4,
    3,  7,  3,  4,  5,  5,  2,  4,  2,  5,  1,  12, 5,  6,  3,  2,  8,  5,  2,  3,  11, 11, 6,  5,
    0,  3,  3,  9,  4,  2,  11, 1,  5,  3,  5,  6,  3,  6,  4,  2,  4,  10, 11, 3,  3,  4,  1,  1,
    1,  3,  5,  5,  1,  1,  4,  1,  5,  1,  6,  8,  6,  4,  6,  7,  6,  3,  5,  3,  6,  6,  6,  4,
    0,  6,  3,  1,  2,  4,  2,  6,  1,  1,  1,  2,  2,  4,  7,  2,  6,  2,  5,  7,  6,  4,  6,  3,
    1,  4,  5,  1,  4,  6,  2,  3,  0,  6,  11, 2,  9,  2,  6,  4,  5,  6,  2,  19, 2,  10, 4,  2,
    3,  3,  11, 7,  3,  3,  1,  5,  3,  6,  4,  3,  0,  6,  6,  6,  4,  2,  5,  2,  2,  2,  6,  10,
    4,  9,  3,  7,  7,  0,  6,  8,  5,  2,  3,  2,  3,  3,  3,  1,  6,  1,  8,  2,  5,  3,  6,  11,
    5,  7,  2,  6,  7,  3,  4,  1,  0,  5,  8,  3,  2,  9,  3,  1,  2,  3,  3,  9,  5,  6,  5,  1,
    4,  5,  6,  7,  6,  1,  5,  1,  6,  6,  2,  6,  7,  2,  4,  6
  };

  // Uniform distribution [0:49]
  vtkm::Float32 uniform[nVerts] = {
    0,  6,  37, 22, 26, 10, 2,  33, 33, 46, 19, 25, 41, 1,  2,  26, 33, 0,  19, 3,  20, 34, 29, 46,
    42, 26, 4,  32, 20, 35, 45, 38, 13, 2,  36, 16, 31, 37, 49, 18, 12, 49, 36, 37, 32, 3,  31, 44,
    13, 21, 38, 23, 11, 13, 17, 8,  24, 44, 45, 3,  45, 25, 25, 15, 49, 24, 13, 4,  47, 3,  25, 19,
    13, 45, 26, 23, 47, 2,  38, 38, 41, 6,  0,  34, 43, 31, 36, 36, 49, 44, 11, 15, 17, 25, 29, 42,
    20, 42, 13, 20, 26, 23, 14, 8,  7,  28, 40, 1,  26, 24, 47, 37, 27, 44, 31, 42, 7,  10, 35, 6,
    4,  13, 0,  20, 1,  35, 46, 11, 9,  15, 44, 32, 7,  34, 19, 19, 24, 7,  29, 42, 29, 47, 27, 7,
    49, 20, 7,  28, 12, 24, 23, 48, 6,  9,  15, 31, 6,  32, 31, 40, 12, 23, 19, 10, 1,  45, 21, 7,
    47, 20, 6,  44, 4,  8,  3,  18, 12, 6,  39, 22, 17, 22, 40, 46, 32, 10, 33, 45, 12, 43, 23, 25,
    30, 40, 37, 23, 47, 31, 21, 41, 34, 35, 49, 47, 42, 14, 26, 25, 5,  20, 28, 43, 22, 36, 43, 35,
    40, 35, 37, 0,  44, 26, 23, 3,  35, 24, 33, 34, 9,  45, 43, 44, 27, 6,  22, 49, 10, 22, 15, 25,
    44, 21, 23, 40, 18, 10, 49, 7,  31, 30, 0,  0,  38, 36, 15, 20, 34, 34, 10, 41, 35, 41, 4,  4,
    38, 31, 10, 10, 4,  19, 47, 47, 19, 13, 34, 14, 38, 39, 21, 14, 9,  0,  9,  49, 12, 40, 6,  19,
    30, 8,  41, 7,  49, 12, 11, 5,  10, 31, 34, 39, 34, 37, 33, 31, 2,  29, 11, 15, 34, 5,  38, 26,
    27, 29, 16, 35, 7,  8,  24, 43, 40, 27, 36, 15, 6,  26, 15, 29, 25, 21, 12, 18, 19, 22, 23, 19,
    13, 3,  18, 12, 33, 33, 25, 36, 36, 47, 23, 47, 16, 23, 25, 33, 20, 30, 49, 7,  33, 17, 27, 26,
    41, 0,  13, 32, 27, 45, 13, 48, 12, 42, 34, 22, 40, 1,  8,  35, 35, 21, 29, 37, 49, 34, 13, 37,
    8,  0,  24, 3,  8,  45, 39, 37, 21, 0,  29, 25, 3,  27, 19, 10, 19, 31, 32, 35, 26, 14, 40, 18,
    34, 15, 0,  5,  26, 38, 11, 2,  3,  8,  36, 14, 2,  23, 22, 25, 22, 7,  14, 41, 34, 28, 34, 16,
    2,  49, 27, 0,  42, 1,  18, 24, 28, 36, 33, 26, 1,  6,  48, 9,  17, 30, 30, 6,  27, 47, 17, 41,
    48, 12, 12, 21, 40, 44, 12, 38, 34, 22, 13, 33, 5,  10, 5,  27, 0,  8,  29, 21, 4,  34, 18, 41,
    6,  48, 1,  4,  24, 38, 46, 12, 17, 38, 24, 37, 33, 34, 37, 1,  11, 11, 28, 32, 30, 18, 11, 11,
    32, 8,  37, 7,  2,  33, 6,  47, 24, 31, 45, 0,  29, 36, 24, 2,  22, 25, 38, 3,  22, 48, 23, 16,
    22, 37, 10, 8,  18, 46, 48, 12, 3,  6,  26, 8,  25, 5,  42, 18, 21, 16, 35, 28, 43, 37, 41, 34,
    19, 46, 30, 18, 26, 22, 20, 12, 4,  21, 23, 14, 5,  10, 40, 26, 33, 43, 12, 35, 13, 19, 4,  22,
    11, 39, 24, 0,  13, 33, 21, 9,  48, 6,  39, 47, 8,  30, 3,  17, 14, 25, 41, 41, 36, 16, 40, 31,
    2,  2,  7,  38, 3,  25, 46, 11, 10, 4,  34, 35, 24, 13, 35, 18, 10, 11, 21, 23, 43, 48, 22, 1,
    26, 1,  37, 29, 41, 16, 11, 26, 21, 20, 49, 48, 42, 43, 15, 7,  49, 31, 23, 46, 34, 40, 27, 28,
    7,  47, 41, 7,  2,  17, 5,  4,  25, 1,  28, 42, 25, 33, 36, 34, 1,  9,  33, 17, 3,  7,  46, 11,
    19, 29, 8,  1,  34, 38, 35, 3,  29, 46, 46, 21, 25, 41, 45, 30, 36, 25, 24, 8,  48, 28, 13, 26,
    34, 33, 4,  27, 30, 33, 24, 28, 29, 22, 7,  25, 36, 1,  2,  26, 16, 1,  12, 5,  19, 27, 29, 30,
    46, 38, 25, 24, 32, 34, 20, 24, 23, 35, 26, 13, 30, 14, 35, 26, 46, 11, 20, 29, 39, 46, 34, 41,
    26, 11, 7,  44, 12, 32, 0,  46, 13, 42, 13, 47, 25, 6,  20, 35, 21, 5,  38, 4,  22, 17, 14, 37,
    16, 16, 2,  28, 24, 10, 5,  48, 43, 24, 18, 40, 8,  7,  2,  7,  23, 19, 44, 21, 20, 32, 15, 3,
    40, 44, 45, 45, 38, 8,  28, 1,  40, 26, 43, 13, 43, 29, 19, 40, 26, 46, 21, 28, 37, 44, 16, 9,
    37, 35, 43, 3,  35, 43, 17, 4,  8,  20, 4,  33, 28, 40, 43, 38, 31, 44, 43, 24, 5,  18, 19, 34,
    6,  3,  7,  23, 35, 11, 19, 48, 31, 34, 45, 18, 42, 39, 21, 3,  24, 24, 22, 24, 37, 46, 15, 7,
    5,  4,  48, 20, 11, 48, 41, 9,  6,  9,  16, 28, 22, 29, 21, 18, 19, 30, 21, 7,  33, 49, 34, 20,
    42, 40, 39, 18, 0,  23, 31, 32, 32, 39, 18, 17, 19, 16, 34, 7,  14, 33, 42, 15, 7,  30, 0,  46,
    19, 25, 17, 13, 14, 41, 6,  31, 2,  22, 18, 7,  37, 33, 0,  39, 28, 14, 20, 16, 25, 35, 42, 11,
    23, 18, 2,  3,  10, 28, 41, 21, 41, 14, 9,  17, 46, 29, 18, 23, 31, 47, 20, 2,  22, 29, 37, 43,
    6,  5,  33, 41, 29, 32, 49, 0,  46, 9,  48, 26, 13, 35, 29, 41, 41, 32, 36, 32, 17, 26, 33, 16,
    43, 22, 45, 13, 47, 5,  20, 41, 48, 16, 26, 26, 40, 46, 33, 12
  };

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vtkm::Id3(xVerts, yVerts, 1));
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  // Set point scalars
  dataSet.AddField(vtkm::cont::make_Field(
    "p_poisson", vtkm::cont::Field::Association::POINTS, poisson, nVerts, vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field(
    "p_normal", vtkm::cont::Field::Association::POINTS, normal, nVerts, vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field(
    "p_chiSquare", vtkm::cont::Field::Association::POINTS, chiSquare, nVerts, vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field(
    "p_uniform", vtkm::cont::Field::Association::POINTS, uniform, nVerts, vtkm::CopyFlag::On));

  // Set cell scalars
  dataSet.AddField(vtkm::cont::make_Field("c_poisson",
                                          vtkm::cont::Field::Association::CELL_SET,
                                          "cells",
                                          poisson,
                                          nCells,
                                          vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field("c_normal",
                                          vtkm::cont::Field::Association::CELL_SET,
                                          "cells",
                                          normal,
                                          nCells,
                                          vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field("c_chiSquare",
                                          vtkm::cont::Field::Association::CELL_SET,
                                          "cells",
                                          chiSquare,
                                          nCells,
                                          vtkm::CopyFlag::On));
  dataSet.AddField(vtkm::cont::make_Field("c_uniform",
                                          vtkm::cont::Field::Association::CELL_SET,
                                          "cells",
                                          poisson,
                                          nCells,
                                          vtkm::CopyFlag::On));

  vtkm::cont::CellSetStructured<dimension> cellSet("cells");

  //Set uniform structure
  cellSet.SetPointDimensions(vtkm::make_Vec(xVerts, yVerts));
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

//
// Create a dataset with known point data and cell data (statistical distributions)
//
void PrintStatInfo(
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::StatInfo statinfo)
{
  std::cout << "   Median " << statinfo.median << std::endl;
  std::cout << "   Minimum " << statinfo.minimum << std::endl;
  std::cout << "   Maximum " << statinfo.maximum << std::endl;
  std::cout << "   Mean " << statinfo.mean << std::endl;
  std::cout << "   Variance " << statinfo.variance << std::endl;
  std::cout << "   Standard Deviation " << statinfo.stddev << std::endl;
  std::cout << "   Skewness " << statinfo.skewness << std::endl;
  std::cout << "   Kurtosis " << statinfo.kurtosis << std::endl;
  std::cout << "   Raw Moment 1-4 [ ";
  for (vtkm::Id i = 0; i < 4; i++)
    std::cout << statinfo.rawMoment[i] << " ";
  std::cout << "]" << std::endl;
  std::cout << "   Central Moment 1-4 [ ";
  for (vtkm::Id i = 0; i < 4; i++)
    std::cout << statinfo.centralMoment[i] << " ";
  std::cout << "]" << std::endl;
}

//
// Test simple dataset 2D Uniform with 10 cells
//
void TestFieldSimple()
{
  // Create the output structure
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::StatInfo statinfo;

  // Data attached is [1:10]
  vtkm::cont::DataSet ds = Make2DUniformStatDataSet0();

  // Cell data
  vtkm::cont::ArrayHandle<vtkm::Float32> data;
  ds.GetField("data").GetData().CopyTo(data);

  // Run
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().Run(data,
                                                                                       statinfo);
  std::cout << "Statistics for CELL data:" << std::endl;
  PrintStatInfo(statinfo);

  vtkm::Float32 expected[8] = { 6.0f, 1.0f, 10.0f, 5.5f, 8.25f, 2.87228f, 0.0f, 1.77576f };
  VTKM_TEST_ASSERT(test_equal(statinfo.median, expected[0]), "Error in median");
  VTKM_TEST_ASSERT(test_equal(statinfo.minimum, expected[1]), "Error in minimum");
  VTKM_TEST_ASSERT(test_equal(statinfo.maximum, expected[2]), "Error in maximum");
  VTKM_TEST_ASSERT(test_equal(statinfo.mean, expected[3]), "Error in mean");
  VTKM_TEST_ASSERT(test_equal(statinfo.variance, expected[4]), "Error in variance");
  VTKM_TEST_ASSERT(test_equal(statinfo.stddev, expected[5]), "Error in stddev");
  VTKM_TEST_ASSERT(test_equal(statinfo.skewness, expected[6]), "Error in skewness");
  VTKM_TEST_ASSERT(test_equal(statinfo.kurtosis, expected[7]), "Error in kurtosis");
}

//
// Create a dataset with known point data and cell data (statistical distributions)
//
void TestFieldStandardDistributions()
{
  // Create the output structure
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::StatInfo statinfo;

  // Data attached is the poisson distribution
  vtkm::cont::DataSet ds = Make2DUniformStatDataSet1();

  // Point data
  vtkm::cont::ArrayHandle<vtkm::Float32> p_poisson;
  ds.GetField("p_poisson").GetData().CopyTo(p_poisson);
  vtkm::cont::ArrayHandle<vtkm::Float32> p_normal;
  ds.GetField("p_normal").GetData().CopyTo(p_normal);
  vtkm::cont::ArrayHandle<vtkm::Float32> p_chiSquare;
  ds.GetField("p_chiSquare").GetData().CopyTo(p_chiSquare);
  vtkm::cont::ArrayHandle<vtkm::Float32> p_uniform;
  ds.GetField("p_uniform").GetData().CopyTo(p_uniform);

  // Run Poisson data
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().Run(p_poisson,
                                                                                       statinfo);
  std::cout << "Poisson distributed POINT data:" << std::endl;
  PrintStatInfo(statinfo);

  // Run Normal data
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().Run(p_normal,
                                                                                       statinfo);
  std::cout << "Normal distributed POINT data:" << std::endl;
  PrintStatInfo(statinfo);

  // Run Chi Square data
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().Run(p_chiSquare,
                                                                                       statinfo);
  std::cout << "Chi Square distributed POINT data:" << std::endl;
  PrintStatInfo(statinfo);

  // Run Uniform data
  vtkm::worklet::FieldStatistics<vtkm::Float32, VTKM_DEFAULT_DEVICE_ADAPTER_TAG>().Run(p_uniform,
                                                                                       statinfo);
  std::cout << "Uniform distributed POINT data:" << std::endl;
  PrintStatInfo(statinfo);
} // TestFieldStatistics

void TestFieldStatistics()
{
  TestFieldStandardDistributions();
  TestFieldSimple();
}

int UnitTestFieldStatistics(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestFieldStatistics);
}
