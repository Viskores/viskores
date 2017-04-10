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
    std::cout << std::endl << std::endl;
    std::cout << "Testing extract structured uniform" << std::endl;
    typedef vtkm::cont::CellSetStructured<2> CellSetType;
  
    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

   // Bounds and subsample
   vtkm::Id3 minBound(1,1,0);
   vtkm::Id3 maxBound(3,3,1);
   vtkm::Id3 sample(1,1,1);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         minBound,
                                         maxBound,
                                         sample,
                                         DeviceAdapter());
  
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 9),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 4),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestUniform3D() const
  {
    std::cout << std::endl << std::endl;
    std::cout << "Testing extract structured uniform" << std::endl;
    typedef vtkm::cont::CellSetStructured<3> CellSetType;
  
    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

   // Bounds and subsample
   vtkm::Id3 minBound(0,0,1);
   vtkm::Id3 maxBound(4,4,3);
   vtkm::Id3 sample(1,1,1);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         minBound,
                                         maxBound,
                                         sample,
                                         DeviceAdapter());
  
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 75),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 32),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestRectilinear2D() const
  {
    std::cout << std::endl << std::endl;
    std::cout << "Testing extract structured rectilinear" << std::endl;
    typedef vtkm::cont::CellSetStructured<2> CellSetType;
  
    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DRectilinearDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

    // Bounds and subsample
    vtkm::Id3 minBound(0,0,0);
    vtkm::Id3 maxBound(1,1,0);
    vtkm::Id3 sample(1,1,1);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         minBound,
                                         maxBound,
                                         sample,
                                         DeviceAdapter());
  
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 4),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");
  }

  void TestRectilinear3D() const
  {
    std::cout << std::endl << std::endl;
    std::cout << "Testing extract structured rectilinear" << std::endl;
    typedef vtkm::cont::CellSetStructured<3> CellSetType;
  
    // Create the input uniform cell set
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DRectilinearDataSet0();
    CellSetType cellSet;
    dataSet.GetCellSet(0).CopyTo(cellSet);

   // Bounds and subsample
   vtkm::Id3 minBound(0,0,0);
   vtkm::Id3 maxBound(1,1,1);
   vtkm::Id3 sample(1,1,1);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         minBound,
                                         maxBound,
                                         sample,
                                         DeviceAdapter());
  
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfPoints(), 8),
                     "Wrong result for ExtractStructured worklet");
    VTKM_TEST_ASSERT(test_equal(outDataSet.GetCellSet(0).GetNumberOfCells(), 1),
                     "Wrong result for ExtractStructured worklet");
  }

  void operator()() const
  {
    TestUniform2D();
    TestUniform3D();
    TestRectilinear2D();
    TestRectilinear3D();
  }
};

int UnitTestExtractStructured(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
    TestingExtractStructured<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
