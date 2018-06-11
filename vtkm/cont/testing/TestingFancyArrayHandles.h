//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_testing_TestingFancyArrayHandles_h
#define vtk_m_cont_testing_TestingFancyArrayHandles_h

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleDiscard.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace fancy_array_detail
{

template <typename ValueType>
struct IndexSquared
{
  VTKM_EXEC_CONT
  ValueType operator()(vtkm::Id index) const
  {
    using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(index * index));
  }
};

template <typename ValueType>
struct ValueSquared
{
  template <typename U>
  VTKM_EXEC_CONT ValueType operator()(U u) const
  {
    return vtkm::Dot(u, u);
  }
};

struct ValueScale
{
  ValueScale()
    : Factor()
  {
  }

  ValueScale(vtkm::Float64 factor)
    : Factor(factor)
  {
  }

  template <typename ValueType>
  VTKM_EXEC_CONT ValueType operator()(const ValueType& v) const
  {
    using Traits = vtkm::VecTraits<ValueType>;
    using TTraits = vtkm::TypeTraits<ValueType>;
    using ComponentType = typename Traits::ComponentType;

    ValueType result = TTraits::ZeroInitialization();
    for (vtkm::IdComponent i = 0; i < Traits::GetNumberOfComponents(v); ++i)
    {
      vtkm::Float64 vi = static_cast<vtkm::Float64>(Traits::GetComponent(v, i));
      vtkm::Float64 ri = vi * this->Factor;
      Traits::SetComponent(result, i, static_cast<ComponentType>(ri));
    }
    return result;
  }

private:
  vtkm::Float64 Factor;
};
}

namespace vtkm
{
namespace cont
{
namespace testing
{

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with the given DeviceAdapter
///
template <class DeviceAdapterTag>
struct TestingFancyArrayHandles
{

private:
  static const int ARRAY_SIZE = 10;

public:
  struct PassThrough : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<>, FieldOut<>);
    using ExecutionSignature = _2(_1);

    template <class ValueType>
    VTKM_EXEC ValueType operator()(const ValueType& inValue) const
    {
      return inValue;
    }
  };

  struct InplaceFunctorPair : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldInOut<>);
    using ExecutionSignature = void(_1);

    template <typename T>
    VTKM_EXEC void operator()(vtkm::Pair<T, T>& value) const
    {
      value.second = value.first;
    }
  };

#ifndef VTKM_CUDA
private:
#endif

  struct TestCompositeAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const ValueType value = TestValue(13, ValueType());
      std::vector<ValueType> compositeData(ARRAY_SIZE, value);
      vtkm::cont::ArrayHandle<ValueType> compositeInput =
        vtkm::cont::make_ArrayHandle(compositeData);

      auto composite =
        vtkm::cont::make_ArrayHandleCompositeVector(compositeInput, compositeInput, compositeInput);

      vtkm::cont::printSummary_ArrayHandle(composite, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<vtkm::Vec<ValueType, 3>> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(composite, result);

      //verify that the control portal works
      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const vtkm::Vec<ValueType, 3> result_v = result.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, vtkm::Vec<ValueType, 3>(value)),
                         "CompositeVector Handle Failed");

        const vtkm::Vec<ValueType, 3> result_c = composite.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_c, vtkm::Vec<ValueType, 3>(value)),
                         "CompositeVector Handle Failed");
      }
    }
  };

  struct TestConstantAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const ValueType value = TestValue(43, ValueType());

      vtkm::cont::ArrayHandleConstant<ValueType> constant =
        vtkm::cont::make_ArrayHandleConstant(value, ARRAY_SIZE);
      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(constant, result);

      vtkm::cont::printSummary_ArrayHandle(constant, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType control_value = constant.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, value), "Counting Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Counting Handle Control Failed");
      }
    }
  };

  struct TestCountingAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

      const vtkm::Id length = ARRAY_SIZE;

      //need to initialize the start value or else vectors will have
      //random values to start
      ComponentType component_value(0);
      const ValueType start = ValueType(component_value);

      vtkm::cont::ArrayHandleCounting<ValueType> counting =
        vtkm::cont::make_ArrayHandleCounting(start, ValueType(1), length);
      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(counting, result);

      vtkm::cont::printSummary_ArrayHandle(counting, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(component_value);
        const ValueType control_value = counting.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Counting Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Counting Handle Control Failed");
        component_value = ComponentType(component_value + ComponentType(1));
      }
    }
  };

  struct TestImplicitAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;
      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;
      FunctorType functor;

      vtkm::cont::ArrayHandleImplicit<FunctorType> implicit =
        vtkm::cont::make_ArrayHandleImplicit(functor, length);

      vtkm::cont::printSummary_ArrayHandle(implicit, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(implicit, result);

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = functor(i);
        const ValueType control_value = implicit.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
      }
    }
  };

  struct TestConcatenateAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;
      using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

      using ValueHandleType = vtkm::cont::ArrayHandleImplicit<FunctorType>;
      using BasicArrayType = vtkm::cont::ArrayHandle<ValueType>;
      using ConcatenateType = vtkm::cont::ArrayHandleConcatenate<ValueHandleType, BasicArrayType>;

      FunctorType functor;
      for (vtkm::Id start_pos = 0; start_pos < length; start_pos += length / 4)
      {
        vtkm::Id implicitLen = length - start_pos;
        vtkm::Id basicLen = start_pos;

        // make an implicit array
        ValueHandleType implicit = vtkm::cont::make_ArrayHandleImplicit(functor, implicitLen);
        // make a basic array
        std::vector<ValueType> basicVec;
        for (vtkm::Id i = 0; i < basicLen; i++)
        {
          basicVec.push_back(ValueType(static_cast<ComponentType>(i)));
          basicVec.push_back(ValueType(ComponentType(i)));
        }
        BasicArrayType basic = vtkm::cont::make_ArrayHandle(basicVec);

        // concatenate two arrays together
        ConcatenateType concatenate = vtkm::cont::make_ArrayHandleConcatenate(implicit, basic);
        vtkm::cont::printSummary_ArrayHandle(concatenate, std::cout);
        std::cout << std::endl;

        vtkm::cont::ArrayHandle<ValueType> result;

        vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
        dispatcher.Invoke(concatenate, result);

        //verify that the control portal works
        for (vtkm::Id i = 0; i < length; ++i)
        {
          const ValueType result_v = result.GetPortalConstControl().Get(i);
          ValueType correct_value;
          if (i < implicitLen)
            correct_value = implicit.GetPortalConstControl().Get(i);
          else
            correct_value = basic.GetPortalConstControl().Get(i - implicitLen);
          const ValueType control_value = concatenate.GetPortalConstControl().Get(i);
          VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                           "ArrayHandleConcatenate as Input Failed");
          VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                           "ArrayHandleConcatenate as Input Failed");
        }
      }
    }
  };

  struct TestPermutationAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;

      using KeyHandleType = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
      using ValueHandleType = vtkm::cont::ArrayHandleImplicit<FunctorType>;
      using PermutationHandleType =
        vtkm::cont::ArrayHandlePermutation<KeyHandleType, ValueHandleType>;

      FunctorType functor;
      for (vtkm::Id start_pos = 0; start_pos < length; start_pos += length / 4)
      {
        const vtkm::Id counting_length = length - start_pos;

        KeyHandleType counting =
          vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(start_pos, 1, counting_length);

        ValueHandleType implicit = vtkm::cont::make_ArrayHandleImplicit(functor, length);

        PermutationHandleType permutation =
          vtkm::cont::make_ArrayHandlePermutation(counting, implicit);

        vtkm::cont::printSummary_ArrayHandle(permutation, std::cout);
        std::cout << std::endl;

        vtkm::cont::ArrayHandle<ValueType> result;

        vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
        dispatcher.Invoke(permutation, result);

        //verify that the control portal works
        for (vtkm::Id i = 0; i < counting_length; ++i)
        {
          const vtkm::Id value_index = i;
          const vtkm::Id key_index = start_pos + i;

          const ValueType result_v = result.GetPortalConstControl().Get(value_index);
          const ValueType correct_value = implicit.GetPortalConstControl().Get(key_index);
          const ValueType control_value = permutation.GetPortalConstControl().Get(value_index);
          VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Implicit Handle Failed");
          VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Implicit Handle Failed");
        }
      }
    }
  };

  struct TestTransformAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;

      const vtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);

      vtkm::cont::ArrayHandle<ValueType> input;
      vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandle<ValueType>, FunctorType>
        transformed = vtkm::cont::make_ArrayHandleTransform(input, functor);

      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      vtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(transformed, result);

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = functor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value), "Transform Handle Control Failed");
      }
    }
  };

  struct TestCountingTransformAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
      using OutputValueType = ComponentType;
      using FunctorType = fancy_array_detail::ValueSquared<OutputValueType>;

      vtkm::Id length = ARRAY_SIZE;
      FunctorType functor;

      //need to initialize the start value or else vectors will have
      //random values to start
      ComponentType component_value(0);
      const ValueType start = ValueType(component_value);

      vtkm::cont::ArrayHandleCounting<ValueType> counting(start, ValueType(1), length);

      vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandleCounting<ValueType>, FunctorType>
        countingTransformed = vtkm::cont::make_ArrayHandleTransform(counting, functor);

      vtkm::cont::printSummary_ArrayHandle(countingTransformed, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<OutputValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(countingTransformed, result);

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const OutputValueType result_v = result.GetPortalConstControl().Get(i);
        const OutputValueType correct_value = functor(ValueType(component_value));
        const OutputValueType control_value = countingTransformed.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Counting Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                         "Transform Counting Handle Control Failed");
        component_value = ComponentType(component_value + ComponentType(1));
      }
    }
  };

  struct TestCastAsInput
  {
    template <typename CastToType>
    VTKM_CONT void operator()(CastToType vtkmNotUsed(type)) const
    {
      using InputArrayType = vtkm::cont::ArrayHandleIndex;

      InputArrayType input(ARRAY_SIZE);
      vtkm::cont::ArrayHandleCast<CastToType, InputArrayType> castArray =
        vtkm::cont::make_ArrayHandleCast(input, CastToType());
      vtkm::cont::ArrayHandle<CastToType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(castArray, result);

      vtkm::cont::printSummary_ArrayHandle(castArray, std::cout);
      std::cout << std::endl;

      // verify results
      vtkm::Id length = ARRAY_SIZE;
      for (vtkm::Id i = 0; i < length; ++i)
      {
        VTKM_TEST_ASSERT(result.GetPortalConstControl().Get(i) ==
                           static_cast<CastToType>(input.GetPortalConstControl().Get(i)),
                         "Casting ArrayHandle Failed");
      }
    }
  };

  template <vtkm::IdComponent NUM_COMPONENTS>
  struct TestGroupVecAsInput
  {
    template <typename ComponentType>
    VTKM_CONT void operator()(ComponentType) const
    {
      using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

      ComponentType testValues[ARRAY_SIZE * NUM_COMPONENTS];

      for (vtkm::Id index = 0; index < ARRAY_SIZE * NUM_COMPONENTS; ++index)
      {
        testValues[index] = TestValue(index, ComponentType());
      }
      vtkm::cont::ArrayHandle<ComponentType> baseArray =
        vtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE * NUM_COMPONENTS);

      vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
        groupArray(baseArray);
      VTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Group array reporting wrong array size.");

      vtkm::cont::printSummary_ArrayHandle(groupArray, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<ValueType> resultArray;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(groupArray, resultArray);

      VTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE, "Got bad result array size.");

      //verify that the control portal works
      vtkm::Id totalIndex = 0;
      for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        const ValueType result = resultArray.GetPortalConstControl().Get(index);
        for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
             componentIndex++)
        {
          const ComponentType expectedValue = TestValue(totalIndex, ComponentType());
          VTKM_TEST_ASSERT(test_equal(result[componentIndex], expectedValue),
                           "Result array got wrong value.");
          totalIndex++;
        }
      }
    }
  };

  template <vtkm::IdComponent NUM_COMPONENTS>
  struct TestGroupVecAsOutput
  {
    template <typename ComponentType>
    VTKM_CONT void operator()(ComponentType) const
    {
      using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;

      vtkm::cont::ArrayHandle<ValueType> baseArray;
      baseArray.Allocate(ARRAY_SIZE);
      SetPortal(baseArray.GetPortalControl());

      vtkm::cont::ArrayHandle<ComponentType> resultArray;

      vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS>
        groupArray(resultArray);

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(baseArray, groupArray);

      vtkm::cont::printSummary_ArrayHandle(groupArray, std::cout);
      std::cout << std::endl;
      vtkm::cont::printSummary_ArrayHandle(resultArray, std::cout);
      std::cout << std::endl;

      VTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Group array reporting wrong array size.");

      VTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE * NUM_COMPONENTS,
                       "Got bad result array size.");

      //verify that the control portal works
      vtkm::Id totalIndex = 0;
      for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        const ValueType expectedValue = TestValue(index, ValueType());
        for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
             componentIndex++)
        {
          const ComponentType result = resultArray.GetPortalConstControl().Get(totalIndex);
          VTKM_TEST_ASSERT(test_equal(result, expectedValue[componentIndex]),
                           "Result array got wrong value.");
          totalIndex++;
        }
      }
    }
  };

  // GroupVecVariable is a bit strange because it supports values of different
  // lengths, so a simple pass through worklet will not work. Use custom
  // worklets.
  struct GroupVariableInputWorklet : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<>);
    using ExecutionSignature = void(_1, WorkIndex);

    template <typename InputType>
    VTKM_EXEC void operator()(const InputType& input, vtkm::Id workIndex) const
    {
      using ComponentType = typename InputType::ComponentType;
      vtkm::IdComponent expectedSize = static_cast<vtkm::IdComponent>(workIndex + 1);
      if (expectedSize != input.GetNumberOfComponents())
      {
        this->RaiseError("Got unexpected number of components.");
      }

      vtkm::Id valueIndex = workIndex * (workIndex + 1) / 2;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
      {
        ComponentType expectedValue = TestValue(valueIndex, ComponentType());
        if (expectedValue != input[componentIndex])
        {
          this->RaiseError("Got bad value in GroupVariableInputWorklet.");
        }
        valueIndex++;
      }
    }
  };

  struct TestGroupVecVariableAsInput
  {
    template <typename ComponentType>
    VTKM_CONT void operator()(ComponentType) const
    {
      vtkm::Id sourceArraySize;

      vtkm::cont::ArrayHandleCounting<vtkm::IdComponent> numComponentsArray(1, 1, ARRAY_SIZE);
      vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray =
        vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, sourceArraySize);

      vtkm::cont::ArrayHandle<ComponentType> sourceArray;
      sourceArray.Allocate(sourceArraySize);
      SetPortal(sourceArray.GetPortalControl());

      vtkm::cont::printSummary_ArrayHandle(
        vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray), std::cout);
      std::cout << std::endl;

      vtkm::worklet::DispatcherMapField<GroupVariableInputWorklet, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray));
    }
  };

  // GroupVecVariable is a bit strange because it supports values of different
  // lengths, so a simple pass through worklet will not work. Use custom
  // worklets.
  struct GroupVariableOutputWorklet : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<>, FieldOut<>);
    using ExecutionSignature = void(_2, WorkIndex);

    template <typename OutputType>
    VTKM_EXEC void operator()(OutputType& output, vtkm::Id workIndex) const
    {
      using ComponentType = typename OutputType::ComponentType;
      vtkm::IdComponent expectedSize = static_cast<vtkm::IdComponent>(workIndex + 1);
      if (expectedSize != output.GetNumberOfComponents())
      {
        this->RaiseError("Got unexpected number of components.");
      }

      vtkm::Id valueIndex = workIndex * (workIndex + 1) / 2;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
      {
        output[componentIndex] = TestValue(valueIndex, ComponentType());
        valueIndex++;
      }
    }
  };

  struct TestGroupVecVariableAsOutput
  {
    template <typename ComponentType>
    VTKM_CONT void operator()(ComponentType) const
    {
      vtkm::Id sourceArraySize;

      vtkm::cont::ArrayHandleCounting<vtkm::IdComponent> numComponentsArray(1, 1, ARRAY_SIZE);
      vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray = vtkm::cont::ConvertNumComponentsToOffsets(
        numComponentsArray, sourceArraySize, DeviceAdapterTag());

      vtkm::cont::ArrayHandle<ComponentType> sourceArray;
      sourceArray.Allocate(sourceArraySize);

      vtkm::worklet::DispatcherMapField<GroupVariableOutputWorklet, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE),
                        vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray));

      vtkm::cont::printSummary_ArrayHandle(
        vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray), std::cout);
      std::cout << std::endl;
      vtkm::cont::printSummary_ArrayHandle(sourceArray, std::cout);
      std::cout << std::endl;

      CheckPortal(sourceArray.GetPortalConstControl());
    }
  };

  struct TestZipAsInput
  {
    template <typename KeyType, typename ValueType>
    VTKM_CONT void operator()(vtkm::Pair<KeyType, ValueType> vtkmNotUsed(pair)) const
    {
      using PairType = vtkm::Pair<KeyType, ValueType>;
      using KeyComponentType = typename vtkm::VecTraits<KeyType>::ComponentType;
      using ValueComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

      KeyType testKeys[ARRAY_SIZE];
      ValueType testValues[ARRAY_SIZE];

      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        testKeys[i] = KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i));
        testValues[i] = ValueType(static_cast<ValueComponentType>(i));
      }
      vtkm::cont::ArrayHandle<KeyType> keys = vtkm::cont::make_ArrayHandle(testKeys, ARRAY_SIZE);
      vtkm::cont::ArrayHandle<ValueType> values =
        vtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE);

      vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<KeyType>,
                                 vtkm::cont::ArrayHandle<ValueType>>
        zip = vtkm::cont::make_ArrayHandleZip(keys, values);

      vtkm::cont::printSummary_ArrayHandle(zip, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<PairType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(zip, result);

      //verify that the control portal works
      for (int i = 0; i < ARRAY_SIZE; ++i)
      {
        const PairType result_v = result.GetPortalConstControl().Get(i);
        const PairType correct_value(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                     ValueType(static_cast<ValueComponentType>(i)));
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "ArrayHandleZip Failed as input");
      }
    }
  };

  struct TestDiscardAsOutput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using DiscardHandleType = vtkm::cont::ArrayHandleDiscard<ValueType>;
      using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

      using Portal = typename vtkm::cont::ArrayHandle<ValueType>::PortalControl;

      const vtkm::Id length = ARRAY_SIZE;

      vtkm::cont::ArrayHandle<ValueType> input;
      input.Allocate(length);
      Portal inputPortal = input.GetPortalControl();
      for (vtkm::Id i = 0; i < length; ++i)
      {
        inputPortal.Set(i, ValueType(ComponentType(i)));
      }

      DiscardHandleType discard;
      discard.Allocate(length);

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(input, discard);

      // No output to verify since none is stored in memory. Just checking that
      // this compiles/runs without errors.
    }
  };

  struct TestPermutationAsOutput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      using KeyHandleType = vtkm::cont::ArrayHandleCounting<vtkm::Id>;
      using ValueHandleType = vtkm::cont::ArrayHandle<ValueType>;
      using PermutationHandleType =
        vtkm::cont::ArrayHandlePermutation<KeyHandleType, ValueHandleType>;

      using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
      vtkm::cont::ArrayHandle<ValueType> input;
      using Portal = typename vtkm::cont::ArrayHandle<ValueType>::PortalControl;
      input.Allocate(length);
      Portal inputPortal = input.GetPortalControl();
      for (vtkm::Id i = 0; i < length; ++i)
      {
        inputPortal.Set(i, ValueType(ComponentType(i)));
      }

      ValueHandleType values;
      values.Allocate(length * 2);

      KeyHandleType counting = vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(length, 1, length);

      PermutationHandleType permutation = vtkm::cont::make_ArrayHandlePermutation(counting, values);
      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(input, permutation);

      vtkm::cont::printSummary_ArrayHandle(permutation, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = permutation.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(ComponentType(i));
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Permutation Handle Failed As Output");
      }
    }
  };

  struct TestZipAsOutput
  {
    template <typename KeyType, typename ValueType>
    VTKM_CONT void operator()(vtkm::Pair<KeyType, ValueType> vtkmNotUsed(pair)) const
    {
      using PairType = vtkm::Pair<KeyType, ValueType>;
      using KeyComponentType = typename vtkm::VecTraits<KeyType>::ComponentType;
      using ValueComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

      PairType testKeysAndValues[ARRAY_SIZE];
      for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        testKeysAndValues[i] = PairType(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                                        ValueType(static_cast<ValueComponentType>(i)));
      }
      vtkm::cont::ArrayHandle<PairType> input =
        vtkm::cont::make_ArrayHandle(testKeysAndValues, ARRAY_SIZE);

      vtkm::cont::ArrayHandle<KeyType> result_keys;
      vtkm::cont::ArrayHandle<ValueType> result_values;
      vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<KeyType>,
                                 vtkm::cont::ArrayHandle<ValueType>>
        result_zip = vtkm::cont::make_ArrayHandleZip(result_keys, result_values);

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(input, result_zip);

      vtkm::cont::printSummary_ArrayHandle(result_zip, std::cout);
      std::cout << std::endl;

      //now the two arrays we have zipped should have data inside them
      for (int i = 0; i < ARRAY_SIZE; ++i)
      {
        const KeyType result_key = result_keys.GetPortalConstControl().Get(i);
        const ValueType result_value = result_values.GetPortalConstControl().Get(i);

        VTKM_TEST_ASSERT(
          test_equal(result_key, KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i))),
          "ArrayHandleZip Failed as input for key");
        VTKM_TEST_ASSERT(test_equal(result_value, ValueType(static_cast<ValueComponentType>(i))),
                         "ArrayHandleZip Failed as input for value");
      }
    }
  };

  struct TestZipAsInPlace
  {
    template <typename ValueType>
    VTKM_CONT void operator()(ValueType) const
    {
      vtkm::cont::ArrayHandle<ValueType> inputValues;
      inputValues.Allocate(ARRAY_SIZE);
      SetPortal(inputValues.GetPortalControl());

      vtkm::cont::ArrayHandle<ValueType> outputValues;
      outputValues.Allocate(ARRAY_SIZE);

      vtkm::worklet::DispatcherMapField<InplaceFunctorPair, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(vtkm::cont::make_ArrayHandleZip(inputValues, outputValues));

      vtkm::cont::printSummary_ArrayHandle(outputValues, std::cout);
      std::cout << std::endl;

      CheckPortal(outputValues.GetPortalConstControl());
    }
  };

  struct ScalarTypesToTest : vtkm::ListTagBase<vtkm::UInt8, vtkm::FloatDefault>
  {
  };

  struct ZipTypesToTest
    : vtkm::ListTagBase<vtkm::Pair<vtkm::UInt8, vtkm::Id>,
                        vtkm::Pair<vtkm::Float64, vtkm::Vec<vtkm::UInt8, 4>>,
                        vtkm::Pair<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Int8, 4>>>
  {
  };

  struct HandleTypesToTest : vtkm::ListTagBase<vtkm::Id,
                                               vtkm::Vec<vtkm::Int32, 2>,
                                               vtkm::FloatDefault,
                                               vtkm::Vec<vtkm::Float64, 3>>
  {
  };

  struct CastTypesToTest : vtkm::ListTagBase<vtkm::Int32, vtkm::UInt32>
  {
  };

  struct TestAll
  {
    VTKM_CONT void operator()() const
    {
      std::cout << "Doing FancyArrayHandle tests" << std::endl;

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCompositeVector as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCompositeAsInput(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConstant as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestConstantAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCounting as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCountingAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleImplicit as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestImplicitAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandlePermutation as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestPermutationAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with Counting as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCountingTransformAsInput(),
        HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCast as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCastAsInput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<3> as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsInput<3>(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<4> as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsInput<4>(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<2> as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsOutput<2>(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<3> as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsOutput<3>(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVecVariable as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecVariableAsInput(),
        ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVecVariable as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecVariableAsOutput(),
        ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsInput(),
                                       ZipTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandlePermutation as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestPermutationAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleDiscard as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestDiscardAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsOutput(), ZipTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as In Place" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsInPlace(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConcatenate as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestConcatenateAsInput(), HandleTypesToTest());
    }
  };

public:
  /// Run a suite of tests to check to see if a DeviceAdapter properly supports
  /// all the fancy array handles that vtkm supports. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  static VTKM_CONT int Run() { return vtkm::cont::testing::Testing::Run(TestAll()); }
};
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingFancyArrayHandles_h
