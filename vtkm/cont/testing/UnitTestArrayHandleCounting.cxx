//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/cont/testing/Testing.h>

#include <string>

namespace UnitTestArrayHandleCountingNamespace
{

const vtkm::Id ARRAY_SIZE = 10;

// An unusual data type that represents a number with a string of a
// particular length. This makes sure that the ArrayHandleCounting
// works correctly with type casts.
class StringInt
{
public:
  StringInt() {}
  StringInt(vtkm::Id v)
  {
    VTKM_ASSERT(v >= 0);
    for (vtkm::Id i = 0; i < v; i++)
    {
      ++(*this);
    }
  }

  operator vtkm::Id() const { return vtkm::Id(this->Value.size()); }

  StringInt operator+(const StringInt& rhs) const { return StringInt(this->Value + rhs.Value); }

  StringInt operator*(const StringInt& rhs) const
  {
    StringInt result;
    for (vtkm::Id i = 0; i < rhs; i++)
    {
      result = result + *this;
    }
    return result;
  }

  bool operator==(const StringInt& other) const { return this->Value.size() == other.Value.size(); }

  StringInt& operator++()
  {
    this->Value.append(".");
    return *this;
  }

private:
  StringInt(const std::string& v)
    : Value(v)
  {
  }

  std::string Value;
};

} // namespace UnitTestArrayHandleCountingNamespace

VTKM_BASIC_TYPE_VECTOR(UnitTestArrayHandleCountingNamespace::StringInt)

namespace UnitTestArrayHandleCountingNamespace
{

template <typename ValueType>
struct TemplatedTests
{
  using ArrayHandleType = vtkm::cont::ArrayHandleCounting<ValueType>;

  using ArrayHandleType2 = vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagCounting>;

  using PortalType = typename ArrayHandleType::PortalConstControl;

  void operator()(const ValueType& startingValue, const ValueType& step)
  {
    ArrayHandleType arrayConst(startingValue, step, ARRAY_SIZE);

    ArrayHandleType arrayMake =
      vtkm::cont::make_ArrayHandleCounting(startingValue, step, ARRAY_SIZE);

    ArrayHandleType2 arrayHandle = ArrayHandleType2(PortalType(startingValue, step, ARRAY_SIZE));

    VTKM_TEST_ASSERT(arrayConst.GetNumberOfValues() == ARRAY_SIZE,
                     "Counting array using constructor has wrong size.");

    VTKM_TEST_ASSERT(arrayMake.GetNumberOfValues() == ARRAY_SIZE,
                     "Counting array using make has wrong size.");

    VTKM_TEST_ASSERT(arrayHandle.GetNumberOfValues() == ARRAY_SIZE,
                     "Counting array using raw array handle + tag has wrong size.");

    ValueType properValue = startingValue;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      VTKM_TEST_ASSERT(arrayConst.GetPortalConstControl().Get(index) == properValue,
                       "Counting array using constructor has unexpected value.");
      VTKM_TEST_ASSERT(arrayMake.GetPortalConstControl().Get(index) == properValue,
                       "Counting array using make has unexpected value.");

      VTKM_TEST_ASSERT(arrayHandle.GetPortalConstControl().Get(index) == properValue,
                       "Counting array using raw array handle + tag has unexpected value.");
      properValue = properValue + step;
    }
  }
};

void TestArrayHandleCounting()
{
  TemplatedTests<vtkm::Id>()(0, 1);
  TemplatedTests<vtkm::Id>()(8, 2);
  TemplatedTests<vtkm::Float32>()(0.0f, 1.0f);
  TemplatedTests<vtkm::Float32>()(3.0f, -0.5f);
  TemplatedTests<vtkm::Float64>()(0.0, 1.0);
  TemplatedTests<vtkm::Float64>()(-3.0, 2.0);
  TemplatedTests<StringInt>()(StringInt(0), StringInt(1));
  TemplatedTests<StringInt>()(StringInt(10), StringInt(2));
}

} // namespace UnitTestArrayHandleCountingNamespace

int UnitTestArrayHandleCounting(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleCountingNamespace;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleCounting, argc, argv);
}
