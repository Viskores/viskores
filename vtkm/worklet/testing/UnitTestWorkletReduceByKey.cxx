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
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/worklet/Keys.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

static const vtkm::Id ARRAY_SIZE = 1033;
static const vtkm::Id NUM_UNIQUE = ARRAY_SIZE/10;

struct CheckReduceByKeyWorklet : vtkm::worklet::WorkletReduceByKey
{
  typedef void ControlSignature(KeysIn);
  typedef void ExecutionSignature(_1, WorkIndex);
  typedef _1 InputDomain;

  template<typename T>
  VTKM_EXEC
  void operator()(const T &key, vtkm::Id workIndex) const
  {
    // These tests only work if keys are in sorted order, which is how we group
    // them.

    if (key != TestValue(workIndex, T()))
    {
      this->RaiseError("Unexpected key");
    }
  }
};

template<typename KeyType>
void TryKeyType(KeyType)
{
  KeyType keyBuffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    keyBuffer[index] = TestValue(index%NUM_UNIQUE, KeyType());
  }

  vtkm::cont::ArrayHandle<KeyType> keyArray =
      vtkm::cont::make_ArrayHandle(keyBuffer, ARRAY_SIZE);

  vtkm::worklet::Keys<KeyType> keys(keyArray,
                                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

  vtkm::worklet::DispatcherReduceByKey<CheckReduceByKeyWorklet> dispatcher;
  dispatcher.Invoke(keys);
}

void TestReduceByKey()
{
  typedef vtkm::cont::DeviceAdapterTraits<
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DeviceAdapterTraits;
  std::cout << "Testing Map Field on device adapter: "
            << DeviceAdapterTraits::GetName() << std::endl;

  std::cout << "Testing vtkm::Id keys." << std::endl;
  TryKeyType(vtkm::Id());

  std::cout << "Testing vtkm::IdComponent keys." << std::endl;
  TryKeyType(vtkm::IdComponent());

  std::cout << "Testing vtkm::UInt8 keys." << std::endl;
  TryKeyType(vtkm::UInt8());

  std::cout << "Testing vtkm::Id3 keys." << std::endl;
  TryKeyType(vtkm::Id3());
}

} // anonymous namespace

int UnitTestWorkletReduceByKey(int, char*[])
{
  return vtkm::cont::testing::Testing::Run(TestReduceByKey);
}
