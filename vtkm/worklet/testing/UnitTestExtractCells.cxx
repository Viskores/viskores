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
class TestingExtractCells
{
public:
  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExtractCellsExplicitById() const
  {
    std::cout << "Testing extract cell explicit by id:" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);

    // Cells to extract
    const int nCells = 2;
    vtkm::Id cellids[nCells] = {1, 2};
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds =
                            vtkm::cont::make_ArrayHandle(cellids, nCells);
  
    // Output dataset contains input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    CellSetType outCellSet = 
        extractGeometry.RunExtractCellsExplicit(cellSet,
                                                cellIds,
                                                DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                                vtkm::TopologyElementTagCell()).
                     GetNumberOfValues(), 9), "Wrong result for ExtractCells");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExtractCellsExplicitWithBox() const
  {
    std::cout << "Testing extract cells with implicit function (box) on explicit:" << std::endl;

    typedef vtkm::cont::CellSetExplicit<> CellSetType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DExplicitDataSet5();
    CellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);

    // Implicit function
    vtkm::Vec<vtkm::FloatDefault, 3> minPoint(0.5, 0.0, 0.0);
    vtkm::Vec<vtkm::FloatDefault, 3> maxPoint(2.0, 2.0, 2.0);
    vtkm::Box box(minPoint, maxPoint);
  
    // Output dataset contains input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    CellSetType outCellSet = 
        extractGeometry.RunExtractCellsExplicit(cellSet,
                                                box,
                                                dataset.GetCoordinateSystem("coordinates"),
                                                DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 2), "Wrong result for ExtractCells");
    VTKM_TEST_ASSERT(test_equal(outCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                                vtkm::TopologyElementTagCell()).
                     GetNumberOfValues(), 9), "Wrong result for ExtractCells");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExtractCellsStructuredById2D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    typedef vtkm::cont::CellSetStructured<2> InCellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make2DUniformDataSet1();
    InCellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Cells to extract
    const int nCells = 5;
    vtkm::Id cellids[nCells] = {0, 4, 5, 10, 15};
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds =
                            vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output dataset contains input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.RunExtractCellsStructured(cellSet,
                                                  cellIds,
                                                  DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells), "Wrong result for ExtractCells");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExtractCellsStructuredById3D() const
  {
    std::cout << "Testing extract cells structured by id:" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> InCellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    InCellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Cells to extract
    const int nCells = 5;
    vtkm::Id cellids[nCells] = {0, 4, 5, 10, 15};
    vtkm::cont::ArrayHandle<vtkm::Id> cellIds =
                            vtkm::cont::make_ArrayHandle(cellids, nCells);

    // Output dataset contains input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted cells and all points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.RunExtractCellsStructured(cellSet,
                                                  cellIds,
                                                  DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), nCells), "Wrong result for ExtractCells");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExtractCellsStructuredWithBox() const
  {
    std::cout << "Testing extract cells with implicit function (box):" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> InCellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    InCellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Implicit function
    vtkm::Vec<vtkm::FloatDefault, 3> minPoint(1.0, 1.0, 1.0);
    vtkm::Vec<vtkm::FloatDefault, 3> maxPoint(3.0, 3.0, 3.0);
    vtkm::Box box(minPoint, maxPoint);
  
    // Output dataset contains input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted points
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.RunExtractCellsStructured(cellSet,
                                                  box,
                                                  dataset.GetCoordinateSystem("coords"),
                                                  DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  void TestExtractCellsStructuredWithSphere() const
  {
    std::cout << "Testing extract cells with implicit function (sphere):" << std::endl;

    typedef vtkm::cont::CellSetStructured<3> InCellSetType;
    typedef vtkm::cont::CellSetSingleType<> OutCellSetType;

    // Input data set created
    vtkm::cont::DataSet dataset = MakeTestDataSet().Make3DUniformDataSet1();
    InCellSetType cellSet;
    dataset.GetCellSet(0).CopyTo(cellSet);
  
    // Implicit function
    vtkm::Vec<vtkm::FloatDefault, 3> center(2, 2, 2);
    vtkm::FloatDefault radius(1.8);
    vtkm::Sphere sphere(center, radius);
  
    // Output dataset contains input coordinate system
    vtkm::cont::DataSet outDataSet;
    outDataSet.AddCoordinateSystem(dataset.GetCoordinateSystem(0));

    // Output data set with cell set containing extracted cells
    vtkm::worklet::ExtractGeometry extractGeometry;
    OutCellSetType outCellSet = 
        extractGeometry.RunExtractCellsStructured(cellSet,
                                                  sphere,
                                                  dataset.GetCoordinateSystem("coords"),
                                                  DeviceAdapter());
    outDataSet.AddCellSet(outCellSet);

    VTKM_TEST_ASSERT(test_equal(outCellSet.GetNumberOfCells(), 8), "Wrong result for ExtractCells");
  }

  void operator()() const
  {
    this->TestExtractCellsExplicitById();
    this->TestExtractCellsExplicitWithBox();
    this->TestExtractCellsStructuredById2D();
    this->TestExtractCellsStructuredById3D();
    this->TestExtractCellsStructuredWithBox();
    this->TestExtractCellsStructuredWithSphere();
  }
};

int UnitTestExtractCells(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
      TestingExtractCells<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
