//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_testing_Testing_h
#define vtk_m_testing_Testing_h

#include <vtkm/Bitset.h>
#include <vtkm/Bounds.h>
#include <vtkm/CellShape.h>
#include <vtkm/List.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Pair.h>
#include <vtkm/Range.h>
#include <vtkm/TypeList.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/Logging.h>

#include <exception>
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

#define VTK_M_BASIC_TYPE(type, name)                                                               \
  template <>                                                                                      \
  struct TypeName<type>                                                                            \
  {                                                                                                \
    static std::string Name() { return #name; }                                                    \
  }

VTK_M_BASIC_TYPE(vtkm::Float32, F32);
VTK_M_BASIC_TYPE(vtkm::Float64, F64);
VTK_M_BASIC_TYPE(vtkm::Int8, I8);
VTK_M_BASIC_TYPE(vtkm::UInt8, UI8);
VTK_M_BASIC_TYPE(vtkm::Int16, I16);
VTK_M_BASIC_TYPE(vtkm::UInt16, UI16);
VTK_M_BASIC_TYPE(vtkm::Int32, I32);
VTK_M_BASIC_TYPE(vtkm::UInt32, UI32);
VTK_M_BASIC_TYPE(vtkm::Int64, I64);
VTK_M_BASIC_TYPE(vtkm::UInt64, UI64);

// types without vtkm::typedefs:
VTK_M_BASIC_TYPE(char, char);
VTK_M_BASIC_TYPE(long, long);
VTK_M_BASIC_TYPE(unsigned long, unsigned long);

#define VTK_M_BASIC_TYPE_HELPER(type) VTK_M_BASIC_TYPE(vtkm::type, type)

// Special containers:
VTK_M_BASIC_TYPE_HELPER(Bounds);
VTK_M_BASIC_TYPE_HELPER(Range);

// Special Vec types:
VTK_M_BASIC_TYPE_HELPER(Vec2f_32);
VTK_M_BASIC_TYPE_HELPER(Vec2f_64);
VTK_M_BASIC_TYPE_HELPER(Vec2i_8);
VTK_M_BASIC_TYPE_HELPER(Vec2i_16);
VTK_M_BASIC_TYPE_HELPER(Vec2i_32);
VTK_M_BASIC_TYPE_HELPER(Vec2i_64);
VTK_M_BASIC_TYPE_HELPER(Vec2ui_8);
VTK_M_BASIC_TYPE_HELPER(Vec2ui_16);
VTK_M_BASIC_TYPE_HELPER(Vec2ui_32);
VTK_M_BASIC_TYPE_HELPER(Vec2ui_64);
VTK_M_BASIC_TYPE_HELPER(Vec3f_32);
VTK_M_BASIC_TYPE_HELPER(Vec3f_64);
VTK_M_BASIC_TYPE_HELPER(Vec3i_8);
VTK_M_BASIC_TYPE_HELPER(Vec3i_16);
VTK_M_BASIC_TYPE_HELPER(Vec3i_32);
VTK_M_BASIC_TYPE_HELPER(Vec3i_64);
VTK_M_BASIC_TYPE_HELPER(Vec3ui_8);
VTK_M_BASIC_TYPE_HELPER(Vec3ui_16);
VTK_M_BASIC_TYPE_HELPER(Vec3ui_32);
VTK_M_BASIC_TYPE_HELPER(Vec3ui_64);
VTK_M_BASIC_TYPE_HELPER(Vec4f_32);
VTK_M_BASIC_TYPE_HELPER(Vec4f_64);
VTK_M_BASIC_TYPE_HELPER(Vec4i_8);
VTK_M_BASIC_TYPE_HELPER(Vec4i_16);
VTK_M_BASIC_TYPE_HELPER(Vec4i_32);
VTK_M_BASIC_TYPE_HELPER(Vec4i_64);
VTK_M_BASIC_TYPE_HELPER(Vec4ui_8);
VTK_M_BASIC_TYPE_HELPER(Vec4ui_16);
VTK_M_BASIC_TYPE_HELPER(Vec4ui_32);
VTK_M_BASIC_TYPE_HELPER(Vec4ui_64);

#undef VTK_M_BASIC_TYPE

template <typename T, vtkm::IdComponent Size>
struct TypeName<vtkm::Vec<T, Size>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Vec<" << TypeName<T>::Name() << ", " << Size << ">";
    return stream.str();
  }
};

template <typename T, vtkm::IdComponent numRows, vtkm::IdComponent numCols>
struct TypeName<vtkm::Matrix<T, numRows, numCols>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Matrix<" << TypeName<T>::Name() << ", " << numRows << ", " << numCols << ">";
    return stream.str();
  }
};

template <typename T, typename U>
struct TypeName<vtkm::Pair<T, U>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Pair<" << TypeName<T>::Name() << ", " << TypeName<U>::Name() << ">";
    return stream.str();
  }
};

template <typename T>
struct TypeName<vtkm::Bitset<T>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Bitset<" << TypeName<T>::Name() << ">";
    return stream.str();
  }
};

template <typename T0, typename... Ts>
struct TypeName<vtkm::List<T0, Ts...>>
{
  static std::string Name()
  {
    std::initializer_list<std::string> subtypeStrings = { TypeName<Ts>::Name()... };

    std::stringstream stream;
    stream << "List<" << TypeName<T0>::Name();
    for (auto&& subtype : subtypeStrings)
    {
      stream << ", " << subtype;
    }
    stream << ">";
    return stream.str();
  }
};
template <>
struct TypeName<vtkm::ListEmpty>
{
  static std::string Name() { return "ListEmpty"; }
};
template <>
struct TypeName<vtkm::ListUniversal>
{
  static std::string Name() { return "ListUniversal"; }
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
  static VTKM_CONT int Run(Func function, int& argc, char* argv[])
  {
    if (argc == 0 || argv == nullptr)
    {
      vtkm::cont::InitLogging();
    }
    else
    {
      vtkm::cont::InitLogging(argc, argv);
    }

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

  using TypeListExemplarTypes =
    vtkm::List<vtkm::UInt8, vtkm::Id, vtkm::FloatDefault, vtkm::Vec3f_64>;

  template <typename FunctionType>
  static void TryTypes(const FunctionType& function)
  {
    TryTypes(function, TypeListExemplarTypes());
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
  //    TryTypes(function, vtkm::TypeListAll());
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

namespace detail
{

// Forward declaration
template <typename T1, typename T2>
struct TestEqualImpl;

} // namespace detail

/// Helper function to test two quanitites for equality accounting for slight
/// variance due to floating point numerical inaccuracies.
///
template <typename T1, typename T2>
static inline VTKM_EXEC_CONT bool test_equal(T1 value1,
                                             T2 value2,
                                             vtkm::Float64 tolerance = 0.00001)
{
  return detail::TestEqualImpl<T1, T2>()(value1, value2, tolerance);
}

namespace detail
{

template <typename T1, typename T2>
struct TestEqualImpl
{
  VTKM_EXEC_CONT bool DoIt(T1 vector1,
                           T2 vector2,
                           vtkm::Float64 tolerance,
                           vtkm::TypeTraitsVectorTag) const
  {
    // If you get a compiler error here, it means you are comparing a vector to
    // a scalar, in which case the types are non-comparable.
    VTKM_STATIC_ASSERT_MSG((std::is_same<typename vtkm::TypeTraits<T2>::DimensionalityTag,
                                         vtkm::TypeTraitsVectorTag>::type::value) ||
                             (std::is_same<typename vtkm::TypeTraits<T2>::DimensionalityTag,
                                           vtkm::TypeTraitsMatrixTag>::type::value),
                           "Trying to compare a vector with a scalar.");

    using Traits1 = vtkm::VecTraits<T1>;
    using Traits2 = vtkm::VecTraits<T2>;

    // If vectors have different number of components, then they cannot be equal.
    if (Traits1::GetNumberOfComponents(vector1) != Traits2::GetNumberOfComponents(vector2))
    {
      return false;
    }

    for (vtkm::IdComponent component = 0; component < Traits1::GetNumberOfComponents(vector1);
         ++component)
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

  VTKM_EXEC_CONT bool DoIt(T1 matrix1,
                           T2 matrix2,
                           vtkm::Float64 tolerance,
                           vtkm::TypeTraitsMatrixTag) const
  {
    // For the purposes of comparison, treat matrices the same as vectors.
    return this->DoIt(matrix1, matrix2, tolerance, vtkm::TypeTraitsVectorTag());
  }

  VTKM_EXEC_CONT bool DoIt(T1 scalar1,
                           T2 scalar2,
                           vtkm::Float64 tolerance,
                           vtkm::TypeTraitsScalarTag) const
  {
    // If you get a compiler error here, it means you are comparing a scalar to
    // a vector, in which case the types are non-comparable.
    VTKM_STATIC_ASSERT_MSG((std::is_same<typename vtkm::TypeTraits<T2>::DimensionalityTag,
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

  VTKM_EXEC_CONT bool operator()(T1 value1, T2 value2, vtkm::Float64 tolerance) const
  {
    return this->DoIt(
      value1, value2, tolerance, typename vtkm::TypeTraits<T1>::DimensionalityTag());
  }
};

// Special cases of test equal where a scalar is compared with a Vec of size 1,
// which we will allow.
template <typename T>
struct TestEqualImpl<vtkm::Vec<T, 1>, T>
{
  VTKM_EXEC_CONT bool operator()(vtkm::Vec<T, 1> value1, T value2, vtkm::Float64 tolerance) const
  {
    return test_equal(value1[0], value2, tolerance);
  }
};
template <typename T>
struct TestEqualImpl<T, vtkm::Vec<T, 1>>
{
  VTKM_EXEC_CONT bool operator()(T value1, vtkm::Vec<T, 1> value2, vtkm::Float64 tolerance) const
  {
    return test_equal(value1, value2[0], tolerance);
  }
};

/// Special implementation of test_equal for strings, which don't fit a model
/// of fixed length vectors of numbers.
///
template <>
struct TestEqualImpl<std::string, std::string>
{
  VTKM_CONT bool operator()(const std::string& string1,
                            const std::string& string2,
                            vtkm::Float64 vtkmNotUsed(tolerance)) const
  {
    return string1 == string2;
  }
};
template <typename T>
struct TestEqualImpl<const char*, T>
{
  VTKM_CONT bool operator()(const char* string1, T value2, vtkm::Float64 tolerance) const
  {
    return TestEqualImpl<std::string, T>()(string1, value2, tolerance);
  }
};
template <typename T>
struct TestEqualImpl<T, const char*>
{
  VTKM_CONT bool operator()(T value1, const char* string2, vtkm::Float64 tolerance) const
  {
    return TestEqualImpl<T, std::string>()(value1, string2, tolerance);
  }
};
template <>
struct TestEqualImpl<const char*, const char*>
{
  VTKM_CONT bool operator()(const char* string1, const char* string2, vtkm::Float64 tolerance) const
  {
    return TestEqualImpl<std::string, std::string>()(string1, string2, tolerance);
  }
};

/// Special implementation of test_equal for Pairs, which are a bit different
/// than a vector of numbers of the same type.
///
template <typename T1, typename T2, typename T3, typename T4>
struct TestEqualImpl<vtkm::Pair<T1, T2>, vtkm::Pair<T3, T4>>
{
  VTKM_EXEC_CONT bool operator()(const vtkm::Pair<T1, T2>& pair1,
                                 const vtkm::Pair<T3, T4>& pair2,
                                 vtkm::Float64 tolerance) const
  {
    return test_equal(pair1.first, pair2.first, tolerance) &&
      test_equal(pair1.second, pair2.second, tolerance);
  }
};

/// Special implementation of test_equal for Ranges.
///
template <>
struct TestEqualImpl<vtkm::Range, vtkm::Range>
{
  VTKM_EXEC_CONT bool operator()(const vtkm::Range& range1,
                                 const vtkm::Range& range2,
                                 vtkm::Float64 tolerance) const
  {
    return (test_equal(range1.Min, range2.Min, tolerance) &&
            test_equal(range1.Max, range2.Max, tolerance));
  }
};

/// Special implementation of test_equal for Bounds.
///
template <>
struct TestEqualImpl<vtkm::Bounds, vtkm::Bounds>
{
  VTKM_EXEC_CONT bool operator()(const vtkm::Bounds& bounds1,
                                 const vtkm::Bounds& bounds2,
                                 vtkm::Float64 tolerance) const
  {
    return (test_equal(bounds1.X, bounds2.X, tolerance) &&
            test_equal(bounds1.Y, bounds2.Y, tolerance) &&
            test_equal(bounds1.Z, bounds2.Z, tolerance));
  }
};

/// Special implementation of test_equal for booleans.
///
template <>
struct TestEqualImpl<bool, bool>
{
  VTKM_EXEC_CONT bool operator()(bool bool1, bool bool2, vtkm::Float64 vtkmNotUsed(tolerance))
  {
    return bool1 == bool2;
  }
};

} // namespace detail

namespace detail
{

template <typename T>
struct TestValueImpl;

} // namespace detail

/// Many tests involve getting and setting values in some index-based structure
/// (like an array). These tests also often involve trying many types. The
/// overloaded TestValue function returns some unique value for an index for a
/// given type. Different types might give different values.
///
template <typename T>
static inline VTKM_EXEC_CONT T TestValue(vtkm::Id index, T)
{
  return detail::TestValueImpl<T>()(index);
}

namespace detail
{

template <typename T>
struct TestValueImpl
{
  VTKM_EXEC_CONT T DoIt(vtkm::Id index, vtkm::TypeTraitsIntegerTag) const
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

  VTKM_EXEC_CONT T DoIt(vtkm::Id index, vtkm::TypeTraitsRealTag) const
  {
    return T(0.01f * static_cast<float>(index) + 1.001f);
  }

  VTKM_EXEC_CONT T operator()(vtkm::Id index) const
  {
    return this->DoIt(index, typename vtkm::TypeTraits<T>::NumericTag());
  }
};

template <typename T, vtkm::IdComponent N>
struct TestValueImpl<vtkm::Vec<T, N>>
{
  VTKM_EXEC_CONT vtkm::Vec<T, N> operator()(vtkm::Id index) const
  {
    vtkm::Vec<T, N> value;
    for (vtkm::IdComponent i = 0; i < N; i++)
    {
      value[i] = TestValue(index * N + i, T());
    }
    return value;
  }
};

template <typename U, typename V>
struct TestValueImpl<vtkm::Pair<U, V>>
{
  VTKM_EXEC_CONT vtkm::Pair<U, V> operator()(vtkm::Id index) const
  {
    return vtkm::Pair<U, V>(TestValue(2 * index, U()), TestValue(2 * index + 1, V()));
  }
};

template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
struct TestValueImpl<vtkm::Matrix<T, NumRow, NumCol>>
{
  VTKM_EXEC_CONT vtkm::Matrix<T, NumRow, NumCol> operator()(vtkm::Id index) const
  {
    vtkm::Matrix<T, NumRow, NumCol> value;
    vtkm::Id runningIndex = index * NumRow * NumCol;
    for (vtkm::IdComponent row = 0; row < NumRow; ++row)
    {
      for (vtkm::IdComponent col = 0; col < NumCol; ++col)
      {
        value(row, col) = TestValue(runningIndex, T());
        ++runningIndex;
      }
    }
    return value;
  }
};

template <>
struct TestValueImpl<std::string>
{
  VTKM_CONT std::string operator()(vtkm::Id index) const
  {
    std::stringstream stream;
    stream << index;
    return stream.str();
  }
};

} // namespace detail

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

#endif //vtk_m_testing_Testing_h
