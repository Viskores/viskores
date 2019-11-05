//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Deprecated.h>

#include <vtkm/testing/Testing.h>

namespace
{

struct NewClass
{
  VTKM_EXEC_CONT
  void ImportantMethod(double x, double tolerance)
  {
    std::cout << "Using " << x << " with tolerance " << tolerance << std::endl;
  }

  VTKM_EXEC_CONT
  VTKM_DEPRECATED(1.6, "You must now specify a tolerance.") void ImportantMethod(double x)
  {
    this->ImportantMethod(x, 1e-6);
  }
};

struct VTKM_DEPRECATED(1.6, "OldClass replaced with NewClass.") OldClass
{
};

using OldAlias VTKM_DEPRECATED(1.6, "Use NewClass instead.") = NewClass;

//// Should be OK for one deprecated alias to use another deprecated thing (you would think).
//using OlderAlias VTKM_DEPRECATED(1.6, "Use NewClass instead.") = OldAlias;

enum struct VTKM_DEPRECATED(1.7, "Use NewEnum instead.") OldEnum
{
  OLD_VALUE
};

enum struct NewEnum
{
  OLD_VALUE1 VTKM_DEPRECATED(1.7, "Use NEW_VALUE instead."),
  NEW_VALUE,
  OLD_VALUE2 VTKM_DEPRECATED(1.7) = 42
};

template <typename T>
void DoSomethingWithObject(T)
{
  std::cout << "Looking at " << typeid(T).name() << std::endl;
}

static void DoTest()
{
  std::cout << "C++14 deprecated supported: " << VTK_M_COMPILER_CXX_ATTRIBUTE_DEPRECATED
            << std::endl;
  std::cout << "Deprecation is: " << VTKM_STRINGIFY_FIRST(VTKM_DEPRECATED(X.Y, "Message."))
            << std::endl;

  VTKM_TEST_ASSERT(test_equal(VTK_M_DEPRECATED_MAKE_MESSAGE(X.Y), " Deprecated in version X.Y."));
  VTKM_TEST_ASSERT(test_equal(VTK_M_DEPRECATED_MAKE_MESSAGE(X.Y.Z, "Use feature foo instead."),
                              "Use feature foo instead. Deprecated in version X.Y.Z."));

  // Using valid classes with unused deprecated parts should be fine.
  NewClass useIt;
  DoSomethingWithObject(useIt);
  useIt.ImportantMethod(1.1, 1e-8);
  DoSomethingWithObject(NewEnum::NEW_VALUE);

// These should each give compiler warnings.
#if 0
  OldClass useOldClass;
  DoSomethingWithObject(useOldClass);
  OldAlias useOldAlias;
  DoSomethingWithObject(useOldAlias);
  //OlderAlias useOlderAlias;
  //DoSomethingWithObject(useOlderAlias);
  useIt.ImportantMethod(1.1);
  DoSomethingWithObject(OldEnum::OLD_VALUE);
  DoSomethingWithObject(NewEnum::OLD_VALUE1);
  DoSomethingWithObject(NewEnum::OLD_VALUE2);
#endif
}

} // anonymous namespace

int UnitTestDeprecated(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(DoTest, argc, argv);
}
