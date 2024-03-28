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
#include <vtkm/CellTraits.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/testing/Testing.h>

namespace CellShapesExamples
{

////
//// BEGIN-EXAMPLE CellShapeIdToTag
////
void CellFunction(vtkm::CellShapeTagTriangle)
{
  std::cout << "In CellFunction for triangles." << std::endl;
}

void DoSomethingWithACell()
{
  // Calls CellFunction overloaded with a vtkm::CellShapeTagTriangle.
  CellFunction(vtkm::CellShapeIdToTag<vtkm::CELL_SHAPE_TRIANGLE>::Tag());
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
VTKM_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormalImpl(
  const PointCoordinatesVector& pointCoordinates,
  vtkm::CellTopologicalDimensionsTag<2>,
  const WorkletType& worklet)
{
  if (pointCoordinates.GetNumberOfComponents() >= 3)
  {
    return vtkm::TriangleNormal(
      pointCoordinates[0], pointCoordinates[1], pointCoordinates[2]);
  }
  else
  {
    worklet.RaiseError("Degenerate polygon.");
    return typename PointCoordinatesVector::ComponentType();
  }
}

template<typename PointCoordinatesVector,
         vtkm::IdComponent Dimensions,
         typename WorkletType>
VTKM_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormalImpl(
  const PointCoordinatesVector&,
  vtkm::CellTopologicalDimensionsTag<Dimensions>,
  const WorkletType& worklet)
{
  worklet.RaiseError("Only polygons supported for cell normals.");
  return typename PointCoordinatesVector::ComponentType();
}

} // namespace detail

template<typename CellShape, typename PointCoordinatesVector, typename WorkletType>
VTKM_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormal(
  CellShape,
  const PointCoordinatesVector& pointCoordinates,
  const WorkletType& worklet)
{
  return detail::CellNormalImpl(
    pointCoordinates,
    typename vtkm::CellTraits<CellShape>::TopologicalDimensionsTag(),
    worklet);
}

template<typename PointCoordinatesVector, typename WorkletType>
VTKM_EXEC_CONT typename PointCoordinatesVector::ComponentType CellNormal(
  vtkm::CellShapeTagGeneric shape,
  const PointCoordinatesVector& pointCoordinates,
  const WorkletType& worklet)
{
  switch (shape.Id)
  {
    vtkmGenericCellShapeMacro(
      return CellNormal(CellShapeTag(), pointCoordinates, worklet));
    default:
      worklet.RaiseError("Unknown cell type.");
      return typename PointCoordinatesVector::ComponentType();
  }
}
////
//// END-EXAMPLE GenericCellNormal
////

struct FakeWorklet : vtkm::exec::FunctorBase
{
};

void Run()
{
  std::cout << "Basic identifier to tag." << std::endl;
  DoSomethingWithACell();

  std::cout << "Function with dynamic lookup of cell shape." << std::endl;

  vtkm::Vec<vtkm::Vec3f, 3> pointCoordinates;
  pointCoordinates[0] = vtkm::Vec3f(0.0f, 0.0f, 0.0f);
  pointCoordinates[1] = vtkm::Vec3f(1.0f, 0.0f, 0.0f);
  pointCoordinates[2] = vtkm::Vec3f(0.0f, 1.0f, 0.0f);

  vtkm::Vec3f expectedNormal(0.0f, 0.0f, 1.0f);

  char errorBuffer[256];
  errorBuffer[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorBuffer, 256);
  FakeWorklet worklet;
  worklet.SetErrorMessageBuffer(errorMessage);

  vtkm::Vec3f normal =
    CellNormal(vtkm::CellShapeTagTriangle(), pointCoordinates, worklet);
  VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), "Error finding normal.");
  VTKM_TEST_ASSERT(test_equal(normal, expectedNormal), "Bad normal.");

  normal = CellNormal(
    vtkm::CellShapeTagGeneric(vtkm::CELL_SHAPE_TRIANGLE), pointCoordinates, worklet);
  VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), "Error finding normal.");
  VTKM_TEST_ASSERT(test_equal(normal, expectedNormal), "Bad normal.");

  CellNormal(vtkm::CellShapeTagLine(), pointCoordinates, worklet);
  VTKM_TEST_ASSERT(errorMessage.IsErrorRaised(), "Expected error not raised.");
}

} // namespace CellShapesExamples

int GuideExampleCellShapes(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(CellShapesExamples::Run, argc, argv);
}
