//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/FunctorBase.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/CellTraits.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/VecVariable.h>

#include <vtkm/testing/Testing.h>

#include <ctime>
#include <random>

namespace
{

std::mt19937 g_RandomGenerator;

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

template <typename PointWCoordsType, typename T, typename CellShapeTag>
static void CompareCoordinates(const PointWCoordsType& pointWCoords,
                               vtkm::Vec<T, 3> truePCoords,
                               vtkm::Vec<T, 3> trueWCoords,
                               CellShapeTag shape)
{
  using Vector3 = vtkm::Vec<T, 3>;

  // Stuff to fake running in the execution environment.
  char messageBuffer[256];
  messageBuffer[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
  vtkm::exec::FunctorBase workletProxy;
  workletProxy.SetErrorMessageBuffer(errorMessage);

  Vector3 computedWCoords = vtkm::exec::ParametricCoordinatesToWorldCoordinates(
    pointWCoords, truePCoords, shape, workletProxy);
  VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
  VTKM_TEST_ASSERT(test_equal(computedWCoords, trueWCoords, 0.01),
                   "Computed wrong world coords from parametric coords.");

  bool success = false;
  Vector3 computedPCoords = vtkm::exec::WorldCoordinatesToParametricCoordinates(
    pointWCoords, trueWCoords, shape, success, workletProxy);
  VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
  VTKM_TEST_ASSERT(test_equal(computedPCoords, truePCoords, 0.01),
                   "Computed wrong parametric coords from world coords.");
}

template <typename PointWCoordsType, typename CellShapeTag>
void TestPCoordsSpecial(const PointWCoordsType& pointWCoords, CellShapeTag shape)
{
  using Vector3 = typename PointWCoordsType::ComponentType;
  using T = typename Vector3::ComponentType;

  // Stuff to fake running in the execution environment.
  char messageBuffer[256];
  messageBuffer[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
  vtkm::exec::FunctorBase workletProxy;
  workletProxy.SetErrorMessageBuffer(errorMessage);

  const vtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();

  std::cout << "    Test parametric coordinates at cell nodes." << std::endl;
  for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
  {
    Vector3 pcoords;
    vtkm::exec::ParametricCoordinatesPoint(numPoints, pointIndex, pcoords, shape, workletProxy);
    VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
    Vector3 wcoords = pointWCoords[pointIndex];
    CompareCoordinates(pointWCoords, pcoords, wcoords, shape);
  }

  {
    std::cout << "    Test parametric coordinates at cell center." << std::endl;
    Vector3 wcoords = pointWCoords[0];
    for (vtkm::IdComponent pointIndex = 1; pointIndex < numPoints; pointIndex++)
    {
      wcoords = wcoords + pointWCoords[pointIndex];
    }
    wcoords = wcoords / Vector3(T(numPoints));

    Vector3 pcoords;
    vtkm::exec::ParametricCoordinatesCenter(numPoints, pcoords, shape, workletProxy);
    CompareCoordinates(pointWCoords, pcoords, wcoords, shape);
  }
}

template <typename PointWCoordsType, typename CellShapeTag>
void TestPCoordsSample(const PointWCoordsType& pointWCoords, CellShapeTag shape)
{
  using Vector3 = typename PointWCoordsType::ComponentType;

  // Stuff to fake running in the execution environment.
  char messageBuffer[256];
  messageBuffer[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
  vtkm::exec::FunctorBase workletProxy;
  workletProxy.SetErrorMessageBuffer(errorMessage);

  const vtkm::IdComponent numPoints = pointWCoords.GetNumberOfComponents();

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

    std::cout << "    Test parametric coordinates at " << pcoords << std::endl;

    // If you convert to world coordinates and back, you should get the
    // same value.
    Vector3 wcoords = vtkm::exec::ParametricCoordinatesToWorldCoordinates(
      pointWCoords, pcoords, shape, workletProxy);
    VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);
    bool success = false;
    Vector3 computedPCoords = vtkm::exec::WorldCoordinatesToParametricCoordinates(
      pointWCoords, wcoords, shape, success, workletProxy);
    VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

    VTKM_TEST_ASSERT(test_equal(pcoords, computedPCoords, 0.05),
                     "pcoord/wcoord transform not symmetrical");
  }
}

template <typename PointWCoordsType, typename CellShellTag>
static void TestPCoords(const PointWCoordsType& pointWCoords, CellShellTag shape)
{
  TestPCoordsSpecial(pointWCoords, shape);
  TestPCoordsSample(pointWCoords, shape);
}

template <typename T>
struct TestPCoordsFunctor
{
  using Vector3 = vtkm::Vec<T, 3>;
  using PointWCoordType = vtkm::VecVariable<Vector3, MAX_POINTS>;

  template <typename CellShapeTag>
  PointWCoordType MakePointWCoords(CellShapeTag, vtkm::IdComponent numPoints) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::uniform_real_distribution<T> randomDist(-1, 1);

    Vector3 sheerVec(randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), 0);

    PointWCoordType pointWCoords;
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      Vector3 pcoords;
      vtkm::exec::ParametricCoordinatesPoint(
        numPoints, pointIndex, pcoords, CellShapeTag(), workletProxy);
      VTKM_TEST_ASSERT(!errorMessage.IsErrorRaised(), messageBuffer);

      Vector3 wCoords = Vector3(pcoords[0], pcoords[1], pcoords[2] + vtkm::Dot(pcoords, sheerVec));
      pointWCoords.Append(wCoords);
    }

    return pointWCoords;
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
      TestPCoords(this->MakePointWCoords(CellShapeTag(), numPoints), CellShapeTag());
    }

    std::cout << "--- Test generic shape tag" << std::endl;
    vtkm::CellShapeTagGeneric genericShape(CellShapeTag::Id);
    for (vtkm::IdComponent numPoints = minPoints; numPoints <= maxPoints; numPoints++)
    {
      TestPCoords(this->MakePointWCoords(CellShapeTag(), numPoints), genericShape);
    }
  }

  void operator()(vtkm::CellShapeTagEmpty) const
  {
    std::cout << "Skipping empty cell shape. No points." << std::endl;
  }
};

void TestAllPCoords()
{
  vtkm::UInt32 seed = static_cast<vtkm::UInt32>(std::time(nullptr));
  std::cout << "Seed: " << seed << std::endl;
  g_RandomGenerator.seed(seed);

  std::cout << "======== Float32 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestPCoordsFunctor<vtkm::Float32>());
  std::cout << "======== Float64 ==========================" << std::endl;
  vtkm::testing::Testing::TryAllCellShapes(TestPCoordsFunctor<vtkm::Float64>());

  std::cout << "======== Rectilinear Shapes ===============" << std::endl;
  std::uniform_real_distribution<vtkm::FloatDefault> randomDist(0.01f, 1.0f);
  vtkm::Vec3f origin(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));
  vtkm::Vec3f spacing(
    randomDist(g_RandomGenerator), randomDist(g_RandomGenerator), randomDist(g_RandomGenerator));

  TestPCoords(vtkm::VecAxisAlignedPointCoordinates<3>(origin, spacing),
              vtkm::CellShapeTagHexahedron());
  TestPCoords(vtkm::VecAxisAlignedPointCoordinates<2>(origin, spacing), vtkm::CellShapeTagQuad());
  TestPCoords(vtkm::VecAxisAlignedPointCoordinates<1>(origin, spacing), vtkm::CellShapeTagLine());
}

} // Anonymous namespace

int UnitTestParametricCoordinates(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestAllPCoords, argc, argv);
}
