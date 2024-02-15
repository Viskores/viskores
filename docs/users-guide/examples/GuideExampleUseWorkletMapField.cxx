//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/VecTraits.h>
#include <vtkm/VectorAnalysis.h>

namespace
{

////
//// BEGIN-EXAMPLE UseWorkletMapField
////
class ComputeMagnitude : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputVectors, FieldOut outputMagnitudes);
  using ExecutionSignature = _2(_1);

  using InputDomain = _1;

  template<typename T, vtkm::IdComponent Size>
  VTKM_EXEC T operator()(const vtkm::Vec<T, Size>& inVector) const
  {
    return vtkm::Magnitude(inVector);
  }
};
////
//// END-EXAMPLE UseWorkletMapField
////

} // anonymous namespace

#include <vtkm/filter/Filter.h>

#define VTKM_FILTER_VECTOR_CALCULUS_EXPORT

////
//// BEGIN-EXAMPLE UseFilterField
////
namespace vtkm
{
namespace filter
{
namespace vector_calculus
{

//// LABEL Export
class VTKM_FILTER_VECTOR_CALCULUS_EXPORT FieldMagnitude : public vtkm::filter::Filter
{
public:
  VTKM_CONT FieldMagnitude();

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;
};

} // namespace vector_calculus
} // namespace filter
} // namespace vtkm
////
//// END-EXAMPLE UseFilterField
////

////
//// BEGIN-EXAMPLE FilterFieldImpl
////
namespace vtkm
{
namespace filter
{
namespace vector_calculus
{

VTKM_CONT
FieldMagnitude::FieldMagnitude()
{
  this->SetOutputFieldName("");
}

VTKM_CONT vtkm::cont::DataSet FieldMagnitude::DoExecute(
  const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  vtkm::cont::UnknownArrayHandle outField;

  // Use a C++ lambda expression to provide a callback for CastAndCall. The lambda
  // will capture references to local variables like outFieldArray (using `[&]`)
  // that it can read and write.
  auto resolveType = [&](const auto& inFieldArray) {
    using InArrayHandleType = std::decay_t<decltype(inFieldArray)>;
    using ComponentType =
      typename vtkm::VecTraits<typename InArrayHandleType::ValueType>::ComponentType;

    vtkm::cont::ArrayHandle<ComponentType> outFieldArray;

    this->Invoke(ComputeMagnitude{}, inFieldArray, outFieldArray);
    outField = outFieldArray;
  };

  //// LABEL CastAndCall
  this->CastAndCallVecField<3>(inField, resolveType);

  std::string outFieldName = this->GetOutputFieldName();
  if (outFieldName == "")
  {
    outFieldName = inField.GetName() + "_magnitude";
  }

  return this->CreateResultField(
    inDataSet, outFieldName, inField.GetAssociation(), outField);
}

} // namespace vector_calculus
} // namespace filter
} // namespace vtkm
////
//// END-EXAMPLE FilterFieldImpl
////

////
//// BEGIN-EXAMPLE RandomArrayAccess
////
namespace vtkm
{
namespace worklet
{

struct ReverseArrayCopyWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputArray, WholeArrayOut outputArray);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  template<typename InputType, typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const InputType& inputValue,
                            const OutputArrayPortalType& outputArrayPortal,
                            vtkm::Id workIndex) const
  {
    vtkm::Id outIndex = outputArrayPortal.GetNumberOfValues() - workIndex - 1;
    if (outIndex >= 0)
    {
      outputArrayPortal.Set(outIndex, inputValue);
    }
    else
    {
      this->RaiseError("Output array not sized correctly.");
    }
  }
};

} // namespace worklet
} // namespace vtkm
////
//// END-EXAMPLE RandomArrayAccess
////

#include <vtkm/cont/testing/Testing.h>

namespace
{

void Test()
{
  static const vtkm::Id ARRAY_SIZE = 10;

  vtkm::cont::ArrayHandle<vtkm::Vec3f> inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outputArray;

  vtkm::cont::DataSet inputDataSet;
  vtkm::cont::CellSetStructured<1> cellSet;
  cellSet.SetPointDimensions(ARRAY_SIZE);
  inputDataSet.SetCellSet(cellSet);
  inputDataSet.AddPointField("test_values", inputArray);

  vtkm::filter::vector_calculus::FieldMagnitude fieldMagFilter;
  fieldMagFilter.SetActiveField("test_values");
  vtkm::cont::DataSet magResult = fieldMagFilter.Execute(inputDataSet);
  magResult.GetField("test_values_magnitude").GetData().AsArrayHandle(outputArray);

  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == ARRAY_SIZE,
                   "Bad output array size.");
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    vtkm::Vec3f testValue = TestValue(index, vtkm::Vec3f());
    vtkm::Float64 expectedValue = sqrt(vtkm::Dot(testValue, testValue));
    vtkm::Float64 gotValue = outputArray.ReadPortal().Get(index);
    VTKM_TEST_ASSERT(test_equal(expectedValue, gotValue), "Got bad value.");
  }

  vtkm::cont::ArrayHandle<vtkm::Vec3f> outputArray2;
  outputArray2.Allocate(ARRAY_SIZE);

  vtkm::cont::Invoker invoker;
  invoker(vtkm::worklet::ReverseArrayCopyWorklet{}, inputArray, outputArray2);

  VTKM_TEST_ASSERT(outputArray2.GetNumberOfValues() == ARRAY_SIZE,
                   "Bad output array size.");
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    vtkm::Vec3f expectedValue = TestValue(ARRAY_SIZE - index - 1, vtkm::Vec3f());
    vtkm::Vec3f gotValue = outputArray2.ReadPortal().Get(index);
    VTKM_TEST_ASSERT(test_equal(expectedValue, gotValue), "Got bad value.");
  }
}

} // anonymous namespace

int GuideExampleUseWorkletMapField(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
