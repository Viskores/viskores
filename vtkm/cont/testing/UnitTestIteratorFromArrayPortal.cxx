//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TemplatedTests
{
  static constexpr vtkm::Id ARRAY_SIZE = 10;

  using ValueType = T;
  using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

  ValueType ExpectedValue(vtkm::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<vtkm::Id>(value)));
  }

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    vtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      *iter = ExpectedValue(index, value);
      index++;
    }
  }

  template <class IteratorType>
  bool CheckIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    vtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      if (ValueType(*iter) != ExpectedValue(index, value))
      {
        return false;
      }
      index++;
    }
    return true;
  }

  template <class PortalType>
  bool CheckPortal(const PortalType& portal, const ComponentType& value)
  {
    vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
    return CheckIterator(iterators.GetBegin(), iterators.GetEnd(), value);
  }

  ComponentType ORIGINAL_VALUE() { return 39; }

  template <class ArrayPortalType>
  void TestIteratorRead(ArrayPortalType portal)
  {
    using IteratorType = vtkm::cont::internal::IteratorFromArrayPortal<ArrayPortalType>;

    IteratorType begin = vtkm::cont::internal::make_IteratorBegin(portal);
    IteratorType end = vtkm::cont::internal::make_IteratorEnd(portal);
    VTKM_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                     "Distance between begin and end incorrect.");
    VTKM_TEST_ASSERT(std::distance(end, begin) == -ARRAY_SIZE,
                     "Distance between begin and end incorrect.");

    std::cout << "    Check forward iteration." << std::endl;
    VTKM_TEST_ASSERT(CheckIterator(begin, end, ORIGINAL_VALUE()), "Forward iteration wrong");

    std::cout << "    Check backward iteration." << std::endl;
    IteratorType middle = end;
    for (vtkm::Id index = portal.GetNumberOfValues() - 1; index >= 0; index--)
    {
      middle--;
      ValueType value = *middle;
      VTKM_TEST_ASSERT(value == ExpectedValue(index, ORIGINAL_VALUE()), "Backward iteration wrong");
    }

    std::cout << "    Check advance" << std::endl;
    middle = begin + ARRAY_SIZE / 2;
    VTKM_TEST_ASSERT(std::distance(begin, middle) == ARRAY_SIZE / 2, "Bad distance to middle.");
    VTKM_TEST_ASSERT(ValueType(*middle) == ExpectedValue(ARRAY_SIZE / 2, ORIGINAL_VALUE()),
                     "Bad value at middle.");
  }

  template <class ArrayPortalType>
  void TestIteratorWrite(ArrayPortalType portal)
  {
    using IteratorType = vtkm::cont::internal::IteratorFromArrayPortal<ArrayPortalType>;

    IteratorType begin = vtkm::cont::internal::make_IteratorBegin(portal);
    IteratorType end = vtkm::cont::internal::make_IteratorEnd(portal);

    static const ComponentType WRITE_VALUE = 73;

    std::cout << "    Write values to iterator." << std::endl;
    FillIterator(begin, end, WRITE_VALUE);

    std::cout << "    Check values in portal." << std::endl;
    VTKM_TEST_ASSERT(CheckPortal(portal, WRITE_VALUE),
                     "Did not get correct values when writing to iterator.");
  }

  void TestOperators()
  {
    struct Functor
    {
      VTKM_EXEC ValueType operator()(vtkm::Id index) const { return TestValue(index, ValueType{}); }
    };
    Functor functor;

    auto array = vtkm::cont::make_ArrayHandleImplicit(functor, ARRAY_SIZE);
    auto portal = array.ReadPortal();

    VTKM_TEST_ASSERT(test_equal(portal.Get(0), functor(0)));
    ::CheckPortal(portal);

    // Normally, you would use `ArrayPortalToIterators`, but we want to test this
    // class specifically.
    using IteratorType = vtkm::cont::internal::IteratorFromArrayPortal<decltype(portal)>;
    IteratorType begin{ portal };
    IteratorType end{ portal, ARRAY_SIZE };

    VTKM_TEST_ASSERT(test_equal(*begin, functor(0)));
    VTKM_TEST_ASSERT(test_equal(begin[0], functor(0)));
    VTKM_TEST_ASSERT(test_equal(begin[3], functor(3)));

    IteratorType iter = begin;
    VTKM_TEST_ASSERT(test_equal(*iter, functor(0)));
    VTKM_TEST_ASSERT(test_equal(*(iter++), functor(0)));
    VTKM_TEST_ASSERT(test_equal(*iter, functor(1)));
    VTKM_TEST_ASSERT(test_equal(*(++iter), functor(2)));
    VTKM_TEST_ASSERT(test_equal(*iter, functor(2)));

    VTKM_TEST_ASSERT(test_equal(*(iter--), functor(2)));
    VTKM_TEST_ASSERT(test_equal(*iter, functor(1)));
    VTKM_TEST_ASSERT(test_equal(*(--iter), functor(0)));
    VTKM_TEST_ASSERT(test_equal(*iter, functor(0)));

    VTKM_TEST_ASSERT(test_equal(*(iter += 3), functor(3)));
    VTKM_TEST_ASSERT(test_equal(*(iter -= 3), functor(0)));

    VTKM_TEST_ASSERT(end - begin == ARRAY_SIZE);

    VTKM_TEST_ASSERT(test_equal(*(iter + 3), functor(3)));
    VTKM_TEST_ASSERT(test_equal(*(3 + iter), functor(3)));
    iter += 3;
    VTKM_TEST_ASSERT(test_equal(*(iter - 3), functor(0)));

    VTKM_TEST_ASSERT(iter == (begin + 3));
    VTKM_TEST_ASSERT(!(iter != (begin + 3)));
    VTKM_TEST_ASSERT(iter != begin);
    VTKM_TEST_ASSERT(!(iter == begin));

    VTKM_TEST_ASSERT(!(iter < begin));
    VTKM_TEST_ASSERT(!(iter < (begin + 3)));
    VTKM_TEST_ASSERT((iter < end));

    VTKM_TEST_ASSERT(!(iter <= begin));
    VTKM_TEST_ASSERT((iter <= (begin + 3)));
    VTKM_TEST_ASSERT((iter <= end));

    VTKM_TEST_ASSERT((iter > begin));
    VTKM_TEST_ASSERT(!(iter > (begin + 3)));
    VTKM_TEST_ASSERT(!(iter > end));

    VTKM_TEST_ASSERT((iter >= begin));
    VTKM_TEST_ASSERT((iter >= (begin + 3)));
    VTKM_TEST_ASSERT(!(iter >= end));
  }

  void operator()()
  {
    ValueType array[ARRAY_SIZE];

    FillIterator(array, array + ARRAY_SIZE, ORIGINAL_VALUE());

    ::vtkm::cont::internal::ArrayPortalFromIterators<ValueType*> portal(array, array + ARRAY_SIZE);
    ::vtkm::cont::internal::ArrayPortalFromIterators<const ValueType*> const_portal(
      array, array + ARRAY_SIZE);

    std::cout << "  Test read from iterator." << std::endl;
    TestIteratorRead(portal);

    std::cout << "  Test read from const iterator." << std::endl;
    TestIteratorRead(const_portal);

    std::cout << "  Test write to iterator." << std::endl;
    TestIteratorWrite(portal);

    std::cout << "  Test operators." << std::endl;
    TestOperators();
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestArrayIteratorFromArrayPortal()
{
  vtkm::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestIteratorFromArrayPortal(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayIteratorFromArrayPortal, argc, argv);
}
