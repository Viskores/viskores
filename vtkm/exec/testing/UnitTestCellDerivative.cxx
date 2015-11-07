//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <vtkm/CellTraits.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/VecVariable.h>

#include <vtkm/testing/Testing.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/static_assert.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <time.h>

namespace {

boost::mt19937 g_RandomGenerator;

// Establish simple mapping between world and parametric coordinates.
// Actuall world/parametric coordinates are in a different test.
template<typename T>
vtkm::Vec<T,3> ParametricToWorld(const vtkm::Vec<T,3> &pcoord)
{
  return T(2)*pcoord - vtkm::Vec<T,3>(0.25f);
}
template<typename T>
vtkm::Vec<T,3> WorldToParametric(const vtkm::Vec<T,3> &wcoord)
{
  return T(0.5)*(wcoord + vtkm::Vec<T,3>(0.25f));
}

/// Simple structure describing a linear field.  Has a convienience class
/// for getting values.
struct LinearField {
  vtkm::Vec<vtkm::Float64,3> Gradient;
  vtkm::Float64 OriginValue;

  template<typename T>
  T GetValue(vtkm::Vec<T,3> coordinates) const {
    return vtkm::dot(coordinates,
                     vtkm::Vec<T,3>(this->Gradient)) + T(this->OriginValue);
  }
};

static const vtkm::IdComponent MAX_POINTS = 8;

template<typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     vtkm::CellTraitsTagSizeFixed,
                     vtkm::IdComponent &minPoints,
                     vtkm::IdComponent &maxPoints)
{
  // If this line fails, then MAX_POINTS is not large enough to support all
  // cell shapes.
  VTKM_STATIC_ASSERT((vtkm::CellTraits<CellShapeTag>::NUM_POINTS <= MAX_POINTS));
  minPoints = maxPoints = vtkm::CellTraits<CellShapeTag>::NUM_POINTS;
}

template<typename CellShapeTag>
void GetMinMaxPoints(CellShapeTag,
                     vtkm::CellTraitsTagSizeVariable,
                     vtkm::IdComponent &minPoints,
                     vtkm::IdComponent &maxPoints)
{
  minPoints = 1;
  maxPoints = MAX_POINTS;
}

template<typename FieldType>
struct TestDerivativeFunctor
{
  template<typename CellShapeTag, typename WCoordsVecType>
  void DoTestWithWCoords(CellShapeTag shape,
                         const WCoordsVecType worldCoordinates,
                         LinearField field,
                         vtkm::Vec<FieldType,3> expectedGradient) const
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
      vtkm::Vec<vtkm::FloatDefault,3> wcoords = worldCoordinates[pointIndex];
      FieldType value = static_cast<FieldType>(field.GetValue(wcoords));
      fieldValues.Append(value);
    }

    std::cout << "    Expected: " << expectedGradient << std::endl;

    boost::random::uniform_real_distribution<vtkm::FloatDefault> randomDist;

    for (vtkm::IdComponent trial = 0; trial < 5; trial++)
    {
      // Generate a random pcoords that we know is in the cell.
      vtkm::Vec<vtkm::FloatDefault,3> pcoords(0);
      vtkm::FloatDefault totalWeight = 0;
      for (vtkm::IdComponent pointIndex = 0;
           pointIndex < numPoints;
           pointIndex++)
      {
        vtkm::Vec<vtkm::FloatDefault,3> pointPcoords =
            vtkm::exec::ParametricCoordinatesPoint(numPoints,
                                                   pointIndex,
                                                   shape,
                                                   workletProxy);
        VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
        vtkm::FloatDefault weight = randomDist(g_RandomGenerator);
        pcoords = pcoords + weight*pointPcoords;
        totalWeight += weight;
      }
      pcoords = (1/totalWeight)*pcoords;

      std::cout << "    Test derivative at " << pcoords << std::endl;

      vtkm::Vec<FieldType,3> computedGradient =
          vtkm::exec::CellDerivative(fieldValues,
                                     worldCoordinates,
                                     pcoords,
                                     shape,
                                     workletProxy);
      VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      std::cout << "     Computed: " << computedGradient << std::endl;
      // Note that some gradients (particularly those near the center of
      // polygons with 5 or more points) are not very precise. Thus the
      // tolarance of the test_equal is raised.
      VTKM_TEST_ASSERT(test_equal(computedGradient, expectedGradient, 0.01),
                       "Gradient is not as expected.");
    }
  }

  template<typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              vtkm::IdComponent numPoints,
              LinearField field,
              vtkm::Vec<FieldType,3> expectedGradient) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    vtkm::VecVariable<vtkm::Vec<vtkm::FloatDefault,3>, MAX_POINTS> worldCoordinates;
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      vtkm::Vec<vtkm::FloatDefault,3> pcoords =
          vtkm::exec::ParametricCoordinatesPoint(numPoints,
                                                 pointIndex,
                                                 shape,
                                                 workletProxy);
      VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
      vtkm::Vec<vtkm::FloatDefault,3> wcoords = ParametricToWorld(pcoords);
      VTKM_TEST_ASSERT(test_equal(pcoords, WorldToParametric(wcoords)),
                       "Test world/parametric conversion broken.");
      worldCoordinates.Append(wcoords);
    }

    this->DoTestWithWCoords(shape, worldCoordinates, field, expectedGradient);
  }

  template<typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              vtkm::IdComponent numPoints,
              vtkm::IdComponent topDim) const
  {
    LinearField field;
    vtkm::Vec<FieldType,3> expectedGradient;

    // Correct topDim for polygons with fewer than 3 points.
    if (topDim > numPoints-1)
    {
      topDim = numPoints-1;
    }

    std::cout << "Simple field, " << numPoints << " points" << std::endl;
    field.OriginValue = 0.0;
    field.Gradient = vtkm::make_Vec(1.0, 1.0, 1.0);
    expectedGradient[0] = static_cast<FieldType>((topDim > 0) ? field.Gradient[0] : 0);
    expectedGradient[1] = static_cast<FieldType>((topDim > 1) ? field.Gradient[1] : 0);
    expectedGradient[2] = static_cast<FieldType>((topDim > 2) ? field.Gradient[2] : 0);
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Uneven gradient, " << numPoints << " points" << std::endl;
    field.OriginValue = -7.0;
    field.Gradient = vtkm::make_Vec(0.25, 14.0, 11.125);
    expectedGradient[0] = static_cast<FieldType>((topDim > 0) ? field.Gradient[0] : 0);
    expectedGradient[1] = static_cast<FieldType>((topDim > 1) ? field.Gradient[1] : 0);
    expectedGradient[2] = static_cast<FieldType>((topDim > 2) ? field.Gradient[2] : 0);
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Negative gradient directions, " << numPoints << " points" << std::endl;
    field.OriginValue = 5.0;
    field.Gradient = vtkm::make_Vec(-11.125, -0.25, 14.0);
    expectedGradient[0] = static_cast<FieldType>((topDim > 0) ? field.Gradient[0] : 0);
    expectedGradient[1] = static_cast<FieldType>((topDim > 1) ? field.Gradient[1] : 0);
    expectedGradient[2] = static_cast<FieldType>((topDim > 2) ? field.Gradient[2] : 0);
    this->DoTest(shape, numPoints, field, expectedGradient);

    std::cout << "Random linear field, " << numPoints << " points" << std::endl;
    boost::random::uniform_real_distribution<vtkm::Float64> randomDist(-20.0, 20.0);
    field.OriginValue = randomDist(g_RandomGenerator);
    field.Gradient = vtkm::make_Vec(randomDist(g_RandomGenerator),
                                    randomDist(g_RandomGenerator),
                                    randomDist(g_RandomGenerator));
    expectedGradient[0] = static_cast<FieldType>((topDim > 0) ? field.Gradient[0] : 0);
    expectedGradient[1] = static_cast<FieldType>((topDim > 1) ? field.Gradient[1] : 0);
    expectedGradient[2] = static_cast<FieldType>((topDim > 2) ? field.Gradient[2] : 0);
    this->DoTest(shape, numPoints, field, expectedGradient);
  }

  template<typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    vtkm::IdComponent minPoints;
    vtkm::IdComponent maxPoints;
    GetMinMaxPoints(CellShapeTag(),
                    typename vtkm::CellTraits<CellShapeTag>::IsSizeFixed(),
                    minPoints,
                    maxPoints);

    std::cout << "--- Test shape tag directly" << std::endl;
    for (vtkm::IdComponent numPoints = minPoints;
         numPoints <= maxPoints;
         numPoints++)
    {
      this->DoTest(CellShapeTag(),
                   numPoints,
                   vtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    vtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (vtkm::IdComponent numPoints = minPoints;
         numPoints <= maxPoints;
         numPoints++)
    {
      this->DoTest(genericShape,
                   numPoints,
                   vtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS);
    }
  }

  void operator()(vtkm::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No derivative." << std::endl;
  }
};

void TestDerivative()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(time(NULL));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<vtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestDerivativeFunctor<vtkm::Float64>());

  boost::random::uniform_real_distribution<vtkm::Float64> randomDist(-20.0, 20.0);
  LinearField field;
  field.OriginValue = randomDist(g_RandomGenerator);
  field.Gradient = vtkm::make_Vec(randomDist(g_RandomGenerator),
                                  randomDist(g_RandomGenerator),
                                  randomDist(g_RandomGenerator));
  vtkm::Vec<vtkm::Float64,3> expectedGradient = field.Gradient;

  TestDerivativeFunctor<vtkm::Float64> testFunctor;
  vtkm::Vec<vtkm::FloatDefault,3> origin = vtkm::Vec<vtkm::FloatDefault,3>(0.25f, 0.25f, 0.25f);
  vtkm::Vec<vtkm::FloatDefault,3> spacing = vtkm::Vec<vtkm::FloatDefault,3>(2.0f, 2.0f, 2.0f);
  std::cout << "======== Uniform Point Coordinates 3D =====" << std::endl;
  testFunctor.DoTestWithWCoords(
        vtkm::CellShapeTagHexahedron(),
        vtkm::VecRectilinearPointCoordinates<3>(origin, spacing),
        field,
        expectedGradient);
  std::cout << "======== Uniform Point Coordinates 2D =====" << std::endl;
  expectedGradient[2] = 0.0;
  testFunctor.DoTestWithWCoords(
        vtkm::CellShapeTagQuad(),
        vtkm::VecRectilinearPointCoordinates<2>(origin, spacing),
        field,
        expectedGradient);
  std::cout << "======== Uniform Point Coordinates 1D =====" << std::endl;
  expectedGradient[1] = 0.0;
  testFunctor.DoTestWithWCoords(
        vtkm::CellShapeTagLine(),
        vtkm::VecRectilinearPointCoordinates<1>(origin, spacing),
        field,
        expectedGradient);
}

} // anonymous namespace

int UnitTestCellDerivative(int, char *[])
{
  return vtkm::testing::Testing::Run(TestDerivative);
}
