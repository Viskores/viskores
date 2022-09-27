//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename ValueType>
struct ExtractComponentTests
{
  using InputArray = vtkm::cont::ArrayHandle<vtkm::Vec<ValueType, 4>>;
  using ExtractArray = vtkm::cont::ArrayHandleExtractComponent<InputArray>;
  using ReferenceComponentArray = vtkm::cont::ArrayHandleCounting<ValueType>;
  using ReferenceCompositeArray =
    typename vtkm::cont::ArrayHandleCompositeVector<ReferenceComponentArray,
                                                    ReferenceComponentArray,
                                                    ReferenceComponentArray,
                                                    ReferenceComponentArray>;

  // This is used to build a ArrayHandleExtractComponent's internal array.
  ReferenceCompositeArray RefComposite;

  void ConstructReferenceArray()
  {
    // Build the Ref array
    const vtkm::Id numValues = 32;
    ReferenceComponentArray c1 = vtkm::cont::make_ArrayHandleCounting<ValueType>(3, 2, numValues);
    ReferenceComponentArray c2 = vtkm::cont::make_ArrayHandleCounting<ValueType>(2, 3, numValues);
    ReferenceComponentArray c3 = vtkm::cont::make_ArrayHandleCounting<ValueType>(4, 4, numValues);
    ReferenceComponentArray c4 = vtkm::cont::make_ArrayHandleCounting<ValueType>(1, 3, numValues);

    this->RefComposite = vtkm::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4);
  }

  InputArray BuildInputArray() const
  {
    InputArray result;
    vtkm::cont::ArrayCopyDevice(this->RefComposite, result);
    return result;
  }

  void SanityCheck(vtkm::IdComponent component) const
  {
    InputArray composite = this->BuildInputArray();
    ExtractArray extract(composite, component);

    VTKM_TEST_ASSERT(composite.GetNumberOfValues() == extract.GetNumberOfValues(),
                     "Number of values in copied ExtractComponent array does not match input.");
  }

  void ReadTestComponentExtraction(vtkm::IdComponent component) const
  {
    // Test that the expected values are read from an ExtractComponent array.
    InputArray composite = this->BuildInputArray();
    ExtractArray extract(composite, component);

    // Test reading the data back in the control env:
    this->ValidateReadTestArray(extract, component);

    // Copy the extract array in the execution environment to test reading:
    vtkm::cont::ArrayHandle<ValueType> execCopy;
    vtkm::cont::ArrayCopy(extract, execCopy);
    this->ValidateReadTestArray(execCopy, component);
  }

  template <typename ArrayHandleType>
  void ValidateReadTestArray(ArrayHandleType testArray, vtkm::IdComponent component) const
  {
    using RefVectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = vtkm::VecTraits<RefVectorType>;

    auto testPortal = testArray.ReadPortal();
    auto refPortal = this->RefComposite.ReadPortal();

    VTKM_TEST_ASSERT(testPortal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in read test output do not match input.");

    for (vtkm::Id i = 0; i < testPortal.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(
        test_equal(testPortal.Get(i), Traits::GetComponent(refPortal.Get(i), component), 0.),
        "Value mismatch in read test.");
    }
  }

  // Doubles the specified component
  struct WriteTestWorklet : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn referenceArray, FieldOut componentArray);

    vtkm::IdComponent Component;

    VTKM_CONT WriteTestWorklet(vtkm::IdComponent component)
      : Component(component)
    {
    }

    template <typename RefType, typename ComponentType>
    VTKM_EXEC void operator()(const RefType& reference, ComponentType& outComponent) const
    {
      outComponent = vtkm::VecTraits<RefType>::GetComponent(reference, this->Component) * 2;
    }
  };

  void WriteTestComponentExtraction(vtkm::IdComponent component) const
  {
    // Control test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray extract(composite, component);

      {
        auto refPortal = this->RefComposite.ReadPortal();
        auto outPortal = extract.WritePortal();
        for (vtkm::Id i = 0; i < extract.GetNumberOfValues(); ++i)
        {
          auto ref = refPortal.Get(i);
          using Traits = vtkm::VecTraits<decltype(ref)>;
          outPortal.Set(i, Traits::GetComponent(ref, component) * 2);
        }
      }

      this->ValidateWriteTestArray(composite, component);
    }

    // Exec test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray extract(composite, component);

      vtkm::cont::Invoker{}(WriteTestWorklet(component), this->RefComposite, extract);

      this->ValidateWriteTestArray(composite, component);
    }
  }

  void ValidateWriteTestArray(InputArray testArray, vtkm::IdComponent component) const
  {
    using VectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = vtkm::VecTraits<VectorType>;

    // Check that the indicated component is twice the reference value.
    auto refPortal = this->RefComposite.ReadPortal();
    auto portal = testArray.ReadPortal();

    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in write test output do not match input.");

    for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);
      Traits::SetComponent(refValue, component, Traits::GetComponent(refValue, component) * 2);

      VTKM_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in write test.");
    }
  }

  void TestComponent(vtkm::IdComponent component) const
  {
    this->SanityCheck(component);
    this->ReadTestComponentExtraction(component);
    this->WriteTestComponentExtraction(component);
  }

  void operator()()
  {
    this->ConstructReferenceArray();

    this->TestComponent(0);
    this->TestComponent(1);
    this->TestComponent(2);
    this->TestComponent(3);
  }
};

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    ExtractComponentTests<ValueType>()();
  }
};

void TestArrayHandleExtractComponent()
{
  using TestTypes = vtkm::List<vtkm::Int32, vtkm::Int64, vtkm::Float32, vtkm::Float64>;
  vtkm::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

} // end anon namespace

int UnitTestArrayHandleExtractComponent(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleExtractComponent, argc, argv);
}
