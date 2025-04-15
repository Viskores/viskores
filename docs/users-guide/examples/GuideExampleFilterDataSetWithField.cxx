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

#include <viskores/filter/Filter.h>

#include <viskores/filter/entity_extraction/Threshold.h>

#include <viskores/TypeList.h>
#include <viskores/UnaryPredicates.h>

#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/DataSet.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#undef VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT
#define VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT

////
//// BEGIN-EXAMPLE BlankCellsFilterDeclaration
////
namespace viskores
{
namespace filter
{
namespace entity_extraction
{

//// PAUSE-EXAMPLE
namespace
{

//// RESUME-EXAMPLE
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT BlankCells
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inDataSet) override;
};

//// PAUSE-EXAMPLE
} // anonymous namespace
//// RESUME-EXAMPLE

} // namespace entity_extraction
} // namespace filter
} // namespace viskores
////
//// END-EXAMPLE BlankCellsFilterDeclaration
////

namespace viskores
{
namespace filter
{
namespace entity_extraction
{

namespace
{

////
//// BEGIN-EXAMPLE BlankCellsFilterDoExecute
////
VISKORES_CONT viskores::cont::DataSet BlankCells::DoExecute(
  const viskores::cont::DataSet& inData)
{
  viskores::cont::Field inField = this->GetFieldFromDataSet(inData);
  if (!inField.IsCellField())
  {
    throw viskores::cont::ErrorBadValue("Blanking field must be a cell field.");
  }

  // Set up this array to have a 0 for any cell to be removed and
  // a 1 for any cell to keep.
  viskores::cont::ArrayHandle<viskores::FloatDefault> blankingArray;

  auto resolveType = [&](const auto& inFieldArray)
  {
    auto transformArray = viskores::cont::make_ArrayHandleTransform(
      inFieldArray, viskores::NotZeroInitialized{});
    viskores::cont::ArrayCopyDevice(transformArray, blankingArray);
  };

  this->CastAndCallScalarField(inField, resolveType);

  // Make a temporary DataSet (shallow copy of the input) to pass blankingArray
  // to threshold.
  viskores::cont::DataSet tempData = inData;
  tempData.AddCellField("viskores-blanking-array", blankingArray);

  // Just use the Threshold filter to implement the actual cell removal.
  viskores::filter::entity_extraction::Threshold thresholdFilter;
  thresholdFilter.SetLowerThreshold(0.5);
  thresholdFilter.SetUpperThreshold(2.0);
  thresholdFilter.SetActiveField("viskores-blanking-array",
                                 viskores::cont::Field::Association::Cells);

  // Make sure threshold filter passes all the fields requested, but not the
  // blanking array.
  thresholdFilter.SetFieldsToPass(this->GetFieldsToPass());
  thresholdFilter.SetFieldsToPass("viskores-blanking-array",
                                  viskores::cont::Field::Association::Cells,
                                  viskores::filter::FieldSelection::Mode::Exclude);

  // Use the threshold filter to generate the actual output.
  return thresholdFilter.Execute(tempData);
}
////
//// END-EXAMPLE BlankCellsFilterDoExecute
////

} // anonymous namespace

} // namespace entity_extraction
} // namespace filter
} // namespace viskores

VISKORES_CONT
static void DoTest()
{
  std::cout << "Setting up data" << std::endl;
  viskores::cont::testing::MakeTestDataSet makedata;
  viskores::cont::DataSet inData = makedata.Make3DExplicitDataSetCowNose();

  viskores::Id numInCells = inData.GetCellSet().GetNumberOfCells();

  using FieldType = viskores::Float32;
  viskores::cont::ArrayHandle<FieldType> inField;
  inField.Allocate(numInCells);
  SetPortal(inField.WritePortal());
  inData.AddCellField("field", inField);

  viskores::cont::ArrayHandle<viskores::IdComponent> maskArray;
  maskArray.Allocate(numInCells);
  auto maskPortal = maskArray.WritePortal();
  for (viskores::Id cellIndex = 0; cellIndex < numInCells; ++cellIndex)
  {
    maskPortal.Set(cellIndex, static_cast<viskores::IdComponent>(cellIndex % 2));
  }
  inData.AddCellField("mask", maskArray);

  std::cout << "Run filter" << std::endl;
  viskores::filter::entity_extraction::BlankCells filter;
  filter.SetActiveField("mask", viskores::cont::Field::Association::Cells);

  // NOTE 2018-03-21: I expect this to fail in the short term. Right now no fields
  // are copied from input to output. The default should be changed to copy them
  // all. (Also, I'm thinking it would be nice to have a mode to select all except
  // a particular field or list of fields.)
  viskores::cont::DataSet outData = filter.Execute(inData);

  std::cout << "Checking output." << std::endl;
  viskores::Id numOutCells = numInCells / 2;
  VISKORES_TEST_ASSERT(outData.GetCellSet().GetNumberOfCells() == numOutCells,
                       "Unexpected number of cells.");

  viskores::cont::Field outCellField = outData.GetField("field");
  viskores::cont::ArrayHandle<FieldType> outField;
  outCellField.GetData().AsArrayHandle(outField);
  auto outFieldPortal = outField.ReadPortal();
  for (viskores::Id cellIndex = 0; cellIndex < numOutCells; ++cellIndex)
  {
    FieldType expectedValue = TestValue(2 * cellIndex + 1, FieldType());
    VISKORES_TEST_ASSERT(test_equal(outFieldPortal.Get(cellIndex), expectedValue));
  }
}

int GuideExampleFilterDataSetWithField(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
