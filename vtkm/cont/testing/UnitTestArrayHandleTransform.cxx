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
//  Copyright 2015. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <vtkm/cont/ArrayHandleTransform.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

template<typename ValueType>
struct MySquare
{
  template<typename U>
  VTKM_EXEC_EXPORT
  ValueType operator()(U u) const
    { return vtkm::dot(u, u); }
};

template<typename OriginalPortalType, typename TransformedPortalType>
struct CheckTransformFunctor : vtkm::exec::FunctorBase
{
  OriginalPortalType OriginalPortal;
  TransformedPortalType TransformedPortal;

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id index) const {
    typedef typename TransformedPortalType::ValueType T;
    typename OriginalPortalType::ValueType original =
        this->OriginalPortal.Get(index);
    T transformed = this->TransformedPortal.Get(index);
    if (!test_equal(transformed, MySquare<T>()(original))) {
      this->RaiseError("Encountered bad transformed value.");
    }
  }
};

template<typename OriginalArrayHandleType,
         typename TransformedArrayHandleType,
         typename Device>
VTKM_CONT_EXPORT
CheckTransformFunctor<
    typename OriginalArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    typename TransformedArrayHandleType::template ExecutionTypes<Device>::PortalConst>
make_CheckTransformFunctor(const OriginalArrayHandleType &originalArray,
                           const TransformedArrayHandleType &transformedArray,
                           Device)
{
  typedef typename OriginalArrayHandleType::template ExecutionTypes<Device>::PortalConst OriginalPortalType;
  typedef typename TransformedArrayHandleType::template ExecutionTypes<Device>::PortalConst TransformedPortalType;
  CheckTransformFunctor<OriginalPortalType, TransformedPortalType> functor;
  functor.OriginalPortal = originalArray.PrepareForInput(Device());
  functor.TransformedPortal = transformedArray.PrepareForInput(Device());
  return functor;
}

template<typename InputValueType>
struct TransformTests
{
  typedef typename vtkm::VecTraits<InputValueType>::ComponentType
      OutputValueType;
  typedef MySquare<OutputValueType> FunctorType;

  typedef vtkm::cont::ArrayHandleTransform<OutputValueType,
                                           vtkm::cont::ArrayHandle<InputValueType>,
                                           FunctorType> TransformHandle;

  typedef  vtkm::cont::ArrayHandleTransform<OutputValueType,
                                    vtkm::cont::ArrayHandleCounting<InputValueType>,
                                    FunctorType> CountingTransformHandle;

  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG Device;
  typedef vtkm::cont::DeviceAdapterAlgorithm<Device> Algorithm;

  void operator()() const
  {
    FunctorType functor;

    //test the make_ArrayHandleTransform method
    //test a transform handle with a counting handle as the values
    vtkm::cont::ArrayHandleCounting<InputValueType> counting =
        vtkm::cont::make_ArrayHandleCounting(InputValueType(OutputValueType(0)),
                                             ARRAY_SIZE);
    CountingTransformHandle countingTransformed =
      vtkm::cont::make_ArrayHandleTransform<OutputValueType>(counting, functor);

    // Verify that the execution portal works.
    Algorithm::Schedule(
          make_CheckTransformFunctor(counting, countingTransformed, Device()),
          ARRAY_SIZE);

    //test a transform handle with a normal handle as the values
    //we are going to connect the two handles up, and than fill
    //the values and make the transform sees the new values in the handle
    vtkm::cont::ArrayHandle<InputValueType> input;
    TransformHandle thandle(input,functor);

    typedef typename vtkm::cont::ArrayHandle<InputValueType>::PortalControl
        Portal;
    input.Allocate(ARRAY_SIZE);
    Portal portal = input.GetPortalControl();
    for(vtkm::Id index=0; index < ARRAY_SIZE; ++index)
    {
      portal.Set(index, InputValueType(index+2) );
    }

    // Verify that the execution portal works.
    Algorithm::Schedule(
          make_CheckTransformFunctor(input, thandle, Device()),
          ARRAY_SIZE);

    //now update the array handle values again, so that
    //we can verify the transform handle gets these updated values
    for(vtkm::Id index=0; index < ARRAY_SIZE; ++index)
    {
      portal.Set(index, InputValueType(index*index));
    }

    // Verify that the execution portal works.
    Algorithm::Schedule(
          make_CheckTransformFunctor(input, thandle, Device()),
          ARRAY_SIZE);

  }

};


struct TryInputType
{
  template<typename InputType>
  void operator()(InputType) const {
    TransformTests<InputType>()();
  }
};

void TestArrayHandleTransform()
{
//  vtkm::testing::Testing::TryAllTypes(TryInputType());
  vtkm::testing::Testing::TryTypes(TryInputType(), vtkm::TypeListTagCommon());
}



} // annonymous namespace

int UnitTestArrayHandleTransform(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleTransform);
}
