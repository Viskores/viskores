//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
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
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/arg/TransportTagAtomicArray.h>
#include <vtkm/cont/arg/TransportTagWholeArrayIn.h>
#include <vtkm/cont/arg/TransportTagWholeArrayInOut.h>
#include <vtkm/cont/arg/TransportTagWholeArrayOut.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

static const vtkm::Id ARRAY_SIZE = 10;

#define OFFSET 10

template<typename PortalType>
struct TestOutKernel : public vtkm::exec::FunctorBase
{
  PortalType Portal;

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("Out whole array has wrong size.");
    }
    typedef typename PortalType::ValueType ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template<typename PortalType>
struct TestInKernel : public vtkm::exec::FunctorBase
{
  PortalType Portal;

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("In whole array has wrong size.");
    }
    typedef typename PortalType::ValueType ValueType;
    if (!test_equal(this->Portal.Get(index), TestValue(index, ValueType())))
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template<typename PortalType>
struct TestInOutKernel : public vtkm::exec::FunctorBase
{
  PortalType Portal;

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id index) const
  {
    if (this->Portal.GetNumberOfValues() != ARRAY_SIZE)
    {
      this->RaiseError("In/Out whole array has wrong size.");
    }
    typedef typename PortalType::ValueType ValueType;
    this->Portal.Set(index, this->Portal.Get(index) + ValueType(OFFSET));
  }
};

template<typename AtomicType>
struct TestAtomicKernel : public vtkm::exec::FunctorBase
{
  VTKM_CONT_EXPORT
  TestAtomicKernel(const AtomicType &atomicArray)
    : AtomicArray(atomicArray) {  }

  AtomicType AtomicArray;

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id index) const
  {
    typedef typename AtomicType::ValueType ValueType;
    this->AtomicArray.Add(0, static_cast<ValueType>(index));
  }
};

template<typename Device>
struct TryWholeArrayType
{
  template<typename T>
  void operator()(T) const
  {
    typedef vtkm::cont::ArrayHandle<T> ArrayHandleType;

    typedef vtkm::cont::arg::Transport<
        vtkm::cont::arg::TransportTagWholeArrayIn, ArrayHandleType, Device>
        InTransportType;
    typedef vtkm::cont::arg::Transport<
        vtkm::cont::arg::TransportTagWholeArrayInOut, ArrayHandleType, Device>
        InOutTransportType;
    typedef vtkm::cont::arg::Transport<
        vtkm::cont::arg::TransportTagWholeArrayOut, ArrayHandleType, Device>
        OutTransportType;

    ArrayHandleType array;
    array.Allocate(ARRAY_SIZE);

    std::cout << "Check Transport WholeArrayOut" << std::endl;
    TestOutKernel<typename OutTransportType::ExecObjectType> outKernel;
    outKernel.Portal = OutTransportType()(array, -1);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(outKernel, ARRAY_SIZE);

    CheckPortal(array.GetPortalConstControl());

    std::cout << "Check Transport WholeArrayIn" << std::endl;
    TestInKernel<typename InTransportType::ExecObjectType> inKernel;
    inKernel.Portal = InTransportType()(array, -1);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(inKernel, ARRAY_SIZE);

    std::cout << "Check Transport WholeArrayInOut" << std::endl;
    TestInOutKernel<typename InOutTransportType::ExecObjectType> inOutKernel;
    inOutKernel.Portal = InOutTransportType()(array, -1);

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(inOutKernel, ARRAY_SIZE);

    VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE,
                     "Array size wrong?");
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      T expectedValue = TestValue(index, T()) + T(OFFSET);
      T retrievedValue = array.GetPortalConstControl().Get(index);
      VTKM_TEST_ASSERT(test_equal(expectedValue, retrievedValue),
                       "In/Out array not set correctly.");
    }
  }
};

template<typename Device>
struct TryAtomicArrayType
{
  template<typename T>
  void operator()(T) const
  {
    typedef vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>
        ArrayHandleType;

    typedef vtkm::cont::arg::Transport<
        vtkm::cont::arg::TransportTagAtomicArray, ArrayHandleType, Device>
        TransportType;

    ArrayHandleType array;
    array.Allocate(1);
    array.GetPortalControl().Set(0, 0);

    std::cout << "Check Transport AtomicArray" << std::endl;
    TestAtomicKernel<typename TransportType::ExecObjectType>
        kernel(TransportType()(array, -1));

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);

    T result = array.GetPortalConstControl().Get(0);
    VTKM_TEST_ASSERT(result == ((ARRAY_SIZE-1)*ARRAY_SIZE)/2,
                     "Got wrong summation in atomic array.");
  }
};

template<typename Device>
void TryArrayOutTransport(Device)
{
  vtkm::testing::Testing::TryTypes(TryWholeArrayType<Device>(),
                                   vtkm::TypeListTagCommon());
  vtkm::testing::Testing::TryTypes(TryAtomicArrayType<Device>(),
                                   vtkm::exec::AtomicArrayTypeListTag());
}

void TestWholeArrayTransport()
{
  std::cout << "Trying WholeArray transport." << std::endl;
  TryArrayOutTransport(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
}

} // Anonymous namespace

int UnitTestTransportWholeArray(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWholeArrayTransport);
}
