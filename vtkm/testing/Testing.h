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
#ifndef vtk_m_testing_Testing_h
#define vtk_m_testing_Testing_h

#include <vtkm/Bounds.h>
#include <vtkm/CellShape.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Pair.h>
#include <vtkm/Range.h>
#include <vtkm/TypeListTag.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include <math.h>

// Try to enforce using the correct testing version. (Those that include the
// control environment have more possible exceptions.) This is not guaranteed
// to work. To make it more likely, place the Testing.h include last.
#ifdef vtk_m_cont_Error_h
#ifndef vtk_m_cont_testing_Testing_h
#error Use vtkm::cont::testing::Testing instead of vtkm::testing::Testing.
#else
#define VTKM_TESTING_IN_CONT
#endif
#endif

/// \def VTKM_STRINGIFY_FIRST(...)
///
/// A utility macro that takes 1 or more arguments and converts it into the C string version
/// of the first argument.

#define VTKM_STRINGIFY_FIRST(...) VTKM_EXPAND(VTK_M_STRINGIFY_FIRST_IMPL(__VA_ARGS__, dummy))
#define VTK_M_STRINGIFY_FIRST_IMPL(first, ...) #first
#define VTKM_EXPAND(x) x

/// \def VTKM_TEST_ASSERT(condition, messages..)
///
/// Asserts a condition for a test to pass. A passing condition is when \a
/// condition resolves to true. If \a condition is false, then the test is
/// aborted and failure is returned. If one or more message arguments are
/// given, they are printed out by concatinating them. If no messages are
/// given, a generic message is given. In any case, the condition that failed
/// is written out.

#define VTKM_TEST_ASSERT(...)                                                                      \
  ::vtkm::testing::Testing::Assert(                                                                \
    VTKM_STRINGIFY_FIRST(__VA_ARGS__), __FILE__, __LINE__, __VA_ARGS__)

/// \def VTKM_TEST_FAIL(messages..)
///
/// Causes a test to fail with the given \a messages. At least one argument must be given.

#define VTKM_TEST_FAIL(...) ::vtkm::testing::Testing::TestFail(__FILE__, __LINE__, __VA_ARGS__)

namespace vtkm
{
namespace testing
{

// If you get an error about this class definition being incomplete, it means
// that you tried to get the name of a type that is not specified. You can
// either not use that type, not try to get the string name, or add it to the
// list.
template <typename T>
struct TypeName;

#define VTK_M_BASIC_TYPE(type)                                                                     \
  template <>                                                                                      \
  struct TypeName<type>                                                                            \
  {                                                                                                \
    static std::string Name() { return #type; }                                                    \
  }

VTK_M_BASIC_TYPE(vtkm::Float32);
VTK_M_BASIC_TYPE(vtkm::Float64);
VTK_M_BASIC_TYPE(vtkm::Int8);
VTK_M_BASIC_TYPE(vtkm::UInt8);
VTK_M_BASIC_TYPE(vtkm::Int16);
VTK_M_BASIC_TYPE(vtkm::UInt16);
VTK_M_BASIC_TYPE(vtkm::Int32);
VTK_M_BASIC_TYPE(vtkm::UInt32);
VTK_M_BASIC_TYPE(vtkm::Int64);
VTK_M_BASIC_TYPE(vtkm::UInt64);

#undef VTK_M_BASIC_TYPE

template <typename T, vtkm::IdComponent Size>
struct TypeName<vtkm::Vec<T, Size>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "vtkm::Vec< " << TypeName<T>::Name() << ", " << Size << " >";
    return stream.str();
  }
};

template <typename T, typename U>
struct TypeName<vtkm::Pair<T, U>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "vtkm::Pair< " << TypeName<T>::Name() << ", " << TypeName<U>::Name() << " >";
    return stream.str();
  }
};

namespace detail
{

template <vtkm::IdComponent cellShapeId>
struct InternalTryCellShape
{
  template <typename FunctionType>
  void operator()(const FunctionType& function) const
  {
    this->PrintAndInvoke(function, typename vtkm::CellShapeIdToTag<cellShapeId>::valid());
    InternalTryCellShape<cellShapeId + 1>()(function);
  }

private:
  template <typename FunctionType>
  void PrintAndInvoke(const FunctionType& function, std::true_type) const
  {
    using CellShapeTag = typename vtkm::CellShapeIdToTag<cellShapeId>::Tag;
    std::cout << "*** " << vtkm::GetCellShapeName(CellShapeTag()) << " ***************"
              << std::endl;
    function(CellShapeTag());
  }

  template <typename FunctionType>
  void PrintAndInvoke(const FunctionType&, std::false_type) const
  {
    // Not a valid cell shape. Do nothing.
  }
};

template <>
struct InternalTryCellShape<vtkm::NUMBER_OF_CELL_SHAPES>
{
  template <typename FunctionType>
  void operator()(const FunctionType&) const
  {
    // Done processing cell sets. Do nothing and return.
  }
};

} // namespace detail

struct Testing
{
public:
  class TestFailure
  {
  public:
    template <typename... Ts>
    VTKM_CONT TestFailure(const std::string& file, vtkm::Id line, Ts&&... messages)
      : File(file)
      , Line(line)
    {
      std::stringstream messageStream;
      this->AppendMessages(messageStream, std::forward<Ts>(messages)...);
      this->Message = messageStream.str();
    }

    VTKM_CONT const std::string& GetFile() const { return this->File; }
    VTKM_CONT vtkm::Id GetLine() const { return this->Line; }
    VTKM_CONT const std::string& GetMessage() const { return this->Message; }
  private:
    template <typename T1>
    VTKM_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1)
    {
      messageStream << m1;
    }
    template <typename T1, typename T2>
    VTKM_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1, T2&& m2)
    {
      messageStream << m1 << m2;
    }
    template <typename T1, typename T2, typename T3>
    VTKM_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1, T2&& m2, T3&& m3)
    {
      messageStream << m1 << m2 << m3;
    }
    template <typename T1, typename T2, typename T3, typename T4>
    VTKM_CONT void AppendMessages(std::stringstream& messageStream,
                                  T1&& m1,
                                  T2&& m2,
                                  T3&& m3,
                                  T4&& m4)
    {
      messageStream << m1 << m2 << m3 << m4;
    }
    template <typename T1, typename T2, typename T3, typename T4, typename... Ts>
    VTKM_CONT void AppendMessages(std::stringstream& messageStream,
                                  T1&& m1,
                                  T2&& m2,
                                  T3&& m3,
                                  T4&& m4,
                                  Ts&&... ms)
    {
      messageStream << m1 << m2 << m3 << m4;
      this->AppendMessages(messageStream, std::forward<Ts>(ms)...);
    }

    std::string File;
    vtkm::Id Line;
    std::string Message;
  };

  template <typename... Ts>
  static VTKM_CONT void Assert(const std::string& conditionString,
                               const std::string& file,
                               vtkm::Id line,
                               bool condition,
                               Ts&&... messages)
  {
    if (condition)
    {
      // Do nothing.
    }
    else
    {
      throw TestFailure(file, line, std::forward<Ts>(messages)..., " (", conditionString, ")");
    }
  }

  static VTKM_CONT void Assert(const std::string& conditionString,
                               const std::string& file,
                               vtkm::Id line,
                               bool condition)
  {
    Assert(conditionString, file, line, condition, "Test assertion failed");
  }

  template <typename... Ts>
  static VTKM_CONT void TestFail(const std::string& file, vtkm::Id line, Ts&&... messages)
  {
    throw TestFailure(file, line, std::forward<Ts>(messages)...);
  }

#ifndef VTKM_TESTING_IN_CONT
  /// Calls the test function \a function with no arguments. Catches any errors
  /// generated by VTKM_TEST_ASSERT or VTKM_TEST_FAIL, reports the error, and
  /// returns "1" (a failure status for a program's main). Returns "0" (a
  /// success status for a program's main).
  ///
  /// The intention is to implement a test's main function with this. For
  /// example, the implementation of UnitTestFoo might look something like
  /// this.
  ///
  /// \code
  /// #include <vtkm/testing/Testing.h>
  ///
  /// namespace {
  ///
  /// void TestFoo()
  /// {
  ///    // Do actual test, which checks in VTKM_TEST_ASSERT or VTKM_TEST_FAIL.
  /// }
  ///
  /// } // anonymous namespace
  ///
  /// int UnitTestFoo(int, char *[])
  /// {
  ///   return vtkm::testing::Testing::Run(TestFoo);
  /// }
  /// \endcode
  ///
  template <class Func>
  static VTKM_CONT int Run(Func function)
  {
    try
    {
      function();
    }
    catch (TestFailure& error)
    {
      std::cout << "***** Test failed @ " << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (std::exception& error)
    {
      std::cout << "***** STL exception throw." << std::endl << error.what() << std::endl;
    }
    catch (...)
    {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
    }
    return 0;
  }
#endif

  template <typename FunctionType>
  struct InternalPrintTypeAndInvoke
  {
    InternalPrintTypeAndInvoke(FunctionType function)
      : Function(function)
    {
    }

    template <typename T>
    void operator()(T t) const
    {
      std::cout << "*** " << vtkm::testing::TypeName<T>::Name() << " ***************" << std::endl;
      this->Function(t);
    }

  private:
    FunctionType Function;
  };

  /// Runs template \p function on all the types in the given list. If no type
  /// list is given, then an exemplar list of types is used.
  ///
  template <typename FunctionType, typename TypeList>
  static void TryTypes(const FunctionType& function, TypeList)
  {
    vtkm::ListForEach(InternalPrintTypeAndInvoke<FunctionType>(function), TypeList());
  }

  struct TypeListTagExemplarTypes
    : vtkm::ListTagBase<vtkm::UInt8, vtkm::Id, vtkm::FloatDefault, vtkm::Vec<vtkm::Float64, 3>>
  {
  };

  template <typename FunctionType>
  static void TryTypes(const FunctionType& function)
  {
    TryTypes(function, TypeListTagExemplarTypes());
  }

  // Disabled: This very long list results is very long compile times.
  //  /// Runs templated \p function on all the basic types defined in VTK-m. This
  //  /// is helpful to test templated functions that should work on all types. If
  //  /// the function is supposed to work on some subset of types, then use
  //  /// \c TryTypes to restrict the call to some other list of types.
  //  ///
  //  template<typename FunctionType>
  //  static void TryAllTypes(const FunctionType &function)
  //  {
  //    TryTypes(function, vtkm::TypeListTagAll());
  //  }

  /// Runs templated \p function on all cell shapes defined in VTK-m. This is
  /// helpful to test templated functions that should work on all cell types.
  ///
  template <typename FunctionType>
  static void TryAllCellShapes(const FunctionType& function)
  {
    detail::InternalTryCellShape<0>()(function);
  }
};
}
} // namespace vtkm::internal

// Prototype declaration
template <typename VectorType1, typename VectorType2>
static inline VTKM_EXEC_CONT bool test_equal(VectorType1 vector1,
                                             VectorType2 vector2,
                                             vtkm::Float64 tolerance = 0.00001);

namespace detail
{

template <typename VectorType1, typename VectorType2>
static inline VTKM_EXEC_CONT bool test_equal_impl(VectorType1 vector1,
                                                  VectorType2 vector2,
                                                  vtkm::Float64 tolerance,
                                                  vtkm::TypeTraitsVectorTag)
{
  // If you get a compiler error here, it means you are comparing a vector to
  // a scalar, in which case the types are non-comparable.
  VTKM_STATIC_ASSERT_MSG((std::is_same<typename vtkm::TypeTraits<VectorType2>::DimensionalityTag,
                                       vtkm::TypeTraitsScalarTag>::type::value == false),
                         "Trying to compare a vector with a scalar.");

  using Traits1 = vtkm::VecTraits<VectorType1>;
  using Traits2 = vtkm::VecTraits<VectorType2>;

  // If vectors have different number of components, then they cannot be equal.
  if (Traits1::GetNumberOfComponents(vector1) != Traits2::GetNumberOfComponents(vector2))
  {
    return false;
  }

  for (vtkm::IdComponent component = 0; component < Traits1::GetNumberOfComponents(vector1);
       component++)
  {
    bool componentEqual = test_equal(Traits1::GetComponent(vector1, component),
                                     Traits2::GetComponent(vector2, component),
                                     tolerance);
    if (!componentEqual)
    {
      return false;
    }
  }

  return true;
}

template <typename MatrixType1, typename MatrixType2>
static inline VTKM_EXEC_CONT bool test_equal_impl(MatrixType1 matrix1,
                                                  MatrixType2 matrix2,
                                                  vtkm::Float64 tolerance,
                                                  vtkm::TypeTraitsMatrixTag)
{
  // For the purposes of comparison, treat matrices the same as vectors.
  return test_equal_impl(matrix1, matrix2, tolerance, vtkm::TypeTraitsVectorTag());
}

template <typename ScalarType1, typename ScalarType2>
static inline VTKM_EXEC_CONT bool test_equal_impl(ScalarType1 scalar1,
                                                  ScalarType2 scalar2,
                                                  vtkm::Float64 tolerance,
                                                  vtkm::TypeTraitsScalarTag)
{
  // If you get a compiler error here, it means you are comparing a scalar to
  // a vector, in which case the types are non-comparable.
  VTKM_STATIC_ASSERT_MSG((std::is_same<typename vtkm::TypeTraits<ScalarType2>::DimensionalityTag,
                                       vtkm::TypeTraitsScalarTag>::type::value),
                         "Trying to compare a scalar with a vector.");

  // Do all comparisons using 64-bit floats.
  vtkm::Float64 value1 = vtkm::Float64(scalar1);
  vtkm::Float64 value2 = vtkm::Float64(scalar2);

  if (vtkm::Abs(value1 - value2) <= tolerance)
  {
    return true;
  }

  // We are using a ratio to compare the relative tolerance of two numbers.
  // Using an ULP based comparison (comparing the bits as integers) might be
  // a better way to go, but this has been working pretty well so far.
  vtkm::Float64 ratio;
  if ((vtkm::Abs(value2) > tolerance) && (value2 != 0))
  {
    ratio = value1 / value2;
  }
  else
  {
    // If we are here, it means that value2 is close to 0 but value1 is not.
    // These cannot be within tolerance, so just return false.
    return false;
  }
  if ((ratio > vtkm::Float64(1.0) - tolerance) && (ratio < vtkm::Float64(1.0) + tolerance))
  {
    // This component is OK. The condition is checked in this way to
    // correctly handle non-finites that fail all comparisons. Thus, if a
    // non-finite is encountered, this condition will fail and false will be
    // returned.
    return true;
  }
  else
  {
    return false;
  }
}

// Special cases of test equal where a scalar is compared with a Vec of size 1,
// which we will allow.
template <typename T>
static inline VTKM_EXEC_CONT bool test_equal_impl(vtkm::Vec<T, 1> value1,
                                                  T value2,
                                                  vtkm::Float64 tolerance,
                                                  vtkm::TypeTraitsVectorTag)
{
  return test_equal(value1[0], value2, tolerance);
}
template <typename T>
static inline VTKM_EXEC_CONT bool test_equal_impl(T value1,
                                                  vtkm::Vec<T, 1> value2,
                                                  vtkm::Float64 tolerance,
                                                  vtkm::TypeTraitsScalarTag)
{
  return test_equal(value1, value2[0], tolerance);
}

} // namespace detail

/// Helper function to test two quanitites for equality accounting for slight
/// variance due to floating point numerical inaccuracies.
///
template <typename VectorType1, typename VectorType2>
static inline VTKM_EXEC_CONT bool test_equal(VectorType1 vector1,
                                             VectorType2 vector2,
                                             vtkm::Float64 tolerance /*= 0.00001*/)
{
  return detail::test_equal_impl(
    vector1, vector2, tolerance, typename vtkm::TypeTraits<VectorType1>::DimensionalityTag());
}

/// Special implementation of test_equal for strings, which don't fit a model
/// of fixed length vectors of numbers.
///
static inline VTKM_CONT bool test_equal(const std::string& string1, const std::string& string2)
{
  return string1 == string2;
}

/// Special implementation of test_equal for Pairs, which are a bit different
/// than a vector of numbers of the same type.
///
template <typename T1, typename T2, typename T3, typename T4>
static inline VTKM_CONT bool test_equal(const vtkm::Pair<T1, T2>& pair1,
                                        const vtkm::Pair<T3, T4>& pair2,
                                        vtkm::Float64 tolerance = 0.0001)
{
  return test_equal(pair1.first, pair2.first, tolerance) &&
    test_equal(pair1.second, pair2.second, tolerance);
}

/// Special implementation of test_equal for Ranges.
///
static inline VTKM_EXEC_CONT bool test_equal(const vtkm::Range& range1,
                                             const vtkm::Range& range2,
                                             vtkm::Float64 tolerance = 0.0001)
{
  return (test_equal(range1.Min, range2.Min, tolerance) &&
          test_equal(range1.Max, range2.Max, tolerance));
}

/// Special implementation of test_equal for Bounds.
///
static inline VTKM_EXEC_CONT bool test_equal(const vtkm::Bounds& bounds1,
                                             const vtkm::Bounds& bounds2,
                                             vtkm::Float64 tolerance = 0.0001)
{
  return (test_equal(bounds1.X, bounds2.X, tolerance) &&
          test_equal(bounds1.Y, bounds2.Y, tolerance) &&
          test_equal(bounds1.Z, bounds2.Z, tolerance));
}

/// Special implementation of test_equal for booleans.
///
static inline VTKM_EXEC_CONT bool test_equal(bool bool1, bool bool2)
{
  return bool1 == bool2;
}

template <typename T>
static inline VTKM_EXEC_CONT T TestValue(vtkm::Id index, T, vtkm::TypeTraitsIntegerTag)
{
  constexpr bool larger_than_2bytes = sizeof(T) > 2;
  if (larger_than_2bytes)
  {
    return T(index * 100);
  }
  else
  {
    return T(index + 100);
  }
}

template <typename T>
static inline VTKM_EXEC_CONT T TestValue(vtkm::Id index, T, vtkm::TypeTraitsRealTag)
{
  return T(0.01 * static_cast<double>(index) + 1.001);
}

/// Many tests involve getting and setting values in some index-based structure
/// (like an array). These tests also often involve trying many types. The
/// overloaded TestValue function returns some unique value for an index for a
/// given type. Different types might give different values.
///
template <typename T>
static inline VTKM_EXEC_CONT T TestValue(vtkm::Id index, T)
{
  return TestValue(index, T(), typename vtkm::TypeTraits<T>::NumericTag());
}

template <typename T, vtkm::IdComponent N>
static inline VTKM_EXEC_CONT vtkm::Vec<T, N> TestValue(vtkm::Id index, vtkm::Vec<T, N>)
{
  vtkm::Vec<T, N> value;
  for (vtkm::IdComponent i = 0; i < N; i++)
  {
    value[i] = T(TestValue(index, T()) + T(i + 1));
  }
  return value;
}

template <typename U, typename V>
static inline VTKM_EXEC_CONT vtkm::Pair<U, V> TestValue(vtkm::Id index, vtkm::Pair<U, V>)
{
  return vtkm::Pair<U, V>(TestValue(2 * index, U()), TestValue(2 * index + 1, V()));
}

static inline VTKM_CONT std::string TestValue(vtkm::Id index, std::string)
{
  std::stringstream stream;
  stream << index;
  return stream.str();
}

/// Verifies that the contents of the given array portal match the values
/// returned by vtkm::testing::TestValue.
///
template <typename PortalType>
static inline VTKM_CONT void CheckPortal(const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;

  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    ValueType expectedValue = TestValue(index, ValueType());
    ValueType foundValue = portal.Get(index);
    if (!test_equal(expectedValue, foundValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array." << std::endl
              << "Expected: " << expectedValue << ", Found: " << foundValue << std::endl;
      VTKM_TEST_FAIL(message.str().c_str());
    }
  }
}

/// Sets all the values in a given array portal to be the values returned
/// by vtkm::testing::TestValue. The ArrayPortal must be allocated first.
///
template <typename PortalType>
static inline VTKM_CONT void SetPortal(const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;

  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    portal.Set(index, TestValue(index, ValueType()));
  }
}

/// Verifies that the contents of the two portals are the same.
///
template <typename PortalType1, typename PortalType2>
static inline VTKM_CONT bool test_equal_portals(const PortalType1& portal1,
                                                const PortalType2& portal2)
{
  if (portal1.GetNumberOfValues() != portal2.GetNumberOfValues())
  {
    return false;
  }

  for (vtkm::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    if (!test_equal(portal1.Get(index), portal2.Get(index)))
    {
      return false;
    }
  }

  return true;
}

/// Convert a size in bytes to a human readable string (e.g. "64 bytes",
/// "1.44 MiB", "128 GiB", etc). @a prec controls the fixed point precision
/// of the stringified number.
static inline VTKM_CONT std::string HumanSize(vtkm::UInt64 bytes, int prec = 2)
{
  std::string suffix = "bytes";

  // Might truncate, but it really doesn't matter unless the precision arg
  // is obscenely huge.
  vtkm::Float64 bytesf = static_cast<vtkm::Float64>(bytes);

  if (bytesf >= 1024.)
  {
    bytesf /= 1024.;
    suffix = "KiB";
  }

  if (bytesf >= 1024.)
  {
    bytesf /= 1024.;
    suffix = "MiB";
  }

  if (bytesf >= 1024.)
  {
    bytesf /= 1024.;
    suffix = "GiB";
  }

  if (bytesf >= 1024.)
  {
    bytesf /= 1024.;
    suffix = "TiB";
  }

  if (bytesf >= 1024.)
  {
    bytesf /= 1024.;
    suffix = "PiB"; // Dream big...
  }

  std::ostringstream out;
  out << std::fixed << std::setprecision(prec) << bytesf << " " << suffix;
  return out.str();
}

#endif //vtk_m_testing_Testing_h
