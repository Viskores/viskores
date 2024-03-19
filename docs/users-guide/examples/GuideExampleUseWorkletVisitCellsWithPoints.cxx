//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/DataSet.h>

#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/ParametricCoordinates.h>

////
//// BEGIN-EXAMPLE UseWorkletVisitCellsWithPoints
////
namespace vtkm
{
namespace worklet
{

struct CellCenter : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInPoint inputPointField,
                                FieldOut outputCellField);
  using ExecutionSignature = void(_1, PointCount, _2, _3);

  using InputDomain = _1;

  template<typename CellShape, typename InputPointFieldType, typename OutputType>
  VTKM_EXEC void operator()(CellShape shape,
                            vtkm::IdComponent numPoints,
                            const InputPointFieldType& inputPointField,
                            OutputType& centerOut) const
  {
    vtkm::Vec3f parametricCenter;
    vtkm::exec::ParametricCoordinatesCenter(numPoints, shape, parametricCenter);
    vtkm::exec::CellInterpolate(inputPointField, parametricCenter, shape, centerOut);
  }
};

} // namespace worklet
} // namespace vtkm
////
//// END-EXAMPLE UseWorkletVisitCellsWithPoints
////

#include <vtkm/filter/Filter.h>

#define VTKM_FILTER_FIELD_CONVERSION_EXPORT

////
//// BEGIN-EXAMPLE UseFilterFieldWithCells
////
namespace vtkm
{
namespace filter
{
namespace field_conversion
{

class VTKM_FILTER_FIELD_CONVERSION_EXPORT CellCenters : public vtkm::filter::Filter
{
public:
  VTKM_CONT CellCenters();

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;
};

} // namespace field_conversion
} // namespace filter
} // namespace vtkm
////
//// END-EXAMPLE UseFilterFieldWithCells
////

////
//// BEGIN-EXAMPLE FilterFieldWithCellsImpl
////
namespace vtkm
{
namespace filter
{
namespace field_conversion
{

VTKM_CONT
CellCenters::CellCenters()
{
  this->SetOutputFieldName("");
}

VTKM_CONT cont::DataSet CellCenters::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  if (!inField.IsPointField())
  {
    throw vtkm::cont::ErrorBadType("Cell Centers filter operates on point data.");
  }

  vtkm::cont::UnknownArrayHandle outUnknownArray;

  auto resolveType = [&](const auto& inArray) {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;
    vtkm::cont::ArrayHandle<ValueType> outArray;

    this->Invoke(vtkm::worklet::CellCenter{}, inDataSet.GetCellSet(), inArray, outArray);

    outUnknownArray = outArray;
  };

  vtkm::cont::UnknownArrayHandle inUnknownArray = inField.GetData();
  //// LABEL CastAndCall
  inUnknownArray.CastAndCallForTypesWithFloatFallback<VTKM_DEFAULT_TYPE_LIST,
                                                      VTKM_DEFAULT_STORAGE_LIST>(
    resolveType);

  std::string outFieldName = this->GetOutputFieldName();
  if (outFieldName == "")
  {
    outFieldName = inField.GetName() + "_center";
  }

  return this->CreateResultFieldCell(inDataSet, outFieldName, outUnknownArray);
}

} // namespace field_conversion
} // namespace filter
} // namespace vtkm
////
//// END-EXAMPLE FilterFieldWithCellsImpl
////

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void CheckCellCenters(const vtkm::cont::DataSet& dataSet)
{
  std::cout << "Checking cell centers." << std::endl;
  vtkm::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> cellCentersArray;
  dataSet.GetCellField("cell_center").GetData().AsArrayHandle(cellCentersArray);

  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == cellCentersArray.GetNumberOfValues(),
                   "Cell centers array has wrong number of values.");

  vtkm::Id3 cellDimensions = cellSet.GetCellDimensions() - vtkm::Id3(1);

  vtkm::cont::ArrayHandle<vtkm::Vec3f>::ReadPortalType cellCentersPortal =
    cellCentersArray.ReadPortal();

  vtkm::Id cellIndex = 0;
  for (vtkm::Id kIndex = 0; kIndex < cellDimensions[2]; kIndex++)
  {
    for (vtkm::Id jIndex = 0; jIndex < cellDimensions[1]; jIndex++)
    {
      for (vtkm::Id iIndex = 0; iIndex < cellDimensions[0]; iIndex++)
      {
        vtkm::Vec3f center = cellCentersPortal.Get(cellIndex);
        VTKM_TEST_ASSERT(test_equal(center[0], iIndex + 0.5), "Bad X coord.");
        VTKM_TEST_ASSERT(test_equal(center[1], jIndex + 0.5), "Bad Y coord.");
        VTKM_TEST_ASSERT(test_equal(center[2], kIndex + 0.5), "Bad Z coord.");
        cellIndex++;
      }
    }
  }
}

void Test()
{
  vtkm::cont::testing::MakeTestDataSet makeTestDataSet;

  std::cout << "Making test data set." << std::endl;
  vtkm::cont::DataSet dataSet = makeTestDataSet.Make3DUniformDataSet0();

  std::cout << "Finding cell centers with filter." << std::endl;
  vtkm::filter::field_conversion::CellCenters cellCentersFilter;
  cellCentersFilter.SetUseCoordinateSystemAsField(true);
  cellCentersFilter.SetOutputFieldName("cell_center");
  vtkm::cont::DataSet results = cellCentersFilter.Execute(dataSet);

  CheckCellCenters(results);
}

} // anonymous namespace

int GuideExampleUseWorkletVisitCellsWithPoints(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
