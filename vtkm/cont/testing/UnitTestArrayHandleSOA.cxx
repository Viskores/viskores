//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleSOA.h>

#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

using ScalarTypesToTest = vtkm::List<vtkm::UInt8, vtkm::FloatDefault>;
using VectorTypesToTest = vtkm::List<vtkm::Vec2i_8, vtkm::Vec3f_32>;

struct PassThrough : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename InValue, typename OutValue>
  VTKM_EXEC void operator()(const InValue& inValue, OutValue& outValue) const
  {
    outValue = inValue;
  }
};

struct TestArrayPortalSOA
{
  template <typename ComponentType>
  VTKM_CONT void operator()(ComponentType) const
  {
    constexpr vtkm::IdComponent NUM_COMPONENTS = 4;
    using ValueType = vtkm::Vec<ComponentType, NUM_COMPONENTS>;
    using ComponentArrayType = vtkm::cont::ArrayHandle<ComponentType>;
    using SOAPortalType =
      vtkm::internal::ArrayPortalSOA<ValueType, typename ComponentArrayType::WritePortalType>;

    std::cout << "Test SOA portal reflects data in component portals." << std::endl;
    SOAPortalType soaPortalIn(ARRAY_SIZE);

    std::array<vtkm::cont::ArrayHandle<ComponentType>, NUM_COMPONENTS> implArrays;
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      vtkm::cont::ArrayHandle<ComponentType> array;
      array.Allocate(ARRAY_SIZE);
      auto portal = array.WritePortal();
      for (vtkm::IdComponent valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        portal.Set(valueIndex, TestValue(valueIndex, ValueType{})[componentIndex]);
      }

      soaPortalIn.SetPortal(componentIndex, portal);

      implArrays[static_cast<std::size_t>(componentIndex)] = array;
    }

    VTKM_TEST_ASSERT(soaPortalIn.GetNumberOfValues() == ARRAY_SIZE);
    CheckPortal(soaPortalIn);

    std::cout << "Test data set in SOA portal gets set in component portals." << std::endl;
    {
      SOAPortalType soaPortalOut(ARRAY_SIZE);
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        vtkm::cont::ArrayHandle<ComponentType> array;
        array.Allocate(ARRAY_SIZE);
        auto portal = array.WritePortal();
        soaPortalOut.SetPortal(componentIndex, portal);

        implArrays[static_cast<std::size_t>(componentIndex)] = array;
      }

      SetPortal(soaPortalOut);
    }

    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      auto portal = implArrays[static_cast<size_t>(componentIndex)].ReadPortal();
      for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        ComponentType x = TestValue(valueIndex, ValueType{})[componentIndex];
        VTKM_TEST_ASSERT(test_equal(x, portal.Get(valueIndex)));
      }
    }
  }
};

struct TestSOAAsInput
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
  {
    using VTraits = vtkm::VecTraits<ValueType>;
    using ComponentType = typename VTraits::ComponentType;
    constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

    {
      vtkm::cont::ArrayHandleSOA<ValueType> soaArray;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
      {
        vtkm::cont::ArrayHandle<ComponentType> componentArray;
        componentArray.Allocate(ARRAY_SIZE);
        auto componentPortal = componentArray.WritePortal();
        for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
        {
          componentPortal.Set(
            valueIndex, VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex));
        }
        soaArray.SetArray(componentIndex, componentArray);
      }

      VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
      VTKM_TEST_ASSERT(soaArray.ReadPortal().GetNumberOfValues() == ARRAY_SIZE);
      CheckPortal(soaArray.ReadPortal());

      vtkm::cont::ArrayHandle<ValueType> basicArray;
      vtkm::cont::ArrayCopyDevice(soaArray, basicArray);
      VTKM_TEST_ASSERT(basicArray.GetNumberOfValues() == ARRAY_SIZE);
      CheckPortal(basicArray.ReadPortal());
    }

    {
      // Check constructors
      using Vec3 = vtkm::Vec<ComponentType, 3>;
      std::vector<ComponentType> vector0;
      std::vector<ComponentType> vector1;
      std::vector<ComponentType> vector2;
      for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        Vec3 value = TestValue(valueIndex, Vec3{});
        vector0.push_back(value[0]);
        vector1.push_back(value[1]);
        vector2.push_back(value[2]);
      }

      {
        vtkm::cont::ArrayHandleSOA<Vec3> soaArray =
          vtkm::cont::make_ArrayHandleSOA<Vec3>({ vector0, vector1, vector2 });
        VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());
      }

      {
        vtkm::cont::ArrayHandleSOA<Vec3> soaArray =
          vtkm::cont::make_ArrayHandleSOA(vtkm::CopyFlag::Off, vector0, vector1, vector2);
        VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());

        // Make sure calling ReleaseResources does not result in error.
        soaArray.ReleaseResources();
      }

      {
        vtkm::cont::ArrayHandleSOA<Vec3> soaArray = vtkm::cont::make_ArrayHandleSOA<Vec3>(
          { vector0.data(), vector1.data(), vector2.data() }, ARRAY_SIZE, vtkm::CopyFlag::Off);
        VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());
      }

      {
        vtkm::cont::ArrayHandleSOA<Vec3> soaArray = vtkm::cont::make_ArrayHandleSOA(
          ARRAY_SIZE, vtkm::CopyFlag::Off, vector0.data(), vector1.data(), vector2.data());
        VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
        CheckPortal(soaArray.ReadPortal());
      }
    }
  }
};

struct TestSOAAsOutput
{
  template <typename ValueType>
  VTKM_CONT void operator()(const ValueType vtkmNotUsed(v)) const
  {
    using VTraits = vtkm::VecTraits<ValueType>;
    using ComponentType = typename VTraits::ComponentType;
    constexpr vtkm::IdComponent NUM_COMPONENTS = VTraits::NUM_COMPONENTS;

    vtkm::cont::ArrayHandle<ValueType> basicArray;
    basicArray.Allocate(ARRAY_SIZE);
    SetPortal(basicArray.WritePortal());

    vtkm::cont::ArrayHandleSOA<ValueType> soaArray;
    vtkm::cont::Invoker{}(PassThrough{}, basicArray, soaArray);

    VTKM_TEST_ASSERT(soaArray.GetNumberOfValues() == ARRAY_SIZE);
    for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; ++componentIndex)
    {
      vtkm::cont::ArrayHandle<ComponentType> componentArray = soaArray.GetArray(componentIndex);
      auto componentPortal = componentArray.ReadPortal();
      for (vtkm::Id valueIndex = 0; valueIndex < ARRAY_SIZE; ++valueIndex)
      {
        ComponentType expected =
          VTraits::GetComponent(TestValue(valueIndex, ValueType{}), componentIndex);
        ComponentType got = componentPortal.Get(valueIndex);
        VTKM_TEST_ASSERT(test_equal(expected, got));
      }
    }
  }
};

static void Run()
{
  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayPortalSOA" << std::endl;
  vtkm::testing::Testing::TryTypes(TestArrayPortalSOA(), ScalarTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleSOA as Input" << std::endl;
  vtkm::testing::Testing::TryTypes(TestSOAAsInput(), VectorTypesToTest());

  std::cout << "-------------------------------------------" << std::endl;
  std::cout << "Testing ArrayHandleSOA as Output" << std::endl;
  vtkm::testing::Testing::TryTypes(TestSOAAsOutput(), VectorTypesToTest());
}

} // anonymous namespace

int UnitTestArrayHandleSOA(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
