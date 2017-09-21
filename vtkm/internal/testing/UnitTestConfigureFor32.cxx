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

#include <vtkm/internal/ConfigureFor32.h>

#include <vtkm/Types.h>

#include <vtkm/testing/Testing.h>

// Size of 32 bits.
#define EXPECTED_SIZE 4

#if VTKM_SIZE_ID != EXPECTED_SIZE
#error VTKM_SIZE_ID an unexpected size.
#endif

#if VTKM_SIZE_SCALAR != EXPECTED_SIZE
#error VTKM_SIZE_SCALAR an unexpected size.
#endif

namespace
{

void TestTypeSizes()
{
  VTKM_TEST_ASSERT(VTKM_SIZE_ID == EXPECTED_SIZE, "VTKM_SIZE_ID an unexpected size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::Id) == EXPECTED_SIZE, "vtkm::Id an unexpected size.");
  VTKM_TEST_ASSERT(VTKM_SIZE_SCALAR == EXPECTED_SIZE, "VTKM_SIZE_SCALAR an unexpected size.");
  VTKM_TEST_ASSERT(sizeof(vtkm::FloatDefault) == EXPECTED_SIZE,
                   "vtkm::FloatDefault an unexpected size.");
}
}

int UnitTestConfigureFor32(int, char* [])
{
  return vtkm::testing::Testing::Run(TestTypeSizes);
}
