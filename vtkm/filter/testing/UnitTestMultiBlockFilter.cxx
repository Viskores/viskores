//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/contour/ClipWithField.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/vector_analysis/Gradient.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/source/Tangle.h>

namespace
{
template <typename T>
vtkm::FloatDefault ValueDifference(const T& a, const T& b)
{
  return vtkm::Abs(a - b);
}
template <typename T>
vtkm::FloatDefault ValueDifference(const vtkm::Vec<T, 3>& a, const vtkm::Vec<T, 3>& b)
{
  return vtkm::Abs(a[0] - b[0]) + vtkm::Abs(a[1] - b[1]) + vtkm::Abs(a[2] - b[2]);
}

template <typename ArrayType>
void ValidateField(const ArrayType& truthField, const ArrayType& resultField)
{
  VTKM_TEST_ASSERT(truthField.GetNumberOfValues() == resultField.GetNumberOfValues(),
                   "Wrong number of field values");
  const vtkm::FloatDefault tol = static_cast<vtkm::FloatDefault>(1e-3);

  vtkm::Id numPts = truthField.GetNumberOfValues();
  const auto truthPortal = truthField.ReadPortal();
  const auto resultPortal = resultField.ReadPortal();
  for (vtkm::Id j = 0; j < numPts; j++)
    VTKM_TEST_ASSERT(ValueDifference(truthPortal.Get(j), resultPortal.Get(j)) < tol,
                     "Wrong value in field");
}

void ValidateResults(const vtkm::cont::PartitionedDataSet& truth,
                     const vtkm::cont::PartitionedDataSet& result,
                     const std::string& varName,
                     bool isScalar = true)
{
  VTKM_TEST_ASSERT(truth.GetNumberOfPartitions() == result.GetNumberOfPartitions());
  vtkm::Id numDS = truth.GetNumberOfPartitions();
  for (vtkm::Id i = 0; i < numDS; i++)
  {
    auto truthDS = truth.GetPartition(i);
    auto resultDS = result.GetPartition(i);

    VTKM_TEST_ASSERT(truthDS.GetNumberOfPoints() == resultDS.GetNumberOfPoints(),
                     "Wrong number of points");
    VTKM_TEST_ASSERT(truthDS.GetNumberOfCells() == resultDS.GetNumberOfCells(),
                     "Wrong number of cells");
    VTKM_TEST_ASSERT(resultDS.HasField(varName), "Missing field");

    if (isScalar)
    {
      vtkm::cont::ArrayHandle<vtkm::Float32> truthField, resultField;
      truthDS.GetField(varName).GetData().AsArrayHandle(truthField);
      resultDS.GetField(varName).GetData().AsArrayHandle(resultField);
      ValidateField(truthField, resultField);
    }
    else
    {
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> truthField, resultField;
      truthDS.GetField(varName).GetData().AsArrayHandle(truthField);
      resultDS.GetField(varName).GetData().AsArrayHandle(resultField);
      ValidateField(truthField, resultField);
    }
  }
}
} //namespace


void TestMultiBlockFilter()
{
  vtkm::cont::PartitionedDataSet pds;

  for (int i = 0; i < 10; i++)
  {
    vtkm::Id3 dims(10 + i, 10 + i, 10 + i);
    vtkm::source::Tangle tangle;
    tangle.SetCellDimensions(dims);
    pds.AppendPartition(tangle.Execute());
  }

  std::cout << "ClipWithField" << std::endl;
  std::vector<vtkm::cont::PartitionedDataSet> results;
  std::vector<bool> flags = { false, true };
  for (const auto doThreading : flags)
  {
    vtkm::filter::contour::ClipWithField clip;
    clip.SetRunMultiThreadedFilter(doThreading);
    clip.SetClipValue(0.0);
    clip.SetActiveField("tangle");
    clip.SetFieldsToPass("tangle", vtkm::cont::Field::Association::Points);
    auto result = clip.Execute(pds);
    VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "tangle");

  std::cout << "Contour" << std::endl;
  results.clear();
  for (const auto doThreading : flags)
  {
    vtkm::filter::contour::Contour mc;
    mc.SetRunMultiThreadedFilter(doThreading);
    mc.SetGenerateNormals(true);
    mc.SetIsoValue(0, 0.5);
    mc.SetActiveField("tangle");
    mc.SetFieldsToPass("tangle", vtkm::cont::Field::Association::Points);
    auto result = mc.Execute(pds);
    VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "tangle");

  std::cout << "CleanGrid" << std::endl;
  results.clear();
  for (const auto doThreading : flags)
  {
    vtkm::filter::clean_grid::CleanGrid clean;
    clean.SetRunMultiThreadedFilter(doThreading);
    clean.SetCompactPointFields(true);
    clean.SetMergePoints(true);
    auto result = clean.Execute(pds);
    VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "tangle");

  std::cout << "Gradient" << std::endl;
  results.clear();
  for (const auto doThreading : flags)
  {
    vtkm::filter::vector_analysis::Gradient grad;
    grad.SetRunMultiThreadedFilter(doThreading);
    grad.SetComputePointGradient(true);
    grad.SetActiveField("tangle");
    grad.SetOutputFieldName("gradient");
    auto result = grad.Execute(pds);
    VTKM_TEST_ASSERT(result.GetNumberOfPartitions() == pds.GetNumberOfPartitions());
    results.push_back(result);
  }
  ValidateResults(results[0], results[1], "gradient", false);
}

int UnitTestMultiBlockFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMultiBlockFilter, argc, argv);
}
