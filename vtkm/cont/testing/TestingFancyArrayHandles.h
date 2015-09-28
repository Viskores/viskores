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
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_testing_TestingFancyArrayHandles_h
#define vtk_m_cont_testing_TestingFancyArrayHandles_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/VecTraits.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace fancy_array_detail
{

template<typename ValueType>
struct IndexSquared
{
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(vtkm::Id index) const
  {
    typedef typename vtkm::VecTraits<ValueType>::ComponentType ComponentType;
    return ValueType( static_cast<ComponentType>(index*index) );
  }
};

template<typename ValueType>
struct ValueSquared
{
  template<typename U>
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(U u) const
    { return vtkm::dot(u, u); }
};

}

namespace vtkm {
namespace cont {
namespace testing {

/// This class has a single static member, Run, that tests that all Fancy Array
/// Handles work with the given DeviceAdapter
///
template<class DeviceAdapterTag>
struct TestingFancyArrayHandles
{

private:
  static const int ARRAY_SIZE = 10;

public:
  struct PassThrough : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef _2 ExecutionSignature(_1);

  template<class ValueType>
  VTKM_EXEC_EXPORT
  ValueType operator()(const ValueType &inValue) const
  { return inValue; }

};


private:

  struct TestCompositeAsInput
  {
    template< typename ValueType >
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      //hard-coded to make a vtkm::Vec<ValueType,3> composite vector
      //for each ValueType.

      typedef typename vtkm::cont::ArrayHandleCompositeVectorType<
        vtkm::cont::ArrayHandle< ValueType >,
        vtkm::cont::ArrayHandle< ValueType >,
        vtkm::cont::ArrayHandle< ValueType > >::type CompositeHandleType;

      const ValueType value = TestValue(13, ValueType());
      std::vector< ValueType > compositeData( ARRAY_SIZE, value );
      vtkm::cont::ArrayHandle< ValueType > compositeInput =
                                vtkm::cont::make_ArrayHandle(compositeData);

      CompositeHandleType composite =
          vtkm::cont::make_ArrayHandleCompositeVector(compositeInput, 0,
                                                      compositeInput, 1,
                                                      compositeInput, 2);

      vtkm::cont::ArrayHandle< vtkm::Vec<ValueType, 3> > result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(composite, result);

      //verify that the control portal works
      for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
        {
        const vtkm::Vec<ValueType, 3> result_v =
                                    result.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, vtkm::Vec<ValueType, 3>(value)),
                        "CompositeVector Handle Failed");
        }
    }
  };


  struct TestConstantAsInput
  {
    template< typename ValueType >
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const ValueType value = TestValue(43, ValueType());

      vtkm::cont::ArrayHandleConstant<ValueType> constant =
          vtkm::cont::make_ArrayHandleConstant(value, ARRAY_SIZE);
      vtkm::cont::ArrayHandle<ValueType> result;

      vtkm::worklet::DispatcherMapField<PassThrough, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(constant, result);

      //verify that the control portal works
      for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
        {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType control_value = constant.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, value),
                         "Counting Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                         "Counting Handle Control Failed");
        }
    }
  };

  struct TestCountingAsInput
  {
    template< typename ValueType >
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      typedef typename vtkm::VecTraits<ValueType>::ComponentType ComponentType;

      const vtkm::Id length = ARRAY_SIZE;

      //need to initialize the start value or else vectors will have
      //random values to start
      ComponentType component_value(0);
      const ValueType start = ValueType(component_value);

      vtkm::cont::ArrayHandleCounting< ValueType > counting =
          vtkm::cont::make_ArrayHandleCounting(start, ValueType(1), length);
      vtkm::cont::ArrayHandle< ValueType > result;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(counting, result);

      //verify that the control portal works
      for(vtkm::Id i=0; i < length; ++i)
        {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = ValueType(component_value);
        const ValueType control_value = counting.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Counting Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                         "Counting Handle Control Failed");
        component_value = ComponentType(component_value + ComponentType(1));
        }
    }
  };

  struct TestImplicitAsInput
  {
    template< typename ValueType>
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;
      typedef ::fancy_array_detail::IndexSquared<ValueType> FunctorType;
      FunctorType functor;

      vtkm::cont::ArrayHandleImplicit< ValueType, FunctorType > implicit =
          vtkm::cont::make_ArrayHandleImplicit<ValueType>(functor, length);

      vtkm::cont::ArrayHandle< ValueType > result;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(implicit, result);

      //verify that the control portal works
      for(vtkm::Id i=0; i < length; ++i)
        {
        const ValueType result_v = result.GetPortalConstControl().Get(i);
        const ValueType correct_value = functor( i );
        const ValueType control_value = implicit.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Implicit Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value)
                         , "Implicit Handle Failed");
        }
    }
  };

  struct TestPermutationAsInput
  {
    template< typename ValueType>
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      typedef ::fancy_array_detail::IndexSquared<ValueType> FunctorType;

      typedef vtkm::cont::ArrayHandleCounting< vtkm::Id > KeyHandleType;
      typedef vtkm::cont::ArrayHandleImplicit< ValueType,
                                               FunctorType > ValueHandleType;
      typedef vtkm::cont::ArrayHandlePermutation< KeyHandleType,
                                                  ValueHandleType
                                                  > PermutationHandleType;

      FunctorType functor;
      for( vtkm::Id start_pos = 0; start_pos < (length-10); start_pos+=10)
        {
        const vtkm::Id counting_length = length - start_pos;

        KeyHandleType counting =
            vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(start_pos,
                                                           1,
                                                           counting_length);

        ValueHandleType implicit =
            vtkm::cont::make_ArrayHandleImplicit<ValueType>(functor,
                                                            length);

        PermutationHandleType permutation =
            vtkm::cont::make_ArrayHandlePermutation(counting,
                                                    implicit);

        vtkm::cont::ArrayHandle< ValueType > result;

        vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
        dispatcher.Invoke(permutation, result);

        //verify that the control portal works
        for(vtkm::Id i=0; i <counting_length; ++i)
          {
          const vtkm::Id value_index = i;
          const vtkm::Id key_index = start_pos + i;

          const ValueType result_v = result.GetPortalConstControl().Get( value_index );
          const ValueType correct_value = implicit.GetPortalConstControl().Get( key_index );
          const ValueType control_value = permutation.GetPortalConstControl().Get( value_index );
          VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                           "Implicit Handle Failed");
          VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                           "Implicit Handle Failed");
          }
        }
    }
  };

  struct TestTransformAsInput
  {
    template< typename ValueType>
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      typedef typename vtkm::VecTraits<ValueType>::ComponentType OutputValueType;
      typedef fancy_array_detail::ValueSquared<OutputValueType> FunctorType;

      const vtkm::Id length = ARRAY_SIZE;
      FunctorType functor;

      vtkm::cont::ArrayHandle<ValueType> input;
      vtkm::cont::ArrayHandleTransform<
          OutputValueType,
          vtkm::cont::ArrayHandle<ValueType>,
          FunctorType> transformed =
            vtkm::cont::make_ArrayHandleTransform<OutputValueType>(input,
                                                                   functor);

      typedef typename vtkm::cont::ArrayHandle<ValueType>::PortalControl Portal;
      input.Allocate(length);
      Portal portal = input.GetPortalControl();
      for(vtkm::Id i=0; i < length; ++i)
        {
        portal.Set(i, TestValue(i, ValueType()) );
        }

      vtkm::cont::ArrayHandle< OutputValueType > result;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(transformed, result);

    //verify that the control portal works
    for(vtkm::Id i=0; i < length; ++i)
      {
      const OutputValueType result_v = result.GetPortalConstControl().Get(i);
      const OutputValueType correct_value = functor(TestValue(i, ValueType()));
      const OutputValueType control_value =
          transformed.GetPortalConstControl().Get(i);
      VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                       "Transform Handle Failed");
      VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                       "Transform Handle Control Failed");
      }
    }
  };

  struct TestCountingTransformAsInput
  {
    template< typename ValueType>
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      typedef typename vtkm::VecTraits<ValueType>::ComponentType ComponentType;
      typedef ComponentType OutputValueType;
      typedef fancy_array_detail::ValueSquared<OutputValueType> FunctorType;

      vtkm::Id length = ARRAY_SIZE;
      FunctorType functor;

      //need to initialize the start value or else vectors will have
      //random values to start
      ComponentType component_value(0);
      const ValueType start = ValueType(component_value);

      vtkm::cont::ArrayHandleCounting< ValueType > counting(start,
                                                            ValueType(1),
                                                            length);

      vtkm::cont::ArrayHandleTransform<
          OutputValueType,
          vtkm::cont::ArrayHandleCounting<ValueType>,
          FunctorType>
          countingTransformed =
            vtkm::cont::make_ArrayHandleTransform<OutputValueType>(counting,
                                                                   functor);

      vtkm::cont::ArrayHandle< OutputValueType > result;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(countingTransformed, result);

      //verify that the control portal works
      for(vtkm::Id i=0; i < length; ++i)
        {
        const OutputValueType result_v = result.GetPortalConstControl().Get(i);
        const OutputValueType correct_value =
            functor(ValueType(component_value));
        const OutputValueType control_value =
            countingTransformed.GetPortalConstControl().Get(i);
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Transform Counting Handle Failed");
        VTKM_TEST_ASSERT(test_equal(result_v, control_value),
                         "Transform Counting Handle Control Failed");
        component_value = ComponentType(component_value + ComponentType(1));
        }
    }
  };

  struct TestCastAsInput
  {
    template<typename CastToType>
    VTKM_CONT_EXPORT
    void operator()(CastToType vtkmNotUsed(type)) const
    {
      typedef vtkm::cont::ArrayHandleIndex InputArrayType;

      InputArrayType input(ARRAY_SIZE);
      vtkm::cont::ArrayHandleCast<CastToType, InputArrayType> castArray =
          vtkm::cont::make_ArrayHandleCast(input, CastToType());
      vtkm::cont::ArrayHandle<CastToType> result;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(castArray, result);

      // verify results
      vtkm::Id length = ARRAY_SIZE;
      for (vtkm::Id i = 0; i < length; ++i)
      {
        VTKM_TEST_ASSERT(
            result.GetPortalConstControl().Get(i) ==
              static_cast<CastToType>(input.GetPortalConstControl().Get(i)),
            "Casting ArrayHandle Failed");
      }
    }
  };

  template<vtkm::IdComponent NUM_COMPONENTS>
  struct TestGroupVecAsInput
  {
    template<typename ComponentType>
    VTKM_CONT_EXPORT
    void operator()(ComponentType) const
    {
      typedef vtkm::Vec<ComponentType, NUM_COMPONENTS> ValueType;

      ComponentType testValues[ARRAY_SIZE*NUM_COMPONENTS];

      for(vtkm::Id index = 0; index < ARRAY_SIZE*NUM_COMPONENTS; ++index)
        {
        testValues[index] = TestValue(index, ComponentType());
        }
      vtkm::cont::ArrayHandle<ComponentType> baseArray =
          vtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE*NUM_COMPONENTS);

      vtkm::cont::ArrayHandleGroupVec<
          vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS> groupArray(
            baseArray);
      VTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Group array reporting wrong array size.");

      vtkm::cont::ArrayHandle<ValueType> resultArray;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(groupArray, resultArray);

      VTKM_TEST_ASSERT(resultArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Got bad result array size.");

      //verify that the control portal works
      vtkm::Id totalIndex = 0;
      for(vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        const ValueType result = resultArray.GetPortalConstControl().Get(index);
        for (vtkm::IdComponent componentIndex = 0;
             componentIndex < NUM_COMPONENTS;
             componentIndex++)
        {
          const ComponentType expectedValue =
              TestValue(totalIndex, ComponentType());
          VTKM_TEST_ASSERT(result[componentIndex] == expectedValue,
                           "Result array got wrong value.");
          totalIndex++;
        }
      }
    }
  };

  template<vtkm::IdComponent NUM_COMPONENTS>
  struct TestGroupVecAsOutput
  {
    template<typename ComponentType>
    VTKM_CONT_EXPORT
    void operator()(ComponentType) const
    {
      typedef vtkm::Vec<ComponentType, NUM_COMPONENTS> ValueType;

      ValueType testValues[ARRAY_SIZE];

      for(vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
        {
        testValues[index] = TestValue(index, ValueType());
        }
      vtkm::cont::ArrayHandle<ValueType> baseArray =
          vtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE);

      vtkm::cont::ArrayHandle<ComponentType> resultArray;

      vtkm::cont::ArrayHandleGroupVec<
          vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS> groupArray(
            resultArray);

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(baseArray, groupArray);

      VTKM_TEST_ASSERT(groupArray.GetNumberOfValues() == ARRAY_SIZE,
                       "Group array reporting wrong array size.");

      VTKM_TEST_ASSERT(
            resultArray.GetNumberOfValues() == ARRAY_SIZE*NUM_COMPONENTS,
            "Got bad result array size.");

      //verify that the control portal works
      vtkm::Id totalIndex = 0;
      for(vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
      {
        const ValueType expectedValue = TestValue(index, ValueType());
        for (vtkm::IdComponent componentIndex = 0;
             componentIndex < NUM_COMPONENTS;
             componentIndex++)
        {
          const ComponentType result =
              resultArray.GetPortalConstControl().Get(totalIndex);
          VTKM_TEST_ASSERT(result == expectedValue[componentIndex],
                           "Result array got wrong value.");
          totalIndex++;
        }
      }
    }
  };

  struct TestZipAsInput
  {
    template< typename KeyType, typename ValueType >
    VTKM_CONT_EXPORT
    void operator()(vtkm::Pair<KeyType,ValueType> vtkmNotUsed(pair)) const
    {
      typedef vtkm::Pair< KeyType, ValueType > PairType;
      typedef typename vtkm::VecTraits<KeyType>::ComponentType KeyComponentType;
      typedef typename vtkm::VecTraits<ValueType>::ComponentType ValueComponentType;


      KeyType testKeys[ARRAY_SIZE];
      ValueType testValues[ARRAY_SIZE];

      for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
        {
        testKeys[i] = KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i));
        testValues[i] = ValueType(static_cast<ValueComponentType>(i));
        }
      vtkm::cont::ArrayHandle< KeyType > keys =
                          vtkm::cont::make_ArrayHandle(testKeys, ARRAY_SIZE);
      vtkm::cont::ArrayHandle< ValueType > values =
                          vtkm::cont::make_ArrayHandle(testValues, ARRAY_SIZE);

      vtkm::cont::ArrayHandleZip<
          vtkm::cont::ArrayHandle< KeyType >,
          vtkm::cont::ArrayHandle< ValueType > > zip =
                                vtkm::cont::make_ArrayHandleZip(keys, values);

      vtkm::cont::ArrayHandle< PairType > result;

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(zip, result);

      //verify that the control portal works
      for(int i=0; i < ARRAY_SIZE; ++i)
        {
        const PairType result_v = result.GetPortalConstControl().Get(i);
        const PairType correct_value(
              KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
              ValueType(static_cast<ValueComponentType>(i)));
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "ArrayHandleZip Failed as input");
        }
    }
  };

  struct TestPermutationAsOutput
  {
    template< typename ValueType>
    VTKM_CONT_EXPORT void operator()(const ValueType vtkmNotUsed(v)) const
    {
      const vtkm::Id length = ARRAY_SIZE;

      typedef vtkm::cont::ArrayHandleCounting< vtkm::Id > KeyHandleType;
      typedef vtkm::cont::ArrayHandle< ValueType > ValueHandleType;
      typedef vtkm::cont::ArrayHandlePermutation< KeyHandleType,
                                                  ValueHandleType
                                                  > PermutationHandleType;

      typedef typename vtkm::VecTraits<ValueType>::ComponentType ComponentType;
      vtkm::cont::ArrayHandle<ValueType> input;
      typedef typename vtkm::cont::ArrayHandle<ValueType>::PortalControl Portal;
      input.Allocate(length);
      Portal inputPortal = input.GetPortalControl();
      for(vtkm::Id i=0; i < length; ++i)
        {
        inputPortal.Set(i,ValueType(ComponentType(i)));
        }

      ValueHandleType values;
      values.Allocate(length*2);

      KeyHandleType counting =
        vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(length, 1, length);

      PermutationHandleType permutation =
        vtkm::cont::make_ArrayHandlePermutation(counting, values);
      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(input, permutation);

      //verify that the control portal works
      for(vtkm::Id i=0; i <length; ++i)
        {
        const ValueType result_v = permutation.GetPortalConstControl().Get( i );
        const ValueType correct_value = ValueType(ComponentType(i));
        VTKM_TEST_ASSERT(test_equal(result_v, correct_value),
                         "Permutation Handle Failed As Output");
        }
    }
  };


  struct TestZipAsOutput
  {
    template< typename KeyType, typename ValueType >
    VTKM_CONT_EXPORT
    void operator()(vtkm::Pair<KeyType,ValueType> vtkmNotUsed(pair)) const
    {
      typedef vtkm::Pair< KeyType, ValueType > PairType;
      typedef typename vtkm::VecTraits<KeyType>::ComponentType KeyComponentType;
      typedef typename vtkm::VecTraits<ValueType>::ComponentType ValueComponentType;

      PairType testKeysAndValues[ARRAY_SIZE];
      for(vtkm::Id i=0; i < ARRAY_SIZE; ++i)
        {
        testKeysAndValues[i] =
            PairType(KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i)),
                     ValueType(static_cast<ValueComponentType>(i)) );
        }
      vtkm::cont::ArrayHandle< PairType > input =
                    vtkm::cont::make_ArrayHandle(testKeysAndValues, ARRAY_SIZE);

      vtkm::cont::ArrayHandle< KeyType > result_keys;
      vtkm::cont::ArrayHandle< ValueType > result_values;
      vtkm::cont::ArrayHandleZip<
          vtkm::cont::ArrayHandle< KeyType >,
          vtkm::cont::ArrayHandle< ValueType > > result_zip =
                      vtkm::cont::make_ArrayHandleZip(result_keys, result_values);

      vtkm::worklet::DispatcherMapField< PassThrough, DeviceAdapterTag > dispatcher;
      dispatcher.Invoke(input, result_zip);

      //now the two arrays we have zipped should have data inside them
      for(int i=0; i < ARRAY_SIZE; ++i)
        {
        const KeyType result_key = result_keys.GetPortalConstControl().Get(i);
        const ValueType result_value = result_values.GetPortalConstControl().Get(i);

        VTKM_TEST_ASSERT(
              test_equal(result_key, KeyType(static_cast<KeyComponentType>(ARRAY_SIZE - i))),
              "ArrayHandleZip Failed as input for key");
        VTKM_TEST_ASSERT(
              test_equal(result_value, ValueType(static_cast<ValueComponentType>(i))),
              "ArrayHandleZip Failed as input for value");
        }
    }
  };


 struct ZipTypesToTest
    : vtkm::ListTagBase< vtkm::Pair< vtkm::UInt8, vtkm::Id >,
                         vtkm::Pair< vtkm::Int32, vtkm::Vec< vtkm::Float32, 3> >,
                         vtkm::Pair< vtkm::Float64,  vtkm::Vec< vtkm::UInt8, 4> >,
                         vtkm::Pair< vtkm::Vec<vtkm::Float32,3>, vtkm::Vec<vtkm::Int8, 4> >,
                         vtkm::Pair< vtkm::Vec<vtkm::Float64,2>, vtkm::Int32 >
                         >
  {  };

  struct HandleTypesToTest
    : vtkm::ListTagBase< vtkm::UInt8,
                         vtkm::UInt32,
                         vtkm::Int32,
                         vtkm::Int64,
                         vtkm::Vec<vtkm::Int32,2>,
                         vtkm::Vec<vtkm::UInt8,4>,
                         vtkm::Float32,
                         vtkm::Float64,
                         vtkm::Vec<vtkm::Float64,3>,
                         vtkm::Vec<vtkm::Float32,4>
                         >
  {  };

  struct CastTypesToTest
    : vtkm::ListTagBase< vtkm::Int32,
                         vtkm::UInt32
                       >
  { };


  struct TestAll
  {
    VTKM_CONT_EXPORT void operator()() const
    {
      std::cout << "Doing FancyArrayHandle tests" << std::endl;

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCompositeVector as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestCompositeAsInput(),
                              vtkm::TypeListTagScalarAll());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleConstant as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestConstantAsInput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCounting as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestCountingAsInput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleImplicit as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestImplicitAsInput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandlePermutation as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestPermutationAsInput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestTransformAsInput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleTransform with Counting as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestCountingTransformAsInput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleCast as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestCastAsInput(),
                              CastTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<3> as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsInput<3>(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<4> as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsInput<4>(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<2> as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsOutput<2>(),
                              vtkm::TypeListTagScalarAll());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleGroupVec<3> as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestGroupVecAsOutput<3>(),
                              vtkm::TypeListTagScalarAll());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as Input" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsInput(),
                              ZipTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandlePermutation as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestPermutationAsOutput(),
                              HandleTypesToTest());

      std::cout << "-------------------------------------------" << std::endl;
      std::cout << "Testing ArrayHandleZip as Output" << std::endl;
      vtkm::testing::Testing::TryTypes(
                              TestingFancyArrayHandles<DeviceAdapterTag>::TestZipAsOutput(),
                              ZipTypesToTest());
    }
  };
  public:

  /// Run a suite of tests to check to see if a DeviceAdapter properly supports
  /// all the fancy array handles that vtkm supports. Returns an
  /// error code that can be returned from the main function of a test.
  ///
  static VTKM_CONT_EXPORT int Run()
  {
    return vtkm::cont::testing::Testing::Run(TestAll());
  }
};

}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingFancyArrayHandles_h
