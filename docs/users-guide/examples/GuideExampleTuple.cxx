//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/Tuple.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 10;

void Define()
{
  ////
  //// BEGIN-EXAMPLE DefineTuple
  ////
  viskores::
    Tuple<viskores::Id, viskores::Vec3f, viskores::cont::ArrayHandle<viskores::Int32>>
      myTuple;
  ////
  //// END-EXAMPLE DefineTuple
  ////
  (void)myTuple;
}

void Init()
{
  viskores::cont::ArrayHandle<viskores::Float32> array;

  ////
  //// BEGIN-EXAMPLE InitTuple
  ////
  // Initialize a tuple with 0, [0, 1, 2], and an existing ArrayHandle.
  viskores::
    Tuple<viskores::Id, viskores::Vec3f, viskores::cont::ArrayHandle<viskores::Float32>>
      myTuple1(0, viskores::Vec3f(0, 1, 2), array);

  // Another way to create the same tuple.
  auto myTuple2 = viskores::MakeTuple(viskores::Id(0), viskores::Vec3f(0, 1, 2), array);
  ////
  //// END-EXAMPLE InitTuple
  ////

  VISKORES_TEST_ASSERT(std::is_same<decltype(myTuple1), decltype(myTuple2)>::value);
  VISKORES_TEST_ASSERT(viskores::Get<0>(myTuple1) == 0);
  VISKORES_TEST_ASSERT(viskores::Get<0>(myTuple2) == 0);
  VISKORES_TEST_ASSERT(test_equal(viskores::Get<1>(myTuple1), viskores::Vec3f(0, 1, 2)));
  VISKORES_TEST_ASSERT(test_equal(viskores::Get<1>(myTuple2), viskores::Vec3f(0, 1, 2)));
}

void Query()
{
  ////
  //// BEGIN-EXAMPLE TupleQuery
  ////
  using TupleType = viskores::Tuple<viskores::Id, viskores::Float32, viskores::Float64>;

  // Becomes 3
  constexpr viskores::IdComponent size = viskores::TupleSize<TupleType>::value;

  using FirstType = viskores::TupleElement<0, TupleType>;  // viskores::Id
  using SecondType = viskores::TupleElement<1, TupleType>; // viskores::Float32
  using ThirdType = viskores::TupleElement<2, TupleType>;  // viskores::Float64
  ////
  //// END-EXAMPLE TupleQuery
  ////

  VISKORES_TEST_ASSERT(size == 3);
  VISKORES_TEST_ASSERT(std::is_same<FirstType, viskores::Id>::value);
  VISKORES_TEST_ASSERT(std::is_same<SecondType, viskores::Float32>::value);
  VISKORES_TEST_ASSERT(std::is_same<ThirdType, viskores::Float64>::value);
}

void Get()
{
  ////
  //// BEGIN-EXAMPLE TupleGet
  ////
  auto myTuple = viskores::MakeTuple(viskores::Id3(0, 1, 2), viskores::Vec3f(3, 4, 5));

  // Gets the value [0, 1, 2]
  viskores::Id3 x = viskores::Get<0>(myTuple);

  // Changes the second object in myTuple to [6, 7, 8]
  viskores::Get<1>(myTuple) = viskores::Vec3f(6, 7, 8);
  ////
  //// END-EXAMPLE TupleGet
  ////

  VISKORES_TEST_ASSERT(x == viskores::Id3(0, 1, 2));
  VISKORES_TEST_ASSERT(test_equal(viskores::Get<1>(myTuple), viskores::Vec3f(6, 7, 8)));
}

viskores::Int16 CreateValue(viskores::Id index)
{
  return TestValue(index, viskores::Int16{});
}

////
//// BEGIN-EXAMPLE TupleCheck
////
void CheckPositive(viskores::Float64 x)
{
  if (x < 0)
  {
    throw viskores::cont::ErrorBadValue("Values need to be positive.");
  }
}

// ...

//// PAUSE-EXAMPLE
void ForEachCheck()
{
  //// RESUME-EXAMPLE
  viskores::Tuple<viskores::Float64, viskores::Float64, viskores::Float64> tuple(
    CreateValue(0), CreateValue(1), CreateValue(2));

  // Will throw an error if any of the values are negative.
  viskores::ForEach(tuple, CheckPositive);
  ////
  //// END-EXAMPLE TupleCheck
  ////
}

////
//// BEGIN-EXAMPLE TupleAggregate
////
struct SumFunctor
{
  viskores::Float64 Sum = 0;

  template<typename T>
  void operator()(const T& x)
  {
    this->Sum = this->Sum + static_cast<viskores::Float64>(x);
  }
};

// ...

//// PAUSE-EXAMPLE
void ForEachAggregate()
{
  //// RESUME-EXAMPLE
  viskores::Tuple<viskores::Float32, viskores::Float64, viskores::Id> tuple(
    CreateValue(0), CreateValue(1), CreateValue(2));

  SumFunctor sum;
  viskores::ForEach(tuple, sum);
  viskores::Float64 average = sum.Sum / 3;
  ////
  //// END-EXAMPLE TupleAggregate
  ////

  VISKORES_TEST_ASSERT(test_equal(average, 101));
}

void ForEachAggregateLambda()
{
  ////
  //// BEGIN-EXAMPLE TupleAggregateLambda
  ////
  viskores::Tuple<viskores::Float32, viskores::Float64, viskores::Id> tuple(
    CreateValue(0), CreateValue(1), CreateValue(2));

  viskores::Float64 sum = 0;
  auto sumFunctor = [&sum](auto x) { sum += static_cast<viskores::Float64>(x); };

  viskores::ForEach(tuple, sumFunctor);
  viskores::Float64 average = sum / 3;
  ////
  //// END-EXAMPLE TupleAggregateLambda
  ////

  VISKORES_TEST_ASSERT(test_equal(average, 101));
}

////
//// BEGIN-EXAMPLE TupleTransform
////
struct GetReadPortalFunctor
{
  template<typename Array>
  typename Array::ReadPortalType operator()(const Array& array) const
  {
    VISKORES_IS_ARRAY_HANDLE(Array);
    return array.ReadPortal();
  }
};

// ...

//// PAUSE-EXAMPLE
void Transform()
{
  viskores::cont::ArrayHandle<viskores::Id> array1;
  array1.Allocate(ARRAY_SIZE);
  SetPortal(array1.WritePortal());

  viskores::cont::ArrayHandle<viskores::Float32> array2;
  array2.Allocate(ARRAY_SIZE);
  SetPortal(array2.WritePortal());

  viskores::cont::ArrayHandle<viskores::Vec3f> array3;
  array3.Allocate(ARRAY_SIZE);
  SetPortal(array3.WritePortal());

  //// RESUME-EXAMPLE
  auto arrayTuple = viskores::MakeTuple(array1, array2, array3);

  auto portalTuple = viskores::Transform(arrayTuple, GetReadPortalFunctor{});
  ////
  //// END-EXAMPLE TupleTransform
  ////

  CheckPortal(viskores::Get<0>(portalTuple));
  CheckPortal(viskores::Get<1>(portalTuple));
  CheckPortal(viskores::Get<2>(portalTuple));
}

////
//// BEGIN-EXAMPLE TupleApply
////
struct AddArraysFunctor
{
  template<typename Array1, typename Array2, typename Array3>
  viskores::Id operator()(Array1 inArray1, Array2 inArray2, Array3 outArray) const
  {
    VISKORES_IS_ARRAY_HANDLE(Array1);
    VISKORES_IS_ARRAY_HANDLE(Array2);
    VISKORES_IS_ARRAY_HANDLE(Array3);

    viskores::Id length = inArray1.GetNumberOfValues();
    VISKORES_ASSERT(inArray2.GetNumberOfValues() == length);
    outArray.Allocate(length);

    auto inPortal1 = inArray1.ReadPortal();
    auto inPortal2 = inArray2.ReadPortal();
    auto outPortal = outArray.WritePortal();
    for (viskores::Id index = 0; index < length; ++index)
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
  viskores::cont::ArrayHandle<viskores::Float32> array1;
  array1.Allocate(ARRAY_SIZE);
  SetPortal(array1.WritePortal());

  viskores::cont::ArrayHandle<viskores::Float32> array2;
  viskores::cont::ArrayCopy(array1, array2);

  viskores::cont::ArrayHandle<viskores::Float32> array3;

  //// RESUME-EXAMPLE
  auto arrayTuple = viskores::MakeTuple(array1, array2, array3);

  viskores::Id arrayLength = viskores::Apply(arrayTuple, AddArraysFunctor{});
  ////
  //// END-EXAMPLE TupleApply
  ////

  VISKORES_TEST_ASSERT(arrayLength == ARRAY_SIZE);

  auto portal = array3.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
  for (viskores::Id i = 0; i < ARRAY_SIZE; ++i)
  {
    VISKORES_TEST_ASSERT(
      test_equal(portal.Get(i), 2 * TestValue(i, viskores::Float32{})));
  }
}

////
//// BEGIN-EXAMPLE TupleApplyExtraArgs
////
struct ScanArrayLengthFunctor
{
  template<viskores::IdComponent N, typename Array, typename... Remaining>
  viskores::Vec<viskores::Id, N + 1 + viskores::IdComponent(sizeof...(Remaining))>
  operator()(const viskores::Vec<viskores::Id, N>& partialResult,
             const Array& nextArray,
             const Remaining&... remainingArrays) const
  {
    viskores::Vec<viskores::Id, N + 1> nextResult;
    std::copy(&partialResult[0], &partialResult[0] + N, &nextResult[0]);
    nextResult[N] = nextResult[N - 1] + nextArray.GetNumberOfValues();
    return (*this)(nextResult, remainingArrays...);
  }

  template<viskores::IdComponent N>
  viskores::Vec<viskores::Id, N> operator()(
    const viskores::Vec<viskores::Id, N>& result) const
  {
    return result;
  }
};

// ...

//// PAUSE-EXAMPLE
void ApplyExtraArgs()
{
  viskores::cont::ArrayHandle<viskores::Id> array1;
  array1.Allocate(ARRAY_SIZE);

  viskores::cont::ArrayHandle<viskores::Id3> array2;
  array2.Allocate(ARRAY_SIZE);

  viskores::cont::ArrayHandle<viskores::Vec3f> array3;
  array3.Allocate(ARRAY_SIZE);

  //// RESUME-EXAMPLE
  auto arrayTuple = viskores::MakeTuple(array1, array2, array3);

  viskores::Vec<viskores::Id, 4> sizeScan = viskores::Apply(
    arrayTuple, ScanArrayLengthFunctor{}, viskores::Vec<viskores::Id, 1>{ 0 });
  ////
  //// END-EXAMPLE TupleApplyExtraArgs
  ////

  VISKORES_TEST_ASSERT(sizeScan[0] == 0 * ARRAY_SIZE);
  VISKORES_TEST_ASSERT(sizeScan[1] == 1 * ARRAY_SIZE);
  VISKORES_TEST_ASSERT(sizeScan[2] == 2 * ARRAY_SIZE);
  VISKORES_TEST_ASSERT(sizeScan[3] == 3 * ARRAY_SIZE);
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
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
