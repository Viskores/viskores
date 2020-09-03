//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/Bitset.h>
#include <vtkm/Bounds.h>
#include <vtkm/Pair.h>
#include <vtkm/Range.h>

#include <vtkm/TypeList.h>
#include <vtkm/cont/testing/Testing.h>

#include <type_traits>

namespace
{

// clang-format off
template<typename T>
void is_noexcept_movable()
{
  constexpr bool valid = std::is_nothrow_move_constructible<T>::value &&
                         std::is_nothrow_move_assignable<T>::value;

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  VTKM_TEST_ASSERT(valid, msg);
}

// DataSet uses a map which is not nothrow constructible/assignable in the
// following implementations
template<>
void is_noexcept_movable<vtkm::cont::DataSet>()
{
  using T = vtkm::cont::DataSet;

  constexpr bool valid =
#if ((defined(__GNUC__) && (__GNUC__ <= 5)) || defined(_WIN32))
    std::is_move_constructible<T>::value &&
    std::is_move_assignable<T>::value;
#else
    std::is_nothrow_move_constructible<T>::value &&
    std::is_nothrow_move_assignable<T>::value;
#endif

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  VTKM_TEST_ASSERT(valid, msg);
}

template<typename T>
void is_triv_noexcept_movable()
{
  constexpr bool valid =
#if !(defined(__GNUC__) && (__GNUC__ <= 5))
                         //GCC 4.X and compilers that act like it such as Intel 17.0
                         //don't have implementations for is_trivially_*
                         std::is_trivially_move_constructible<T>::value &&
                         std::is_trivially_move_assignable<T>::value &&
#endif
                         std::is_nothrow_move_constructible<T>::value &&
                         std::is_nothrow_move_assignable<T>::value &&
                         std::is_nothrow_constructible<T, T&&>::value;

  std::string msg = typeid(T).name() + std::string(" should be noexcept moveable");
  VTKM_TEST_ASSERT(valid, msg);
}
// clang-format on

struct IsTrivNoExcept
{
  template <typename T>
  void operator()(T) const
  {
    is_triv_noexcept_movable<T>();
  }
};

struct IsNoExceptHandle
{
  template <typename T>
  void operator()(T) const
  {
    using HandleType = vtkm::cont::ArrayHandle<T>;
    using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<HandleType>;

    //verify the handle type
    is_noexcept_movable<HandleType>();
    is_noexcept_movable<MultiplexerType>();

    //verify the input portals of the handle
    is_noexcept_movable<decltype(std::declval<HandleType>().PrepareForInput(
      vtkm::cont::DeviceAdapterTagSerial{}, std::declval<vtkm::cont::Token&>()))>();
    is_noexcept_movable<decltype(std::declval<MultiplexerType>().PrepareForInput(
      vtkm::cont::DeviceAdapterTagSerial{}, std::declval<vtkm::cont::Token&>()))>();

    //verify the output portals of the handle
    is_noexcept_movable<decltype(std::declval<HandleType>().PrepareForOutput(
      2, vtkm::cont::DeviceAdapterTagSerial{}, std::declval<vtkm::cont::Token&>()))>();
    is_noexcept_movable<decltype(std::declval<MultiplexerType>().PrepareForOutput(
      2, vtkm::cont::DeviceAdapterTagSerial{}, std::declval<vtkm::cont::Token&>()))>();
  }
};

using vtkmComplexCustomTypes = vtkm::List<vtkm::Vec<vtkm::Vec<float, 3>, 3>,
                                          vtkm::Pair<vtkm::UInt64, vtkm::UInt64>,
                                          vtkm::Bitset<vtkm::UInt64>,
                                          vtkm::Bounds,
                                          vtkm::Range>;
}

//-----------------------------------------------------------------------------
void TestContDataTypesHaveMoveSemantics()
{
  //verify the Vec types are triv and noexcept
  vtkm::testing::Testing::TryTypes(IsTrivNoExcept{}, vtkm::TypeListVecCommon{});
  //verify that vtkm::Pair, Bitset, Bounds, and Range are triv and noexcept
  vtkm::testing::Testing::TryTypes(IsTrivNoExcept{}, vtkmComplexCustomTypes{});


  //verify that ArrayHandles and related portals are noexcept movable
  //allowing for efficient storage in containers such as std::vector
  vtkm::testing::Testing::TryTypes(IsNoExceptHandle{}, vtkm::TypeListAll{});

  vtkm::testing::Testing::TryTypes(IsNoExceptHandle{}, ::vtkmComplexCustomTypes{});

  //verify the DataSet, Field, and CoordinateSystem
  //all have efficient storage in containers such as std::vector
  is_noexcept_movable<vtkm::cont::DataSet>();
  is_noexcept_movable<vtkm::cont::Field>();
  is_noexcept_movable<vtkm::cont::CoordinateSystem>();

  //verify the CellSetStructured, and CellSetExplicit
  //have efficient storage in containers such as std::vector
  is_noexcept_movable<vtkm::cont::CellSetStructured<2>>();
  is_noexcept_movable<vtkm::cont::CellSetStructured<3>>();
  is_noexcept_movable<vtkm::cont::CellSetExplicit<>>();
}


//-----------------------------------------------------------------------------
int UnitTestMoveConstructors(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContDataTypesHaveMoveSemantics, argc, argv);
}
