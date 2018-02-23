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

#include <vtkm/worklet/ExtractStructured.h>

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
    std::cout << "Testing extract structured uniform 2D" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<2>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // RangeId3 and subsample
    vtkm::RangeId3 range(1, 4, 1, 4, 0, 1);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestUniform3D() const
  {
    std::cout << "Testing extract structured uniform 3D" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    vtkm::worklet::ExtractStructured worklet;
    vtkm::worklet::ExtractStructured::DynamicCellSetStructured outCellSet;

    // RangeId3 within dataset
    vtkm::RangeId3 range0(1, 4, 1, 4, 1, 4);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    outCellSet = worklet.Run(cellSet, range0, sample, includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 surrounds dataset
    vtkm::RangeId3 range1(-1, 8, -1, 8, -1, 8);
    outCellSet = worklet.Run(cellSet, range1, sample, includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 125),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 64),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset on near boundary
    vtkm::RangeId3 range2(-1, 3, -1, 3, -1, 3);
    outCellSet = worklet.Run(cellSet, range2, sample, includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset on far boundary
    vtkm::RangeId3 range3(1, 8, 1, 8, 1, 8);
    outCellSet = worklet.Run(cellSet, range3, sample, includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 64),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset without corner
    vtkm::RangeId3 range4(2, 8, 1, 4, 1, 4);
    outCellSet = worklet.Run(cellSet, range4, sample, includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset with plane
    vtkm::RangeId3 range5(2, 8, 1, 2, 1, 4);
    outCellSet = worklet.Run(cellSet, range5, sample, includeBoundary, DeviceAdapter());
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestUniform3D1() const
  {
    std::cout << "Testing extract structured uniform with sampling" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    vtkm::worklet::ExtractStructured worklet;
    vtkm::worklet::ExtractStructured::DynamicCellSetStructured outCellSet;

    // RangeId3 within data set with sampling
    vtkm::RangeId3 range0(0, 5, 0, 5, 1, 4);
    vtkm::Id3 sample0(2, 2, 1);
    bool includeBoundary0 = false;

    outCellSet = worklet.Run(cellSet, range0, sample0, includeBoundary0, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 and subsample
    vtkm::RangeId3 range1(0, 5, 0, 5, 1, 4);
    vtkm::Id3 sample1(3, 3, 2);
    bool includeBoundary1 = false;

    outCellSet = worklet.Run(cellSet, range1, sample1, includeBoundary1, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 and subsample
    vtkm::RangeId3 range2(0, 5, 0, 5, 1, 4);
    vtkm::Id3 sample2(3, 3, 2);
    bool includeBoundary2 = true;

    outCellSet = worklet.Run(cellSet, range2, sample2, includeBoundary2, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 18),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestRectilinear2D() const
  {
    std::cout << "Testing extract structured rectilinear" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<2>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DRectilinearDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // RangeId3 and subsample
    vtkm::RangeId3 range(0, 2, 0, 2, 0, 1);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 4),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestRectilinear3D() const
  {
    std::cout << "Testing extract structured rectilinear" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;

    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DRectilinearDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // RangeId3 and subsample
    vtkm::RangeId3 range(0, 2, 0, 2, 0, 2);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;

    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, DeviceAdapter());

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 1),
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
