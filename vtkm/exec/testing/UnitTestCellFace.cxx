//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/exec/CellFace.h>

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/testing/Testing.h>

namespace {

struct TestCellFacesFunctor
{
  template<typename CellShapeTag>
  void DoTest(CellShapeTag shape,
              vtkm::CellTopologicalDimensionsTag<3>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    vtkm::IdComponent numFaces =
        vtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    VTKM_TEST_ASSERT(numFaces > 0, "No faces?");

    for (vtkm::IdComponent faceIndex = 0; faceIndex < numFaces; faceIndex++)
    {
      vtkm::VecCConst<vtkm::IdComponent> facePoints =
          vtkm::exec::CellFaceLocalIndices(faceIndex, shape, workletProxy);
      vtkm::IdComponent numPointsInFace = facePoints.GetNumberOfComponents();
      VTKM_TEST_ASSERT(numPointsInFace >= 3,
                       "Face has fewer points than a triangle.");
      // Currently no face has more than 5 points
      VTKM_TEST_ASSERT(numPointsInFace <= 5,
                       "Face has too many points.");

      for (vtkm::IdComponent pointIndex = 0;
           pointIndex < numPointsInFace;
           pointIndex++)
      {
        VTKM_TEST_ASSERT(facePoints[pointIndex] >= 0,
                         "Invalid point index for face.");
        // Currently no cell has more than 10 points
        VTKM_TEST_ASSERT(facePoints[pointIndex] <= 10,
                         "Invalid point index for face.");
      }
    }
  }

  // Less important case of cells that have less than 3 dimensions (no faces)
  template<typename CellShapeTag, vtkm::IdComponent NumDimensions>
  void DoTest(CellShapeTag shape,
              vtkm::CellTopologicalDimensionsTag<NumDimensions>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    vtkm::IdComponent numFaces =
        vtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    VTKM_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  template<typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    std::cout << "--- Test shape tag directly" << std::endl;
    this->DoTest(
          CellShapeTag(),
          typename vtkm::CellTraits<CellShapeTag>::TopologicalDimensionsTag());

    std::cout << "--- Test generic shape tag" << std::endl;
    this->DoTest(
          vtkm::CellShapeTagGeneric(CellShapeTag::Id),
          typename vtkm::CellTraits<CellShapeTag>::TopologicalDimensionsTag());
  }
};

void TestAllShapes()
{
  vtkm::testing::Testing::TryAllCellShapes(TestCellFacesFunctor());
}

} // anonymous namespace

int UnitTestCellFace(int, char *[])
{
  return vtkm::testing::Testing::Run(TestAllShapes);
}
