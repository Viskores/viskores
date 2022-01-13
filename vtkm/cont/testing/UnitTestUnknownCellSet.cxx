//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/UncertainCellSet.h>

#include <vtkm/cont/ArrayHandleConstant.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

using NonDefaultCellSetList =
  vtkm::List<vtkm::cont::CellSetStructured<1>,
             vtkm::cont::CellSetExplicit<vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag>>;

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

class DummyCellSet : public vtkm::cont::CellSet
{
};

void CheckEmptyUnknownCellSet()
{
  vtkm::cont::UnknownCellSet empty;

  VTKM_TEST_ASSERT(empty.GetNumberOfCells() == 0, "UnknownCellSet should have no cells");
  VTKM_TEST_ASSERT(empty.GetNumberOfFaces() == 0, "UnknownCellSet should have no faces");
  VTKM_TEST_ASSERT(empty.GetNumberOfEdges() == 0, "UnknownCellSet should have no edges");
  VTKM_TEST_ASSERT(empty.GetNumberOfPoints() == 0, "UnknownCellSet should have no points");

  empty.PrintSummary(std::cout);

  using CellSet2D = vtkm::cont::CellSetStructured<2>;
  using CellSet3D = vtkm::cont::CellSetStructured<3>;
  VTKM_TEST_ASSERT(!empty.IsType<CellSet2D>(), "UnknownCellSet reports wrong type.");
  VTKM_TEST_ASSERT(!empty.IsType<CellSet3D>(), "UnknownCellSet reports wrong type.");
  VTKM_TEST_ASSERT(!empty.IsType<DummyCellSet>(), "UnknownCellSet reports wrong type.");

  VTKM_TEST_ASSERT(!empty.CanConvert<CellSet2D>(), "UnknownCellSet reports wrong type.");
  VTKM_TEST_ASSERT(!empty.CanConvert<CellSet3D>(), "UnknownCellSet reports wrong type.");
  VTKM_TEST_ASSERT(!empty.CanConvert<DummyCellSet>(), "UnknownCellSet reports wrong type.");

  bool gotException = false;
  try
  {
    CellSet2D instance = empty.AsCellSet<CellSet2D>();
  }
  catch (vtkm::cont::ErrorBadType&)
  {
    gotException = true;
  }
  VTKM_TEST_ASSERT(gotException, "Empty UnknownCellSet should have thrown on casting");

  auto empty2 = empty.NewInstance();
  VTKM_TEST_ASSERT(empty.GetCellSetBase() == nullptr, "UnknownCellSet should contain a nullptr");
  VTKM_TEST_ASSERT(empty2.GetCellSetBase() == nullptr, "UnknownCellSet should contain a nullptr");
}

template <typename CellSetType, typename CellSetList>
void CheckUnknownCellSet(vtkm::cont::UnknownCellSet unknownCellSet)
{
  VTKM_TEST_ASSERT(unknownCellSet.CanConvert<CellSetType>());
  VTKM_TEST_ASSERT(!unknownCellSet.CanConvert<DummyCellSet>());

  unknownCellSet.AsCellSet<CellSetType>();

  bool called = false;
  unknownCellSet.CastAndCallForTypes<CellSetList>(CheckFunctor<CellSetType>(), called);
  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  if (vtkm::ListHas<CellSetList, VTKM_DEFAULT_CELL_SET_LIST>::value)
  {
    called = false;
    CastAndCall(unknownCellSet, CheckFunctor<CellSetType>(), called);
    VTKM_TEST_ASSERT(
      called, "The functor was never called (and apparently a bad value exception not thrown).");
  }

  vtkm::cont::UncertainCellSet<CellSetList> uncertainCellSet(unknownCellSet);

  called = false;
  uncertainCellSet.CastAndCall(CheckFunctor<CellSetType>(), called);
  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");

  called = false;
  CastAndCall(uncertainCellSet, CheckFunctor<CellSetType>(), called);
  VTKM_TEST_ASSERT(
    called, "The functor was never called (and apparently a bad value exception not thrown).");
}

template <typename CellSetType>
void TryNewInstance(vtkm::cont::UnknownCellSet& originalCellSet)
{
  vtkm::cont::UnknownCellSet newCellSet = originalCellSet.NewInstance();

  VTKM_TEST_ASSERT(newCellSet.IsType<CellSetType>(), "New cell set wrong type.");

  VTKM_TEST_ASSERT(originalCellSet.GetCellSetBase() != newCellSet.GetCellSetBase(),
                   "NewInstance did not make a copy.");
}

template <typename CellSetType, typename CellSetList>
void TryCellSet(vtkm::cont::UnknownCellSet& unknownCellSet)
{
  CheckUnknownCellSet<CellSetType, CellSetList>(unknownCellSet);

  CheckUnknownCellSet<CellSetType, vtkm::List<CellSetType>>(unknownCellSet);

  TryNewInstance<CellSetType>(unknownCellSet);
}

template <typename CellSetType>
void TryDefaultCellSet(CellSetType cellSet)
{
  vtkm::cont::UnknownCellSet unknownCellSet(cellSet);

  TryCellSet<CellSetType, VTKM_DEFAULT_CELL_SET_LIST>(unknownCellSet);
}

template <typename CellSetType>
void TryNonDefaultCellSet(CellSetType cellSet)
{
  vtkm::cont::UnknownCellSet unknownCellSet(cellSet);

  TryCellSet<CellSetType, NonDefaultCellSetList>(unknownCellSet);
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

  std::cout << std::endl << "Try empty DynamicCellSet." << std::endl;
  CheckEmptyUnknownCellSet();
}

} // anonymous namespace

int UnitTestUnknownCellSet(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDynamicCellSet, argc, argv);
}
