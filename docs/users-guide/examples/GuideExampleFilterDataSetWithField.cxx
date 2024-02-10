//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/Filter.h>

#include <vtkm/filter/entity_extraction/Threshold.h>

#include <vtkm/TypeList.h>
#include <vtkm/UnaryPredicates.h>

#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#undef VTKM_FILTER_ENTITY_EXTRACTION_EXPORT
#define VTKM_FILTER_ENTITY_EXTRACTION_EXPORT

////
//// BEGIN-EXAMPLE BlankCellsFilterDeclaration
////
namespace vtkm
{
namespace filter
{
namespace entity_extraction
{

//// PAUSE-EXAMPLE
namespace
{

//// RESUME-EXAMPLE
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT BlankCells : public vtkm::filter::Filter
{
public:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;
};

//// PAUSE-EXAMPLE
} // anonymous namespace
//// RESUME-EXAMPLE

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
////
//// END-EXAMPLE BlankCellsFilterDeclaration
////

namespace vtkm
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
VTKM_CONT vtkm::cont::DataSet BlankCells::DoExecute(const vtkm::cont::DataSet& inData)
{
  vtkm::cont::Field inField = this->GetFieldFromDataSet(inData);
  if (!inField.IsCellField())
  {
    throw vtkm::cont::ErrorBadValue("Blanking field must be a cell field.");
  }

  // Set up this array to have a 0 for any cell to be removed and
  // a 1 for any cell to keep.
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> blankingArray;

  auto resolveType = [&](const auto& inFieldArray) {
    auto transformArray =
      vtkm::cont::make_ArrayHandleTransform(inFieldArray, vtkm::NotZeroInitialized{});
    vtkm::cont::ArrayCopyDevice(transformArray, blankingArray);
  };

  this->CastAndCallScalarField(inField, resolveType);

  // Make a temporary DataSet (shallow copy of the input) to pass blankingArray
  // to threshold.
  vtkm::cont::DataSet tempData = inData;
  tempData.AddCellField("vtkm-blanking-array", blankingArray);

  // Just use the Threshold filter to implement the actual cell removal.
  vtkm::filter::entity_extraction::Threshold thresholdFilter;
  thresholdFilter.SetLowerThreshold(0.5);
  thresholdFilter.SetUpperThreshold(2.0);
  thresholdFilter.SetActiveField("vtkm-blanking-array",
                                 vtkm::cont::Field::Association::Cells);

  // Make sure threshold filter passes all the fields requested, but not the
  // blanking array.
  thresholdFilter.SetFieldsToPass(this->GetFieldsToPass());
  thresholdFilter.SetFieldsToPass("vtkm-blanking-array",
                                  vtkm::cont::Field::Association::Cells,
                                  vtkm::filter::FieldSelection::Mode::Exclude);

  // Use the threshold filter to generate the actual output.
  return thresholdFilter.Execute(tempData);
}
////
//// END-EXAMPLE BlankCellsFilterDoExecute
////

} // anonymous namespace

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

VTKM_CONT
static void DoTest()
{
  std::cout << "Setting up data" << std::endl;
  vtkm::cont::testing::MakeTestDataSet makedata;
  vtkm::cont::DataSet inData = makedata.Make3DExplicitDataSetCowNose();

  vtkm::Id numInCells = inData.GetCellSet().GetNumberOfCells();

  using FieldType = vtkm::Float32;
  vtkm::cont::ArrayHandle<FieldType> inField;
  inField.Allocate(numInCells);
  SetPortal(inField.WritePortal());
  inData.AddCellField("field", inField);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray;
  maskArray.Allocate(numInCells);
  auto maskPortal = maskArray.WritePortal();
  for (vtkm::Id cellIndex = 0; cellIndex < numInCells; ++cellIndex)
  {
    maskPortal.Set(cellIndex, static_cast<vtkm::IdComponent>(cellIndex % 2));
  }
  inData.AddCellField("mask", maskArray);

  std::cout << "Run filter" << std::endl;
  vtkm::filter::entity_extraction::BlankCells filter;
  filter.SetActiveField("mask", vtkm::cont::Field::Association::Cells);

  // NOTE 2018-03-21: I expect this to fail in the short term. Right now no fields
  // are copied from input to output. The default should be changed to copy them
  // all. (Also, I'm thinking it would be nice to have a mode to select all except
  // a particular field or list of fields.)
  vtkm::cont::DataSet outData = filter.Execute(inData);

  std::cout << "Checking output." << std::endl;
  vtkm::Id numOutCells = numInCells / 2;
  VTKM_TEST_ASSERT(outData.GetCellSet().GetNumberOfCells() == numOutCells,
                   "Unexpected number of cells.");

  vtkm::cont::Field outCellField = outData.GetField("field");
  vtkm::cont::ArrayHandle<FieldType> outField;
  outCellField.GetData().AsArrayHandle(outField);
  auto outFieldPortal = outField.ReadPortal();
  for (vtkm::Id cellIndex = 0; cellIndex < numOutCells; ++cellIndex)
  {
    FieldType expectedValue = TestValue(2 * cellIndex + 1, FieldType());
    VTKM_TEST_ASSERT(test_equal(outFieldPortal.Get(cellIndex), expectedValue));
  }
}

int GuideExampleFilterDataSetWithField(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
