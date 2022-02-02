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
#include <vtkm/cont/ArrayHandleSOA.h>
#include <vtkm/cont/ArrayHandleSwizzle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleZip.h>

#include <vtkm/cont/UncertainArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/cont/testing/TestingSerialization.h>

#include <vtkm/VecTraits.h>

#include <ctime>
#include <type_traits>
#include <vector>

using namespace vtkm::cont::testing::serialization;

namespace
{

using TestTypesListScalar = vtkm::List<vtkm::Int8, vtkm::Id, vtkm::FloatDefault>;
using TestTypesListVec = vtkm::List<vtkm::Vec3f_32, vtkm::Vec3f_64>;
using TestTypesList = vtkm::ListAppend<TestTypesListScalar, TestTypesListVec>;

using StorageListInefficientExtract = vtkm::List<
  vtkm::cont::StorageTagCast<vtkm::Int8, vtkm::cont::StorageTagBasic>,
  vtkm::cont::StorageTagConstant,
  vtkm::cont::StorageTagCounting,
  vtkm::cont::StorageTagIndex,
  vtkm::cont::StorageTagPermutation<vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagBasic>>;

//-----------------------------------------------------------------------------
struct TestEqualArrayHandle
{
public:
  template <typename ArrayHandle1, typename ArrayHandle2>
  VTKM_CONT void operator()(const ArrayHandle1& array1, const ArrayHandle2& array2) const
  {
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(array1, array2));
  }

  template <typename TypeList, typename StorageList>
  VTKM_CONT void operator()(const vtkm::cont::UncertainArrayHandle<TypeList, StorageList>& array1,
                            const vtkm::cont::UnknownArrayHandle& array2) const
  {
    // This results in an excessive amount of compiling. However, we do it here to avoid
    // warnings about inefficient copies of the weirder arrays. That slowness might be OK
    // to test arrays, but we want to make sure that the serialization itself does not do
    // that.
    array1.CastAndCall([array2](const auto& concreteArray1) {
      using ArrayType = std::decay_t<decltype(concreteArray1)>;
      ArrayType concreteArray2;
      array2.AsArrayHandle(concreteArray2);
      test_equal_ArrayHandles(concreteArray1, concreteArray2);
    });
  }

  template <typename TypeList1, typename StorageList1, typename TypeList2, typename StorageList2>
  VTKM_CONT void operator()(
    const vtkm::cont::UncertainArrayHandle<TypeList1, StorageList1>& array1,
    const vtkm::cont::UncertainArrayHandle<TypeList2, StorageList2>& array2) const
  {
    (*this)(array1, vtkm::cont::UnknownArrayHandle(array2));
  }

  VTKM_CONT void operator()(const vtkm::cont::UnknownArrayHandle& array1,
                            const vtkm::cont::UnknownArrayHandle& array2) const
  {
    bool isInefficient = false;
    vtkm::ListForEach(
      [&](auto type) {
        using StorageTag = std::decay_t<decltype(type)>;
        if (array1.IsStorageType<StorageTag>())
        {
          isInefficient = true;
        }
      },
      StorageListInefficientExtract{});

    if (isInefficient)
    {
      (*this)(array1.ResetTypes<TestTypesList, StorageListInefficientExtract>(), array2);
    }
    else
    {
      test_equal_ArrayHandles(array1, array2);
    }
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

template <typename T, typename S>
inline vtkm::cont::UnknownArrayHandle MakeTestUnknownArrayHandle(
  const vtkm::cont::ArrayHandle<T, S>& array)
{
  return array;
}

template <typename T, typename S>
inline vtkm::cont::UncertainArrayHandle<vtkm::List<T>, vtkm::List<S>> MakeTestUncertainArrayHandle(
  const vtkm::cont::ArrayHandle<T, S>& array)
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
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleSOA
{
  template <typename T>
  void operator()(T) const
  {
    vtkm::cont::ArrayHandleSOA<T> array;
    vtkm::cont::ArrayCopy(RandomArrayHandle<T>::Make(ArraySize), array);
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
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
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleCast
{
  template <typename T>
  void operator()(T) const
  {
    auto array =
      vtkm::cont::make_ArrayHandleCast<T>(RandomArrayHandle<vtkm::Int8>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }

  template <typename T, vtkm::IdComponent N>
  void operator()(vtkm::Vec<T, N>) const
  {
    auto array = vtkm::cont::make_ArrayHandleCast<vtkm::Vec<T, N>>(
      RandomArrayHandle<vtkm::Vec<vtkm::Int8, N>>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
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
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
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
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
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
        RunTest(MakeTestUnknownArrayHandle(array));
        RunTest(MakeTestUncertainArrayHandle(array));
        break;
      }
      case 4:
      {
        auto array = vtkm::cont::make_ArrayHandleGroupVec<4>(flat);
        RunTest(array);
        RunTest(MakeTestUnknownArrayHandle(array));
        RunTest(MakeTestUncertainArrayHandle(array));
        break;
      }
      default:
      {
        auto array = vtkm::cont::make_ArrayHandleGroupVec<2>(flat);
        RunTest(array);
        RunTest(MakeTestUnknownArrayHandle(array));
        RunTest(MakeTestUncertainArrayHandle(array));
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

    auto array = vtkm::cont::make_ArrayHandleGroupVecVariable(
      RandomArrayHandle<T>::Make(size), vtkm::cont::make_ArrayHandle(comps, vtkm::CopyFlag::On));
    RunTest(array);

    // cannot make a UnknownArrayHandle containing ArrayHandleGroupVecVariable
    // because of the variable number of components of its values.
    // RunTest(MakeTestUnknownArrayHandle(array));
  }
};

void TestArrayHandleIndex()
{
  auto size = RandomValue<vtkm::Id>::Make(2, 10);
  auto array = vtkm::cont::ArrayHandleIndex(size);
  RunTest(array);
  RunTest(MakeTestUnknownArrayHandle(array));
  RunTest(MakeTestUncertainArrayHandle(array));
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
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleReverse
{
  template <typename T>
  void operator()(T) const
  {
    auto array = vtkm::cont::make_ArrayHandleReverse(RandomArrayHandle<T>::Make(ArraySize));
    RunTest(array);
    RunTest(MakeTestUnknownArrayHandle(array));
    RunTest(MakeTestUncertainArrayHandle(array));
  }
};

struct TestArrayHandleSwizzle
{
  template <typename T>
  void operator()(T) const
  {
    constexpr vtkm::IdComponent NUM_COMPONENTS = vtkm::VecTraits<T>::NUM_COMPONENTS;
    vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS> map;
    for (vtkm::IdComponent i = 0; i < NUM_COMPONENTS; ++i)
    {
      map[i] = NUM_COMPONENTS - (i + 1);
    }
    auto array = vtkm::cont::make_ArrayHandleSwizzle(RandomArrayHandle<T>::Make(ArraySize), map);
    RunTest(array);
  }
};


vtkm::cont::ArrayHandleUniformPointCoordinates MakeRandomArrayHandleUniformPointCoordinates()
{
  auto dimensions = RandomValue<vtkm::Id3>::Make(1, 3);
  auto origin = RandomValue<vtkm::Vec3f>::Make();
  auto spacing = RandomValue<vtkm::Vec3f>::Make(0.1f, 10.0f);
  return vtkm::cont::ArrayHandleUniformPointCoordinates(dimensions, origin, spacing);
}

void TestArrayHandleUniformPointCoordinates()
{
  auto array = MakeRandomArrayHandleUniformPointCoordinates();
  RunTest(array);
  RunTest(MakeTestUnknownArrayHandle(array));
  RunTest(MakeTestUncertainArrayHandle(array));
}


//-----------------------------------------------------------------------------
void TestArrayHandleSerialization()
{
  std::cout << "Testing ArrayHandleBasic\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleBasic(), TestTypesList());
  vtkm::testing::Testing::TryTypes(
    TestArrayHandleBasic(), vtkm::List<char, long, long long, unsigned long, unsigned long long>());

  std::cout << "Testing ArrayHandleSOA\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleSOA(), TestTypesListVec());

  std::cout << "Testing ArrayHandleCartesianProduct\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCartesianProduct(),
                                   vtkm::List<vtkm::Float32, vtkm::Float64>());

  std::cout << "Testing TestArrayHandleCast\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCast(), TestTypesList());

  std::cout << "Testing ArrayHandleConstant\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleConstant(), TestTypesList());

  std::cout << "Testing ArrayHandleCounting\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleCounting(), TestTypesList());

  std::cout << "Testing ArrayHandleGroupVec\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleGroupVec(), TestTypesListScalar());

  std::cout << "Testing ArrayHandleGroupVecVariable\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleGroupVecVariable(), TestTypesList());

  std::cout << "Testing ArrayHandleIndex\n";
  TestArrayHandleIndex();

  std::cout << "Testing ArrayHandlePermutation\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandlePermutation(), TestTypesList());

  std::cout << "Testing ArrayHandleReverse\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleReverse(), TestTypesList());

  std::cout << "Testing ArrayHandleSwizzle\n";
  vtkm::testing::Testing::TryTypes(TestArrayHandleSwizzle(), TestTypesList());

  std::cout << "Testing ArrayHandleUniformPointCoordinates\n";
  TestArrayHandleUniformPointCoordinates();
}

} // anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestSerializationArrayHandle(int argc, char* argv[])
{
  // Normally VTK-m `Testing::Run` would setup the diy MPI env,
  // but since we need to access it before execution we have
  // to manually set it  up
  vtkmdiy::mpi::environment env(argc, argv);
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  decltype(generator)::result_type seed = 0;
  if (comm.rank() == 0)
  {
    seed = static_cast<decltype(seed)>(std::time(nullptr));
    std::cout << "using seed: " << seed << "\n";
  }
  vtkmdiy::mpi::broadcast(comm, seed, 0);
  generator.seed(seed);

  return vtkm::cont::testing::Testing::Run(TestArrayHandleSerialization, argc, argv);
}
