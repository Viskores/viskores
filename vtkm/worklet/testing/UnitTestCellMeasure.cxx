//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/CellMeasure.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void TestCellMeasureUniform3D()
{
  std::cout << "Testing CellMeasure Worklet on 3D structured data" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;
  vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellMeasure<vtkm::Volume>> dispatcher;
  dispatcher.Invoke(dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), result);

  vtkm::Float32 expected[4] = { 1.f, 1.f, 1.f, 1.f };
  for (int i = 0; i < 4; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(vtkm::Id(i)), expected[i]),
                     "Wrong result for CellMeasure worklet on 3D uniform data");
  }
}

template <typename IntegrationType>
void TestCellMeasureWorklet(vtkm::cont::DataSet& dataset,
                            const char* msg,
                            const std::vector<vtkm::Float32>& expected,
                            const IntegrationType&)
{
  std::cout << "Testing CellMeasures Filter on " << msg << "\n";

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> result;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellMeasure<IntegrationType>> dispatcher;
  dispatcher.Invoke(dataset.GetCellSet(), dataset.GetCoordinateSystem(), result);

  VTKM_TEST_ASSERT(result.GetNumberOfValues() == static_cast<vtkm::Id>(expected.size()),
                   "Wrong number of values in the output array");

  for (unsigned int i = 0; i < static_cast<unsigned int>(expected.size()); ++i)
  {
    VTKM_TEST_ASSERT(test_equal(result.GetPortalConstControl().Get(vtkm::Id(i)), expected[i]),
                     "Wrong result for CellMeasure filter");
  }
}

void TestCellMeasure()
{
  using vtkm::ArcLength;
  using vtkm::Area;
  using vtkm::Volume;
  using vtkm::AllMeasures;

  TestCellMeasureUniform3D();

  vtkm::cont::testing::MakeTestDataSet factory;
  vtkm::cont::DataSet data;

  data = factory.Make3DExplicitDataSet2();
  TestCellMeasureWorklet(data, "explicit dataset 2", { -1.f }, Volume());

  data = factory.Make3DExplicitDataSet3();
  TestCellMeasureWorklet(data, "explicit dataset 3", { -1.f / 6.f }, Volume());

  data = factory.Make3DExplicitDataSet4();
  TestCellMeasureWorklet(data, "explicit dataset 4", { -1.f, -1.f }, Volume());

  data = factory.Make3DExplicitDataSet5();
  TestCellMeasureWorklet(
    data, "explicit dataset 5", { 1.f, 1.f / 3.f, 1.f / 6.f, -1.f / 2.f }, Volume());

  data = factory.Make3DExplicitDataSet6();
  TestCellMeasureWorklet(
    data,
    "explicit dataset 6 (all)",
    { 0.999924f, 0.999924f, 0.f, 0.f, 3.85516f, 1.00119f, 0.083426f, 0.25028f },
    AllMeasures());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (arc length)",
                         { 0.999924f, 0.999924f, 0.f, 0.f, 0.0f, 0.0f, 0.0f, 0.0f },
                         ArcLength());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (area)",
                         { 0.0f, 0.0f, 0.f, 0.f, 3.85516f, 1.00119f, 0.0f, 0.0f },
                         Area());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (volume)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.083426f, 0.25028f },
                         Volume());
  TestCellMeasureWorklet(data,
                         "explicit dataset 6 (empty)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.0f, 0.0f },
                         vtkm::List<>());
}
}

int UnitTestCellMeasure(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellMeasure, argc, argv);
}
