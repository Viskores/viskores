//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/connected_components/CellSetConnectivity.h>
#include <vtkm/filter/contour/Contour.h>

#include <vtkm/source/Tangle.h>

namespace
{

class TestCellSetConnectivity
{
public:
  static void TestTangleIsosurface()
  {
    vtkm::source::Tangle tangle;
    tangle.SetCellDimensions({ 4, 4, 4 });
    vtkm::cont::DataSet dataSet = tangle.Execute();

    vtkm::filter::contour::Contour filter;
    filter.SetGenerateNormals(true);
    filter.SetMergeDuplicatePoints(true);
    filter.SetIsoValue(0, 0.1);
    filter.SetActiveField("tangle");
    vtkm::cont::DataSet iso = filter.Execute(dataSet);

    vtkm::filter::connected_components::CellSetConnectivity connectivity;
    const vtkm::cont::DataSet output = connectivity.Execute(iso);

    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 8,
                     "Wrong number of connected components");
  }

  static void TestExplicitDataSet()
  {
    vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

    vtkm::filter::connected_components::CellSetConnectivity connectivity;
    const vtkm::cont::DataSet output = connectivity.Execute(dataSet);

    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  static void TestUniformDataSet()
  {
    vtkm::cont::DataSet dataSet = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet1();
    vtkm::filter::connected_components::CellSetConnectivity connectivity;
    const vtkm::cont::DataSet output = connectivity.Execute(dataSet);

    vtkm::cont::ArrayHandle<vtkm::Id> componentArray;
    auto temp = output.GetField("component").GetData();
    temp.AsArrayHandle(componentArray);

    using Algorithm = vtkm::cont::Algorithm;
    Algorithm::Sort(componentArray);
    Algorithm::Unique(componentArray);
    VTKM_TEST_ASSERT(componentArray.GetNumberOfValues() == 1,
                     "Wrong number of connected components");
  }

  void operator()() const
  {
    TestCellSetConnectivity::TestTangleIsosurface();
    TestCellSetConnectivity::TestExplicitDataSet();
    TestCellSetConnectivity::TestUniformDataSet();
  }
};
}

int UnitTestCellSetConnectivityFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellSetConnectivity(), argc, argv);
}
