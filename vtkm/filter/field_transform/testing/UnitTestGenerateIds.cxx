//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/field_transform/GenerateIds.h>
#include <vtkm/source/Tangle.h>

namespace
{

void CheckField(const vtkm::cont::UnknownArrayHandle& array, vtkm::Id expectedSize, bool isFloat)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == expectedSize);
  if (isFloat)
  {
    VTKM_TEST_ASSERT(array.IsBaseComponentType<vtkm::FloatDefault>());
    VTKM_TEST_ASSERT(
      test_equal_ArrayHandles(array.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::FloatDefault>>(),
                              vtkm::cont::ArrayHandleIndex(expectedSize)));
  }
  else
  {
    VTKM_TEST_ASSERT(array.IsBaseComponentType<vtkm::Id>());
    VTKM_TEST_ASSERT(
      test_equal_ArrayHandles(array.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>(),
                              vtkm::cont::ArrayHandleIndex(expectedSize)));
  }
}

void TryGenerateIds(
  vtkm::filter::field_transform::GenerateIds& filter, // Why is Filter::Execute not const?
  const vtkm::cont::DataSet& input)
{
  vtkm::cont::DataSet output = filter.Execute(input);
  VTKM_TEST_ASSERT(output.GetNumberOfPoints() == input.GetNumberOfPoints());
  VTKM_TEST_ASSERT(output.GetNumberOfCells() == input.GetNumberOfCells());

  vtkm::IdComponent expectedNumberOfFields = input.GetNumberOfFields();
  if (filter.GetGeneratePointIds())
  {
    ++expectedNumberOfFields;
  }
  if (filter.GetGenerateCellIds())
  {
    ++expectedNumberOfFields;
  }
  VTKM_TEST_ASSERT(expectedNumberOfFields == output.GetNumberOfFields());

  if (filter.GetGeneratePointIds())
  {
    CheckField(output.GetPointField(filter.GetPointFieldName()).GetData(),
               output.GetNumberOfPoints(),
               filter.GetUseFloat());
  }

  if (filter.GetGeneratePointIds())
  {
    CheckField(output.GetPointField(filter.GetPointFieldName()).GetData(),
               output.GetNumberOfPoints(),
               filter.GetUseFloat());
  }

  if (filter.GetGenerateCellIds())
  {
    CheckField(output.GetCellField(filter.GetCellFieldName()).GetData(),
               output.GetNumberOfCells(),
               filter.GetUseFloat());
  }
}

void TestGenerateIds()
{
  vtkm::cont::DataSet input = vtkm::source::Tangle{ vtkm::Id3(8) }.Execute();
  vtkm::filter::field_transform::GenerateIds filter;

  TryGenerateIds(filter, input);

  filter.SetUseFloat(true);
  TryGenerateIds(filter, input);

  filter.SetUseFloat(false);
  filter.SetGenerateCellIds(false);
  filter.SetPointFieldName("indices");
  TryGenerateIds(filter, input);

  filter.SetGenerateCellIds(true);
  filter.SetGeneratePointIds(false);
  filter.SetCellFieldName("cell-indices");
  TryGenerateIds(filter, input);
}

} // anonymous namespace

int UnitTestGenerateIds(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestGenerateIds, argc, argv);
}
