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
   vtkm::IdComponent bounds[6] = {1, 3, 1, 3, 0, 1};
   vtkm::IdComponent sample[3] = {1, 1, 1};
   vtkm::cont::ArrayHandle<vtkm::IdComponent> boundsArray =
                           vtkm::cont::make_ArrayHandle(bounds, 6);
   vtkm::cont::ArrayHandle<vtkm::IdComponent> sampleArray =
                           vtkm::cont::make_ArrayHandle(sample, 6);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         boundsArray,
                                         sampleArray,
                                         DeviceAdapter());
  
/*
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 5),
                     "Wrong result for Tetrahedralize filter");
*/
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
   vtkm::IdComponent bounds[6] = {0, 4, 0, 4, 1, 3};
   vtkm::IdComponent sample[3] = {2, 2, 1};
   vtkm::cont::ArrayHandle<vtkm::IdComponent> boundsArray =
                           vtkm::cont::make_ArrayHandle(bounds, 6);
   vtkm::cont::ArrayHandle<vtkm::IdComponent> sampleArray =
                           vtkm::cont::make_ArrayHandle(sample, 6);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         boundsArray,
                                         sampleArray,
                                         DeviceAdapter());
  
/*
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 5),
                     "Wrong result for Tetrahedralize filter");
*/
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
   vtkm::IdComponent bounds[6] = {0, 2, 0, 2, 0, 1};
   vtkm::IdComponent sample[3] = {1, 1, 1};
   vtkm::cont::ArrayHandle<vtkm::IdComponent> boundsArray =
                           vtkm::cont::make_ArrayHandle(bounds, 6);
   vtkm::cont::ArrayHandle<vtkm::IdComponent> sampleArray =
                           vtkm::cont::make_ArrayHandle(sample, 6);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         boundsArray,
                                         sampleArray,
                                         DeviceAdapter());
  
/*
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 5),
                     "Wrong result for Tetrahedralize filter");
*/
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
   vtkm::IdComponent bounds[6] = {0, 2, 0, 2, 0, 2};
   vtkm::IdComponent sample[3] = {1, 1, 1};
   vtkm::cont::ArrayHandle<vtkm::IdComponent> boundsArray =
                           vtkm::cont::make_ArrayHandle(bounds, 6);
   vtkm::cont::ArrayHandle<vtkm::IdComponent> sampleArray =
                           vtkm::cont::make_ArrayHandle(sample, 6);
  
    // Extract subset
    vtkm::worklet::ExtractStructured worklet;
    vtkm::cont::DataSet outDataSet = worklet.Run(
                                         cellSet,
                                         dataSet.GetCoordinateSystem(0),
                                         boundsArray,
                                         sampleArray,
                                         DeviceAdapter());
  
/*
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), cellSet.GetNumberOfCells() * 5),
                     "Wrong result for Tetrahedralize filter");
*/
  }

  void operator()() const
  {
    TestUniform2D();
    TestUniform3D();
    TestRectilinear3D();
    TestRectilinear2D();
  }
};

int UnitTestExtractStructured(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
    TestingExtractStructured<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
