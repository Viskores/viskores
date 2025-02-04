//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/worklet/WorkletPointNeighborhood.h>

#include <viskores/exec/BoundaryState.h>
#include <viskores/exec/FieldNeighborhood.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/DefaultTypes.h>
#include <viskores/cont/UncertainCellSet.h>

namespace boxfilter
{

////
//// BEGIN-EXAMPLE UseWorkletPointNeighborhood
////
class ApplyBoxKernel : public viskores::worklet::WorkletPointNeighborhood
{
private:
  viskores::IdComponent NumberOfLayers;

public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInNeighborhood inputField,
                                FieldOut outputField);
  using ExecutionSignature = _3(_2, Boundary);

  using InputDomain = _1;

  ApplyBoxKernel(viskores::IdComponent kernelSize)
  {
    VISKORES_ASSERT(kernelSize >= 3);
    VISKORES_ASSERT((kernelSize % 2) == 1);

    this->NumberOfLayers = (kernelSize - 1) / 2;
  }

  template<typename InputFieldPortalType>
  VISKORES_EXEC typename InputFieldPortalType::ValueType operator()(
    const viskores::exec::FieldNeighborhood<InputFieldPortalType>& inputField,
    const viskores::exec::BoundaryState& boundary) const
  {
    using T = typename InputFieldPortalType::ValueType;

    ////
    //// BEGIN-EXAMPLE GetNeighborhoodBoundary
    ////
    auto minIndices = boundary.MinNeighborIndices(this->NumberOfLayers);
    auto maxIndices = boundary.MaxNeighborIndices(this->NumberOfLayers);

    T sum = 0;
    viskores::IdComponent size = 0;
    for (viskores::IdComponent k = minIndices[2]; k <= maxIndices[2]; ++k)
    {
      for (viskores::IdComponent j = minIndices[1]; j <= maxIndices[1]; ++j)
      {
        for (viskores::IdComponent i = minIndices[0]; i <= maxIndices[0]; ++i)
        {
          ////
          //// BEGIN-EXAMPLE GetNeighborhoodFieldValue
          ////
          sum = sum + inputField.Get(i, j, k);
          ////
          //// END-EXAMPLE GetNeighborhoodFieldValue
          ////
          ++size;
        }
      }
    }
    ////
    //// END-EXAMPLE GetNeighborhoodBoundary
    ////

    return static_cast<T>(sum / size);
  }
};
////
//// END-EXAMPLE UseWorkletPointNeighborhood
////

} // namespace boxfilter

#include <viskores/filter/Filter.h>

namespace viskores
{
namespace filter
{
namespace convolution
{

class BoxFilter : public viskores::filter::Filter
{
public:
  VISKORES_CONT BoxFilter() = default;

  VISKORES_CONT void SetKernelSize(viskores::IdComponent size) { this->KernelSize = size; }
  VISKORES_CONT viskores::IdComponent GetKernelSize() const { return this->KernelSize; }

protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inDataSet) override;

private:
  viskores::IdComponent KernelSize = 3;
};

} // namespace convolution
} // namespace filter
} // namespace viskores

namespace viskores
{
namespace filter
{
namespace convolution
{

VISKORES_CONT cont::DataSet BoxFilter::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  if (!inField.IsPointField())
  {
    throw viskores::cont::ErrorBadType("Box Filter operates on point data.");
  }

  viskores::cont::UncertainCellSet<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED> cellSet =
    inDataSet.GetCellSet();

  viskores::cont::UnknownArrayHandle outFieldArray;

  auto resolve_field = [&](auto inArray) {
    using T = typename std::decay_t<decltype(inArray)>::ValueType;
    viskores::cont::ArrayHandle<T> outArray;
    this->Invoke(
      boxfilter::ApplyBoxKernel{ this->KernelSize }, cellSet, inArray, outArray);
    outFieldArray = outArray;
  };
  this->CastAndCallScalarField(inField, resolve_field);

  std::string outFieldName = this->GetOutputFieldName();
  if (outFieldName == "")
  {
    outFieldName = inField.GetName() + "_blurred";
  }

  return this->CreateResultField(
    inDataSet, outFieldName, inField.GetAssociation(), outFieldArray);
}

} // namespace convolution
} // namespace filter
} // namespace viskores

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void CheckBoxFilter(const viskores::cont::DataSet& dataSet)
{
  std::cout << "Check box filter." << std::endl;

  static const viskores::Id NUM_POINTS = 18;
  static const viskores::Float32 expected[NUM_POINTS] = {
    60.1875f, 65.2f,    70.2125f, 60.1875f, 65.2f,    70.2125f,
    90.2667f, 95.2778f, 100.292f, 90.2667f, 95.2778f, 100.292f,
    120.337f, 125.35f,  130.363f, 120.337f, 125.35f,  130.363f
  };

  viskores::cont::ArrayHandle<viskores::Float32> outputArray;
  dataSet.GetPointField("pointvar_average").GetData().AsArrayHandle(outputArray);

  viskores::cont::printSummary_ArrayHandle(outputArray, std::cout, true);

  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == NUM_POINTS);

  auto portal = outputArray.ReadPortal();
  for (viskores::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    viskores::Float32 computed = portal.Get(index);
    VISKORES_TEST_ASSERT(
      test_equal(expected[index], computed), "Unexpected value at index ", index);
  }
}

void Test()
{
  viskores::cont::testing::MakeTestDataSet makeTestDataSet;

  std::cout << "Making test data set." << std::endl;
  viskores::cont::DataSet dataSet = makeTestDataSet.Make3DUniformDataSet0();

  std::cout << "Running box filter." << std::endl;
  viskores::filter::convolution::BoxFilter boxFilter;
  boxFilter.SetKernelSize(3);
  boxFilter.SetActiveField("pointvar");
  boxFilter.SetOutputFieldName("pointvar_average");
  viskores::cont::DataSet results = boxFilter.Execute(dataSet);

  CheckBoxFilter(results);
}

} // anonymous namespace

int GuideExampleUseWorkletPointNeighborhood(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
