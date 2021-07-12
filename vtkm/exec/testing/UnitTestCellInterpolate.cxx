//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/CellInterpolate.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <vtkm/CellTraits.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/VecVariable.h>

#include <vtkm/testing/Testing.h>

#define CHECK_CALL(call) \
  VTKM_TEST_ASSERT((call) == vtkm::ErrorCode::Success, "Call resulted in error.")

namespace
{

static constexpr vtkm::IdComponent MAX_POINTS = 8;

template <typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     vtkm::CellTraitsTagSizeFixed,
                     vtkm::IdComponent& minPoints,
                     vtkm::IdComponent& maxPoints)
{
  // If this line fails, then MAX_POINTS is not large enough to support all
  // cell shapes.
  VTKM_STATIC_ASSERT((vtkm::CellTraits<CellShapeTag>::NUM_POINTS <= MAX_POINTS));
  minPoints = maxPoints = vtkm::CellTraits<CellShapeTag>::NUM_POINTS;
}

template <typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     vtkm::CellTraitsTagSizeVariable,
                     vtkm::IdComponent& minPoints,
                     vtkm::IdComponent& maxPoints)
{
  minPoints = 1;
  maxPoints = MAX_POINTS;
}

template <typename FieldType>
struct TestInterpolateFunctor
{
  using ComponentType = typename vtkm::VecTraits<FieldType>::ComponentType;

  template <typename CellShapeTag, typename FieldVecType>
  void DoTestWithField(CellShapeTag shape, const FieldVecType& fieldValues) const
  {
    vtkm::IdComponent numPoints = fieldValues.GetNumberOfComponents();
    if (numPoints < 1)
    {
      return;
    }

    FieldType averageValue = vtkm::TypeTraits<FieldType>::ZeroInitialization();
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      averageValue = averageValue + fieldValues[pointIndex];
    }
    averageValue = static_cast<ComponentType>(1.0 / numPoints) * averageValue;

    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      vtkm::Vec3f pcoord;
      CHECK_CALL(vtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, pcoord));
      FieldType interpolatedValue;
      CHECK_CALL(vtkm::exec::CellInterpolate(fieldValues, pcoord, shape, interpolatedValue));

      VTKM_TEST_ASSERT(test_equal(fieldValues[pointIndex], interpolatedValue),
                       "Interpolation at point not point value.");
    }

    vtkm::Vec3f pcoord;
    CHECK_CALL(vtkm::exec::ParametricCoordinatesCenter(numPoints, shape, pcoord));
    FieldType interpolatedValue;
    CHECK_CALL(vtkm::exec::CellInterpolate(fieldValues, pcoord, shape, interpolatedValue));

    VTKM_TEST_ASSERT(test_equal(averageValue, interpolatedValue),
                     "Interpolation at center not average value.");
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape, vtkm::IdComponent numPoints) const
  {
    vtkm::VecVariable<FieldType, MAX_POINTS> fieldValues;
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      FieldType value = TestValue(pointIndex + 1, FieldType());
      fieldValues.Append(value);
    }

    this->DoTestWithField(shape, fieldValues);
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    vtkm::IdComponent minPoints;
    vtkm::IdComponent maxPoints;
    GetMinMaxPoints(
      CellShapeTag(), typename vtkm::CellTraits<CellShapeTag>::IsSizeFixed(), minPoints, maxPoints);

    for (vtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(CellShapeTag(), numPoints);
    }

    vtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (vtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(genericShape, numPoints);
    }
  }
};

void TestInterpolate()
{
  std::cout << "======== Float32 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<vtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<vtkm::Float64>());
  std::cout << "======== Vec<Float32,3> ===================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<vtkm::Vec3f_32>());
  std::cout << "======== Vec<Float64,3> ===================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestInterpolateFunctor<vtkm::Vec3f_64>());

  TestInterpolateFunctor<vtkm::Vec3f> testFunctor;
  vtkm::Vec3f origin = TestValue(0, vtkm::Vec3f());
  vtkm::Vec3f spacing = TestValue(1, vtkm::Vec3f());
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  testFunctor.DoTestWithField(vtkm::CellShapeTagLine(),
                              vtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing));
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  testFunctor.DoTestWithField(vtkm::CellShapeTagQuad(),
                              vtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing));
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctor.DoTestWithField(vtkm::CellShapeTagHexahedron(),
                              vtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing));
}

} // anonymous namespace

int UnitTestCellInterpolate(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestInterpolate, argc, argv);
}
