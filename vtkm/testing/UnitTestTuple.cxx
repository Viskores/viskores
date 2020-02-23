//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Tuple.h>

#include <vtkm/testing/Testing.h>

namespace
{

template <vtkm::IdComponent Index>
struct TypePlaceholder
{
  vtkm::Id X;
  TypePlaceholder(vtkm::Id x)
    : X(x)
  {
  }
};

void Check2(TypePlaceholder<0> a0, TypePlaceholder<1> a1)
{
  VTKM_TEST_ASSERT(a0.X == TestValue(0, vtkm::Id{}));
  VTKM_TEST_ASSERT(a1.X == TestValue(1, vtkm::Id{}));
}

void Check22(TypePlaceholder<0> a0,
             TypePlaceholder<1> a1,
             TypePlaceholder<2> a2,
             TypePlaceholder<3> a3,
             TypePlaceholder<4> a4,
             TypePlaceholder<5> a5,
             TypePlaceholder<6> a6,
             TypePlaceholder<7> a7,
             TypePlaceholder<8> a8,
             TypePlaceholder<9> a9,
             TypePlaceholder<10> a10,
             TypePlaceholder<11> a11,
             TypePlaceholder<12> a12,
             TypePlaceholder<13> a13,
             TypePlaceholder<14> a14,
             TypePlaceholder<15> a15,
             TypePlaceholder<16> a16,
             TypePlaceholder<17> a17,
             TypePlaceholder<18> a18,
             TypePlaceholder<19> a19,
             TypePlaceholder<20> a20,
             TypePlaceholder<21> a21)
{
  VTKM_TEST_ASSERT(a0.X == TestValue(0, vtkm::Id{}));
  VTKM_TEST_ASSERT(a1.X == TestValue(1, vtkm::Id{}));
  VTKM_TEST_ASSERT(a2.X == TestValue(2, vtkm::Id{}));
  VTKM_TEST_ASSERT(a3.X == TestValue(3, vtkm::Id{}));
  VTKM_TEST_ASSERT(a4.X == TestValue(4, vtkm::Id{}));
  VTKM_TEST_ASSERT(a5.X == TestValue(5, vtkm::Id{}));
  VTKM_TEST_ASSERT(a6.X == TestValue(6, vtkm::Id{}));
  VTKM_TEST_ASSERT(a7.X == TestValue(7, vtkm::Id{}));
  VTKM_TEST_ASSERT(a8.X == TestValue(8, vtkm::Id{}));
  VTKM_TEST_ASSERT(a9.X == TestValue(9, vtkm::Id{}));
  VTKM_TEST_ASSERT(a10.X == TestValue(10, vtkm::Id{}));
  VTKM_TEST_ASSERT(a11.X == TestValue(11, vtkm::Id{}));
  VTKM_TEST_ASSERT(a12.X == TestValue(12, vtkm::Id{}));
  VTKM_TEST_ASSERT(a13.X == TestValue(13, vtkm::Id{}));
  VTKM_TEST_ASSERT(a14.X == TestValue(14, vtkm::Id{}));
  VTKM_TEST_ASSERT(a15.X == TestValue(15, vtkm::Id{}));
  VTKM_TEST_ASSERT(a16.X == TestValue(16, vtkm::Id{}));
  VTKM_TEST_ASSERT(a17.X == TestValue(17, vtkm::Id{}));
  VTKM_TEST_ASSERT(a18.X == TestValue(18, vtkm::Id{}));
  VTKM_TEST_ASSERT(a19.X == TestValue(19, vtkm::Id{}));
  VTKM_TEST_ASSERT(a20.X == TestValue(20, vtkm::Id{}));
  VTKM_TEST_ASSERT(a21.X == TestValue(21, vtkm::Id{}));
}

struct CheckReturn
{
  template <typename Function, typename... Ts>
  vtkm::Id operator()(Function f, Ts... args)
  {
    f(args...);
    return vtkm::Id(sizeof...(Ts));
  }
};

struct CheckValues
{
  vtkm::IdComponent NumChecked = 0;

  template <vtkm::IdComponent Index>
  void operator()(TypePlaceholder<Index> x)
  {
    VTKM_TEST_ASSERT(x.X == TestValue(Index, vtkm::Id{}));
    this->NumChecked++;
  }
};

struct TransformValues
{
  vtkm::Id AddValue;
  TransformValues(vtkm::Id addValue)
    : AddValue(addValue)
  {
  }

  template <vtkm::IdComponent Index>
  vtkm::Id operator()(TypePlaceholder<Index> x) const
  {
    return x.X + this->AddValue;
  }
};

void TestTuple2()
{
  using TupleType = vtkm::Tuple<TypePlaceholder<0>, TypePlaceholder<1>>;

  VTKM_STATIC_ASSERT(vtkm::TupleSize<TupleType>::value == 2);
  VTKM_STATIC_ASSERT((std::is_same<TypePlaceholder<0>, vtkm::TupleElement<0, TupleType>>::value));
  VTKM_STATIC_ASSERT((std::is_same<TypePlaceholder<1>, vtkm::TupleElement<1, TupleType>>::value));

  TupleType tuple(TestValue(0, vtkm::Id()), TestValue(1, vtkm::Id()));

  tuple.Apply(Check2);

  vtkm::Id result = tuple.Apply(CheckReturn{}, Check2);
  VTKM_TEST_ASSERT(result == 2);

  CheckValues checkFunctor;
  VTKM_TEST_ASSERT(checkFunctor.NumChecked == 0);
  tuple.ForEach(checkFunctor);
  VTKM_TEST_ASSERT(checkFunctor.NumChecked == 2);

  auto transformedTuple = tuple.Transform(TransformValues{ 10 });
  using TransformedTupleType = decltype(transformedTuple);
  VTKM_STATIC_ASSERT((std::is_same<vtkm::TupleElement<0, TransformedTupleType>, vtkm::Id>::value));
  VTKM_STATIC_ASSERT((std::is_same<vtkm::TupleElement<1, TransformedTupleType>, vtkm::Id>::value));

  VTKM_TEST_ASSERT(vtkm::Get<0>(transformedTuple) == TestValue(0, vtkm::Id{}) + 10);
  VTKM_TEST_ASSERT(vtkm::Get<1>(transformedTuple) == TestValue(1, vtkm::Id{}) + 10);
}

void TestTuple22()
{
  using TupleType = vtkm::Tuple<TypePlaceholder<0>,
                                TypePlaceholder<1>,
                                TypePlaceholder<2>,
                                TypePlaceholder<3>,
                                TypePlaceholder<4>,
                                TypePlaceholder<5>,
                                TypePlaceholder<6>,
                                TypePlaceholder<7>,
                                TypePlaceholder<8>,
                                TypePlaceholder<9>,
                                TypePlaceholder<10>,
                                TypePlaceholder<11>,
                                TypePlaceholder<12>,
                                TypePlaceholder<13>,
                                TypePlaceholder<14>,
                                TypePlaceholder<15>,
                                TypePlaceholder<16>,
                                TypePlaceholder<17>,
                                TypePlaceholder<18>,
                                TypePlaceholder<19>,
                                TypePlaceholder<20>,
                                TypePlaceholder<21>>;

  VTKM_STATIC_ASSERT(vtkm::TupleSize<TupleType>::value == 22);
  VTKM_STATIC_ASSERT((std::is_same<TypePlaceholder<0>, vtkm::TupleElement<0, TupleType>>::value));
  VTKM_STATIC_ASSERT((std::is_same<TypePlaceholder<1>, vtkm::TupleElement<1, TupleType>>::value));
  VTKM_STATIC_ASSERT((std::is_same<TypePlaceholder<20>, vtkm::TupleElement<20, TupleType>>::value));
  VTKM_STATIC_ASSERT((std::is_same<TypePlaceholder<21>, vtkm::TupleElement<21, TupleType>>::value));

  TupleType tuple(TestValue(0, vtkm::Id()),
                  TestValue(1, vtkm::Id()),
                  TestValue(2, vtkm::Id()),
                  TestValue(3, vtkm::Id()),
                  TestValue(4, vtkm::Id()),
                  TestValue(5, vtkm::Id()),
                  TestValue(6, vtkm::Id()),
                  TestValue(7, vtkm::Id()),
                  TestValue(8, vtkm::Id()),
                  TestValue(9, vtkm::Id()),
                  TestValue(10, vtkm::Id()),
                  TestValue(11, vtkm::Id()),
                  TestValue(12, vtkm::Id()),
                  TestValue(13, vtkm::Id()),
                  TestValue(14, vtkm::Id()),
                  TestValue(15, vtkm::Id()),
                  TestValue(16, vtkm::Id()),
                  TestValue(17, vtkm::Id()),
                  TestValue(18, vtkm::Id()),
                  TestValue(19, vtkm::Id()),
                  TestValue(20, vtkm::Id()),
                  TestValue(21, vtkm::Id()));

  tuple.Apply(Check22);

  vtkm::Id result = tuple.Apply(CheckReturn{}, Check22);
  VTKM_TEST_ASSERT(result == 22);

  CheckValues checkFunctor;
  VTKM_TEST_ASSERT(checkFunctor.NumChecked == 0);
  tuple.ForEach(checkFunctor);
  VTKM_TEST_ASSERT(checkFunctor.NumChecked == 22);

  auto transformedTuple = tuple.Transform(TransformValues{ 10 });
  using TransformedTupleType = decltype(transformedTuple);
  VTKM_STATIC_ASSERT((std::is_same<vtkm::TupleElement<0, TransformedTupleType>, vtkm::Id>::value));
  VTKM_STATIC_ASSERT((std::is_same<vtkm::TupleElement<1, TransformedTupleType>, vtkm::Id>::value));
  VTKM_STATIC_ASSERT((std::is_same<vtkm::TupleElement<20, TransformedTupleType>, vtkm::Id>::value));
  VTKM_STATIC_ASSERT((std::is_same<vtkm::TupleElement<21, TransformedTupleType>, vtkm::Id>::value));

  VTKM_TEST_ASSERT(vtkm::Get<0>(transformedTuple) == TestValue(0, vtkm::Id{}) + 10);
  VTKM_TEST_ASSERT(vtkm::Get<1>(transformedTuple) == TestValue(1, vtkm::Id{}) + 10);
  VTKM_TEST_ASSERT(vtkm::Get<20>(transformedTuple) == TestValue(20, vtkm::Id{}) + 10);
  VTKM_TEST_ASSERT(vtkm::Get<21>(transformedTuple) == TestValue(21, vtkm::Id{}) + 10);
}

void TestTuple()
{
  TestTuple2();
  TestTuple22();
}

} // anonymous namespace

int UnitTestTuple(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestTuple, argc, argv);
}
