//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/VecTraits.h>
#include <viskores/VectorAnalysis.h>

namespace
{

////
//// BEGIN-EXAMPLE UseWorkletMapField
////
class ComputeMagnitude : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputVectors, FieldOut outputMagnitudes);
  using ExecutionSignature = _2(_1);

  using InputDomain = _1;

  template<typename T, viskores::IdComponent Size>
  VISKORES_EXEC T operator()(const viskores::Vec<T, Size>& inVector) const
  {
    return viskores::Magnitude(inVector);
  }
};
////
//// END-EXAMPLE UseWorkletMapField
////

} // anonymous namespace

#include <viskores/filter/Filter.h>

#define VISKORES_FILTER_VECTOR_CALCULUS_EXPORT

////
//// BEGIN-EXAMPLE UseFilterField
////
namespace viskores
{
namespace filter
{
namespace vector_calculus
{

//// LABEL Export
class VISKORES_FILTER_VECTOR_CALCULUS_EXPORT FieldMagnitude
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT FieldMagnitude();

  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inDataSet) override;
};

} // namespace vector_calculus
} // namespace filter
} // namespace viskores
////
//// END-EXAMPLE UseFilterField
////

////
//// BEGIN-EXAMPLE FilterFieldImpl
////
namespace viskores
{
namespace filter
{
namespace vector_calculus
{

VISKORES_CONT
FieldMagnitude::FieldMagnitude()
{
  this->SetOutputFieldName("");
}

VISKORES_CONT viskores::cont::DataSet FieldMagnitude::DoExecute(
  const viskores::cont::DataSet& inDataSet)
{
  viskores::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  viskores::cont::UnknownArrayHandle outField;

  // Use a C++ lambda expression to provide a callback for CastAndCall. The lambda
  // will capture references to local variables like outFieldArray (using `[&]`)
  // that it can read and write.
  auto resolveType = [&](const auto& inFieldArray)
  {
    using InArrayHandleType = std::decay_t<decltype(inFieldArray)>;
    using ComponentType =
      typename viskores::VecTraits<typename InArrayHandleType::ValueType>::ComponentType;

    viskores::cont::ArrayHandle<ComponentType> outFieldArray;

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
} // namespace viskores
////
//// END-EXAMPLE FilterFieldImpl
////

////
//// BEGIN-EXAMPLE RandomArrayAccess
////
namespace viskores
{
namespace worklet
{

struct ReverseArrayCopyWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputArray, WholeArrayOut outputArray);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  using InputDomain = _1;

  template<typename InputType, typename OutputArrayPortalType>
  VISKORES_EXEC void operator()(const InputType& inputValue,
                                const OutputArrayPortalType& outputArrayPortal,
                                viskores::Id workIndex) const
  {
    viskores::Id outIndex = outputArrayPortal.GetNumberOfValues() - workIndex - 1;
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
} // namespace viskores
////
//// END-EXAMPLE RandomArrayAccess
////

#include <viskores/cont/testing/Testing.h>

namespace
{

void Test()
{
  static const viskores::Id ARRAY_SIZE = 10;

  viskores::cont::ArrayHandle<viskores::Vec3f> inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());

  viskores::cont::ArrayHandle<viskores::FloatDefault> outputArray;

  viskores::cont::DataSet inputDataSet;
  viskores::cont::CellSetStructured<1> cellSet;
  cellSet.SetPointDimensions(ARRAY_SIZE);
  inputDataSet.SetCellSet(cellSet);
  inputDataSet.AddPointField("test_values", inputArray);

  viskores::filter::vector_calculus::FieldMagnitude fieldMagFilter;
  fieldMagFilter.SetActiveField("test_values");
  viskores::cont::DataSet magResult = fieldMagFilter.Execute(inputDataSet);
  magResult.GetField("test_values_magnitude").GetData().AsArrayHandle(outputArray);

  VISKORES_TEST_ASSERT(outputArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Bad output array size.");
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    viskores::Vec3f testValue = TestValue(index, viskores::Vec3f());
    viskores::Float64 expectedValue = sqrt(viskores::Dot(testValue, testValue));
    viskores::Float64 gotValue = outputArray.ReadPortal().Get(index);
    VISKORES_TEST_ASSERT(test_equal(expectedValue, gotValue), "Got bad value.");
  }

  viskores::cont::ArrayHandle<viskores::Vec3f> outputArray2;
  outputArray2.Allocate(ARRAY_SIZE);

  viskores::cont::Invoker invoker;
  invoker(viskores::worklet::ReverseArrayCopyWorklet{}, inputArray, outputArray2);

  VISKORES_TEST_ASSERT(outputArray2.GetNumberOfValues() == ARRAY_SIZE,
                       "Bad output array size.");
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    viskores::Vec3f expectedValue = TestValue(ARRAY_SIZE - index - 1, viskores::Vec3f());
    viskores::Vec3f gotValue = outputArray2.ReadPortal().Get(index);
    VISKORES_TEST_ASSERT(test_equal(expectedValue, gotValue), "Got bad value.");
  }
}

} // anonymous namespace

int GuideExampleUseWorkletMapField(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
