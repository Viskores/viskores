//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/field_transform/CompositeVectors.h>

namespace
{
template <typename ScalarType>
vtkm::cont::DataSet MakeDataSet(vtkm::IdComponent numArrays)
{
  vtkm::cont::DataSet dataSet;

  vtkm::IdComponent arrayLen = 100;

  for (vtkm::IdComponent fieldIndex = 0; fieldIndex < numArrays; ++fieldIndex)
  {
    std::vector<ScalarType> pointarray;
    std::vector<ScalarType> cellarray;

    for (vtkm::Id valueIndex = 0; valueIndex < arrayLen; ++valueIndex)
    {
      pointarray.push_back(static_cast<ScalarType>(fieldIndex * 1.1 + valueIndex * 1.1));
      cellarray.push_back(static_cast<ScalarType>(fieldIndex * 2.1 + valueIndex * 2.1));
    }

    dataSet.AddPointField("pointArray" + std::to_string(fieldIndex), pointarray);
    dataSet.AddCellField("cellArray" + std::to_string(fieldIndex), cellarray);
  }

  return dataSet;
}

template <typename ScalarType, typename VecType>
void CheckResults(vtkm::cont::DataSet inDataSet,
                  const std::vector<std::string> FieldNames,
                  const std::string compositedName)
{
  //there are only three fields for this testing , it is ok to use vtkm::IdComponent
  vtkm::IdComponent numComponents = static_cast<vtkm::IdComponent>(FieldNames.size());
  auto compositedField = inDataSet.GetField(compositedName);
  vtkm::IdComponent compositedFieldLen =
    static_cast<vtkm::IdComponent>(compositedField.GetNumberOfValues());

  using ArrayHandleType = vtkm::cont::ArrayHandle<VecType>;
  ArrayHandleType compFieldArrayCopy;
  compositedField.GetData().AsArrayHandle(compFieldArrayCopy);

  auto compFieldReadPortal = compFieldArrayCopy.ReadPortal();

  for (vtkm::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++)
  {
    auto field = inDataSet.GetField(FieldNames[componentIndex]);
    VTKM_TEST_ASSERT(compositedField.GetAssociation() == field.GetAssociation(),
                     "Got bad association value.");

    vtkm::IdComponent fieldLen = static_cast<vtkm::IdComponent>(field.GetNumberOfValues());
    VTKM_TEST_ASSERT(compositedFieldLen == fieldLen, "Got wrong field length.");

    vtkm::cont::ArrayHandle<ScalarType> fieldArrayHandle;
    field.GetData().AsArrayHandle(fieldArrayHandle);
    auto fieldReadPortal = fieldArrayHandle.ReadPortal();

    for (vtkm::Id valueIndex = 0; valueIndex < fieldLen; valueIndex++)
    {
      auto compFieldVec = compFieldReadPortal.Get(valueIndex);
      auto comFieldValue = compFieldVec[static_cast<vtkm::UInt64>(componentIndex)];
      auto fieldValue = fieldReadPortal.Get(valueIndex);
      VTKM_TEST_ASSERT(comFieldValue == fieldValue, "Got bad field value.");
    }
  }
}


template <typename ScalarType, typename VecType>
void TestCompositeVectors(vtkm::IdComponent numComponents)
{
  vtkm::cont::DataSet inDataSet = MakeDataSet<ScalarType>(numComponents);
  vtkm::filter::field_transform::CompositeVectors filter;

  // For the first pass (point field), we are going to use the generic `SetActiveField` method
  // and let the filter figure out how many fields.
  std::vector<std::string> pointFieldNames;
  for (vtkm::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++)
  {
    pointFieldNames.push_back("pointArray" + std::to_string(componentIndex));
    filter.SetActiveField(componentIndex, "pointArray" + std::to_string(componentIndex));
  }
  filter.SetOutputFieldName("CompositedFieldPoint");
  vtkm::cont::DataSet outDataSetPointAssoci = filter.Execute(inDataSet);

  CheckResults<ScalarType, VecType>(
    outDataSetPointAssoci, pointFieldNames, filter.GetOutputFieldName());

  // For the second pass (cell field), we will use the `SetFieldNameList` method.
  std::vector<std::string> cellFieldNames;
  for (vtkm::IdComponent componentIndex = 0; componentIndex < numComponents; componentIndex++)
  {
    cellFieldNames.push_back("cellArray" + std::to_string(componentIndex));
  }
  filter.SetFieldNameList(cellFieldNames, vtkm::cont::Field::Association::Cells);
  filter.SetOutputFieldName("CompositedFieldCell");

  vtkm::cont::DataSet outDataSetCellAssoci = filter.Execute(inDataSet);
  CheckResults<ScalarType, VecType>(
    outDataSetCellAssoci, cellFieldNames, filter.GetOutputFieldName());
}

void CompositeVectors()
{
  TestCompositeVectors<vtkm::FloatDefault, vtkm::Vec2f>(2);
  TestCompositeVectors<vtkm::FloatDefault, vtkm::Vec3f>(3);
  TestCompositeVectors<vtkm::FloatDefault, vtkm::Vec<vtkm::FloatDefault, 5>>(5);
  TestCompositeVectors<vtkm::Id, vtkm::Vec2i>(2);
  TestCompositeVectors<vtkm::Id, vtkm::Vec3i>(3);
  TestCompositeVectors<vtkm::Id, vtkm::Vec<vtkm::Id, 5>>(5);
}

} // anonymous namespace

int UnitTestCompositeVectors(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CompositeVectors, argc, argv);
}
