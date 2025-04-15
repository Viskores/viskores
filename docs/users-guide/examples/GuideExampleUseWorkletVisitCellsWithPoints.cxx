//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/DataSet.h>

#include <viskores/exec/CellInterpolate.h>
#include <viskores/exec/ParametricCoordinates.h>

////
//// BEGIN-EXAMPLE UseWorkletVisitCellsWithPoints
////
namespace viskores
{
namespace worklet
{

struct CellCenter : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInPoint inputPointField,
                                FieldOut outputCellField);
  using ExecutionSignature = void(_1, PointCount, _2, _3);

  using InputDomain = _1;

  template<typename CellShape, typename InputPointFieldType, typename OutputType>
  VISKORES_EXEC void operator()(CellShape shape,
                                viskores::IdComponent numPoints,
                                const InputPointFieldType& inputPointField,
                                OutputType& centerOut) const
  {
    viskores::Vec3f parametricCenter;
    viskores::exec::ParametricCoordinatesCenter(numPoints, shape, parametricCenter);
    viskores::exec::CellInterpolate(inputPointField, parametricCenter, shape, centerOut);
  }
};

} // namespace worklet
} // namespace viskores
////
//// END-EXAMPLE UseWorkletVisitCellsWithPoints
////

#include <viskores/filter/Filter.h>

#define VISKORES_FILTER_FIELD_CONVERSION_EXPORT

////
//// BEGIN-EXAMPLE UseFilterFieldWithCells
////
namespace viskores
{
namespace filter
{
namespace field_conversion
{

class VISKORES_FILTER_FIELD_CONVERSION_EXPORT CellCenters
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT CellCenters();

  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inDataSet) override;
};

} // namespace field_conversion
} // namespace filter
} // namespace viskores
////
//// END-EXAMPLE UseFilterFieldWithCells
////

////
//// BEGIN-EXAMPLE FilterFieldWithCellsImpl
////
namespace viskores
{
namespace filter
{
namespace field_conversion
{

VISKORES_CONT
CellCenters::CellCenters()
{
  this->SetOutputFieldName("");
}

VISKORES_CONT cont::DataSet CellCenters::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  if (!inField.IsPointField())
  {
    throw viskores::cont::ErrorBadType("Cell Centers filter operates on point data.");
  }

  viskores::cont::UnknownArrayHandle outUnknownArray;

  auto resolveType = [&](const auto& inArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inArray)>;
    using ValueType = typename InArrayHandleType::ValueType;
    viskores::cont::ArrayHandle<ValueType> outArray;

    this->Invoke(
      viskores::worklet::CellCenter{}, inDataSet.GetCellSet(), inArray, outArray);

    outUnknownArray = outArray;
  };

  viskores::cont::UnknownArrayHandle inUnknownArray = inField.GetData();
  //// LABEL CastAndCall
  inUnknownArray.CastAndCallForTypesWithFloatFallback<VISKORES_DEFAULT_TYPE_LIST,
                                                      VISKORES_DEFAULT_STORAGE_LIST>(
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
} // namespace viskores
////
//// END-EXAMPLE FilterFieldWithCellsImpl
////

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void CheckCellCenters(const viskores::cont::DataSet& dataSet)
{
  std::cout << "Checking cell centers." << std::endl;
  viskores::cont::CellSetStructured<3> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  viskores::cont::ArrayHandle<viskores::Vec3f> cellCentersArray;
  dataSet.GetCellField("cell_center").GetData().AsArrayHandle(cellCentersArray);

  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() ==
                         cellCentersArray.GetNumberOfValues(),
                       "Cell centers array has wrong number of values.");

  viskores::Id3 cellDimensions = cellSet.GetCellDimensions() - viskores::Id3(1);

  viskores::cont::ArrayHandle<viskores::Vec3f>::ReadPortalType cellCentersPortal =
    cellCentersArray.ReadPortal();

  viskores::Id cellIndex = 0;
  for (viskores::Id kIndex = 0; kIndex < cellDimensions[2]; kIndex++)
  {
    for (viskores::Id jIndex = 0; jIndex < cellDimensions[1]; jIndex++)
    {
      for (viskores::Id iIndex = 0; iIndex < cellDimensions[0]; iIndex++)
      {
        viskores::Vec3f center = cellCentersPortal.Get(cellIndex);
        VISKORES_TEST_ASSERT(test_equal(center[0], iIndex + 0.5), "Bad X coord.");
        VISKORES_TEST_ASSERT(test_equal(center[1], jIndex + 0.5), "Bad Y coord.");
        VISKORES_TEST_ASSERT(test_equal(center[2], kIndex + 0.5), "Bad Z coord.");
        cellIndex++;
      }
    }
  }
}

void Test()
{
  viskores::cont::testing::MakeTestDataSet makeTestDataSet;

  std::cout << "Making test data set." << std::endl;
  viskores::cont::DataSet dataSet = makeTestDataSet.Make3DUniformDataSet0();

  std::cout << "Finding cell centers with filter." << std::endl;
  viskores::filter::field_conversion::CellCenters cellCentersFilter;
  cellCentersFilter.SetUseCoordinateSystemAsField(true);
  cellCentersFilter.SetOutputFieldName("cell_center");
  viskores::cont::DataSet results = cellCentersFilter.Execute(dataSet);

  CheckCellCenters(results);
}

} // anonymous namespace

int GuideExampleUseWorkletVisitCellsWithPoints(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
