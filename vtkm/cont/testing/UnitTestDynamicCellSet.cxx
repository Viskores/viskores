//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/cont/ArrayHandleConstant.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

struct NonDefaultCellSetList
  : vtkm::ListTagBase<
      vtkm::cont::CellSetStructured<1>,
      vtkm::cont::CellSetExplicit<vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag>>
{
};

template <typename ExpectedCellType>
struct CheckFunctor
{
  void operator()(const ExpectedCellType&, bool& called) const { called = true; }

  template <typename UnexpectedType>
  void operator()(const UnexpectedType&, bool& called) const
  {
    VTKM_TEST_FAIL("CastAndCall functor called with wrong type.");
    called = false;
  }
};

template <typename CellSetType, typename CellSetList>
void CheckDynamicCellSet(const CellSetType& cellSet,
                         vtkm::cont::DynamicCellSetBase<CellSetList> dynamicCellSet)
{
  VTKM_TEST_ASSERT(dynamicCellSet.template IsType<CellSetType>(),
                   "DynamicCellSet reports wrong type.");
  VTKM_TEST_ASSERT(dynamicCellSet.IsSameType(cellSet), "DynamicCellSet reports wrong type.");
  VTKM_TEST_ASSERT(!dynamicCellSet.template IsType<vtkm::Id>(),
                   "DynamicCellSet reports wrong type.");

  dynamicCellSet.template Cast<CellSetType>();

  bool called = false;
  dynamicCellSet.CastAndCall(CheckFunctor<CellSetType>(), called);

  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  called = false;
  CastAndCall(dynamicCellSet, CheckFunctor<CellSetType>(), called);

  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

template <typename CellSetType, typename CellSetList>
void TryNewInstance(CellSetType, vtkm::cont::DynamicCellSetBase<CellSetList>& originalCellSet)
{
  vtkm::cont::DynamicCellSetBase<CellSetList> newCellSet = originalCellSet.NewInstance();

  VTKM_TEST_ASSERT(newCellSet.template IsType<CellSetType>(), "New cell set wrong type.");

  VTKM_TEST_ASSERT(&originalCellSet.CastToBase() != &newCellSet.CastToBase(),
                   "NewInstance did not make a copy.");
}

template <typename CellSetType, typename CellSetList>
void TryCellSet(CellSetType cellSet, vtkm::cont::DynamicCellSetBase<CellSetList>& dynamicCellSet)
{
  CheckDynamicCellSet(cellSet, dynamicCellSet);

  CheckDynamicCellSet(cellSet, dynamicCellSet.ResetCellSetList(vtkm::ListTagBase<CellSetType>()));

  TryNewInstance(cellSet, dynamicCellSet);
}

template <typename CellSetType>
void TryDefaultCellSet(CellSetType cellSet)
{
  vtkm::cont::DynamicCellSet dynamicCellSet(cellSet);

  TryCellSet(cellSet, dynamicCellSet);
}

template <typename CellSetType>
void TryNonDefaultCellSet(CellSetType cellSet)
{
  vtkm::cont::DynamicCellSetBase<NonDefaultCellSetList> dynamicCellSet(cellSet);

  TryCellSet(cellSet, dynamicCellSet);
}

void TestDynamicCellSet()
{
  std::cout << "Try default types with default type lists." << std::endl;
  std::cout << "*** 2D Structured Grid ******************" << std::endl;
  TryDefaultCellSet(vtkm::cont::CellSetStructured<2>());
  std::cout << "*** 3D Structured Grid ******************" << std::endl;
  TryDefaultCellSet(vtkm::cont::CellSetStructured<3>());
  std::cout << "*** Explicit Grid ***********************" << std::endl;
  TryDefaultCellSet(vtkm::cont::CellSetExplicit<>());

  std::cout << std::endl << "Try non-default types." << std::endl;
  std::cout << "*** 1D Structured Grid ******************" << std::endl;
  TryNonDefaultCellSet(vtkm::cont::CellSetStructured<1>());
  std::cout << "*** Explicit Grid Constant Shape ********" << std::endl;
  TryNonDefaultCellSet(
    vtkm::cont::CellSetExplicit<vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag>());
}

} // anonymous namespace

int UnitTestDynamicCellSet(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestDynamicCellSet);
}
