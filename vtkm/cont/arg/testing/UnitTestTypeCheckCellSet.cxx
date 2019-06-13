//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

int UnitTestTypeCheckCellSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCheckCellSet, argc, argv);
}
