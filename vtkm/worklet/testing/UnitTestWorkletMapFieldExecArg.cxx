//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/UncertainArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/exec/Variant.h>

#include <vtkm/cont/testing/Testing.h>

namespace map_exec_field
{

struct SimpleExecObject : vtkm::cont::ExecutionObjectBase
{
  template <typename Device>
  Device PrepareForExecution(Device, vtkm::cont::Token&) const
  {
    return Device();
  }
};

struct TestExecObjectWorklet
{
  template <typename T>
  class Worklet : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut, FieldOut, ExecObject);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, Device);

    template <typename InPortalType, typename OutPortalType, typename DeviceTag>
    VTKM_EXEC void operator()(const vtkm::Id& index,
                              const InPortalType& execIn,
                              OutPortalType& execOut,
                              T& out,
                              DeviceTag,
                              DeviceTag) const
    {
      VTKM_IS_DEVICE_ADAPTER_TAG(DeviceTag);

      if (!test_equal(execIn.Get(index), TestValue(index, T()) + T(100)))
      {
        this->RaiseError("Got wrong input value.");
      }
      out = static_cast<T>(execIn.Get(index) - T(100));
      execOut.Set(index, out);
    }
  };
};

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = static_cast<T>(TestValue(index, T()) + T(100));
    }

    vtkm::cont::ArrayHandleIndex counting(ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> inputHandle =
      vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE, vtkm::CopyFlag::Off);
    vtkm::cont::ArrayHandle<T> outputHandle;
    vtkm::cont::ArrayHandle<T> outputFieldArray;
    outputHandle.Allocate(ARRAY_SIZE);

    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<typename WorkletType::template Worklet<T>> dispatcher;
    dispatcher.Invoke(counting, inputHandle, outputHandle, outputFieldArray, SimpleExecObject());

    std::cout << "Check result." << std::endl;
    CheckPortal(outputHandle.ReadPortal());
    CheckPortal(outputFieldArray.ReadPortal());

    std::cout << "Repeat with dynamic arrays." << std::endl;
    // Clear out output arrays.
    outputFieldArray = vtkm::cont::ArrayHandle<T>();
    outputHandle = vtkm::cont::ArrayHandle<T>();
    outputHandle.Allocate(ARRAY_SIZE);

    vtkm::cont::UncertainArrayHandle<vtkm::List<T>, vtkm::List<vtkm::cont::StorageTagBasic>>
      outputFieldDynamic(outputFieldArray);
    dispatcher.Invoke(counting, inputHandle, outputHandle, outputFieldDynamic, SimpleExecObject());

    std::cout << "Check dynamic array result." << std::endl;
    CheckPortal(outputHandle.ReadPortal());
    CheckPortal(outputFieldArray.ReadPortal());
  }
};

struct StructWithPadding
{
  vtkm::Int32 A;
  // Padding here
  vtkm::Int64 C;
};

struct StructWithoutPadding
{
  vtkm::Int32 A;
  vtkm::Int32 B;
  vtkm::Int64 C;
};

struct LargerStruct
{
  vtkm::Int64 C;
  vtkm::Int64 D;
  vtkm::Int64 E;
};

using VariantTypePadding = vtkm::exec::Variant<StructWithPadding, StructWithoutPadding>;
using VariantTypeSizes = vtkm::exec::Variant<StructWithPadding, StructWithoutPadding, LargerStruct>;

struct VarientPaddingExecObj : vtkm::cont::ExecutionObjectBase
{
  VariantTypePadding Variant;
  VTKM_CONT VariantTypePadding PrepareForExecution(const vtkm::cont::DeviceAdapterId&,
                                                   vtkm::cont::Token&) const
  {
    return this->Variant;
  }
};
struct VarientSizesExecObj : vtkm::cont::ExecutionObjectBase
{
  VariantTypeSizes Variant;
  VTKM_CONT VariantTypeSizes PrepareForExecution(const vtkm::cont::DeviceAdapterId&,
                                                 vtkm::cont::Token&) const
  {
    return this->Variant;
  }
};

struct TestVariantExecObjectPadding : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut a, FieldOut c, ExecObject varient);
  // Using an output field as the domain is weird, but it works.
  using InputDomain = _1;

  VTKM_EXEC void operator()(vtkm::Int32& a, vtkm::Int64& c, const VariantTypePadding& variant) const
  {
    a = variant.Get<StructWithPadding>().A;
    c = variant.Get<StructWithPadding>().C;
  }
};

struct TestVariantExecObjectNoPadding : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut a, FieldOut b, FieldOut c, ExecObject varient);
  // Using an output field as the domain is weird, but it works.
  using InputDomain = _1;

  VTKM_EXEC void operator()(vtkm::Int32& a,
                            vtkm::Int32& b,
                            vtkm::Int64& c,
                            const VariantTypePadding& variant) const
  {
    a = variant.Get<StructWithoutPadding>().A;
    b = variant.Get<StructWithoutPadding>().B;
    c = variant.Get<StructWithoutPadding>().C;
  }
};

struct TestVariantExecObjectLarger : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut c, FieldOut d, FieldOut e, ExecObject varient);
  // Using an output field as the domain is weird, but it works.
  using InputDomain = _1;

  VTKM_EXEC void operator()(vtkm::Int64& c,
                            vtkm::Int64& d,
                            vtkm::Int64& e,
                            const VariantTypeSizes& variant) const
  {
    c = variant.Get<LargerStruct>().C;
    d = variant.Get<LargerStruct>().D;
    e = variant.Get<LargerStruct>().E;
  }
};

void DoTestVariant()
{
  vtkm::cont::ArrayHandle<vtkm::Int32> a;
  vtkm::cont::ArrayHandle<vtkm::Int32> b;
  vtkm::cont::ArrayHandle<vtkm::Int64> c;
  vtkm::cont::ArrayHandle<vtkm::Int64> d;
  vtkm::cont::ArrayHandle<vtkm::Int64> e;

  // Usually you don't need to allocate output arrays, but these worklets do a
  // weird thing of using an output array as the input domain (because the
  // generative worklets have no input). It's weird to use an output field as
  // the input domain, but it works as long as you preallocate the data.
  a.Allocate(ARRAY_SIZE);
  b.Allocate(ARRAY_SIZE);
  c.Allocate(ARRAY_SIZE);
  d.Allocate(ARRAY_SIZE);
  e.Allocate(ARRAY_SIZE);

  vtkm::cont::Invoker invoke;

  std::cout << "Struct with Padding" << std::endl;
  {
    VarientPaddingExecObj execObject;
    execObject.Variant =
      StructWithPadding{ TestValue(0, vtkm::Int32{}), TestValue(1, vtkm::Int64{}) };
    invoke(TestVariantExecObjectPadding{}, a, c, execObject);
    auto aPortal = a.ReadPortal();
    auto cPortal = c.ReadPortal();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(aPortal.Get(index) == TestValue(0, vtkm::Int32{}));
      VTKM_TEST_ASSERT(cPortal.Get(index) == TestValue(1, vtkm::Int64{}));
    }
  }

  std::cout << "Struct without Padding" << std::endl;
  {
    VarientPaddingExecObj execObject;
    execObject.Variant = StructWithoutPadding{ TestValue(2, vtkm::Int32{}),
                                               TestValue(3, vtkm::Int32{}),
                                               TestValue(4, vtkm::Int64{}) };
    invoke(TestVariantExecObjectNoPadding{}, a, b, c, execObject);
    auto aPortal = a.ReadPortal();
    auto bPortal = b.ReadPortal();
    auto cPortal = c.ReadPortal();
    // An odd bug was observed with some specific compilers. (Specifically, this was
    // last observed with GCC5 used with nvcc compiling CUDA code for the Pascal
    // architecture.) It concerned a Variant that contained 2 or more objects of the
    // same `sizeof` and the first one listed had some padding (to satisfy alignment)
    // and the second one did not. Internally, the `Variant` object constructs a
    // `union` of types in the order listed. The compiler seemed to recognize that the
    // first entry in the union was the "largest" and used that for trivial copies.
    // However, it also recognized the padding in that first object and skipped
    // copying that value even if the union was set to the second object. If that
    // condition is happening, you will probably see a failure when testing the
    // bPortal below.
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(aPortal.Get(index) == TestValue(2, vtkm::Int32{}));
      VTKM_TEST_ASSERT(bPortal.Get(index) == TestValue(3, vtkm::Int32{}));
      VTKM_TEST_ASSERT(cPortal.Get(index) == TestValue(4, vtkm::Int64{}));
    }
  }

  std::cout << "LargerStruct" << std::endl;
  {
    VarientSizesExecObj execObject;
    execObject.Variant = LargerStruct{ TestValue(5, vtkm::Int64{}),
                                       TestValue(6, vtkm::Int64{}),
                                       TestValue(7, vtkm::Int64{}) };
    invoke(TestVariantExecObjectLarger{}, c, d, e, execObject);
    auto cPortal = c.ReadPortal();
    auto dPortal = d.ReadPortal();
    auto ePortal = e.ReadPortal();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(cPortal.Get(index) == TestValue(5, vtkm::Int64{}));
      VTKM_TEST_ASSERT(dPortal.Get(index) == TestValue(6, vtkm::Int64{}));
      VTKM_TEST_ASSERT(ePortal.Get(index) == TestValue(7, vtkm::Int64{}));
    }
  }
}

void TestWorkletMapFieldExecArg(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Worklet with WholeArray on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(map_exec_field::DoTestWorklet<TestExecObjectWorklet>(),
                                   vtkm::TypeListCommon());

  std::cout << "--- Worklet passing variant." << std::endl;
  DoTestVariant();
}

} // anonymous-ish namespace

int UnitTestWorkletMapFieldExecArg(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(
    map_exec_field::TestWorkletMapFieldExecArg, argc, argv);
}
