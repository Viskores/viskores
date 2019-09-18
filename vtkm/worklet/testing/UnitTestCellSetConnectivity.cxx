//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/Contour.h>

#include <vtkm/cont/ArrayCopy.h>

#include <vtkm/worklet/connectivities/CellSetConnectivity.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/source/Tangle.h>

class TestCellSetConnectivity
{
public:
  void TestTangleIsosurface() const
  {
    vtkm::Id3 dims(4, 4, 4);
    vtkm::source::Tangle tangle(dims);
    vtkm::cont::DataSet dataSet = tangle.Execute();

    vtkm::filter::Contour filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.1);
    filter.SetActiveField("nodevar");
    vtkm::cont::DataSet outputData = filter.Execute(dataSet);

    auto cellSet = outputData.GetCellSet().Cast<vtkm::cont::CellSetSingleType<>>();
    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    vtkm::worklet::connectivity::CellSetConnectivity().Run(cellSet, componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 8,
                     "Wrong number of connected components");
  }

  void TestExplicitDataSet() const
  {
    vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

    auto cellSet = dataSet.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    vtkm::worklet::connectivity::CellSetConnectivity().Run(cellSet, componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void TestUniformDataSet() const
  {
    vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();

    auto cellSet = dataSet.GetCellSet();
    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    vtkm::worklet::connectivity::CellSetConnectivity().Run(cellSet, componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void operator()() const
  {
    this->TestTangleIsosurface();
    this->TestExplicitDataSet();
    this->TestUniformDataSet();
  }
};

int UnitTestCellSetConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetConnectivity(), argc, argv);
}
