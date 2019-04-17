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
