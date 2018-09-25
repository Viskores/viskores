//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleExtractComponent.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleReverse.h>
#include <vtkm/cont/ArrayHandleSwizzle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/testing/TestingSerialization.h>

#include <vtkm/VecTraits.h>

#include <ctime>
#include <type_traits>
#include <vector>

using namespace vtkm::cont::testing::serialization;

namespace
{

//-----------------------------------------------------------------------------
struct TestEqualArrayHandle
{
public:
  template <typename ArrayHandle1, typename ArrayHandle2>
  VTKM_CONT void operator()(const ArrayHandle1& array1, const ArrayHandle2& array2) const
  {
    auto result = vtkm::cont::testing::test_equal_ArrayHandles(array1, array2);
    VTKM_TEST_ASSERT(result, result.GetMergedMessage());
  }
};

//-----------------------------------------------------------------------------
template <typename T>
inline void RunTest(const T& obj)
{
  TestSerialization(obj, TestEqualArrayHandle{});
}

//-----------------------------------------------------------------------------
constexpr vtkm::Id ArraySize = 10;

using TestTypesList =
  vtkm::ListTagBase<vtkm::Int8, vtkm::Id, vtkm::FloatDefault, vtkm::Vec<vtkm::FloatDefault, 3>>;
using TestStorageList = vtkm::ListTagBase<vtkm::cont::StorageTagBasic>;

template <typename T, typename S>
inline vtkm::cont::DynamicArrayHandleBase<vtkm::ListTagAppendUnique<TestTypesList, T>,
                                          vtkm::ListTagAppendUnique<TestStorageList, S>>
MakeTestDynamicArrayHandle(const vtkm::cont::ArrayHandle<T, S>& array)
{
  return array;
}

struct TestArrayHandleBasic
{
  template <typename T>
  void operator()(T) const
  {
    auto array = RandomArrayHandle<T>::Make(ArraySize);
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleCartesianProduct
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      vtkm::cont::make_ArrayHandleCartesianProduct(RandomArrayHandle<T>::Make(ArraySize),
                                                   RandomArrayHandle<T>::Make(ArraySize),
                                                   RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleCast
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      vtkm::cont::make_ArrayHandleCast(RandomArrayHandle<vtkm::Int8>::Make(ArraySize), T{});
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleCompositeVector
{
  template <typename T>
  void operator()(T) const
  {
    auto array = vtkm::cont::make_ArrayHandleCompositeVector(RandomArrayHandle<T>::Make(ArraySize),
                                                             RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleConcatenate
{
  template <typename T>
  void operator()(T) const
  {
    auto array = vtkm::cont::make_ArrayHandleConcatenate(RandomArrayHandle<T>::Make(ArraySize),
                                                         RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleConstant
{
  template <typename T>
  void operator()(T) const
  {
    T cval = RandomValue<T>::Make();
    auto array = vtkm::cont::make_ArrayHandleConstant(cval, ArraySize);
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleCounting
{
  template <typename T>
  void operator()(T) const
  {
    T start = RandomValue<T>::Make();
    T step = RandomValue<T>::Make(0, 5);
    auto array = vtkm::cont::make_ArrayHandleCounting(start, step, ArraySize);
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleExtractComponent
{
  template <typename T>
  void operator()(T) const
  {
    auto numComps = vtkm::VecTraits<T>::NUM_COMPONENTS;
    auto array = vtkm::cont::make_ArrayHandleExtractComponent(
      RandomArrayHandle<T>::Make(ArraySize), RandomValue<vtkm::IdComponent>::Make(0, numComps - 1));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleGroupVec
{
  template <typename T>
  void operator()(T) const
  {
    auto numComps = RandomValue<vtkm::IdComponent>::Make(2, 4);
    auto flat = RandomArrayHandle<T>::Make(ArraySize * numComps);
    switch (numComps)
    {
      case 3:
      {
        auto array = vtkm::cont::make_ArrayHandleGroupVec<3>(flat);
        RunTest(array);
        RunTest(MakeTestDynamicArrayHandle(array));
        break;
      }
      case 4:
      {
        auto array = vtkm::cont::make_ArrayHandleGroupVec<4>(flat);
        RunTest(array);
        RunTest(MakeTestDynamicArrayHandle(array));
        break;
      }
      default:
      {
        auto array = vtkm::cont::make_ArrayHandleGroupVec<2>(flat);
        RunTest(array);
        RunTest(MakeTestDynamicArrayHandle(array));
        break;
      }
    }
  }
};

struct TestArrayHandleGroupVecVariable
{
  template <typename T>
  void operator()(T) const
  {
    auto rangen = UniformRandomValueGenerator<vtkm::IdComponent>(1, 4);
    vtkm::Id size = 0;

    std::vector<vtkm::Id> comps(ArraySize);
    std::generate(comps.begin(), comps.end(), [&size, &rangen]() {
      auto offset = size;
      size += rangen();
      return offset;
    });

    auto array = vtkm::cont::make_ArrayHandleGroupVecVariable(RandomArrayHandle<T>::Make(size),
                                                              vtkm::cont::make_ArrayHandle(comps));
    RunTest(array);

    // cannot make a DynamicArrayHandle containing ArrayHandleGroupVecVariable
    // because of the variable number of components of its values.
    // RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleImplicit
{
  template <typename T>
  struct ImplicitFunctor
  {
    ImplicitFunctor() = default;

    explicit ImplicitFunctor(const T& factor)
      : Factor(factor)
    {
    }

    VTKM_EXEC_CONT T operator()(vtkm::Id index) const
    {
      return static_cast<T>(this->Factor *
                            static_cast<typename vtkm::VecTraits<T>::ComponentType>(index));
    }

    T Factor;
  };

  template <typename T>
  void operator()(T) const
  {
    ImplicitFunctor<T> functor(RandomValue<T>::Make(2, 9));
    auto array = vtkm::cont::make_ArrayHandleImplicit(functor, ArraySize);
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

void TestArrayHandleIndex()
{
  auto size = RandomValue<vtkm::Id>::Make(2, 10);
  auto array = vtkm::cont::ArrayHandleIndex(size);
  RunTest(array);
  RunTest(MakeTestDynamicArrayHandle(array));
}

struct TestArrayHandlePermutation
{
  template <typename T>
  void operator()(T) const
  {
    std::uniform_int_distribution<vtkm::Id> distribution(0, ArraySize - 1);

    std::vector<vtkm::Id> inds(ArraySize);
    std::generate(inds.begin(), inds.end(), [&distribution]() { return distribution(generator); });

    auto array = vtkm::cont::make_ArrayHandlePermutation(
      RandomArrayHandle<vtkm::Id>::Make(ArraySize, 0, ArraySize - 1),
      RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleReverse
{
  template <typename T>
  void operator()(T) const
  {
    auto array = vtkm::cont::make_ArrayHandleReverse(RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

struct TestArrayHandleSwizzle
{
  template <typename T>
  void operator()(T) const
  {
    static const vtkm::Vec<vtkm::IdComponent, 2> map2s[6] = { { 0, 1 }, { 0, 2 }, { 1, 0 },
                                                              { 1, 2 }, { 2, 0 }, { 2, 1 } };
    static const vtkm::Vec<vtkm::IdComponent, 3> map3s[6] = {
      { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 }
    };

    auto numOutComps = RandomValue<vtkm::IdComponent>::Make(2, 3);
    switch (numOutComps)
    {
      case 2:
      {
        auto array = make_ArrayHandleSwizzle(RandomArrayHandle<vtkm::Vec<T, 3>>::Make(ArraySize),
                                             map2s[RandomValue<int>::Make(0, 5)]);
        RunTest(array);
        RunTest(MakeTestDynamicArrayHandle(array));
        break;
      }
      case 3:
      default:
      {
        auto array = make_ArrayHandleSwizzle(RandomArrayHandle<vtkm::Vec<T, 3>>::Make(ArraySize),
                                             map3s[RandomValue<int>::Make(0, 5)]);
        RunTest(array);
        RunTest(MakeTestDynamicArrayHandle(array));
        break;
      }
    }
  }
};

struct TestArrayHandleTransform
{
  struct TransformFunctor
  {
    template <typename T>
    VTKM_EXEC_CONT T operator()(const T& in) const
    {
      return static_cast<T>(in * T{ 2 });
    }
  };

  struct InverseTransformFunctor
  {
    template <typename T>
    VTKM_EXEC_CONT T operator()(const T& in) const
    {
      return in / T{ 2 };
    }
  };

  template <typename T>
  void TestType1() const
  {
    auto array = vtkm::cont::make_ArrayHandleTransform(RandomArrayHandle<T>::Make(ArraySize),
                                                       TransformFunctor{});
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }

  template <typename T>
  void TestType2() const
  {
    auto array = vtkm::cont::make_ArrayHandleTransform(
      RandomArrayHandle<T>::Make(ArraySize), TransformFunctor{}, InverseTransformFunctor{});
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }

  template <typename T>
  void operator()(T) const
  {
    this->TestType1<T>();
    this->TestType2<T>();
  }
};

vtkm::cont::ArrayHandleUniformPointCoordinates MakeRandomArrayHandleUniformPointCoordinates()
{
  auto dimensions = RandomValue<vtkm::Id3>::Make(1, 3);
  auto origin = RandomValue<vtkm::Vec<vtkm::FloatDefault, 3>>::Make();
  auto spacing = RandomValue<vtkm::Vec<vtkm::FloatDefault, 3>>::Make(0.1f, 10.0f);
  return vtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing);
}

void TestArrayHandleUniformPointCoordinates()
{
  auto array = MakeRandomArrayHandleUniformPointCoordinates();
  RunTest(array);
  RunTest(MakeTestDynamicArrayHandle(array));
}

void TestArrayHandleVirtualCoordinates()
{
  int type = RandomValue<int>::Make(0, 2);

  vtkm::cont::ArrayHandleVirtualCoordinates array;
  switch (type)
  {
    case 0:
      array =
        vtkm::cont::ArrayHandleVirtualCoordinates(MakeRandomArrayHandleUniformPointCoordinates());
      break;
    case 1:
      array =
        vtkm::cont::ArrayHandleVirtualCoordinates(vtkm::cont::make_ArrayHandleCartesianProduct(
          RandomArrayHandle<vtkm::FloatDefault>::Make(ArraySize),
          RandomArrayHandle<vtkm::FloatDefault>::Make(ArraySize),
          RandomArrayHandle<vtkm::FloatDefault>::Make(ArraySize)));
      break;
    default:
      array = vtkm::cont::ArrayHandleVirtualCoordinates(
        RandomArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>::Make(ArraySize));
      break;
  }

  RunTest(array);
  RunTest(MakeTestDynamicArrayHandle(array));
}

struct TestArrayHandleZip
{
  template <typename T>
  void operator()(T) const
  {
    auto array = vtkm::cont::make_ArrayHandleZip(RandomArrayHandle<T>::Make(ArraySize),
                                                 vtkm::cont::ArrayHandleIndex(ArraySize));
    RunTest(array);
    RunTest(MakeTestDynamicArrayHandle(array));
  }
};

//-----------------------------------------------------------------------------
void TestArrayHandleSerialization()
{
  std::cout << "Testing ArrayHandleBasic\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleBasic(), TestTypesList());

  std::cout << "Testing ArrayHandleCartesianProduct\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCartesianProduct(), TestTypesList());

  std::cout << "Testing TestArrayHandleCast\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCast(), TestTypesList());

  std::cout << "Testing ArrayHandleCompositeVector\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCompositeVector(), TestTypesList());

  std::cout << "Testing ArrayHandleConcatenate\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleConcatenate(), TestTypesList());

  std::cout << "Testing ArrayHandleConstant\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleConstant(), TestTypesList());

  std::cout << "Testing ArrayHandleCounting\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCounting(), TestTypesList());

  std::cout << "Testing ArrayHandleExtractComponent\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleExtractComponent(), TestTypesList());

  std::cout << "Testing ArrayHandleGroupVec\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleGroupVec(), TestTypesList());

  std::cout << "Testing ArrayHandleGroupVecVariable\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleGroupVecVariable(), TestTypesList());

  std::cout << "Testing ArrayHandleImplicit\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleImplicit(), TestTypesList());

  std::cout << "Testing ArrayHandleIndex\n";
  TestArrayHandleIndex();

  std::cout << "Testing ArrayHandlePermutation\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandlePermutation(), TestTypesList());

  std::cout << "Testing ArrayHandleReverse\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleReverse(), TestTypesList());

  std::cout << "Testing ArrayHandleSwizzle\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleSwizzle(), TestTypesList());

  std::cout << "Testing ArrayHandleTransform\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleTransform(), TestTypesList());

  std::cout << "Testing ArrayHandleUniformPointCoordinates\n";
  TestArrayHandleUniformPointCoordinates();

  std::cout << "Testing ArrayHandleVirtualCoordinates\n";
  TestArrayHandleVirtualCoordinates();

  std::cout << "Testing ArrayHandleZip\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleZip(), TestTypesList());
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestSerializationArrayHandle(int, char* [])
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  decltype(generator)::result_type seed = 0;
  if (comm.rank() == 0)
  {
    seed = static_cast<decltype(seed)>(std::time(nullptr));
    std::cout << "using seed: " << seed << "\n";
  }
  diy::mpi::broadcast(comm, seed, 0);
  generator.seed(seed);

  return vtkm::cont::testing::Testing::Run(TestArrayHandleSerialization);
}

//-----------------------------------------------------------------------------
namespace vtkm
{
namespace cont
{

template <typename T>
struct TypeString<TestArrayHandleImplicit::ImplicitFunctor<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "TestArrayHandleImplicit::ImplicitFunctor<" + TypeString<T>::Get() + ">";
    return name;
  }
};

template <>
struct TypeString<TestArrayHandleTransform::TransformFunctor>
{
  static VTKM_CONT const std::string Get() { return "TestArrayHandleTransform::TransformFunctor"; }
};

template <>
struct TypeString<TestArrayHandleTransform::InverseTransformFunctor>
{
  static VTKM_CONT const std::string Get()
  {
    return "TestArrayHandleTransform::InverseTransformFunctor";
  }
};
}
} // vtkm::cont
