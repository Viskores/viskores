//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define VTKM_NO_ASSERT

#include <vtkm/Assert.h>

int UnitTestNoAssert(int, char* [])
{
  VTKM_ASSERT(false);
  return 0;
}
