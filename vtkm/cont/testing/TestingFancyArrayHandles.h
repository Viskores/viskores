//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
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
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/VirtualObjectHandle.h>

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
    : Factor(1.0)
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

struct InverseValueScale
{
  InverseValueScale()
    : InverseFactor(1.0)
  {
  }

  InverseValueScale(vtkm::Float64 factor)
    : InverseFactor(1.0 / factor)
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
      vtkm::Float64 ri = vi * this->InverseFactor;
      Traits::SetComponent(result, i, static_cast<ComponentType>(ri));
    }
    return result;
  }

private:
  vtkm::Float64 InverseFactor;
};

template <typename ValueType>
struct VirtualTransformFunctorBase : public vtkm::VirtualObjectBase
{
  VirtualTransformFunctorBase() = default;

  VTKM_EXEC_CONT
  virtual ValueType operator()(const ValueType& v) const = 0;
};

template <typename ValueType, typename FunctorType>
struct VirtualTransformFunctor : VirtualTransformFunctorBase<ValueType>
{
  FunctorType Functor;

  VTKM_CONT
  VirtualTransformFunctor(const FunctorType& functor)
    : Functor(functor)
  {
  }

  VTKM_EXEC_CONT
  ValueType operator()(const ValueType& v) const override { return this->Functor(v); }
};

template <typename ValueType>
struct TransformExecObject : public vtkm::cont::ExecutionAndControlObjectBase
{
  vtkm::cont::VirtualObjectHandle<VirtualTransformFunctorBase<ValueType>> VirtualFunctor;

  VTKM_CONT TransformExecObject() = default;

  template <typename FunctorType>
  VTKM_CONT TransformExecObject(const FunctorType& functor)
  {
    // Need to make sure the serial device is supported, since that is what is used on the
    // control side. Therefore we reset to all supported devices.
    vtkm::cont::ScopedRuntimeDeviceTracker scopedTracker(
      vtkm::cont::DeviceAdapterTagSerial{}, vtkm::cont::RuntimeDeviceTrackerMode::Enable);
    this->VirtualFunctor.Reset(new VirtualTransformFunctor<ValueType, FunctorType>(functor));
  }

  struct FunctorWrapper
  {
    const VirtualTransformFunctorBase<ValueType>* FunctorPointer;

    FunctorWrapper() = default;

    VTKM_CONT
    FunctorWrapper(const VirtualTransformFunctorBase<ValueType>* functorPointer)
      : FunctorPointer(functorPointer)
    {
    }

    template <typename InValueType>
    VTKM_EXEC ValueType operator()(const InValueType& value) const
    {
      return (*this->FunctorPointer)(value);
    }
  };

  template <typename DeviceAdapterTag>
  VTKM_CONT FunctorWrapper PrepareForExecution(DeviceAdapterTag device) const
  {
    return FunctorWrapper(this->VirtualFunctor.PrepareForExecution(device));
  }

  VTKM_CONT FunctorWrapper PrepareForControl() const
  {
    return FunctorWrapper(this->VirtualFunctor.Get());
  }
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
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = _2(_1);

    template <class ValueType>
    VTKM_EXEC ValueType operator()(const ValueType& inValue) const
    {
      return inValue;
    }
  };

  struct InplaceFunctorPair : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldInOut);
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

  struct TestArrayPortalSOA
  {
    template <typename ComponentType>
    VTKM_CONT void operator()(ComponentType) const
    {
      constexpr vtkm::IdComponent NUM_COMPONENTS = 4;
      using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;
      using ComponentArrayType = vtkm::cont::ArrayHandle<ComponentType>;
      using SOAPortalType =
        vtkm::internal::ArrayPortalSOA<ValueType, typename ComponentArrayType::PortalControl>;

      std::cout << "Test SOA portal reflects data in component portals." << std::endl;
      SOAPortalType soaPortalIn(ARRAY_SIZE);

      std::array<vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS> implArrays;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        vtkm::cont::ArrayHandle<ComponentType> array;
        array.Allocate(ARRAY_SIZE);
        auto portal = array.GetPortalControl();
        for (vtkm::IdComponent valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          portal.Set(valueIndex, TestValue(valueIndex, ValueType{})[componentIndex]);
        }

        soaPortalIn.SetPortal(componentIndex, portal);

        implArrays[static_cast<std::size_t>(componentIndex)] = array;
      }

      VTKM_TEST_ASSERT(soaPortalIn.GetNumberOfValues() == ARRAY_SIZE);
      CheckPortal(soaPortalIn);

      std::cout << "Test data set in SOA portal gets set in component portals." << std::endl;
      SOAPortalType soaPortalOut(ARRAY_SIZE);
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        vtkm::cont::ArrayHandle<ComponentType> array;
        array.Allocate(ARRAY_SIZE);
        auto portal = array.GetPortalControl();
        soaPortalOut.SetPortal(componentIndex, portal);

        implArrays[static_cast<std::size_t>(componentIndex)] = array;
      }

      SetPortal(soaPortalOut);

      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        auto portal = implArrays[static_cast<size_t>(componentIndex)].GetPortalConstControl();
        for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          ComponentType x = TestValue(valueIndex, ValueType{})[componentIndex];
          VTKM_TEST_ASSERT(test_equal(x, portal.Get(valueIndex)));
        }
      }
    }
  };

  struct TestSOAAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using VTraits = vtkm::VecTraits<ValueType>;
      using ComponentType = typename VTraits::ComponentType;
      constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

      {
        vtkm::cont::ArrayHandleSOA<ValueType> soaArray;
        for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS;
             ++componentIndex)
        {
          vtkm::cont::ArrayHandle<ComponentType> componentArray;
          componentArray.Allocate(ARRAY_SIZE);
          auto componentPortal = componentArray.GetPortalControl();
          for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
          {
            componentPortal.Set(
              valueIndex,
              VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex));
          }
          soaArray.SetArray(componentIndex, componentArray);
        }

        VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        VTKM_TEST_ASSERT(soaArray.GetPortalConstControl().GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.GetPortalConstControl());

        vtkm::cont::ArrayHandle<ValueType> basicArray;
        vtkm::cont::ArrayCopy(soaArray, basicArray);
        VTKM_TEST_ASSERT(basicArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(basicArray.GetPortalConstControl());
      }

      {
        // Check constructors
        using Vec3 = vtkm::Vec<ComponentType, 3>;
        std::vector<ComponentType> vector0;
        std::vector<ComponentType> vector1;
        std::vector<ComponentType> vector2;
        for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          Vec3 value = TestValue(valueIndex, Vec3{});
          vector0.push_back(value[0]);
          vector1.push_back(value[1]);
          vector2.push_back(value[2]);
        }

        {
          vtkm::cont::ArrayHandleSOA<Vec3> soaArray = { vector0, vector1, vector2 };
          VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          vtkm::cont::ArrayHandleSOA<Vec3> soaArray =
            vtkm::cont::make_ArrayHandleSOA<Vec3>({ vector0, vector1, vector2 });
          VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          vtkm::cont::ArrayHandleSOA<Vec3> soaArray =
            vtkm::cont::make_ArrayHandleSOA(vector0, vector1, vector2);
          VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          vtkm::cont::ArrayHandleSOA<Vec3> soaArray = vtkm::cont::make_ArrayHandleSOA<Vec3>(
            { &vector0.front(), &vector1.front(), &vector2.front() }, ARRAY_SIZE);
          VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }

        {
          vtkm::cont::ArrayHandleSOA<Vec3> soaArray = vtkm::cont::make_ArrayHandleSOA(
            ARRAY_SIZE, &vector0.front(), &vector1.front(), &vector2.front());
          VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
          CheckPortal(soaArray.GetPortalConstControl());
        }
      }
    }
  };

  struct TestSOAAsOutput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using VTraits = vtkm::VecTraits<ValueType>;
      using ComponentType = typename VTraits::ComponentType;
      constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

      vtkm::cont::ArrayHandle<ValueType> basicArray;
      basicArray.Allocate(ARRAY_SIZE);
      SetPortal(basicArray.GetPortalControl());

      vtkm::cont::ArrayHandleSOA<ValueType> soaArray;
      vtkm::cont::ArrayCopy(basicArray, soaArray);

      VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        vtkm::cont::ArrayHandle<ComponentType> componentArray = soaArray.GetArray(componentIndex);
        auto componentPortal = componentArray.GetPortalConstControl();
        for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          ComponentType expected =
            VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex);
          ComponentType got = componentPortal.Get(valueIndex);
          VTKM_TEST_ASSERT(test_equal(expected, got));
        }
      }
    }
  };

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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

        vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

        vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

  struct TestViewAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      using FunctorType = ::fancy_array_detail::IndexSquared<ValueType>;

      using ValueHandleType = vtkm::cont::ArrayHandleImplicit<FunctorType>;
      using ViewHandleType = vtkm::cont::ArrayHandleView<ValueHandleType>;

      FunctorType functor;
      for (vtkm::Id start_pos = 0; start_pos < length; start_pos += length / 4)
      {
        const vtkm::Id counting_length = length - start_pos;

        ValueHandleType implicit = vtkm::cont::make_ArrayHandleImplicit(functor, length);

        ViewHandleType view =
          vtkm::cont::make_ArrayHandleView(implicit, start_pos, counting_length);

        vtkm::cont::printSummary_ArrayHandle(view, std::cout);
        std::cout << std::endl;

        vtkm::cont::ArrayHandle<ValueType> result;

        vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
        dispatcher.Invoke(view, result);

        //verify that the control portal works
        for (vtkm::Id i = 0; i < counting_length; ++i)
        {
          const vtkm::Id value_index = i;
          const vtkm::Id key_index = start_pos + i;

          const ValueType result_v = result.GetPortalConstControl().Get(value_index);
          const ValueType correct_value = implicit.GetPortalConstControl().Get(key_index);
          const ValueType control_value = view.GetPortalConstControl().Get(value_index);
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

  struct TestTransformVirtualAsInput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;
      using VirtualFunctorType = fancy_array_detail::TransformExecObject<ValueType>;

      const vtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);
      VirtualFunctorType virtualFunctor(functor);

      vtkm::cont::ArrayHandle<ValueType> input;
      auto transformed = vtkm::cont::make_ArrayHandleTransform(input, virtualFunctor);

      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      vtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

  struct TestCastAsOutput
  {
    template <typename CastFromType>
    VTKM_CONT void operator()(CastFromType vtkmNotUsed(type)) const
    {
      using InputArrayType = vtkm::cont::ArrayHandleIndex;
      using ResultArrayType = vtkm::cont::ArrayHandle<CastFromType>;

      InputArrayType input(ARRAY_SIZE);

      ResultArrayType result;
      vtkm::cont::ArrayHandleCast<vtkm::Id, ResultArrayType> castArray =
        vtkm::cont::make_ArrayHandleCast<CastFromType>(result);

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, castArray);

      vtkm::cont::printSummary_ArrayHandle(castArray, std::cout);
      std::cout << std::endl;

      // verify results
      vtkm::Id length = ARRAY_SIZE;
      for (vtkm::Id i = 0; i < length; ++i)
      {
        VTKM_TEST_ASSERT(input.GetPortalConstControl().Get(i) ==
                           static_cast<vtkm::Id>(result.GetPortalConstControl().Get(i)),
                         "Casting ArrayHandle Failed");
      }
    }
  };

  struct TestMultiplexerAsInput
  {
    vtkm::cont::Invoker Invoke;

    template <typename T>
    VTKM_CONT void operator()(T vtkmNotUsed(type)) const
    {
      using InputArrayType = vtkm::cont::ArrayHandleCounting<T>;

      InputArrayType input(T(1), T(2), ARRAY_SIZE);
      vtkm::cont::ArrayHandleMultiplexer<
        vtkm::cont::ArrayHandle<T>,
        InputArrayType,
        vtkm::cont::ArrayHandleCast<T, vtkm::cont::ArrayHandleIndex>>
        multiplexArray(input);
      vtkm::cont::ArrayHandle<T> result;

      this->Invoke(PassThrough{}, multiplexArray, result);

      vtkm::cont::printSummary_ArrayHandle(multiplexArray, std::cout);
      std::cout << std::endl;

      // verify results
      VTKM_TEST_ASSERT(
        test_equal_portals(result.GetPortalConstControl(), input.GetPortalConstControl()),
        "CastingArrayHandle failed");
    }
  };

  struct TestMultiplexerAsOutput
  {
    vtkm::cont::Invoker Invoke;

    template <typename CastFromType>
    VTKM_CONT void operator()(CastFromType vtkmNotUsed(type)) const
    {
      using InputArrayType = vtkm::cont::ArrayHandleIndex;
      using ResultArrayType = vtkm::cont::ArrayHandle<CastFromType>;

      InputArrayType input(ARRAY_SIZE);

      ResultArrayType result;
      vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<vtkm::Id>,
                                         vtkm::cont::ArrayHandleCast<vtkm::Id, ResultArrayType>>
        multiplexerArray = vtkm::cont::make_ArrayHandleCast<vtkm::Id>(result);

      this->Invoke(PassThrough{}, input, multiplexerArray);

      vtkm::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
      std::cout << std::endl;

      // verify results
      VTKM_TEST_ASSERT(
        test_equal_portals(input.GetPortalConstControl(), result.GetPortalConstControl()),
        "Multiplexing ArrayHandle failed");
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(_1, WorkIndex, _2);

    template <typename InputType>
    VTKM_EXEC void operator()(const InputType& input, vtkm::Id workIndex, vtkm::Id& dummyOut) const
    {
      using ComponentType = typename InputType::ComponentType;
      vtkm::IdComponent expectedSize = static_cast<vtkm::IdComponent>(workIndex + 1);
      if (expectedSize != input.GetNumberOfComponents())
      {
        this->RaiseError("Got unexpected number of components.");
      }

      vtkm::Id valueIndex = workIndex * (workIndex + 1) / 2;
      dummyOut = valueIndex;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < expectedSize; componentIndex++)
      {
        ComponentType expectedValue = TestValue(valueIndex, ComponentType());
        if (vtkm::Abs(expectedValue - input[componentIndex]) > 0.000001)
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

      vtkm::cont::ArrayHandle<vtkm::Id> dummyArray;

      vtkm::worklet::DispatcherMapField<GroupVariableInputWorklet> dispatcher;
      dispatcher.Invoke(vtkm::cont::make_ArrayHandleGroupVecVariable(sourceArray, offsetsArray),
                        dummyArray);

      dummyArray.GetPortalConstControl();
    }
  };

  // GroupVecVariable is a bit strange because it supports values of different
  // lengths, so a simple pass through worklet will not work. Use custom
  // worklets.
  struct GroupVariableOutputWorklet : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut);
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

      vtkm::worklet::DispatcherMapField<GroupVariableOutputWorklet> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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
      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

  struct TestViewAsOutput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      using ValueHandleType = vtkm::cont::ArrayHandle<ValueType>;
      using ViewHandleType = vtkm::cont::ArrayHandleView<ValueHandleType>;

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

      ViewHandleType view = vtkm::cont::make_ArrayHandleView(values, length, length);
      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, view);

      vtkm::cont::printSummary_ArrayHandle(view, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = view.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(ComponentType(i));
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Permutation Handle Failed As Output");
      }
    }
  };

  struct TestTransformAsOutput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;
      using InverseFunctorType = fancy_array_detail::InverseValueScale;

      const vtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);
      InverseFunctorType inverseFunctor(2.0);

      vtkm::cont::ArrayHandle<ValueType> input;
      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      vtkm::cont::ArrayHandle<ValueType> output;
      auto transformed = vtkm::cont::make_ArrayHandleTransform(output, functor, inverseFunctor);

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, transformed);

      vtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = output.GetPortalConstControl().Get(i);
        const ValueType correct_value = inverseFunctor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        VTKM_TEST_ASSERT(test_equal(functor(result_v), control_value),
                         "Transform Handle Control Failed");
      }
    }
  };

  struct TestTransformVirtualAsOutput
  {
    template <typename ValueType>
    VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      using FunctorType = fancy_array_detail::ValueScale;
      using InverseFunctorType = fancy_array_detail::InverseValueScale;

      using VirtualFunctorType = fancy_array_detail::TransformExecObject<ValueType>;

      const vtkm::Id length = ARRAY_SIZE;
      FunctorType functor(2.0);
      InverseFunctorType inverseFunctor(2.0);

      VirtualFunctorType virtualFunctor(functor);
      VirtualFunctorType virtualInverseFunctor(inverseFunctor);

      vtkm::cont::ArrayHandle<ValueType> input;
      input.Allocate(length);
      SetPortal(input.GetPortalControl());

      vtkm::cont::ArrayHandle<ValueType> output;
      auto transformed =
        vtkm::cont::make_ArrayHandleTransform(output, virtualFunctor, virtualInverseFunctor);

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
      dispatcher.Invoke(input, transformed);

      vtkm::cont::printSummary_ArrayHandle(transformed, std::cout);
      std::cout << std::endl;

      //verify that the control portal works
      for (vtkm::Id i = 0; i < length; ++i)
      {
        const ValueType result_v = output.GetPortalConstControl().Get(i);
        const ValueType correct_value = inverseFunctor(TestValue(i, ValueType()));
        const ValueType control_value = transformed.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value), "Transform Handle Failed");
        VTKM_TEST_ASSERT(test_equal(functor(result_v), control_value),
                         "Transform Handle Control Failed");
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

      vtkm::worklet::DispatcherMapField<PassThrough> dispatcher;
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

      vtkm::worklet::DispatcherMapField<InplaceFunctorPair> dispatcher;
      dispatcher.Invoke(vtkm::cont::make_ArrayHandleZip(inputValues, outputValues));

      vtkm::cont::printSummary_ArrayHandle(outputValues, std::cout);
      std::cout << std::endl;

      CheckPortal(outputValues.GetPortalConstControl());
    }
  };

  using ScalarTypesToTest = vtkm::List<vtkm::UInt8, vtkm::FloatDefault>;

  using ZipTypesToTest = vtkm::List<vtkm::Pair<vtkm::UInt8, vtkm::Id>,
                                    vtkm::Pair<vtkm::Float64, vtkm::Vec4ui_8>,
                                    vtkm::Pair<vtkm::Vec3f_32, vtkm::Vec4i_8>>;

  using HandleTypesToTest =
    vtkm::List<vtkm::Id, vtkm::Vec2i_32, vtkm::FloatDefault, vtkm::Vec3f_64>;

  using CastTypesToTest = vtkm::List<vtkm::Int32, vtkm::UInt32>;

  struct TestAll
  {
    VTKM_CONT void operator()() const
    {
      std::cout << "Doing FancyArrayHandle tests" << std::endl;

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayPortalSOA" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestArrayPortalSOA(), ScalarTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleSOA as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(TestingFancyArrayHandles<DeviceAdapterTag>::TestSOAAsInput(),
                                       HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleSOA as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestSOAAsOutput(), HandleTypesToTest());

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
      std::cout << "Testing ArrayHandleView as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestViewAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformAsInput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with virtual as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformVirtualAsInput(),
        HandleTypesToTest());

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
      std::cout << "Testing ArrayHandleCast as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestCastAsOutput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleMultiplexer as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestMultiplexerAsInput(), CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleMultiplexer as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestMultiplexerAsOutput(), CastTypesToTest());

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
      std::cout << "Testing ArrayHandleView as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestViewAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformAsOutput(), HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with virtual as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
        TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformVirtualAsOutput(),
        HandleTypesToTest());

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
  static VTKM_CONT int Run(int argc, char* argv[])
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return vtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingFancyArrayHandles_h
