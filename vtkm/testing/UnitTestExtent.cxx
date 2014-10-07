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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/Extent.h>

#include <vtkm/testing/Testing.h>

namespace {

const vtkm::Id MIN_VALUES[] = { -5,  8, 40, -8, -3 };
const vtkm::Id MAX_VALUES[] = { 10, 25, 44, -2,  1 };
const vtkm::Id POINT_DIMS[] = { 16, 18,  5,  7,  5 };
const vtkm::Id CELL_DIMS[] =  { 15, 17,  4,  6,  4 };
const vtkm::Id NUM_POINTS[] = { 0, 16, 288, 1440, 10080, 50400 };
const vtkm::Id NUM_CELLS[] =  { 0, 15, 255, 1020,  6120, 24480 };

template<vtkm::IdComponent Dimensions>
void TestDimensions(vtkm::Extent<Dimensions>)
{
  std::cout << "Testing Dimension sizes for " << Dimensions << " dimensions"
            << std::endl;

  vtkm::Extent<Dimensions> extent;
  vtkm::Tuple<vtkm::Id,Dimensions> pointDims;
  vtkm::Tuple<vtkm::Id,Dimensions> cellDims;
  vtkm::Id numPoints;
  vtkm::Id numCells;

  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
  {
    extent.Min[dimIndex] = 0;  extent.Max[dimIndex] = 10;
  }
  pointDims = vtkm::ExtentPointDimensions(extent);
  cellDims = vtkm::ExtentCellDimensions(extent);
  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
  {
    VTKM_TEST_ASSERT(pointDims[dimIndex] == 11,
                     "Got incorrect point dimensions for extent.");
    VTKM_TEST_ASSERT(cellDims[dimIndex] == 10,
                     "Got incorrect point dimensions for extent.");
  }

  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
  {
    extent.Min[dimIndex] = MIN_VALUES[dimIndex];
    extent.Max[dimIndex] = MAX_VALUES[dimIndex];
  }
  pointDims = vtkm::ExtentPointDimensions(extent);
  cellDims = vtkm::ExtentCellDimensions(extent);
  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
  {
    VTKM_TEST_ASSERT(pointDims[dimIndex] == POINT_DIMS[dimIndex],
                     "Got incorrect point dimensions for extent.");
    VTKM_TEST_ASSERT(cellDims[dimIndex] == CELL_DIMS[dimIndex],
                     "Got incorrect point dimensions for extent.");
  }
  numPoints = vtkm::ExtentNumberOfPoints(extent);
  numCells = vtkm::ExtentNumberOfCells(extent);
  VTKM_TEST_ASSERT(numPoints == NUM_POINTS[Dimensions],
                   "Got wrong number of points.");
  VTKM_TEST_ASSERT(numCells == NUM_CELLS[Dimensions],
                   "Got wrong number of cells.");
}

template<vtkm::IdComponent Dimensions>
void TryIndexConversion(const vtkm::Extent<Dimensions> &extent)
{
  typedef vtkm::Tuple<vtkm::Id,Dimensions> IdX;
  vtkm::Id lastFlatIndex;
  IdX correctTopologyIndex;

  std::cout << "  Testing point index conversion" << std::endl;
  correctTopologyIndex = IdX(100000);
  lastFlatIndex = vtkm::ExtentNumberOfPoints(extent);
  for (vtkm::Id correctFlatIndex = 0;
       correctFlatIndex < lastFlatIndex;
       correctFlatIndex++)
  {
    // Increment topology index
    for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
    {
      correctTopologyIndex[dimIndex]++;
      if (correctTopologyIndex[dimIndex] <= extent.Max[dimIndex]) { break; }
      correctTopologyIndex[dimIndex] = extent.Min[dimIndex];
      // Iterate to increment the next index.
    }

    vtkm::Id computedFlatIndex =
        vtkm::ExtentPointTopologyIndexToFlatIndex(correctTopologyIndex, extent);
    VTKM_TEST_ASSERT(computedFlatIndex == correctFlatIndex,
                     "Got incorrect flat index.");

    IdX computedTopologyIndex =
        vtkm::ExtentPointFlatIndexToTopologyIndex(correctFlatIndex, extent);
    VTKM_TEST_ASSERT(computedTopologyIndex == correctTopologyIndex,
                     "Got incorrect topology index.");
  }
  // Sanity check to make sure we got to the last topology index.
  VTKM_TEST_ASSERT(correctTopologyIndex == extent.Max,
                   "Test code error. Indexing problem.");

  std::cout << "  Testing cell index conversion" << std::endl;
  correctTopologyIndex = IdX(100000);
  lastFlatIndex = vtkm::ExtentNumberOfCells(extent);
  for (vtkm::Id correctFlatIndex = 0;
       correctFlatIndex < lastFlatIndex;
       correctFlatIndex++)
  {
    // Increment topology index
    for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
    {
      correctTopologyIndex[dimIndex]++;
      if (correctTopologyIndex[dimIndex] < extent.Max[dimIndex]) { break; }
      correctTopologyIndex[dimIndex] = extent.Min[dimIndex];
      // Iterate to increment the next index.
    }

    vtkm::Id computedFlatIndex =
        vtkm::ExtentCellTopologyIndexToFlatIndex(correctTopologyIndex, extent);
    VTKM_TEST_ASSERT(computedFlatIndex == correctFlatIndex,
                     "Got incorrect flat index.");

    IdX computedTopologyIndex =
        vtkm::ExtentCellFlatIndexToTopologyIndex(correctFlatIndex, extent);
    VTKM_TEST_ASSERT(computedTopologyIndex == correctTopologyIndex,
                     "Got incorrect topology index.");

    vtkm::Id expectedFirstPointIndex =
        vtkm::ExtentPointTopologyIndexToFlatIndex(correctTopologyIndex, extent);
    vtkm::Id computedFirstPointIndex =
        vtkm::ExtentFirstPointOnCell(correctFlatIndex, extent);
    VTKM_TEST_ASSERT(computedFirstPointIndex == expectedFirstPointIndex,
                     "Got wrong first point index.");
  }
  // Sanity check to make sure we got to the last topology index.
  VTKM_TEST_ASSERT(correctTopologyIndex == extent.Max - IdX(1),
                   "Test code error. Indexing problem.");
}

template<vtkm::IdComponent Dimensions>
void TestIndexConversion(vtkm::Extent<Dimensions>)
{
  std::cout << "Testing index conversion for " << Dimensions << " dimensions."
            << std::endl;

  vtkm::Extent<Dimensions> extent;

  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
  {
    extent.Min[dimIndex] = 0;  extent.Max[dimIndex] = 10;
  }
  TryIndexConversion(extent);

  for (vtkm::IdComponent dimIndex = 0; dimIndex < Dimensions; dimIndex++)
  {
    extent.Min[dimIndex] = MIN_VALUES[dimIndex];
    extent.Max[dimIndex] = MAX_VALUES[dimIndex];
  }
  TryIndexConversion(extent);
}

void ExtentTests()
{
  TestDimensions(vtkm::Extent<1>());
  TestDimensions(vtkm::Extent2());
  TestDimensions(vtkm::Extent3());
  TestDimensions(vtkm::Extent<5>());

  TestIndexConversion(vtkm::Extent<1>());
  TestIndexConversion(vtkm::Extent2());
  TestIndexConversion(vtkm::Extent3());
  TestIndexConversion(vtkm::Extent<5>());
}

} // anonymous namespace

int UnitTestExtent(int, char *[])
{
  return vtkm::testing::Testing::Run(ExtentTests);
}
