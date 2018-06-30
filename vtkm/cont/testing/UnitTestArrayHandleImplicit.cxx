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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

template <typename ValueType>
struct IndexSquared
{
  VTKM_EXEC_CONT
  ValueType operator()(vtkm::Id i) const
  {
    using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;
    return ValueType(static_cast<ComponentType>(i * i));
  }
};

struct ImplicitTests
{
  template <typename ValueType>
  void operator()(const ValueType) const
  {
    using FunctorType = IndexSquared<ValueType>;
    FunctorType functor;

    using ImplicitHandle = vtkm::cont::ArrayHandleImplicit<FunctorType>;

    ImplicitHandle implict = vtkm::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);

    //verify that the control portal works
    for (int i = 0; i < ARRAY_SIZE; ++i)
    {
      const ValueType v = implict.GetPortalConstControl().Get(i);
      const ValueType correct_value = functor(i);
      VTKM_TEST_ASSERT(v == correct_value, "Implicit Handle Failed");
    }

    //verify that the execution portal works
    using Device = vtkm::cont::DeviceAdapterTagSerial;
    using CEPortal = typename ImplicitHandle::template ExecutionTypes<Device>::PortalConst;
    CEPortal execPortal = implict.PrepareForInput(Device());
    for (int i = 0; i < ARRAY_SIZE; ++i)
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

} // anonymous namespace

int UnitTestArrayHandleImplicit(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleImplicit);
}
