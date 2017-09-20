//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_testing_TestingVirtualObjectCache_h
#define vtk_m_cont_testing_TestingVirtualObjectCache_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/VirtualObjectCache.h>
#include <vtkm/cont/testing/Testing.h>

#define ARRAY_LEN 8

namespace vtkm
{
namespace cont
{
namespace testing
{

namespace virtual_object_detail
{

class Transformer
{
public:
  template <typename T>
  VTKM_EXEC void Bind(const T* target)
  {
    this->Concrete = target;
    this->Caller = [](const void* concrete, vtkm::FloatDefault val) {
      return static_cast<const T*>(concrete)->operator()(val);
    };
  }

  VTKM_EXEC
  vtkm::FloatDefault operator()(vtkm::FloatDefault val) const
  {
    return this->Caller(this->Concrete, val);
  }

private:
  using Signature = vtkm::FloatDefault(const void*, vtkm::FloatDefault);

  const void* Concrete;
  Signature* Caller;
};

struct Square
{
  VTKM_EXEC
  vtkm::FloatDefault operator()(vtkm::FloatDefault val) const { return val * val; }
};

struct Multiply
{
  VTKM_EXEC
  vtkm::FloatDefault operator()(vtkm::FloatDefault val) const { return val * this->Multiplicand; }

  vtkm::FloatDefault Multiplicand;
};

} // virtual_object_detail

template <typename DeviceAdapterList>
class TestingVirtualObjectCache
{
private:
  using FloatArrayHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using ArrayTransform =
    vtkm::cont::ArrayHandleTransform<FloatArrayHandle, virtual_object_detail::Transformer>;
  using TransformerCache = vtkm::cont::VirtualObjectCache<virtual_object_detail::Transformer>;

  class TestStage1
  {
  public:
    TestStage1(const FloatArrayHandle& input, TransformerCache& manager)
      : Input(&input)
      , Manager(&manager)
    {
    }

    template <typename DeviceAdapter>
    void operator()(DeviceAdapter device) const
    {
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
      std::cout << "\tDeviceAdapter: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
                << std::endl;

      for (int n = 0; n < 2; ++n)
      {
        ArrayTransform transformed(*this->Input, this->Manager->GetVirtualObject(device));
        FloatArrayHandle output;
        Algorithm::Copy(transformed, output);
        auto portal = output.GetPortalConstControl();
        for (vtkm::Id i = 0; i < ARRAY_LEN; ++i)
        {
          VTKM_TEST_ASSERT(portal.Get(i) == FloatDefault(i * i), "\tIncorrect result");
        }
        std::cout << "\tSuccess." << std::endl;

        if (n == 0)
        {
          std::cout << "\tReleaseResources and test again..." << std::endl;
          this->Manager->ReleaseResources();
        }
      }
    }

  private:
    const FloatArrayHandle* Input;
    TransformerCache* Manager;
  };

  class TestStage2
  {
  public:
    TestStage2(const FloatArrayHandle& input,
               virtual_object_detail::Multiply& mul,
               TransformerCache& manager)
      : Input(&input)
      , Mul(&mul)
      , Manager(&manager)
    {
    }

    template <typename DeviceAdapter>
    void operator()(DeviceAdapter device) const
    {
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
      std::cout << "\tDeviceAdapter: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
                << std::endl;

      this->Mul->Multiplicand = 2;
      this->Manager->SetRefreshFlag(true);
      for (int n = 0; n < 2; ++n)
      {
        ArrayTransform transformed(*this->Input, this->Manager->GetVirtualObject(device));
        FloatArrayHandle output;
        Algorithm::Copy(transformed, output);
        auto portal = output.GetPortalConstControl();
        for (vtkm::Id i = 0; i < ARRAY_LEN; ++i)
        {
          VTKM_TEST_ASSERT(portal.Get(i) == FloatDefault(i) * this->Mul->Multiplicand,
                           "\tIncorrect result");
        }
        std::cout << "\tSuccess." << std::endl;

        if (n == 0)
        {
          std::cout << "\tUpdate and test again..." << std::endl;
          this->Mul->Multiplicand = 3;
          this->Manager->SetRefreshFlag(true);
        }
      }
    }

  private:
    const FloatArrayHandle* Input;
    virtual_object_detail::Multiply* Mul;
    TransformerCache* Manager;
  };

public:
  static void Run()
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> input;
    input.Allocate(ARRAY_LEN);
    auto portal = input.GetPortalControl();
    for (vtkm::Id i = 0; i < ARRAY_LEN; ++i)
    {
      portal.Set(i, vtkm::FloatDefault(i));
    }

    TransformerCache manager;

    std::cout << "Testing with concrete type 1 (Square)..." << std::endl;
    virtual_object_detail::Square sqr;
    manager.Bind(&sqr, DeviceAdapterList());
    vtkm::ListForEach(TestStage1(input, manager), DeviceAdapterList());

    std::cout << "Reset..." << std::endl;
    manager.Reset();

    std::cout << "Testing with concrete type 2 (Multiply)..." << std::endl;
    virtual_object_detail::Multiply mul;
    manager.Bind(&mul, DeviceAdapterList());
    vtkm::ListForEach(TestStage2(input, mul, manager), DeviceAdapterList());
  }
};
}
}
} // vtkm::cont::testing

#endif
