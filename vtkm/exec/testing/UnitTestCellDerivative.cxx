//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <vtkm/CellTraits.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/VecVariable.h>

#include <vtkm/testing/Testing.h>

#include <ctime>
#include <random>

namespace
{

std::mt19937 g_RandomGenerator;

// Establish simple mapping between world and parametric coordinates.
// Actual world/parametric coordinates are in a different test.
template <typename T>
vtkm::Vec<T, 3> ParametricToWorld(const vtkm::Vec<T, 3>& pcoord)
{
  return T(2) * pcoord - vtkm::Vec<T, 3>(0.25f);
}
template <typename T>
vtkm::Vec<T, 3> WorldToParametric(const vtkm::Vec<T, 3>& wcoord)
{
  return T(0.5) * (wcoord + vtkm::Vec<T, 3>(0.25f));
}

/// Simple structure describing a linear field.  Has a convenience class
/// for getting values.
template <typename FieldType>
struct LinearField
{
  vtkm::Vec<FieldType, 3> Gradient;
  FieldType OriginValue;

  template <typename T>
  FieldType GetValue(vtkm::Vec<T, 3> coordinates) const
  {
    return static_cast<FieldType>((coordinates[0] * this->Gradient[0] +
                                   coordinates[1] * this->Gradient[1] +
                                   coordinates[2] * this->Gradient[2]) +
                                  this->OriginValue);
  }
};

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
struct TestDerivativeFunctor
{
  template <typename CellShapeTag, typename WCoordsVecType>
  void DoTestWithWCoords(CellShapeTag shape,
                         const WCoordsVecType worldCoordinates,
                         LinearField<FieldType> field,
                         vtkm::Vec<FieldType, 3> expectedGradient) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    vtkm::IdComponent numPoints = worldCoordinates.GetNumberOfComponents();

    vtkm::VecVariable<FieldType, MAX_POINTS> fieldValues;
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      vtkm::Vec3f wcoords = worldCoordinates[pointIndex];
      FieldType value = static_cast<FieldType>(field.GetValue(wcoords));
      fieldValues.Append(value);
    }

    std::cout << "    Expected: " << expectedGradient << std::endl;

    std::uniform_real_distribution<vtkm::FloatDefault> randomDist;

    for (vtkm::IdComponent trial = 0; trial < 5; trial++)
    {
      // Generate a random pcoords that we know is in the cell.
      vtkm::Vec3f pcoords(0);
      vtkm::FloatDefault totalWeight = 0;
      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        vtkm::Vec3f pointPcoords =
          vtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, workletProxy);
        VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
        vtkm::FloatDefault weight = randomDist(g_RandomGenerator);
        pcoords = pcoords + weight * pointPcoords;
        totalWeight += weight;
      }
      pcoords = (1 / totalWeight) * pcoords;

      std::cout << "    Test derivative at " << pcoords << std::endl;

      vtkm::Vec<FieldType, 3> computedGradient =
        vtkm::exec::CellDerivative(fieldValues, worldCoordinates, pcoords, shape, workletProxy);
      VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      std::cout << "     Computed: " << computedGradient << std::endl;
      // Note that some gradients (particularly those near the center of
      // polygons with 5 or more points) are not very precise. Thus the
      // tolarance of the test_equal is raised.
      VTKM_TEST_ASSERT(test_equal(computedGradient, expectedGradient, 0.01),
                       "Gradient is not as expected.");
    }
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              vtkm::IdComponent numPoints,
              LinearField<FieldType> field,
              vtkm::Vec<FieldType, 3> expectedGradient) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    vtkm::VecVariable<vtkm::Vec3f, MAX_POINTS> worldCoordinates;
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      vtkm::Vec3f pcoords =
        vtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, shape, workletProxy);
      VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
      vtkm::Vec3f wcoords = ParametricToWorld(pcoords);
      VTKM_TEST_ASSERT(test_equal(pcoords, WorldToParametric(wcoords)),
                       "Test world/parametric conversion broken.");
      worldCoordinates.Append(wcoords);
    }

    this->DoTestWithWCoords(shape, worldCoordinates, field, expectedGradient);
  }

  template <typename CellShapeTag>
  void DoTest(CellShapeTag shape, vtkm::IdComponent numPoints, vtkm::IdComponent topDim) const
  {
    LinearField<FieldType> field;
    vtkm::Vec<FieldType, 3> expectedGradient;

    using FieldTraits = vtkm::VecTraits<FieldType>;
    using FieldComponentType = typename FieldTraits::ComponentType;

    // Correct topDim for polygons with fewer than 3 points.
    if (topDim > numPoints - 1)
    {
      topDim = numPoints - 1;
    }

    std::cout << "Simple field, " << numPoints << " points" << std::endl;
    for (vtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, 0.0);
    }
    field.Gradient = vtkm::make_Vec(FieldType(1.0f), FieldType(1.0f), FieldType(1.0f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Uneven gradient, " << numPoints << " points" << std::endl;
    for (vtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, FieldComponentType(-7.0f));
    }
    field.Gradient = vtkm::make_Vec(FieldType(0.25f), FieldType(14.0f), FieldType(11.125f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Negative gradient directions, " << numPoints << " points" << std::endl;
    for (vtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, FieldComponentType(5.0f));
    }
    field.Gradient = vtkm::make_Vec(FieldType(-11.125f), FieldType(-0.25f), FieldType(14.0f));
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Random linear field, " << numPoints << " points" << std::endl;
    std::uniform_real_distribution<FieldComponentType> randomDist(-20.0f, 20.0f);
    for (vtkm::IdComponent fieldComponent = 0;
         fieldComponent < FieldTraits::GetNumberOfComponents(FieldType());
         fieldComponent++)
    {
      FieldTraits::SetComponent(field.OriginValue, fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[0], fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[1], fieldComponent, randomDist(g_RandomGenerator));
      FieldTraits::SetComponent(field.Gradient[2], fieldComponent, randomDist(g_RandomGenerator));
    }
    expectedGradient[0] = ((topDim > 0) ? field.Gradient[0] : FieldType(0));
    expectedGradient[1] = ((topDim > 1) ? field.Gradient[1] : FieldType(0));
    expectedGradient[2] = ((topDim > 2) ? field.Gradient[2] : FieldType(0));
    this->DoTest(shape, numPoints, field, expectedGradient);
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    vtkm::IdComponent minPoints;
    vtkm::IdComponent maxPoints;
    GetMinMaxPoints(
      CellShapeTag(), typename vtkm::CellTraits<CellShapeTag>::IsSizeFixed(), minPoints, maxPoints);

    std::cout << "--- Test shape tag directly" << std::endl;
    for (vtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(
        CellShapeTag(), numPoints, vtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    vtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (vtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      this->DoTest(genericShape, numPoints, vtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }
  }

  void operator()(vtkm::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No derivative." << std::endl;
  }
};

void TestDerivative()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<vtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<vtkm::Float64>());
  std::cout << "======== Vec<Float32,3> ===================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<vtkm::Vec3f_32>());
  std::cout << "======== Vec<Float64,3> ===================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<vtkm::Vec3f_64>());

  std::uniform_real_distribution<vtkm::Float64> randomDist(-20.0, 20.0);
  vtkm::Vec3f origin = vtkm::Vec3f(0.25f, 0.25f, 0.25f);
  vtkm::Vec3f spacing = vtkm::Vec3f(2.0f, 2.0f, 2.0f);

  LinearField<vtkm::Float64> scalarField;
  scalarField.OriginValue = randomDist(g_RandomGenerator);
  scalarField.Gradient = vtkm::make_Vec(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  vtkm::Vec3f_64 expectedScalarGradient = scalarField.Gradient;

  TestDerivativeFunctor<vtkm::Float64> testFunctorScalar;
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctorScalar.DoTestWithWCoords(vtkm::CellShapeTagHexahedron(),
                                      vtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedScalarGradient[2] = 0.0;
  testFunctorScalar.DoTestWithWCoords(vtkm::CellShapeTagQuad(),
                                      vtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedScalarGradient[1] = 0.0;
  testFunctorScalar.DoTestWithWCoords(vtkm::CellShapeTagLine(),
                                      vtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing),
                                      scalarField,
                                      expectedScalarGradient);

  LinearField<vtkm::Vec3f_64> vectorField;
  vectorField.OriginValue = vtkm::make_Vec(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  vectorField.Gradient = vtkm::make_Vec(
    vtkm::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)),
    vtkm::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)),
    vtkm::make_Vec(
      randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator)));
  vtkm::Vec<vtkm::Vec3f_64, 3> expectedVectorGradient = vectorField.Gradient;

  TestDerivativeFunctor<vtkm::Vec3f_64> testFunctorVector;
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctorVector.DoTestWithWCoords(vtkm::CellShapeTagHexahedron(),
                                      vtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedVectorGradient[2] = vtkm::Vec3f_64(0.0);
  testFunctorVector.DoTestWithWCoords(vtkm::CellShapeTagQuad(),
                                      vtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedVectorGradient[1] = vtkm::Vec3f_64(0.0);
  testFunctorVector.DoTestWithWCoords(vtkm::CellShapeTagLine(),
                                      vtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing),
                                      vectorField,
                                      expectedVectorGradient);
}

} // anonymous namespace

int UnitTestCellDerivative(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestDerivative, argc, argv);
}
