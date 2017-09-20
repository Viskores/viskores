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
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename ValueType>
struct ExtractComponentTests
{
  using InputArray = vtkm::cont::ArrayHandle<vtkm::Vec<ValueType, 4>>;
  template <vtkm::IdComponent Component>
  using ExtractArray = vtkm::cont::ArrayHandleExtractComponent<InputArray, Component>;
  using ReferenceComponentArray = vtkm::cont::ArrayHandleCounting<ValueType>;
  using ReferenceCompositeArray =
    typename vtkm::cont::ArrayHandleCompositeVectorType<ReferenceComponentArray,
                                                        ReferenceComponentArray,
                                                        ReferenceComponentArray,
                                                        ReferenceComponentArray>::type;

  using DeviceTag = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

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

    this->RefComposite = vtkm::cont::make_ArrayHandleCompositeVector(c1, 0, c2, 0, c3, 0, c4, 0);
  }

  InputArray BuildInputArray() const
  {
    InputArray result;
    Algo::Copy(this->RefComposite, result);
    return result;
  }

  template <vtkm::IdComponent Component>
  void SanityCheck() const
  {
    InputArray composite = this->BuildInputArray();
    ExtractArray<Component> extract =
      vtkm::cont::make_ArrayHandleExtractComponent<Component>(composite);

    VTKM_TEST_ASSERT(composite.GetNumberOfValues() == extract.GetNumberOfValues(),
                     "Number of values in copied ExtractComponent array does not match input.");
  }

  template <vtkm::IdComponent Component>
  void ReadTestComponentExtraction() const
  {
    // Test that the expected values are read from an ExtractComponent array.
    InputArray composite = this->BuildInputArray();
    ExtractArray<Component> extract =
      vtkm::cont::make_ArrayHandleExtractComponent<Component>(composite);

    // Test reading the data back in the control env:
    this->ValidateReadTestArray<Component>(extract);

    // Copy the extract array in the execution environment to test reading:
    vtkm::cont::ArrayHandle<ValueType> execCopy;
    Algo::Copy(extract, execCopy);
    this->ValidateReadTestArray<Component>(execCopy);
  }

  template <vtkm::IdComponent Component, typename ArrayHandleType>
  void ValidateReadTestArray(ArrayHandleType testArray) const
  {
    using RefVectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = vtkm::VecTraits<RefVectorType>;

    auto testPortal = testArray.GetPortalConstControl();
    auto refPortal = this->RefComposite.GetPortalConstControl();

    VTKM_TEST_ASSERT(testPortal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in read test output do not match input.");

    for (vtkm::Id i = 0; i < testPortal.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(
        test_equal(testPortal.Get(i), Traits::GetComponent(refPortal.Get(i), Component), 0.),
        "Value mismatch in read test.");
    }
  }

  // Doubles the specified component (reading from RefVectorType).
  template <typename PortalType, typename RefPortalType, vtkm::IdComponent Component>
  struct WriteTestFunctor : vtkm::exec::FunctorBase
  {
    using RefVectorType = typename RefPortalType::ValueType;
    using Traits = vtkm::VecTraits<RefVectorType>;

    PortalType Portal;
    RefPortalType RefPortal;

    VTKM_CONT
    WriteTestFunctor(const PortalType& portal, const RefPortalType& ref)
      : Portal(portal)
      , RefPortal(ref)
    {
    }

    VTKM_EXEC_CONT
    void operator()(vtkm::Id index) const
    {
      this->Portal.Set(index, Traits::GetComponent(this->RefPortal.Get(index), Component) * 2);
    }
  };

  template <vtkm::IdComponent Component>
  void WriteTestComponentExtraction() const
  {
    // Control test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray<Component> extract =
        vtkm::cont::make_ArrayHandleExtractComponent<Component>(composite);

      WriteTestFunctor<typename ExtractArray<Component>::PortalControl,
                       typename ReferenceCompositeArray::PortalConstControl,
                       Component>
        functor(extract.GetPortalControl(), this->RefComposite.GetPortalConstControl());

      for (vtkm::Id i = 0; i < extract.GetNumberOfValues(); ++i)
      {
        functor(i);
      }

      this->ValidateWriteTestArray<Component>(composite);
    }

    // Exec test:
    {
      InputArray composite = this->BuildInputArray();
      ExtractArray<Component> extract =
        vtkm::cont::make_ArrayHandleExtractComponent<Component>(composite);

      using Portal = typename ExtractArray<Component>::template ExecutionTypes<DeviceTag>::Portal;
      using RefPortal =
        typename ReferenceCompositeArray::template ExecutionTypes<DeviceTag>::PortalConst;

      WriteTestFunctor<Portal, RefPortal, Component> functor(
        extract.PrepareForInPlace(DeviceTag()), this->RefComposite.PrepareForInput(DeviceTag()));

      Algo::Schedule(functor, extract.GetNumberOfValues());
      this->ValidateWriteTestArray<Component>(composite);
    }
  }

  template <vtkm::IdComponent Component>
  void ValidateWriteTestArray(InputArray testArray) const
  {
    using VectorType = typename ReferenceCompositeArray::ValueType;
    using Traits = vtkm::VecTraits<VectorType>;

    // Check that the indicated component is twice the reference value.
    auto refPortal = this->RefComposite.GetPortalConstControl();
    auto portal = testArray.GetPortalConstControl();

    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in write test output do not match input.");

    for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);
      Traits::SetComponent(refValue, Component, Traits::GetComponent(refValue, Component) * 2);

      VTKM_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in write test.");
    }
  }

  template <vtkm::IdComponent Component>
  void TestComponent() const
  {
    this->SanityCheck<Component>();
    this->ReadTestComponentExtraction<Component>();
    this->WriteTestComponentExtraction<Component>();
  }

  void operator()()
  {
    this->ConstructReferenceArray();

    this->TestComponent<0>();
    this->TestComponent<1>();
    this->TestComponent<2>();
    this->TestComponent<3>();
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
  using TestTypes = vtkm::ListTagBase<vtkm::Int32, vtkm::Int64, vtkm::Float32, vtkm::Float64>;
  vtkm::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

} // end anon namespace

int UnitTestArrayHandleExtractComponent(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestArrayHandleExtractComponent);
}
