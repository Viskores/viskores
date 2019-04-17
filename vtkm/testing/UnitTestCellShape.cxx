//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/CellShape.h>

#include <vtkm/testing/Testing.h>

namespace
{

template <typename T>
void CheckTypeSame(T, T)
{
  std::cout << "  Success" << std::endl;
}

template <typename T1, typename T2>
void CheckTypeSame(T1, T2)
{
  VTKM_TEST_FAIL("Got unexpected types.");
}

struct CellShapeTestFunctor
{
  template <typename ShapeTag>
  void operator()(ShapeTag) const
  {
    VTKM_IS_CELL_SHAPE_TAG(ShapeTag);

    const vtkm::IdComponent cellShapeId = ShapeTag::Id;
    std::cout << "Cell shape id: " << cellShapeId << std::endl;

    std::cout << "Check conversion between id and tag is consistent." << std::endl;
    CheckTypeSame(ShapeTag(), typename vtkm::CellShapeIdToTag<cellShapeId>::Tag());

    std::cout << "Check vtkmGenericCellShapeMacro." << std::endl;
    switch (cellShapeId)
    {
      vtkmGenericCellShapeMacro(CheckTypeSame(ShapeTag(), CellShapeTag()));
      default:
        VTKM_TEST_FAIL("Generic shape switch not working.");
    }
  }
};

void CellShapeTest()
{
  vtkm::testing::Testing::TryAllCellShapes(CellShapeTestFunctor());
}

} // anonymous namespace

int UnitTestCellShape(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(CellShapeTest, argc, argv);
}
