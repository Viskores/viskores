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
//=============================================================================

#include <vtkm/Tuple.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

void Define()
{
  ////
  //// BEGIN-EXAMPLE DefineTuple
  ////
  vtkm::Tuple<vtkm::Id, vtkm::Vec3f, vtkm::cont::ArrayHandle<vtkm::Int32>> myTuple;
  ////
  //// END-EXAMPLE DefineTuple
  ////
  (void)myTuple;
}

void Init()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> array;

  ////
  //// BEGIN-EXAMPLE InitTuple
  ////
  // Initialize a tuple with 0, [0, 1, 2], and an existing ArrayHandle.
  vtkm::Tuple<vtkm::Id, vtkm::Vec3f, vtkm::cont::ArrayHandle<vtkm::Float32>> myTuple1(
    0, vtkm::Vec3f(0, 1, 2), array);

  // Another way to create the same tuple.
  auto myTuple2 = vtkm::MakeTuple(vtkm::Id(0), vtkm::Vec3f(0, 1, 2), array);
  ////
  //// END-EXAMPLE InitTuple
  ////

  VTKM_TEST_ASSERT(std::is_same<decltype(myTuple1), decltype(myTuple2)>::value);
  VTKM_TEST_ASSERT(vtkm::Get<0>(myTuple1) == 0);
  VTKM_TEST_ASSERT(vtkm::Get<0>(myTuple2) == 0);
  VTKM_TEST_ASSERT(test_equal(vtkm::Get<1>(myTuple1), vtkm::Vec3f(0, 1, 2)));
  VTKM_TEST_ASSERT(test_equal(vtkm::Get<1>(myTuple2), vtkm::Vec3f(0, 1, 2)));
}

void Query()
{
  ////
  //// BEGIN-EXAMPLE TupleQuery
  ////
  using TupleType = vtkm::Tuple<vtkm::Id, vtkm::Float32, vtkm::Float64>;

  // Becomes 3
  constexpr vtkm::IdComponent size = vtkm::TupleSize<TupleType>::value;

  using FirstType = vtkm::TupleElement<0, TupleType>;  // vtkm::Id
  using SecondType = vtkm::TupleElement<1, TupleType>; // vtkm::Float32
  using ThirdType = vtkm::TupleElement<2, TupleType>;  // vtkm::Float64
  ////
  //// END-EXAMPLE TupleQuery
  ////

  VTKM_TEST_ASSERT(size == 3);
  VTKM_TEST_ASSERT(std::is_same<FirstType, vtkm::Id>::value);
  VTKM_TEST_ASSERT(std::is_same<SecondType, vtkm::Float32>::value);
  VTKM_TEST_ASSERT(std::is_same<ThirdType, vtkm::Float64>::value);
}

void Get()
{
  ////
  //// BEGIN-EXAMPLE TupleGet
  ////
  auto myTuple = vtkm::MakeTuple(vtkm::Id3(0, 1, 2), vtkm::Vec3f(3, 4, 5));

  // Gets the value [0, 1, 2]
  vtkm::Id3 x = vtkm::Get<0>(myTuple);

  // Changes the second object in myTuple to [6, 7, 8]
  vtkm::Get<1>(myTuple) = vtkm::Vec3f(6, 7, 8);
  ////
  //// END-EXAMPLE TupleGet
  ////

  VTKM_TEST_ASSERT(x == vtkm::Id3(0, 1, 2));
  VTKM_TEST_ASSERT(test_equal(vtkm::Get<1>(myTuple), vtkm::Vec3f(6, 7, 8)));
}

vtkm::Int16 CreateValue(vtkm::Id index)
{
  return TestValue(index, vtkm::Int16{});
}

////
//// BEGIN-EXAMPLE TupleCheck
////
void CheckPositive(vtkm::Float64 x)
{
  if (x < 0)
  {
    throw vtkm::cont::ErrorBadValue("Values need to be positive.");
  }
}

// ...

//// PAUSE-EXAMPLE
void ForEachCheck()
{
  //// RESUME-EXAMPLE
  vtkm::Tuple<vtkm::Float64, vtkm::Float64, vtkm::Float64> tuple(
    CreateValue(0), CreateValue(1), CreateValue(2));

  // Will throw an error if any of the values are negative.
  vtkm::ForEach(tuple, CheckPositive);
  ////
  //// END-EXAMPLE TupleCheck
  ////
}

////
//// BEGIN-EXAMPLE TupleAggregate
////
struct SumFunctor
{
  vtkm::Float64 Sum = 0;

  template<typename T>
  void operator()(const T& x)
  {
    this->Sum = this->Sum + static_cast<vtkm::Float64>(x);
  }
};

// ...

//// PAUSE-EXAMPLE
void ForEachAggregate()
{
  //// RESUME-EXAMPLE
  vtkm::Tuple<vtkm::Float32, vtkm::Float64, vtkm::Id> tuple(
    CreateValue(0), CreateValue(1), CreateValue(2));

  SumFunctor sum;
  vtkm::ForEach(tuple, sum);
  vtkm::Float64 average = sum.Sum / 3;
  ////
  //// END-EXAMPLE TupleAggregate
  ////

  VTKM_TEST_ASSERT(test_equal(average, 101));
}

void ForEachAggregateLambda()
{
  ////
  //// BEGIN-EXAMPLE TupleAggregateLambda
  ////
  vtkm::Tuple<vtkm::Float32, vtkm::Float64, vtkm::Id> tuple(
    CreateValue(0), CreateValue(1), CreateValue(2));

  vtkm::Float64 sum = 0;
  auto sumFunctor = [&sum](auto x) { sum += static_cast<vtkm::Float64>(x); };

  vtkm::ForEach(tuple, sumFunctor);
  vtkm::Float64 average = sum / 3;
  ////
  //// END-EXAMPLE TupleAggregateLambda
  ////

  VTKM_TEST_ASSERT(test_equal(average, 101));
}

////
//// BEGIN-EXAMPLE TupleTransform
////
struct GetReadPortalFunctor
{
  template<typename Array>
  typename Array::ReadPortalType operator()(const Array& array) const
  {
    VTKM_IS_ARRAY_HANDLE(Array);
    return array.ReadPortal();
  }
};

// ...

//// PAUSE-EXAMPLE
void Transform()
{
  vtkm::cont::ArrayHandle<vtkm::Id> array1;
  array1.Allocate(ARRAY_SIZE);
  SetPortal(array1.WritePortal());

  vtkm::cont::ArrayHandle<vtkm::Float32> array2;
  array2.Allocate(ARRAY_SIZE);
  SetPortal(array2.WritePortal());

  vtkm::cont::ArrayHandle<vtkm::Vec3f> array3;
  array3.Allocate(ARRAY_SIZE);
  SetPortal(array3.WritePortal());

  //// RESUME-EXAMPLE
  auto arrayTuple = vtkm::MakeTuple(array1, array2, array3);

  auto portalTuple = vtkm::Transform(arrayTuple, GetReadPortalFunctor{});
  ////
  //// END-EXAMPLE TupleTransform
  ////

  CheckPortal(vtkm::Get<0>(portalTuple));
  CheckPortal(vtkm::Get<1>(portalTuple));
  CheckPortal(vtkm::Get<2>(portalTuple));
}

////
//// BEGIN-EXAMPLE TupleApply
////
struct AddArraysFunctor
{
  template<typename Array1, typename Array2, typename Array3>
  vtkm::Id operator()(Array1 inArray1, Array2 inArray2, Array3 outArray) const
  {
    VTKM_IS_ARRAY_HANDLE(Array1);
    VTKM_IS_ARRAY_HANDLE(Array2);
    VTKM_IS_ARRAY_HANDLE(Array3);

    vtkm::Id length = inArray1.GetNumberOfValues();
    VTKM_ASSERT(inArray2.GetNumberOfValues() == length);
    outArray.Allocate(length);

    auto inPortal1 = inArray1.ReadPortal();
    auto inPortal2 = inArray2.ReadPortal();
    auto outPortal = outArray.WritePortal();
    for (vtkm::Id index = 0; index < length; ++index)
    {
      outPortal.Set(index, inPortal1.Get(index) + inPortal2.Get(index));
    }

    return length;
  }
};

// ...

//// PAUSE-EXAMPLE
void Apply()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> array1;
  array1.Allocate(ARRAY_SIZE);
  SetPortal(array1.WritePortal());

  vtkm::cont::ArrayHandle<vtkm::Float32> array2;
  vtkm::cont::ArrayCopy(array1, array2);

  vtkm::cont::ArrayHandle<vtkm::Float32> array3;

  //// RESUME-EXAMPLE
  auto arrayTuple = vtkm::MakeTuple(array1, array2, array3);

  vtkm::Id arrayLength = vtkm::Apply(arrayTuple, AddArraysFunctor{});
  ////
  //// END-EXAMPLE TupleApply
  ////

  VTKM_TEST_ASSERT(arrayLength == ARRAY_SIZE);

  auto portal = array3.ReadPortal();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
  for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(i), 2 * TestValue(i, vtkm::Float32{})));
  }
}

////
//// BEGIN-EXAMPLE TupleApplyExtraArgs
////
struct ScanArrayLengthFunctor
{
  template<vtkm::IdComponent N, typename Array, typename... Remaining>
  vtkm::Vec<vtkm::Id, N + 1 + vtkm::IdComponent(sizeof...(Remaining))> operator()(
    const vtkm::Vec<vtkm::Id, N>& partialResult,
    const Array& nextArray,
    const Remaining&... remainingArrays) const
  {
    vtkm::Vec<vtkm::Id, N + 1> nextResult;
    std::copy(&partialResult[0], &partialResult[0] + N, &nextResult[0]);
    nextResult[N] = nextResult[N - 1] + nextArray.GetNumberOfValues();
    return (*this)(nextResult, remainingArrays...);
  }

  template<vtkm::IdComponent N>
  vtkm::Vec<vtkm::Id, N> operator()(const vtkm::Vec<vtkm::Id, N>& result) const
  {
    return result;
  }
};

// ...

//// PAUSE-EXAMPLE
void ApplyExtraArgs()
{
  vtkm::cont::ArrayHandle<vtkm::Id> array1;
  array1.Allocate(ARRAY_SIZE);

  vtkm::cont::ArrayHandle<vtkm::Id3> array2;
  array2.Allocate(ARRAY_SIZE);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> array3;
  array3.Allocate(ARRAY_SIZE);

  //// RESUME-EXAMPLE
  auto arrayTuple = vtkm::MakeTuple(array1, array2, array3);

  vtkm::Vec<vtkm::Id, 4> sizeScan =
    vtkm::Apply(arrayTuple, ScanArrayLengthFunctor{}, vtkm::Vec<vtkm::Id, 1>{ 0 });
  ////
  //// END-EXAMPLE TupleApplyExtraArgs
  ////

  VTKM_TEST_ASSERT(sizeScan[0] == 0 * ARRAY_SIZE);
  VTKM_TEST_ASSERT(sizeScan[1] == 1 * ARRAY_SIZE);
  VTKM_TEST_ASSERT(sizeScan[2] == 2 * ARRAY_SIZE);
  VTKM_TEST_ASSERT(sizeScan[3] == 3 * ARRAY_SIZE);
}

void Run()
{
  Define();
  Init();
  Query();
  Get();
  ForEachCheck();
  ForEachAggregate();
  ForEachAggregateLambda();
  Transform();
  Apply();
  ApplyExtraArgs();
}

} // anonymous namespace

int GuideExampleTuple(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
