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

#include <vtkm/worklet/ExtractStructured.h>

#include <vtkm/Bounds.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

template <typename DeviceAdapter>
class TestingExtractStructured
{
public:
  void TestUniform2D() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    typedef vtkm::cont::CellSetStructured<2> CellSetType;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // Bounds and subsample
    vtkm::Bounds bounds(1, 3, 1, 3, 0, 0);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds,
                                                 sample, includeBoundary, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing extract structured uniform" << std::endl;
    typedef vtkm::cont::CellSetStructured<3> CellSetType;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet;

    // Bounding box within dataset
    vtkm::Bounds bounds0(1, 3, 1, 3, 1, 3);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds0, sample,
                             includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // Bounding box surrounds dataset
    vtkm::Bounds bounds1(-1, 7, -1, 7, -1, 7);
    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds1, sample,
                             includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 125),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 64),
                     "Wrong result for ExtractStructured worklet");

    // Bounding box intersects dataset on near boundary
    vtkm::Bounds bounds2(-1, 2, -1, 2, -1, 2);
    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds2, sample,
                             includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // Bounding box intersects dataset on far boundary
    vtkm::Bounds bounds3(1, 7, 1, 7, 1, 7);
    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds3, sample,
                             includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 64),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 27),
                     "Wrong result for ExtractStructured worklet");

    // Bounding box intersects dataset without corner
    vtkm::Bounds bounds4(2, 7, 1, 3, 1, 3);
    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds4, sample,
                             includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // Bounding box intersects dataset with plane
    vtkm::Bounds bounds5(2, 7, 1, 1, 1, 3);
    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds5, sample,
                             includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestUniform3D1() const
  {
    std::cout << "Testing extract structured uniform with sampling" << std::endl;
    typedef vtkm::cont::CellSetStructured<3> CellSetType;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet;

    // Bounding box within data set with sampling
    vtkm::Bounds bounds0(0, 4, 0, 4, 1, 3);
    vtkm::Id3 sample0(2, 2, 1);
    bool includeBoundary0 = false;

    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds0, sample0,
                             includeBoundary0, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // Bounds and subsample
    vtkm::Bounds bounds1(0, 4, 0, 4, 1, 3);
    vtkm::Id3 sample1(3, 3, 2);
    bool includeBoundary1 = false;

    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds1, sample1,
                             includeBoundary1, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");

    // Bounds and subsample
    vtkm::Bounds bounds2(0, 4, 0, 4, 1, 3);
    vtkm::Id3 sample2(3, 3, 2);
    bool includeBoundary2 = true;

    outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds2, sample2,
                             includeBoundary2, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 18),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestRectilinear2D() const
  {
    std::cout << "Testing extract structured rectilinear" << std::endl;
    typedef vtkm::cont::CellSetStructured<2> CellSetType;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DRectilinearDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // Bounds and subsample
    vtkm::Bounds bounds(0, 1, 0, 1, 0, 0);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds,
                                                 sample, includeBoundary, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 4),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestRectilinear3D() const
  {
    std::cout << "Testing extract structured rectilinear" << std::endl;
    typedef vtkm::cont::CellSetStructured<3> CellSetType;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DRectilinearDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // Bounds and subsample
    vtkm::Bounds bounds(0, 1, 0, 1, 0, 1);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(cellSet, dataSet.GetCoordinateSystem(0), bounds,
                                                 sample, includeBoundary, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");
  }

  void operator()() const
  {
    TestUniform2D();
    TestUniform3D();
    TestUniform3D1();
    TestRectilinear2D();
    TestRectilinear3D();
  }
};

int UnitTestExtractStructured(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(
    TestingExtractStructured<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
