//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/exec/BoundaryState.h>
#include <vtkm/exec/FieldNeighborhood.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DefaultTypes.h>
#include <vtkm/cont/UncertainCellSet.h>

namespace boxfilter
{

////
//// BEGIN-EXAMPLE UseWorkletPointNeighborhood
////
class ApplyBoxKernel : public vtkm::worklet::WorkletPointNeighborhood
{
private:
  vtkm::IdComponent NumberOfLayers;

public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInNeighborhood inputField,
                                FieldOut outputField);
  using ExecutionSignature = _3(_2, Boundary);

  using InputDomain = _1;

  ApplyBoxKernel(vtkm::IdComponent kernelSize)
  {
    VTKM_ASSERT(kernelSize >= 3);
    VTKM_ASSERT((kernelSize % 2) == 1);

    this->NumberOfLayers = (kernelSize - 1) / 2;
  }

  template<typename InputFieldPortalType>
  VTKM_EXEC typename InputFieldPortalType::ValueType operator()(
    const vtkm::exec::FieldNeighborhood<InputFieldPortalType>& inputField,
    const vtkm::exec::BoundaryState& boundary) const
  {
    using T = typename InputFieldPortalType::ValueType;

    ////
    //// BEGIN-EXAMPLE GetNeighborhoodBoundary
    ////
    auto minIndices = boundary.MinNeighborIndices(this->NumberOfLayers);
    auto maxIndices = boundary.MaxNeighborIndices(this->NumberOfLayers);

    T sum = 0;
    vtkm::IdComponent size = 0;
    for (vtkm::IdComponent k = minIndices[2]; k <= maxIndices[2]; ++k)
    {
      for (vtkm::IdComponent j = minIndices[1]; j <= maxIndices[1]; ++j)
      {
        for (vtkm::IdComponent i = minIndices[0]; i <= maxIndices[0]; ++i)
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

#include <vtkm/filter/Filter.h>

namespace vtkm
{
namespace filter
{
namespace convolution
{

class BoxFilter : public vtkm::filter::Filter
{
public:
  VTKM_CONT BoxFilter() = default;

  VTKM_CONT void SetKernelSize(vtkm::IdComponent size) { this->KernelSize = size; }
  VTKM_CONT vtkm::IdComponent GetKernelSize() const { return this->KernelSize; }

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;

private:
  vtkm::IdComponent KernelSize = 3;
};

} // namespace convolution
} // namespace filter
} // namespace vtkm

namespace vtkm
{
namespace filter
{
namespace convolution
{

VTKM_CONT cont::DataSet BoxFilter::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  if (!inField.IsPointField())
  {
    throw vtkm::cont::ErrorBadType("Box Filter operates on point data.");
  }

  vtkm::cont::UncertainCellSet<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED> cellSet =
    inDataSet.GetCellSet();

  vtkm::cont::UnknownArrayHandle outFieldArray;

  auto resolve_field = [&](auto inArray) {
    using T = typename std::decay_t<decltype(inArray)>::ValueType;
    vtkm::cont::ArrayHandle<T> outArray;
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
} // namespace vtkm

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void CheckBoxFilter(const vtkm::cont::DataSet& dataSet)
{
  std::cout << "Check box filter." << std::endl;

  static const vtkm::Id NUM_POINTS = 18;
  static const vtkm::Float32 expected[NUM_POINTS] = {
    60.1875f, 65.2f,    70.2125f, 60.1875f, 65.2f,    70.2125f,
    90.2667f, 95.2778f, 100.292f, 90.2667f, 95.2778f, 100.292f,
    120.337f, 125.35f,  130.363f, 120.337f, 125.35f,  130.363f
  };

  vtkm::cont::ArrayHandle<vtkm::Float32> outputArray;
  dataSet.GetPointField("pointvar_average").GetData().AsArrayHandle(outputArray);

  vtkm::cont::printSummary_ArrayHandle(outputArray, std::cout, true);

  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == NUM_POINTS);

  auto portal = outputArray.ReadPortal();
  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
  {
    vtkm::Float32 computed = portal.Get(index);
    VTKM_TEST_ASSERT(
      test_equal(expected[index], computed), "Unexpected value at index ", index);
  }
}

void Test()
{
  vtkm::cont::testing::MakeTestDataSet makeTestDataSet;

  std::cout << "Making test data set." << std::endl;
  vtkm::cont::DataSet dataSet = makeTestDataSet.Make3DUniformDataSet0();

  std::cout << "Running box filter." << std::endl;
  vtkm::filter::convolution::BoxFilter boxFilter;
  boxFilter.SetKernelSize(3);
  boxFilter.SetActiveField("pointvar");
  boxFilter.SetOutputFieldName("pointvar_average");
  vtkm::cont::DataSet results = boxFilter.Execute(dataSet);

  CheckBoxFilter(results);
}

} // anonymous namespace

int GuideExampleUseWorkletPointNeighborhood(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
