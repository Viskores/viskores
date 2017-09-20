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

#include <vtkm/cont/arg/TypeCheckTagCellSet.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

struct TestNotCellSet
{
};

void TestCheckCellSet()
{
  std::cout << "Checking reporting of type checking cell set." << std::endl;

  using vtkm::cont::arg::TypeCheck;
  using vtkm::cont::arg::TypeCheckTagCellSet;

  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, vtkm::cont::CellSetExplicit<>>::value),
                   "Type check failed.");

  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, vtkm::cont::CellSetStructured<2>>::value),
                   "Type check failed.");

  VTKM_TEST_ASSERT((TypeCheck<TypeCheckTagCellSet, vtkm::cont::CellSetStructured<3>>::value),
                   "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, TestNotCellSet>::value), "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, vtkm::Id>::value), "Type check failed.");

  VTKM_TEST_ASSERT(!(TypeCheck<TypeCheckTagCellSet, vtkm::cont::ArrayHandle<vtkm::Id>>::value),
                   "Type check failed.");
}

} // anonymous namespace

int UnitTestTypeCheckCellSet(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCheckCellSet);
}
