//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#define VISKORES_NO_ASSERT

#include <viskores/Assert.h>

int UnitTestNoAssert(int, char*[])
{
  VISKORES_ASSERT(false);
  return 0;
}
