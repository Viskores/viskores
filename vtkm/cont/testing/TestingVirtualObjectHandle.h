//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingVirtualObjectHandle_h
#define vtk_m_cont_testing_TestingVirtualObjectHandle_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/VirtualObjectHandle.h>
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

class Transformer : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC
  virtual vtkm::FloatDefault Eval(vtkm::FloatDefault val) const = 0;
};

class Square : public Transformer
{
public:
  VTKM_EXEC
  vtkm::FloatDefault Eval(vtkm::FloatDefault val) const override { return val * val; }
};

class Multiply : public Transformer
{
public:
  VTKM_CONT
  void SetMultiplicand(vtkm::FloatDefault val)
  {
    this->Multiplicand = val;
    this->Modified();
  }

  VTKM_CONT
  vtkm::FloatDefault GetMultiplicand() const { return this->Multiplicand; }

  VTKM_EXEC
  vtkm::FloatDefault Eval(vtkm::FloatDefault val) const override
  {
    return val * this->Multiplicand;
  }

private:
  vtkm::FloatDefault Multiplicand = 0.0f;
};

class TransformerFunctor
{
public:
  TransformerFunctor() = default;
  explicit TransformerFunctor(const Transformer* impl)
    : Impl(impl)
  {
  }

  VTKM_EXEC
  vtkm::FloatDefault operator()(vtkm::FloatDefault val) const { return this->Impl->Eval(val); }

private:
  const Transformer* Impl;
};

} // virtual_object_detail

template <typename DeviceAdapterList>
class TestingVirtualObjectHandle
{
private:
  using FloatArrayHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using ArrayTransform =
    vtkm::cont::ArrayHandleTransform<FloatArrayHandle, virtual_object_detail::TransformerFunctor>;
  using TransformerHandle = vtkm::cont::VirtualObjectHandle<virtual_object_detail::Transformer>;

  class TestStage1
  {
  public:
    TestStage1(const FloatArrayHandle& input, TransformerHandle& handle)
      : Input(&input)
      , Handle(&handle)
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
        virtual_object_detail::TransformerFunctor tfnctr(this->Handle->PrepareForExecution(device));
        ArrayTransform transformed(*this->Input, tfnctr);

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
          this->Handle->ReleaseExecutionResources();
        }
      }
    }

  private:
    const FloatArrayHandle* Input;
    TransformerHandle* Handle;
  };

  class TestStage2
  {
  public:
    TestStage2(const FloatArrayHandle& input,
               virtual_object_detail::Multiply& mul,
               TransformerHandle& handle)
      : Input(&input)
      , Mul(&mul)
      , Handle(&handle)
    {
    }

    template <typename DeviceAdapter>
    void operator()(DeviceAdapter device) const
    {
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
      std::cout << "\tDeviceAdapter: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
                << std::endl;

      this->Mul->SetMultiplicand(2);
      for (int n = 0; n < 2; ++n)
      {
        virtual_object_detail::TransformerFunctor tfnctr(this->Handle->PrepareForExecution(device));
        ArrayTransform transformed(*this->Input, tfnctr);

        FloatArrayHandle output;
        Algorithm::Copy(transformed, output);
        auto portal = output.GetPortalConstControl();
        for (vtkm::Id i = 0; i < ARRAY_LEN; ++i)
        {
          VTKM_TEST_ASSERT(portal.Get(i) == FloatDefault(i) * this->Mul->GetMultiplicand(),
                           "\tIncorrect result");
        }
        std::cout << "\tSuccess." << std::endl;

        if (n == 0)
        {
          std::cout << "\tUpdate and test again..." << std::endl;
          this->Mul->SetMultiplicand(3);
        }
      }
    }

  private:
    const FloatArrayHandle* Input;
    virtual_object_detail::Multiply* Mul;
    TransformerHandle* Handle;
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

    TransformerHandle handle;

    std::cout << "Testing with concrete type 1 (Square)..." << std::endl;
    virtual_object_detail::Square sqr;
    handle.Reset(&sqr, false, DeviceAdapterList());
    vtkm::ListForEach(TestStage1(input, handle), DeviceAdapterList());

    std::cout << "ReleaseResources..." << std::endl;
    handle.ReleaseResources();

    std::cout << "Testing with concrete type 2 (Multiply)..." << std::endl;
    virtual_object_detail::Multiply mul;
    handle.Reset(&mul, false, DeviceAdapterList());
    vtkm::ListForEach(TestStage2(input, mul, handle), DeviceAdapterList());
  }
};
}
}
} // vtkm::cont::testing

#endif
