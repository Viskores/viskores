//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/CellMeasures.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

template <typename IntegrationType>
void TestCellMeasuresFilter(vtkm::cont::DataSet& dataset,
                            const char* msg,
                            const std::vector<vtkm::Float32>& expected,
                            const IntegrationType&)
{
  std::cout << "Testing CellMeasures Filter on " << msg << "\n";

  vtkm::filter::CellMeasures<IntegrationType> vols;
  vtkm::cont::DataSet outputData = vols.Execute(dataset);

  VTKM_TEST_ASSERT(vols.GetCellMeasureName().empty(), "Default output field name should be empty.");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");
  VTKM_TEST_ASSERT(outputData.GetNumberOfCells() == static_cast<vtkm::Id>(expected.size()),
                   "Wrong number of cells in the output dataset");

  // Check that the empty measure name above produced a field with the expected name.
  vols.SetCellMeasureName("measure");
  auto temp = outputData.GetField(vols.GetCellMeasureName()).GetData();
  VTKM_TEST_ASSERT(temp.GetNumberOfValues() == static_cast<vtkm::Id>(expected.size()),
                   "Output field could not be found or was improper.");

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> resultArrayHandle;
  temp.CopyTo(resultArrayHandle);
  VTKM_TEST_ASSERT(resultArrayHandle.GetNumberOfValues() == static_cast<vtkm::Id>(expected.size()),
                   "Wrong number of entries in the output dataset");

  for (unsigned int i = 0; i < static_cast<unsigned int>(expected.size()); ++i)
  {
    VTKM_TEST_ASSERT(
      test_equal(resultArrayHandle.GetPortalConstControl().Get(vtkm::Id(i)), expected[i]),
      "Wrong result for CellMeasure filter");
  }
}

void TestCellMeasures()
{
  using vtkm::Volume;
  using vtkm::AllMeasures;

  vtkm::cont::testing::MakeTestDataSet factory;
  vtkm::cont::DataSet data;

  data = factory.Make3DExplicitDataSet2();
  TestCellMeasuresFilter(data, "explicit dataset 2", { -1.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet3();
  TestCellMeasuresFilter(data, "explicit dataset 3", { -1.f / 6.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet4();
  TestCellMeasuresFilter(data, "explicit dataset 4", { -1.f, -1.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet5();
  TestCellMeasuresFilter(
    data, "explicit dataset 5", { 1.f, 1.f / 3.f, 1.f / 6.f, -1.f / 2.f }, AllMeasures());

  data = factory.Make3DExplicitDataSet6();
  TestCellMeasuresFilter(data,
                         "explicit dataset 6 (only volume)",
                         { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.083426f, 0.25028f },
                         Volume());
  TestCellMeasuresFilter(
    data,
    "explicit dataset 6 (all)",
    { 0.999924f, 0.999924f, 0.f, 0.f, 3.85516f, 1.00119f, 0.083426f, 0.25028f },
    AllMeasures());
}

} // anonymous namespace

int UnitTestCellMeasuresFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCellMeasures, argc, argv);
}
