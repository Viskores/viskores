//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/ExtractStructured.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using vtkm::cont::testing::MakeTestDataSet;

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
    dataSet.GetCellSet().CopyTo(cellSet);

    // RangeId3 and subsample
    vtkm::RangeId3 range(1, 4, 1, 4, 0, 1);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;
    bool includeOffset = false;

    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);

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
    dataSet.GetCellSet().CopyTo(cellSet);

    vtkm::worklet::ExtractStructured worklet;
    vtkm::worklet::ExtractStructured::DynamicCellSetStructured outCellSet;

    // RangeId3 within dataset
    vtkm::RangeId3 range0(1, 4, 1, 4, 1, 4);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;
    bool includeOffset = false;

    outCellSet = worklet.Run(cellSet, range0, sample, includeBoundary, includeOffset);
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 surrounds dataset
    vtkm::RangeId3 range1(-1, 8, -1, 8, -1, 8);
    outCellSet = worklet.Run(cellSet, range1, sample, includeBoundary, includeOffset);
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 125),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 64),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset on near boundary
    vtkm::RangeId3 range2(-1, 3, -1, 3, -1, 3);
    outCellSet = worklet.Run(cellSet, range2, sample, includeBoundary, includeOffset);
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset on far boundary
    vtkm::RangeId3 range3(1, 8, 1, 8, 1, 8);
    outCellSet = worklet.Run(cellSet, range3, sample, includeBoundary, includeOffset);
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 64),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 27),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset without corner
    vtkm::RangeId3 range4(2, 8, 1, 4, 1, 4);
    outCellSet = worklet.Run(cellSet, range4, sample, includeBoundary, includeOffset);
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 intersects dataset with plane
    vtkm::RangeId3 range5(2, 8, 1, 2, 1, 4);
    outCellSet = worklet.Run(cellSet, range5, sample, includeBoundary, includeOffset);
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
    dataSet.GetCellSet().CopyTo(cellSet);

    vtkm::worklet::ExtractStructured worklet;
    vtkm::worklet::ExtractStructured::DynamicCellSetStructured outCellSet;

    // RangeId3 within data set with sampling
    vtkm::RangeId3 range0(0, 5, 0, 5, 1, 4);
    vtkm::Id3 sample0(2, 2, 1);
    bool includeBoundary0 = false;
    bool includeOffset = false;

    outCellSet = worklet.Run(cellSet, range0, sample0, includeBoundary0, includeOffset);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 27),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 and subsample
    vtkm::RangeId3 range1(0, 5, 0, 5, 1, 4);
    vtkm::Id3 sample1(3, 3, 2);
    bool includeBoundary1 = false;

    outCellSet = worklet.Run(cellSet, range1, sample1, includeBoundary1, includeOffset);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");

    // RangeId3 and subsample
    vtkm::RangeId3 range2(0, 5, 0, 5, 1, 4);
    vtkm::Id3 sample2(3, 3, 2);
    bool includeBoundary2 = true;

    outCellSet = worklet.Run(cellSet, range2, sample2, includeBoundary2, includeOffset);

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
    dataSet.GetCellSet().CopyTo(cellSet);

    // RangeId3 and subsample
    vtkm::RangeId3 range(0, 2, 0, 2, 0, 1);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;
    bool includeOffset = false;

    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);

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
    dataSet.GetCellSet().CopyTo(cellSet);

    // RangeId3 and subsample
    vtkm::RangeId3 range(0, 2, 0, 2, 0, 2);
    vtkm::Id3 sample(1, 1, 1);
    bool includeBoundary = false;
    bool includeOffset = false;

    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestOffset3D1() const
  {
    std::cout << "Testing offset 3D-1" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;

    CellSetType cellSet;

    // RangeID3 and subsample
    vtkm::RangeId3 range(5, 15, 0, 10, 0, 10);
    vtkm::Id3 sample(1, 1, 1);
    vtkm::Id3 test_offset(10, 0, 0);
    vtkm::Id3 no_offset(0, 0, 0);
    vtkm::Id3 new_dims(5, 10, 10);
    bool includeBoundary = false;
    bool includeOffset = false;
    cellSet.SetPointDimensions(vtkm::make_Vec(10, 10, 10));

    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);

    VTKM_TEST_ASSERT(test_equal(cellSet.GetGlobalPointIndexStart(), no_offset));
    vtkm::Id3 cellDims =
      outCellSet.Cast<CellSetType>().GetSchedulingRange(vtkm::TopologyElementTagCell());

    includeOffset = true;
    cellSet.SetGlobalPointIndexStart(test_offset);
    outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);
    cellDims = outCellSet.Cast<CellSetType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    CellSetType cs = outCellSet.Cast<CellSetType>();
    cellDims = cs.GetPointDimensions();
    VTKM_TEST_ASSERT(test_equal(cellDims, new_dims));
    VTKM_TEST_ASSERT(test_equal(cellSet.GetGlobalPointIndexStart(), test_offset));
  }

  void TestOffset3D2() const
  {
    std::cout << "Testing Offset 3D-2" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;
    CellSetType cellSet;
    vtkm::RangeId3 range(15, 20, 0, 10, 0, 10);
    vtkm::Id3 sample(1, 1, 1);
    vtkm::Id3 test_dims(5, 10, 10);
    vtkm::Id3 gpis(10, 0, 0);
    vtkm::Id3 test_offset(15, 0, 0);
    bool includeBoundary = false;
    bool includeOffset = true;
    cellSet.SetPointDimensions(vtkm::make_Vec(10, 10, 10));
    cellSet.SetGlobalPointIndexStart(gpis);
    vtkm::worklet::ExtractStructured worklet;

    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);
    CellSetType cs = outCellSet.Cast<CellSetType>();
    vtkm::Id3 cellDims = cs.GetPointDimensions();
    VTKM_TEST_ASSERT(test_equal(cellDims, test_dims));
    VTKM_TEST_ASSERT(test_equal(cs.GetGlobalPointIndexStart(), test_offset));
  }

  void TestOffset3D3() const
  {
    std::cout << "Testing Offset 3D-3" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<3>;
    CellSetType cellSet;
    vtkm::RangeId3 range(100, 110, 0, 10, 0, 10);
    vtkm::Id3 sample(1, 1, 1);
    vtkm::Id3 test_dims(0, 0, 0);
    bool includeBoundary = false;
    bool includeOffset = true;
    cellSet.SetPointDimensions(vtkm::make_Vec(10, 10, 10));
    vtkm::worklet::ExtractStructured worklet;

    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);
    CellSetType cs = outCellSet.Cast<CellSetType>();
    VTKM_TEST_ASSERT(test_equal(cs.GetPointDimensions(), test_dims));
  }
  void TestOffset2D() const
  {
    std::cout << "Testing offset 2D" << std::endl;
    using CellSetType = vtkm::cont::CellSetStructured<2>;
    CellSetType cellSet;
    // RangeID3 and subsample
    vtkm::RangeId3 range(5, 15, 0, 10, 0, 1);
    vtkm::Id3 sample(1, 1, 1);
    vtkm::Id2 test_offset(10, 0);
    vtkm::Id2 no_offset(0, 0);
    vtkm::Id2 new_dims(5, 10);
    bool includeBoundary = false;
    bool includeOffset = false;
    cellSet.SetPointDimensions(vtkm::make_Vec(10, 10));
    vtkm::worklet::ExtractStructured worklet;
    auto outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);
    VTKM_TEST_ASSERT(test_equal(cellSet.GetGlobalPointIndexStart(), no_offset));
    vtkm::Id2 cellDims =
      outCellSet.Cast<CellSetType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    // Test with offset now
    includeOffset = true;
    cellSet.SetGlobalPointIndexStart(test_offset);
    outCellSet = worklet.Run(cellSet, range, sample, includeBoundary, includeOffset);
    cellDims = outCellSet.Cast<CellSetType>().GetSchedulingRange(vtkm::TopologyElementTagCell());
    CellSetType cs = outCellSet.Cast<CellSetType>();
    cellDims = cs.GetPointDimensions();
    VTKM_TEST_ASSERT(test_equal(cellDims, new_dims));
    VTKM_TEST_ASSERT(test_equal(cellSet.GetGlobalPointIndexStart(), test_offset));
  }

  void operator()() const
  {
    TestUniform2D();
    TestUniform3D();
    TestUniform3D1();
    TestRectilinear2D();
    TestRectilinear3D();
    TestOffset3D1();
    TestOffset3D2();
    TestOffset3D3();
    TestOffset2D();
  }
};

int UnitTestExtractStructured(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestingExtractStructured(), argc, argv);
}
