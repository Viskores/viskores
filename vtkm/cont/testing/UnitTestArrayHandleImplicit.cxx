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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

template<typename ValueType>
struct IndexSquared
{
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(vtkm::Id i) const
  {
    typedef typename vtkm::VecTraits<ValueType>::ComponentType ComponentType;
    return ValueType(static_cast<ComponentType>(i*i));
  }
};


struct ImplicitTests
{
  template<typename ValueType>
  void operator()(const ValueType) const
  {
    typedef IndexSquared<ValueType> FunctorType;
    FunctorType functor;

    typedef vtkm::cont::ArrayHandleImplicit<ValueType,FunctorType>
        ImplicitHandle;

    ImplicitHandle implict =
            vtkm::cont::make_ArrayHandleImplicit<ValueType>(functor,ARRAY_SIZE);

    //verify that the control portal works
    for(int i=0; i < ARRAY_SIZE; ++i)
      {
      const ValueType v = implict.GetPortalConstControl().Get(i);
      const ValueType correct_value = functor(i);
        VTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
      }

    //verify that the execution portal works
    typedef vtkm::cont::DeviceAdapterTagSerial Device;
    typedef typename ImplicitHandle::template ExecutionTypes<Device>
        ::PortalConst CEPortal;
    CEPortal execPortal = implict.PrepareForInput(Device());
    for(int i=0; i < ARRAY_SIZE; ++i)
      {
      const ValueType v = execPortal.Get(i);
      const ValueType correct_value = functor(i);
      VTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
      }
  }

};

void TestArrayHandleImplicit()
{
  vtkm::testing::Testing::TryTypes(ImplicitTests(), vtkm::TypeListTagCommon());
}



} // annonymous namespace

int UnitTestArrayHandleImplicit(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleImplicit);
}
