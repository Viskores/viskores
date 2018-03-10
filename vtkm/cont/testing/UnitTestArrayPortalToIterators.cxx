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

#include <vtkm/cont/ArrayPortalToIterators.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TemplatedTests
{
  static constexpr vtkm::Id ARRAY_SIZE = 10;

  using ValueType = T;
  using ComponentType = typename vtkm::VecTraits<ValueType>::ComponentType;

  static ValueType ExpectedValue(vtkm::Id index, ComponentType value)
  {
    return ValueType(static_cast<ComponentType>(index + static_cast<vtkm::Id>(value)));
  }

  class ReadOnlyArrayPortal
  {
  public:
    using ValueType = T;

    VTKM_CONT
    ReadOnlyArrayPortal(ComponentType value)
      : Value(value)
    {
    }

    VTKM_CONT
    vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

    VTKM_CONT
    ValueType Get(vtkm::Id index) const { return ExpectedValue(index, this->Value); }

  private:
    ComponentType Value;
  };

  class WriteOnlyArrayPortal
  {
  public:
    using ValueType = T;

    VTKM_CONT
    WriteOnlyArrayPortal(ComponentType value)
      : Value(value)
    {
    }

    VTKM_CONT
    vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

    VTKM_CONT
    void Set(vtkm::Id index, const ValueType& value) const
    {
      VTKM_TEST_ASSERT(value == ExpectedValue(index, this->Value),
                       "Set unexpected value in array portal.");
    }

  private:
    ComponentType Value;
  };

  template <class IteratorType>
  void FillIterator(IteratorType begin, IteratorType end, ComponentType value)
  {
    std::cout << "    Check distance" << std::endl;
    VTKM_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                     "Distance between begin and end incorrect.");

    std::cout << "    Write expected value in iterator." << std::endl;
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
    std::cout << "    Check distance" << std::endl;
    VTKM_TEST_ASSERT(std::distance(begin, end) == ARRAY_SIZE,
                     "Distance between begin and end incorrect.");

    std::cout << "    Read expected value from iterator." << std::endl;
    vtkm::Id index = 0;
    for (IteratorType iter = begin; iter != end; iter++)
    {
      VTKM_TEST_ASSERT(ValueType(*iter) == ExpectedValue(index, value),
                       "Got bad value from iterator.");
      index++;
    }
    return true;
  }

  void TestIteratorRead()
  {
    using ArrayPortalType = ReadOnlyArrayPortal;
    using GetIteratorsType = vtkm::cont::ArrayPortalToIterators<ArrayPortalType>;

    static const ComponentType READ_VALUE = 23;
    ArrayPortalType portal(READ_VALUE);

    std::cout << "  Testing read-only iterators with ArrayPortalToIterators." << std::endl;
    GetIteratorsType iterators(portal);
    CheckIterator(iterators.GetBegin(), iterators.GetEnd(), READ_VALUE);

    std::cout << "  Testing read-only iterators with convenience functions." << std::endl;
    CheckIterator(vtkm::cont::ArrayPortalToIteratorBegin(portal),
                  vtkm::cont::ArrayPortalToIteratorEnd(portal),
                  READ_VALUE);
  }

  void TestIteratorWrite()
  {
    using ArrayPortalType = WriteOnlyArrayPortal;
    using GetIteratorsType = vtkm::cont::ArrayPortalToIterators<ArrayPortalType>;

    static const ComponentType WRITE_VALUE = 63;
    ArrayPortalType portal(WRITE_VALUE);

    std::cout << "  Testing write-only iterators with ArrayPortalToIterators." << std::endl;
    GetIteratorsType iterators(portal);
    FillIterator(iterators.GetBegin(), iterators.GetEnd(), WRITE_VALUE);

    std::cout << "  Testing write-only iterators with convenience functions." << std::endl;
    FillIterator(vtkm::cont::ArrayPortalToIteratorBegin(portal),
                 vtkm::cont::ArrayPortalToIteratorEnd(portal),
                 WRITE_VALUE);
  }

  void operator()()
  {
    TestIteratorRead();
    TestIteratorWrite();
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

void TestArrayPortalToIterators()
{
  vtkm::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestArrayPortalToIterators(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestArrayPortalToIterators);
}
