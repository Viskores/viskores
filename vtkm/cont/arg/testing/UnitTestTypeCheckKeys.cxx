//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/arg/TypeCheckTagKeys.h>

#include <vtkm/worklet/Keys.h>

#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

struct TestNotKeys
{
};

void TestCheckKeys()
{
  std::cout << "Checking reporting of type checking keys." << std::endl;

  using vtkm::cont::arg::TypeCheck;
  using vtkm::cont::arg::TypeCheckTagKeys;

  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, vtkm::worklet::Keys<vtkm::Id>>::value),
                   "Type check failed.");
  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, vtkm::worklet::Keys<vtkm::Float32>>::value),
                   "Type check failed.");
  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagKeys, vtkm::worklet::Keys<vtkm::Id3>>::value),
                   "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, TestNotKeys>::value), "Type check failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, vtkm::Id>::value), "Type check failed.");
  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagKeys, vtkm::cont::ArrayHandle<vtkm::Id>>::value),
                   "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckKeys(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCheckKeys);
}
