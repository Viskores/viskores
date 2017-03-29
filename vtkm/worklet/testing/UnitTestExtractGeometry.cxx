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

#include <vtkm/worklet/ExtractGeometry.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CellSet.h>

#include <algorithm>
#include <iostream>
#include <vector>

using vtkm::cont::testing::MakeTestDataSet;

template <typename DeviceAdapter>
class TestingExtractGeometry
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicitById() const
  {
    std::cout << "Testing extract cell explicit by id:" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);

    // Cells to extract
    const int nCells = 2;
    vtkm::Id cellids[nCells] = {1, 2};
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds =
                            vtkm::cont::make_ArrayHandle(cellids, nCells);
  
    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.Run(cellSet,
                            cellIds,
                            DeviceAdapter());

    vtkm::cont::Field cellField =
        extractGeometry.ProcessCellField(dataset.GetField("cellvar"));
    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 110.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExplicitByBox() const
  {
    std::cout << "Testing extract cells with implicit function (box) on explicit:" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);

    // Implicit function
    vtkm::Vec<vtkm::FloatDefault, 3> minPoint(0.5, 0.0, 0.0);
    vtkm::Vec<vtkm::FloatDefault, 3> maxPoint(2.0, 2.0, 2.0);
    vtkm::Box box(minPoint, maxPoint);
  
    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.Run(cellSet,
                            box,
                            dataset.GetCoordinateSystem("coordinates"),
                            DeviceAdapter());

    vtkm::cont::Field cellField =
        extractGeometry.ProcessCellField(dataset.GetField("cellvar"));
    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 2), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 2 &&
                     cellFieldArray.GetPortalConstControl().Get(1) == 120.2f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformById2D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    typedef vtkm::cont::CellSetStructured<2> CellSetType;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Cells to extract
    const int nCells = 5;
    vtkm::Id cellids[nCells] = {0, 4, 5, 10, 15};
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds =
                            vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set permutation of with only extracted cells
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.Run(cellSet,
                            cellIds,
                            DeviceAdapter());

    vtkm::cont::Field cellField =
        extractGeometry.ProcessCellField(dataset.GetField("cellvar"));
    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                     cellFieldArray.GetPortalConstControl().Get(1) == 4.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformById3D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Cells to extract
    const int nCells = 5;
    vtkm::Id cellids[nCells] = {0, 4, 5, 10, 15};
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds =
                            vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.Run(cellSet,
                            cellIds,
                            DeviceAdapter());

    vtkm::cont::Field cellField =
        extractGeometry.ProcessCellField(dataset.GetField("cellvar"));
    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == nCells &&
                     cellFieldArray.GetPortalConstControl().Get(2) == 5.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformByBox() const
  {
    std::cout << "Testing extract cells with implicit function (box):" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Implicit function
    vtkm::Vec<vtkm::FloatDefault, 3> minPoint(1.0, 1.0, 1.0);
    vtkm::Vec<vtkm::FloatDefault, 3> maxPoint(3.0, 3.0, 3.0);
    vtkm::Box box(minPoint, maxPoint);
  
    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.Run(cellSet,
                            box,
                            dataset.GetCoordinateSystem("coords"),
                            DeviceAdapter());

    vtkm::cont::Field cellField =
        extractGeometry.ProcessCellField(dataset.GetField("cellvar"));
    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                     cellFieldArray.GetPortalConstControl().Get(0) == 21.f,
                     "Wrong cell field data");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestUniformBySphere() const
  {
    std::cout << "Testing extract cells with implicit function (sphere):" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> CellSetType;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutCellSetType;
    typedef vtkm::cont::ArrayHandlePermutation<
      vtkm::cont::ArrayHandle<vtkm::Id>,
      vtkm::cont::ArrayHandle<vtkm::Float32> > OutCellFieldArrayHandleType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Implicit function
    vtkm::Vec<vtkm::FloatDefault, 3> center(2, 2, 2);
    vtkm::FloatDefault radius(FloatDefault(1.8));
    vtkm::Sphere sphere(center, radius);
  
    // Output data set with cell set containing extracted cells
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.Run(cellSet,
                            sphere,
                            dataset.GetCoordinateSystem("coords"),
                            DeviceAdapter());

    vtkm::cont::Field cellField =
        extractGeometry.ProcessCellField(dataset.GetField("cellvar"));
    OutCellFieldArrayHandleType cellFieldArray;
    cellField.GetData().CopyTo(cellFieldArray);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(cellFieldArray.GetNumberOfValues() == 8 &&
                     cellFieldArray.GetPortalConstControl().Get(1) == 22.f,
                     "Wrong cell field data");
  }

  void operator()() const
  {
    this->TestExplicitById();
    //this->TestExplicitByBox();
    this->TestUniformById2D();
    this->TestUniformById3D();
    //this->TestUniformByBox();
    this->TestUniformBySphere();
  }
};

int UnitTestExtractGeometry(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
      TestingExtractGeometry<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
