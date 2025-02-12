//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/CellShape.h>
#include <viskores/CellTraits.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/exec/FunctorBase.h>

#include <viskores/testing/Testing.h>

namespace CellShapesExamples
{

////
//// BEGIN-EXAMPLE CellShapeIdToTag
////
void CellFunction(viskores::CellShapeTagTriangle)
{
  std::cout << "In CellFunction for triangles." << std::endl;
}

void DoSomethingWithACell()
{
  // Calls CellFunction overloaded with a viskores::CellShapeTagTriangle.
  CellFunction(viskores::CellShapeIdToTag<viskores::CELL_SHAPE_TRIANGLE>::Tag());
}
////
//// END-EXAMPLE CellShapeIdToTag
////

////
//// BEGIN-EXAMPLE GenericCellNormal
////
namespace detail
{

template<typename PointCoordinatesVector, typename WorkletType>
VISKORES_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormalImpl(
  const PointCoordinatesVector& pointCoordinates,
  viskores::CellTopologicalDimensionsTag<2>,
  const WorkletType& worklet)
{
  if (pointCoordinates.GetNumberOfComponents() >= 3)
  {
    return viskores::TriangleNormal(
      pointCoordinates[0], pointCoordinates[1], pointCoordinates[2]);
  }
  else
  {
    worklet.RaiseError("Degenerate polygon.");
    return typename PointCoordinatesVector::ComponentType();
  }
}

template<typename PointCoordinatesVector,
         viskores::IdComponent Dimensions,
         typename WorkletType>
VISKORES_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormalImpl(
  const PointCoordinatesVector&,
  viskores::CellTopologicalDimensionsTag<Dimensions>,
  const WorkletType& worklet)
{
  worklet.RaiseError("Only polygons supported for cell normals.");
  return typename PointCoordinatesVector::ComponentType();
}

} // namespace detail

template<typename CellShape, typename PointCoordinatesVector, typename WorkletType>
VISKORES_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormal(
  CellShape,
  const PointCoordinatesVector& pointCoordinates,
  const WorkletType& worklet)
{
  return detail::CellNormalImpl(
    pointCoordinates,
    typename viskores::CellTraits<CellShape>::TopologicalDimensionsTag(),
    worklet);
}

template<typename PointCoordinatesVector, typename WorkletType>
VISKORES_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormal(
  viskores::CellShapeTagGeneric shape,
  const PointCoordinatesVector& pointCoordinates,
  const WorkletType& worklet)
{
  switch (shape.Id)
  {
    viskoresGenericCellShapeMacro(
      return CellNormal(CellShapeTag(), pointCoordinates, worklet));
    default:
      worklet.RaiseError("Unknown cell type.");
      return typename PointCoordinatesVector::ComponentType();
  }
}
////
//// END-EXAMPLE GenericCellNormal
////

struct FakeWorklet : viskores::exec::FunctorBase
{
};

void Run()
{
  std::cout << "Basic identifier to tag." << std::endl;
  DoSomethingWithACell();

  std::cout << "Function with dynamic lookup of cell shape." << std::endl;

  viskores::Vec<viskores::Vec3f, 3> pointCoordinates;
  pointCoordinates[0] = viskores::Vec3f(0.0f, 0.0f, 0.0f);
  pointCoordinates[1] = viskores::Vec3f(1.0f, 0.0f, 0.0f);
  pointCoordinates[2] = viskores::Vec3f(0.0f, 1.0f, 0.0f);

  viskores::Vec3f expectedNormal(0.0f, 0.0f, 1.0f);

  char errorBuffer[256];
  errorBuffer[0] = '\0';
  viskores::exec::internal::ErrorMessageBuffer errorMessage(errorBuffer, 256);
  FakeWorklet worklet;
  worklet.SetErrorMessageBuffer(errorMessage);

  viskores::Vec3f normal =
    CellNormal(viskores::CellShapeTagTriangle(), pointCoordinates, worklet);
  VISKORES_TEST_ASSERT(!errorMessage.IsErrorRaised(), "Error finding normal.");
  VISKORES_TEST_ASSERT(test_equal(normal, expectedNormal), "Bad normal.");

  normal = CellNormal(
    viskores::CellShapeTagGeneric(viskores::CELL_SHAPE_TRIANGLE), pointCoordinates, worklet);
  VISKORES_TEST_ASSERT(!errorMessage.IsErrorRaised(), "Error finding normal.");
  VISKORES_TEST_ASSERT(test_equal(normal, expectedNormal), "Bad normal.");

  CellNormal(viskores::CellShapeTagLine(), pointCoordinates, worklet);
  VISKORES_TEST_ASSERT(errorMessage.IsErrorRaised(), "Expected error not raised.");
}

} // namespace CellShapesExamples

int GuideExampleCellShapes(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(CellShapesExamples::Run, argc, argv);
}
