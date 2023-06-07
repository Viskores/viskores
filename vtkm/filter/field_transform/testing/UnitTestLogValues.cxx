//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/field_transform/LogValues.h>

namespace
{
const vtkm::Id dim = 10;
using BaseType = vtkm::filter::field_transform::LogValues::LogBase;

template <typename T>
vtkm::cont::DataSet MakeLogValuesTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<T> pointarray;
  std::vector<T> cellarray;

  for (vtkm::Id j = 0; j < dim; ++j)
  {
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      pointarray.push_back(static_cast<T>((j * dim + i) * 0.1));
      cellarray.push_back(static_cast<T>((i * dim + j) * 0.1));
    }
  }

  dataSet.AddPointField("pointScalarField", pointarray);
  dataSet.AddCellField("cellScalarField", cellarray);

  return dataSet;
}

void TestLogGeneral(
  BaseType base,
  std::string ActiveFieldName,
  vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
{
  vtkm::cont::DataSet input = MakeLogValuesTestDataSet<vtkm::FloatDefault>();
  vtkm::filter::field_transform::LogValues filter;

  std::string LogFieldName = ActiveFieldName + "LogValues";

  filter.SetActiveField(ActiveFieldName, association);
  filter.SetOutputFieldName(LogFieldName);
  filter.SetBaseValue(base);

  vtkm::cont::DataSet output = filter.Execute(input);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> rawArrayHandle;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> logArrayHandle;

  input.GetField(ActiveFieldName, association).GetData().AsArrayHandle(rawArrayHandle);
  output.GetField(LogFieldName, association).GetData().AsArrayHandle(logArrayHandle);

  auto rawPortal = rawArrayHandle.ReadPortal();
  auto logPortal = logArrayHandle.ReadPortal();

  typedef vtkm::FloatDefault (*LogFuncPtr)(vtkm::FloatDefault);
  LogFuncPtr LogFunc = nullptr;

  switch (base)
  {
    case BaseType::E:
    {
      LogFunc = &vtkm::Log;
      break;
    }
    case BaseType::TWO:
    {
      LogFunc = &vtkm::Log2;
      break;
    }
    case BaseType::TEN:
    {
      LogFunc = &vtkm::Log10;
      break;
    }
    default:
    {
      VTKM_TEST_ASSERT("unsupported base value");
    }
  }

  for (vtkm::Id i = 0; i < rawArrayHandle.GetNumberOfValues(); ++i)
  {
    auto raw = rawPortal.Get(i);
    auto logv = logPortal.Get(i);
    if (raw == 0)
    {
      VTKM_TEST_ASSERT(test_equal(logv, LogFunc(filter.GetMinValue())),
                       "log value was wrong for min value");
      continue;
    }
    VTKM_TEST_ASSERT(test_equal(logv, LogFunc(raw)), "log value was wrong for test");
  }
}

void TestLogValues()
{
  std::string pointScalarField = "pointScalarField";
  using AsscoType = vtkm::cont::Field::Association;

  TestLogGeneral(BaseType::TEN, pointScalarField, AsscoType::Points);
  TestLogGeneral(BaseType::TWO, pointScalarField, AsscoType::Points);
  TestLogGeneral(BaseType::E, pointScalarField, AsscoType::Points);

  std::string cellScalarField = "cellScalarField";
  TestLogGeneral(BaseType::TEN, cellScalarField, AsscoType::Cells);
  TestLogGeneral(BaseType::TWO, cellScalarField, AsscoType::Cells);
  TestLogGeneral(BaseType::E, cellScalarField, AsscoType::Cells);
}

} // anonymous namespace

int UnitTestLogValues(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestLogValues, argc, argv);
}
