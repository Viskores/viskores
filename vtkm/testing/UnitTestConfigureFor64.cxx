//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/internal/ConfigureFor64.h>

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

// Size of 64 bits.
#define EXPECTED_SIZE 8

#if defined(VTKM_NO_64BIT_IDS)
#error vtkm::Id an unexpected size.
#endif

#if defined(VTKM_NO_DOUBLE_PRECISION)
#error vtkm::FloatDefault an unexpected size.
#endif

namespace
{

void TestTypeSizes()
{
  VTKM_TEST_ASSERT(sizeof(vtkm::Id) == EXPECTED_SIZE, "vtkm::Id an unexpected size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::FloatDefault) == EXPECTED_SIZE,
                   "vtkm::FloatDefault an unexpected size.");
}
}

int UnitTestConfigureFor64(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestTypeSizes, argc, argv);
}
