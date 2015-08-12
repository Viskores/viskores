//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_testing_Testing_h
#define vtk_m_testing_Testing_h

#include <vtkm/Pair.h>
#include <vtkm/TypeListTag.h>
#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/static_assert.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <iostream>
#include <sstream>
#include <string>

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

/// \def VTKM_TEST_ASSERT(condition, message)
///
/// Asserts a condition for a test to pass. A passing condition is when \a
/// condition resolves to true. If \a condition is false, then the test is
/// aborted and failure is returned.

#define VTKM_TEST_ASSERT(condition, message) \
  ::vtkm::testing::Testing::Assert( \
      condition, __FILE__, __LINE__, message, #condition)

/// \def VTKM_TEST_FAIL(message)
///
/// Causes a test to fail with the given \a message.

#define VTKM_TEST_FAIL(message) \
  throw ::vtkm::testing::Testing::TestFailure(__FILE__, __LINE__, message)

namespace vtkm {
namespace testing {

// If you get an error about this class definition being incomplete, it means
// that you tried to get the name of a type that is not specified. You can
// either not use that type, not try to get the string name, or add it to the
// list.
template<typename T>
struct TypeName;

#define VTK_M_BASIC_TYPE(type) \
  template<> struct TypeName<type> { \
    static std::string Name() { return #type; } \
  } \

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

template<typename T, vtkm::IdComponent Size>
struct TypeName<vtkm::Vec<T,Size> >
{
  static std::string Name() {
    std::stringstream stream;
    stream << "vtkm::Vec< "
           << TypeName<T>::Name()
           << ", "
           << Size
           << " >";
    return stream.str();
  }
};

template<typename T, typename U>
struct TypeName<vtkm::Pair<T,U> >
{
  static std::string Name() {
    std::stringstream stream;
    stream << "vtkm::Pair< "
           << TypeName<T>::Name()
           << ", "
           << TypeName<U>::Name()
           << " >";
    return stream.str();
  }
};


struct Testing
{
public:
  class TestFailure
  {
  public:
    VTKM_CONT_EXPORT TestFailure(const std::string &file,
                                 vtkm::Id line,
                                 const std::string &message)
      : File(file), Line(line), Message(message) { }

    VTKM_CONT_EXPORT TestFailure(const std::string &file,
                                 vtkm::Id line,
                                 const std::string &message,
                                 const std::string &condition)
      : File(file), Line(line)
    {
      this->Message.append(message);
      this->Message.append(" (");
      this->Message.append(condition);
      this->Message.append(")");
    }

    VTKM_CONT_EXPORT const std::string &GetFile() const { return this->File; }
    VTKM_CONT_EXPORT vtkm::Id GetLine() const { return this->Line; }
    VTKM_CONT_EXPORT const std::string &GetMessage() const
    {
      return this->Message;
    }
  private:
    std::string File;
    vtkm::Id Line;
    std::string Message;
  };

  static VTKM_CONT_EXPORT void Assert(bool condition,
                                      const std::string &file,
                                      vtkm::Id line,
                                      const std::string &message,
                                      const std::string &conditionString)
  {
    if (condition)
    {
      // Do nothing.
    }
    else
    {
      throw TestFailure(file, line, message, conditionString);
    }
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
  template<class Func>
  static VTKM_CONT_EXPORT int Run(Func function)
  {
    try
    {
      function();
    }
    catch (TestFailure error)
    {
      std::cout << "***** Test failed @ "
                << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (...)
    {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
    }
    return 0;
  }
#endif

  template<typename FunctionType>
  struct InternalPrintTypeAndInvoke
  {
    InternalPrintTypeAndInvoke(FunctionType function) : Function(function) {  }

    template<typename T>
    void operator()(T t) const
    {
      std::cout << "*** "
                << vtkm::testing::TypeName<T>::Name()
                << " ***************" << std::endl;
      this->Function(t);
    }

  private:
    FunctionType Function;
  };

  /// Runs template \p function on all the types in the given list.
  ///
  template<class FunctionType, class TypeList>
  static void TryTypes(const FunctionType &function, TypeList)
  {
    vtkm::ListForEach(InternalPrintTypeAndInvoke<FunctionType>(function),
                      TypeList());
  }

  /// Runs templated \p function on all the basic types defined in VTK-m. This
  /// is helpful to test templated functions that should work on all types. If
  /// the function is supposed to work on some subset of types, then use
  /// \c TryTypes to restrict the call to some other list of types.
  ///
  template<class FunctionType>
  static void TryAllTypes(const FunctionType &function)
  {
    TryTypes(function, vtkm::TypeListTagAll());
  }

};

}
} // namespace vtkm::internal

/// Helper function to test two quanitites for equality accounting for slight
/// variance due to floating point numerical inaccuracies.
///
template<typename VectorType1, typename VectorType2>
VTKM_EXEC_CONT_EXPORT
bool test_equal(VectorType1 vector1,
                VectorType2 vector2,
                vtkm::Float64 tolerance = 0.0001)
{
  typedef typename vtkm::VecTraits<VectorType1> Traits1;
  typedef typename vtkm::VecTraits<VectorType2> Traits2;
  BOOST_STATIC_ASSERT(Traits1::NUM_COMPONENTS == Traits2::NUM_COMPONENTS);

  for (vtkm::IdComponent component = 0;
       component < Traits1::NUM_COMPONENTS;
       component++)
  {
    vtkm::Float64 value1 =
        vtkm::Float64(Traits1::GetComponent(vector1, component));
    vtkm::Float64 value2 =
        vtkm::Float64(Traits2::GetComponent(vector2, component));
    if ((fabs(value1) <= 2*tolerance) && (fabs(value2) <= 2*tolerance))
    {
      continue;
    }
    vtkm::Float64 ratio;
    // The following condition is redundant since the previous check
    // guarantees neither value will be zero, but the MSVC compiler
    // sometimes complains about it.
    if (value2 != 0)
    {
      ratio = value1 / value2;
    }
    else
    {
      ratio = 1.0;
    }
    if ((ratio > vtkm::Float64(1.0) - tolerance)
        && (ratio < vtkm::Float64(1.0) + tolerance))
    {
      // This component is OK. The condition is checked in this way to
      // correctly handle non-finites that fail all comparisons. Thus, if a
      // non-finite is encountered, this condition will fail and false will be
      // returned.
    }
    else
    {
      return false;
    }
  }
  return true;
}

/// Special implementation of test_equal for strings, which don't fit a model
/// of fixed length vectors of numbers.
///
VTKM_CONT_EXPORT
bool test_equal(const std::string &string1, const std::string &string2)
{
  return string1 == string2;
}

/// Special implementation of test_equal for Pairs, which are a bit different
/// than a vector of numbers of the same type.
///
template<typename T1, typename T2, typename T3, typename T4>
VTKM_CONT_EXPORT
bool test_equal(const vtkm::Pair<T1,T2> &pair1,
                const vtkm::Pair<T3,T4> &pair2,
                vtkm::Float64 tolerance = 0.0001)
{
  return test_equal(pair1.first, pair2.first, tolerance)
      && test_equal(pair1.second, pair2.second, tolerance);
}

/// Helper function for printing out vectors during testing.
///
template<typename T, vtkm::IdComponent Size>
VTKM_CONT_EXPORT
std::ostream &operator<<(std::ostream &stream, const vtkm::Vec<T,Size> &vec)
{
  stream << "[";
  for (vtkm::IdComponent component = 0; component < Size-1; component++)
  {
    stream << vec[component] << ",";
  }
  return stream << vec[Size-1] << "]";
}

/// Helper function for printing out pairs during testing.
///
template<typename T, typename U>
VTKM_EXEC_CONT_EXPORT
std::ostream &operator<<(std::ostream &stream, const vtkm::Pair<T,U> &vec)
{
  return stream << "[" << vec.first << "," << vec.second << "]";
}


template<typename T>
VTKM_EXEC_CONT_EXPORT
T TestValue(vtkm::Id index, T, vtkm::TypeTraitsIntegerTag)
{
  return T(index*100);
}

template<typename T>
VTKM_EXEC_CONT_EXPORT
T TestValue(vtkm::Id index, T, vtkm::TypeTraitsRealTag)
{
  return T(0.01*index + 1.001);
}

/// Many tests involve getting and setting values in some index-based structure
/// (like an array). These tests also often involve trying many types. The
/// overloaded TestValue function returns some unique value for an index for a
/// given type. Different types might give different values.
///
template<typename T>
VTKM_EXEC_CONT_EXPORT
T TestValue(vtkm::Id index, T)
{
  return TestValue(index, T(), typename vtkm::TypeTraits<T>::NumericTag());
}

template<typename T, vtkm::IdComponent N>
VTKM_EXEC_CONT_EXPORT
vtkm::Vec<T,N> TestValue(vtkm::Id index, vtkm::Vec<T,N>) {
  vtkm::Vec<T,N> value;
  for (vtkm::IdComponent i = 0; i < N; i++)
  {
    value[i] = T(TestValue(index, T()) + T(i + 1));
  }
  return value;
}

VTKM_CONT_EXPORT
std::string TestValue(vtkm::Id index, std::string) {
  std::stringstream stream;
  stream << index;
  return stream.str();
}

/// Verifies that the contents of the given array portal match the values
/// returned by vtkm::testing::TestValue.
///
template<typename PortalType>
VTKM_CONT_EXPORT
void CheckPortal(const PortalType &portal)
{
  typedef typename PortalType::ValueType ValueType;

  for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    ValueType expectedValue = TestValue(index, ValueType());
    ValueType foundValue = portal.Get(index);
    if (!test_equal(expectedValue, foundValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array." << std::endl
              << "Expected: " << expectedValue
              << ", Found: " << foundValue << std::endl;
      VTKM_TEST_FAIL(message.str().c_str());
    }
  }
}

#endif //vtk_m_testing_Testing_h
