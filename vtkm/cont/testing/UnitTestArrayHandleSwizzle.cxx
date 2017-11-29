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
#include <vtkm/cont/ArrayHandleSwizzle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/testing/Testing.h>

#include <type_traits>

namespace
{

template <typename ValueType>
struct SwizzleTests
{
  using SwizzleInputArrayType = vtkm::cont::ArrayHandle<vtkm::Vec<ValueType, 4>>;

  template <vtkm::IdComponent... ComponentMap>
  using SwizzleArrayType = vtkm::cont::ArrayHandleSwizzle<SwizzleInputArrayType, ComponentMap...>;

  using ReferenceComponentArrayType = vtkm::cont::ArrayHandleCounting<ValueType>;
  using ReferenceArrayType =
    typename vtkm::cont::ArrayHandleCompositeVectorType<ReferenceComponentArrayType,
                                                        ReferenceComponentArrayType,
                                                        ReferenceComponentArrayType,
                                                        ReferenceComponentArrayType>::type;

  using DeviceTag = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

  // This is used to build a ArrayHandleSwizzle's internal array.
  ReferenceArrayType RefArray;

  void ConstructReferenceArray()
  {
    // Build the Ref array
    const vtkm::Id numValues = 32;
    ReferenceComponentArrayType c1 =
      vtkm::cont::make_ArrayHandleCounting<ValueType>(3, 2, numValues);
    ReferenceComponentArrayType c2 =
      vtkm::cont::make_ArrayHandleCounting<ValueType>(2, 3, numValues);
    ReferenceComponentArrayType c3 =
      vtkm::cont::make_ArrayHandleCounting<ValueType>(4, 4, numValues);
    ReferenceComponentArrayType c4 =
      vtkm::cont::make_ArrayHandleCounting<ValueType>(1, 3, numValues);

    this->RefArray = vtkm::cont::make_ArrayHandleCompositeVector(c1, 0, c2, 0, c3, 0, c4, 0);
  }

  SwizzleInputArrayType BuildSwizzleInputArray() const
  {
    SwizzleInputArrayType result;
    Algo::Copy(this->RefArray, result);
    return result;
  }

  template <vtkm::IdComponent... ComponentMap>
  void SanityCheck() const
  {
    using Swizzle = SwizzleArrayType<ComponentMap...>;
    using Traits = typename Swizzle::SwizzleTraits;

    VTKM_TEST_ASSERT(Traits::COUNT == vtkm::VecTraits<typename Swizzle::ValueType>::NUM_COMPONENTS,
                     "Traits::COUNT invalid.");
    VTKM_TEST_ASSERT(
      VTKM_PASS_COMMAS(std::is_same<typename Traits::ComponentType, ValueType>::value),
      "Traits::ComponentType invalid.");
    VTKM_TEST_ASSERT(
      VTKM_PASS_COMMAS(
        std::is_same<
          typename Traits::OutputType,
          vtkm::Vec<ValueType, static_cast<vtkm::IdComponent>(sizeof...(ComponentMap))>>::value),
      "Traits::OutputType invalid.");

    SwizzleInputArrayType input = this->BuildSwizzleInputArray();
    SwizzleArrayType<ComponentMap...> swizzle =
      vtkm::cont::make_ArrayHandleSwizzle<ComponentMap...>(input);

    VTKM_TEST_ASSERT(input.GetNumberOfValues() == swizzle.GetNumberOfValues(),
                     "Number of values in copied Swizzle array does not match input.");
  }

  template <vtkm::IdComponent... ComponentMap>
  void ReadTest() const
  {
    using Traits = typename SwizzleArrayType<ComponentMap...>::SwizzleTraits;

    // Test that the expected values are read from an Swizzle array.
    SwizzleInputArrayType input = this->BuildSwizzleInputArray();
    SwizzleArrayType<ComponentMap...> swizzle =
      vtkm::cont::make_ArrayHandleSwizzle<ComponentMap...>(input);

    // Test reading the data back in the control env:
    this->ValidateReadTest<ComponentMap...>(swizzle);

    // Copy the extract array in the execution environment to test reading:
    vtkm::cont::ArrayHandle<typename Traits::OutputType> execCopy;
    Algo::Copy(swizzle, execCopy);
    this->ValidateReadTest<ComponentMap...>(execCopy);
  }

  template <vtkm::IdComponent... ComponentMap, typename ArrayHandleType>
  void ValidateReadTest(ArrayHandleType testArray) const
  {
    using Traits = typename SwizzleArrayType<ComponentMap...>::SwizzleTraits;
    using MapType = typename Traits::RuntimeComponentMapType;
    const MapType map = Traits::GenerateRuntimeComponentMap();

    using ReferenceVectorType = typename ReferenceArrayType::ValueType;
    using SwizzleVectorType = typename Traits::OutputType;

    VTKM_TEST_ASSERT(map.size() == vtkm::VecTraits<SwizzleVectorType>::NUM_COMPONENTS,
                     "Unexpected runtime component map size.");
    VTKM_TEST_ASSERT(testArray.GetNumberOfValues() == this->RefArray.GetNumberOfValues(),
                     "Number of values incorrect in Read test.");

    auto refPortal = this->RefArray.GetPortalConstControl();
    auto testPortal = testArray.GetPortalConstControl();

    SwizzleVectorType refVecSwizzle(vtkm::TypeTraits<SwizzleVectorType>::ZeroInitialization());
    for (vtkm::Id i = 0; i < testArray.GetNumberOfValues(); ++i)
    {
      ReferenceVectorType refVec = refPortal.Get(i);

      // Manually swizzle the reference vector using the runtime map information:
      for (size_t j = 0; j < map.size(); ++j)
      {
        refVecSwizzle[static_cast<vtkm::IdComponent>(j)] = refVec[map[j]];
      }

      VTKM_TEST_ASSERT(test_equal(refVecSwizzle, testPortal.Get(i), 0.),
                       "Invalid value encountered in Read test.");
    }
  }

  // Doubles everything in the input portal.
  template <typename PortalType>
  struct WriteTestFunctor : vtkm::exec::FunctorBase
  {
    PortalType Portal;

    VTKM_CONT
    WriteTestFunctor(const PortalType& portal)
      : Portal(portal)
    {
    }

    VTKM_EXEC_CONT
    void operator()(vtkm::Id index) const { this->Portal.Set(index, this->Portal.Get(index) * 2.); }
  };

  template <vtkm::IdComponent... ComponentMap>
  void WriteTest() const
  {
    // Control test:
    {
      SwizzleInputArrayType input = this->BuildSwizzleInputArray();
      SwizzleArrayType<ComponentMap...> swizzle =
        vtkm::cont::make_ArrayHandleSwizzle<ComponentMap...>(input);

      WriteTestFunctor<typename SwizzleArrayType<ComponentMap...>::PortalControl> functor(
        swizzle.GetPortalControl());

      for (vtkm::Id i = 0; i < swizzle.GetNumberOfValues(); ++i)
      {
        functor(i);
      }

      this->ValidateWriteTestArray<ComponentMap...>(input);
    }

    // Exec test:
    {
      SwizzleInputArrayType input = this->BuildSwizzleInputArray();
      SwizzleArrayType<ComponentMap...> swizzle =
        vtkm::cont::make_ArrayHandleSwizzle<ComponentMap...>(input);

      using Portal =
        typename SwizzleArrayType<ComponentMap...>::template ExecutionTypes<DeviceTag>::Portal;

      WriteTestFunctor<Portal> functor(swizzle.PrepareForInPlace(DeviceTag()));

      Algo::Schedule(functor, swizzle.GetNumberOfValues());
      this->ValidateWriteTestArray<ComponentMap...>(input);
    }
  }

  // Check that the swizzled components are twice the reference value.
  template <vtkm::IdComponent... ComponentMap>
  void ValidateWriteTestArray(SwizzleInputArrayType testArray) const
  {
    using Traits = typename SwizzleArrayType<ComponentMap...>::SwizzleTraits;
    using MapType = typename Traits::RuntimeComponentMapType;
    const MapType map = Traits::GenerateRuntimeComponentMap();

    auto refPortal = this->RefArray.GetPortalConstControl();
    auto portal = testArray.GetPortalConstControl();

    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == refPortal.GetNumberOfValues(),
                     "Number of values in write test output do not match input.");

    for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
    {
      auto value = portal.Get(i);
      auto refValue = refPortal.Get(i);

      // Double all of the components that appear in the map to replicate the
      // test result:
      for (size_t j = 0; j < map.size(); ++j)
      {
        refValue[map[j]] *= 2;
      }

      VTKM_TEST_ASSERT(test_equal(refValue, value, 0.), "Value mismatch in Write test.");
    }
  }

  template <vtkm::IdComponent... ComponentMap>
  void TestSwizzle() const
  {
    this->SanityCheck<ComponentMap...>();
    this->ReadTest<ComponentMap...>();
    this->WriteTest<ComponentMap...>();
  }

  void operator()()
  {
    this->ConstructReferenceArray();

// Enable for full test. We normally test a reduced set of component maps
// to keep compile times/sizes down:
#if 0
    this->TestSwizzle<0, 1>();
    this->TestSwizzle<0, 2>();
    this->TestSwizzle<0, 3>();
    this->TestSwizzle<1, 0>();
    this->TestSwizzle<1, 2>();
    this->TestSwizzle<1, 3>();
    this->TestSwizzle<2, 0>();
    this->TestSwizzle<2, 1>();
    this->TestSwizzle<2, 3>();
    this->TestSwizzle<3, 0>();
    this->TestSwizzle<3, 1>();
    this->TestSwizzle<3, 2>();
    this->TestSwizzle<0, 1, 2>();
    this->TestSwizzle<0, 1, 3>();
    this->TestSwizzle<0, 2, 1>();
    this->TestSwizzle<0, 2, 3>();
    this->TestSwizzle<0, 3, 1>();
    this->TestSwizzle<0, 3, 2>();
    this->TestSwizzle<1, 0, 2>();
    this->TestSwizzle<1, 0, 3>();
    this->TestSwizzle<1, 2, 0>();
    this->TestSwizzle<1, 2, 3>();
    this->TestSwizzle<1, 3, 0>();
    this->TestSwizzle<1, 3, 2>();
    this->TestSwizzle<2, 0, 1>();
    this->TestSwizzle<2, 0, 3>();
    this->TestSwizzle<2, 1, 0>();
    this->TestSwizzle<2, 1, 3>();
    this->TestSwizzle<2, 3, 0>();
    this->TestSwizzle<2, 3, 1>();
    this->TestSwizzle<3, 0, 1>();
    this->TestSwizzle<3, 0, 2>();
    this->TestSwizzle<3, 1, 0>();
    this->TestSwizzle<3, 1, 2>();
    this->TestSwizzle<3, 2, 0>();
    this->TestSwizzle<3, 2, 1>();
    this->TestSwizzle<0, 1, 2, 3>();
    this->TestSwizzle<0, 1, 3, 2>();
    this->TestSwizzle<0, 2, 1, 3>();
    this->TestSwizzle<0, 2, 3, 1>();
    this->TestSwizzle<0, 3, 1, 2>();
    this->TestSwizzle<0, 3, 2, 1>();
    this->TestSwizzle<1, 0, 2, 3>();
    this->TestSwizzle<1, 0, 3, 2>();
    this->TestSwizzle<1, 2, 0, 3>();
    this->TestSwizzle<1, 2, 3, 0>();
    this->TestSwizzle<1, 3, 0, 2>();
    this->TestSwizzle<1, 3, 2, 0>();
    this->TestSwizzle<2, 0, 1, 3>();
    this->TestSwizzle<2, 0, 3, 1>();
    this->TestSwizzle<2, 1, 0, 3>();
    this->TestSwizzle<2, 1, 3, 0>();
    this->TestSwizzle<2, 3, 0, 1>();
    this->TestSwizzle<2, 3, 1, 0>();
    this->TestSwizzle<3, 0, 1, 2>();
    this->TestSwizzle<3, 0, 2, 1>();
    this->TestSwizzle<3, 1, 0, 2>();
    this->TestSwizzle<3, 1, 2, 0>();
    this->TestSwizzle<3, 2, 0, 1>();
    this->TestSwizzle<3, 2, 1, 0>();
#else
    this->TestSwizzle<0, 1>();
    this->TestSwizzle<1, 0>();
    this->TestSwizzle<2, 3>();
    this->TestSwizzle<3, 2>();
    this->TestSwizzle<0, 1, 2>();
    this->TestSwizzle<0, 3, 1>();
    this->TestSwizzle<2, 0, 3>();
    this->TestSwizzle<3, 2, 1>();
    this->TestSwizzle<0, 1, 2, 3>();
    this->TestSwizzle<1, 3, 2, 0>();
    this->TestSwizzle<2, 0, 1, 3>();
    this->TestSwizzle<3, 1, 0, 2>();
    this->TestSwizzle<3, 2, 1, 0>();
#endif
  }
};

struct ArgToTemplateType
{
  template <typename ValueType>
  void operator()(ValueType) const
  {
    SwizzleTests<ValueType>()();
  }
};

void TestArrayHandleSwizzle()
{
  using TestTypes = vtkm::ListTagBase<vtkm::Int32, vtkm::Int64, vtkm::Float32, vtkm::Float64>;
  vtkm::testing::Testing::TryTypes(ArgToTemplateType(), TestTypes());
}

template <vtkm::IdComponent InputSize, vtkm::IdComponent... ComponentMap>
using Validator = vtkm::cont::internal::ValidateComponentMap<InputSize, ComponentMap...>;

void TestComponentMapValidator()
{
  using RepeatComps = Validator<5, 0, 3, 2, 3, 1, 4>;
  VTKM_TEST_ASSERT(!RepeatComps::Valid, "Repeat components allowed.");

  using NegativeComps = Validator<5, 0, 4, -3, 1, 2>;
  VTKM_TEST_ASSERT(!NegativeComps::Valid, "Negative components allowed.");

  using OutOfBoundsComps = Validator<5, 0, 2, 3, 5>;
  VTKM_TEST_ASSERT(!OutOfBoundsComps::Valid, "Out-of-bounds components allowed.");
}

void TestRuntimeComponentMapGenerator()
{
  // Dummy input vector type. Only concerned with the component map:
  using Dummy = vtkm::Vec<char, 7>;

  using Traits = vtkm::cont::ArrayHandleSwizzleTraits<Dummy, 3, 2, 4, 1, 6, 0>;
  using MapType = Traits::RuntimeComponentMapType;

  const MapType map = Traits::GenerateRuntimeComponentMap();

  VTKM_TEST_ASSERT(map.size() == 6, "Invalid map size.");
  VTKM_TEST_ASSERT(map[0] == 3, "Invalid map entry.");
  VTKM_TEST_ASSERT(map[1] == 2, "Invalid map entry.");
  VTKM_TEST_ASSERT(map[2] == 4, "Invalid map entry.");
  VTKM_TEST_ASSERT(map[3] == 1, "Invalid map entry.");
  VTKM_TEST_ASSERT(map[4] == 6, "Invalid map entry.");
  VTKM_TEST_ASSERT(map[5] == 0, "Invalid map entry.");
}

} // end anon namespace

int UnitTestArrayHandleSwizzle(int, char* [])
{
  try
  {
    TestComponentMapValidator();
    TestRuntimeComponentMapGenerator();
  }
  catch (vtkm::cont::Error& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return vtkm::cont::testing::Testing::Run(TestArrayHandleSwizzle);
}
