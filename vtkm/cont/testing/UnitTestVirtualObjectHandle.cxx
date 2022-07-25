//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/cont/testing/Testing.h>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "This test should be disabled if the VTKm_NO_DEPRECATED_VIRTUAL is true."
#endif //VTKM_NO_DEPRECATED_VIRTUAL

VTKM_DEPRECATED_SUPPRESS_BEGIN

#define ARRAY_LEN 8

namespace
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

using FloatArrayHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
using ArrayTransform = vtkm::cont::ArrayHandleTransform<FloatArrayHandle, TransformerFunctor>;
using TransformerHandle = vtkm::cont::VirtualObjectHandle<Transformer>;

class TestStage1
{
public:
  TestStage1(const FloatArrayHandle& input, TransformerHandle& handle)
    : Input(&input)
    , Handle(&handle)
  {
  }

  template <typename DeviceAdapter>
  bool operator()(DeviceAdapter device) const
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    std::cout << "\tDeviceAdapter: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
              << std::endl;

    for (int n = 0; n < 2; ++n)
    {
      vtkm::cont::Token token;
      TransformerFunctor tfnctr(this->Handle->PrepareForExecution(device, token));
      ArrayTransform transformed(*this->Input, tfnctr);

      FloatArrayHandle output;
      Algorithm::Copy(transformed, output);
      auto portal = output.ReadPortal();
      for (vtkm::Id i = 0; i < ARRAY_LEN; ++i)
      {
        vtkm::FloatDefault expected = TestValue(i, vtkm::FloatDefault{});
        expected = expected * expected;
        VTKM_TEST_ASSERT(
          test_equal(portal.Get(i), expected), "Expected ", expected, " but got ", portal.Get(i));
      }
      std::cout << "\tSuccess." << std::endl;

      if (n == 0)
      {
        std::cout << "\tReleaseResources and test again..." << std::endl;
        this->Handle->ReleaseExecutionResources();
      }
    }
    return true;
  }

private:
  const FloatArrayHandle* Input;
  TransformerHandle* Handle;
};

class TestStage2
{
public:
  TestStage2(const FloatArrayHandle& input, Multiply& mul, TransformerHandle& handle)
    : Input(&input)
    , Mul(&mul)
    , Handle(&handle)
  {
  }

  template <typename DeviceAdapter>
  bool operator()(DeviceAdapter device) const
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    std::cout << "\tDeviceAdapter: " << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
              << std::endl;

    this->Mul->SetMultiplicand(2);
    for (int n = 0; n < 2; ++n)
    {
      vtkm::cont::Token token;
      TransformerFunctor tfnctr(this->Handle->PrepareForExecution(device, token));
      ArrayTransform transformed(*this->Input, tfnctr);

      FloatArrayHandle output;
      Algorithm::Copy(transformed, output);
      auto portal = output.ReadPortal();
      for (vtkm::Id i = 0; i < ARRAY_LEN; ++i)
      {
        vtkm::FloatDefault expected =
          TestValue(i, vtkm::FloatDefault{}) * this->Mul->GetMultiplicand();
        VTKM_TEST_ASSERT(
          test_equal(portal.Get(i), expected), "Expected ", expected, " but got ", portal.Get(i));
      }
      std::cout << "\tSuccess." << std::endl;

      if (n == 0)
      {
        std::cout << "\tUpdate and test again..." << std::endl;
        this->Mul->SetMultiplicand(3);
      }
    }
    return true;
  }

private:
  const FloatArrayHandle* Input;
  Multiply* Mul;
  TransformerHandle* Handle;
};

void Run()
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> input;
  input.Allocate(ARRAY_LEN);
  SetPortal(input.WritePortal());

  TransformerHandle handle;

  std::cout << "Testing with concrete type 1 (Square)..." << std::endl;
  Square sqr;
  handle.Reset(&sqr, false);
  vtkm::cont::TryExecute(TestStage1(input, handle));

  std::cout << "ReleaseResources..." << std::endl;
  handle.ReleaseResources();

  std::cout << "Testing with concrete type 2 (Multiply)..." << std::endl;
  Multiply mul;
  handle.Reset(&mul, false);
  vtkm::cont::TryExecute(TestStage2(input, mul, handle));
}

} // anonymous namespace

VTKM_DEPRECATED_SUPPRESS_END

int UnitTestVirtualObjectHandle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
