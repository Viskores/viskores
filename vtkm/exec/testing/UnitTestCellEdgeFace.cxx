//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/CellEdge.h>
#include <vtkm/exec/CellFace.h>

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/testing/Testing.h>

#include <set>
#include <vector>

namespace
{

using EdgeType = vtkm::IdComponent2;

void MakeEdgeCanonical(EdgeType& edge)
{
  if (edge[1] < edge[0])
  {
    std::swap(edge[0], edge[1]);
  }
}

struct TestCellFacesFunctor
{
  template <typename CellShapeTag>
  void DoTest(vtkm::IdComponent numPoints,
              CellShapeTag shape,
              vtkm::CellTopologicalDimensionsTag<3>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::vector<vtkm::Id> pointIndexProxyBuffer(static_cast<std::size_t>(numPoints));
    for (std::size_t index = 0; index < pointIndexProxyBuffer.size(); ++index)
    {
      pointIndexProxyBuffer[index] = static_cast<vtkm::Id>(1000000 - index);
    }
    vtkm::VecCConst<vtkm::Id> pointIndexProxy(&pointIndexProxyBuffer.at(0), numPoints);

    vtkm::IdComponent numEdges = vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, workletProxy);
    VTKM_TEST_ASSERT(numEdges > 0, "No edges?");

    std::set<EdgeType> edgeSet;
    for (vtkm::IdComponent edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
      EdgeType edge(vtkm::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, workletProxy),
                    vtkm::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, workletProxy));
      VTKM_TEST_ASSERT(edge[0] >= 0, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[0] < numPoints, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[1] >= 0, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[1] < numPoints, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[0] != edge[1], "Degenerate edge.");
      MakeEdgeCanonical(edge);
      VTKM_TEST_ASSERT(edge[0] < edge[1], "Internal test error: MakeEdgeCanonical failed");
      VTKM_TEST_ASSERT(edgeSet.find(edge) == edgeSet.end(), "Found duplicate edge");
      edgeSet.insert(edge);

      vtkm::Id2 canonicalEdgeId =
        vtkm::exec::CellEdgeCanonicalId(numPoints, edgeIndex, shape, pointIndexProxy, workletProxy);
      VTKM_TEST_ASSERT(canonicalEdgeId[0] > 0, "Not using global ids?");
      VTKM_TEST_ASSERT(canonicalEdgeId[0] < canonicalEdgeId[1], "Bad order.");
    }

    vtkm::IdComponent numFaces = vtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    VTKM_TEST_ASSERT(numFaces > 0, "No faces?");

    std::set<EdgeType> edgesFoundInFaces;
    for (vtkm::IdComponent faceIndex = 0; faceIndex < numFaces; faceIndex++)
    {
      const vtkm::IdComponent numPointsInFace =
        vtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, workletProxy);

      VTKM_TEST_ASSERT(numPointsInFace >= 3, "Face has fewer points than a triangle.");

      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPointsInFace; pointIndex++)
      {
        vtkm::IdComponent localFaceIndex =
          vtkm::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, workletProxy);
        VTKM_TEST_ASSERT(localFaceIndex >= 0, "Invalid point index for face.");
        VTKM_TEST_ASSERT(localFaceIndex < numPoints, "Invalid point index for face.");
        EdgeType edge;
        if (pointIndex < numPointsInFace - 1)
        {
          edge = EdgeType(
            vtkm::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, workletProxy),
            vtkm::exec::CellFaceLocalIndex(pointIndex + 1, faceIndex, shape, workletProxy));
        }
        else
        {
          edge =
            EdgeType(vtkm::exec::CellFaceLocalIndex(0, faceIndex, shape, workletProxy),
                     vtkm::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, workletProxy));
        }
        MakeEdgeCanonical(edge);
        VTKM_TEST_ASSERT(edgeSet.find(edge) != edgeSet.end(), "Edge in face not in cell's edges");
        edgesFoundInFaces.insert(edge);
      }

      vtkm::Id3 canonicalFaceId =
        vtkm::exec::CellFaceCanonicalId(faceIndex, shape, pointIndexProxy, workletProxy);
      VTKM_TEST_ASSERT(canonicalFaceId[0] > 0, "Not using global ids?");
      VTKM_TEST_ASSERT(canonicalFaceId[0] < canonicalFaceId[1], "Bad order.");
      VTKM_TEST_ASSERT(canonicalFaceId[1] < canonicalFaceId[2], "Bad order.");
    }
    VTKM_TEST_ASSERT(edgesFoundInFaces.size() == edgeSet.size(),
                     "Faces did not contain all edges in cell");
  }

  // Case of cells that have 2 dimensions (no faces)
  template <typename CellShapeTag>
  void DoTest(vtkm::IdComponent numPoints,
              CellShapeTag shape,
              vtkm::CellTopologicalDimensionsTag<2>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::vector<vtkm::Id> pointIndexProxyBuffer(static_cast<std::size_t>(numPoints));
    for (std::size_t index = 0; index < pointIndexProxyBuffer.size(); ++index)
    {
      pointIndexProxyBuffer[index] = static_cast<vtkm::Id>(1000000 - index);
    }
    vtkm::VecCConst<vtkm::Id> pointIndexProxy(&pointIndexProxyBuffer.at(0), numPoints);

    vtkm::IdComponent numEdges = vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, workletProxy);
    VTKM_TEST_ASSERT(numEdges == numPoints, "Polygons should have same number of points and edges");

    std::set<EdgeType> edgeSet;
    for (vtkm::IdComponent edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
      EdgeType edge(vtkm::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, workletProxy),
                    vtkm::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, workletProxy));
      VTKM_TEST_ASSERT(edge[0] >= 0, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[0] < numPoints, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[1] >= 0, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[1] < numPoints, "Bad index in edge.");
      VTKM_TEST_ASSERT(edge[0] != edge[1], "Degenerate edge.");
      MakeEdgeCanonical(edge);
      VTKM_TEST_ASSERT(edge[0] < edge[1], "Internal test error: MakeEdgeCanonical failed");
      VTKM_TEST_ASSERT(edgeSet.find(edge) == edgeSet.end(), "Found duplicate edge");
      edgeSet.insert(edge);

      vtkm::Id2 canonicalEdgeId =
        vtkm::exec::CellEdgeCanonicalId(numPoints, edgeIndex, shape, pointIndexProxy, workletProxy);
      VTKM_TEST_ASSERT(canonicalEdgeId[0] > 0, "Not using global ids?");
      VTKM_TEST_ASSERT(canonicalEdgeId[0] < canonicalEdgeId[1], "Bad order.");
    }

    vtkm::IdComponent numFaces = vtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    VTKM_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  // Less important case of cells that have less than 2 dimensions
  // (no faces or edges)
  template <typename CellShapeTag, vtkm::IdComponent NumDimensions>
  void DoTest(vtkm::IdComponent numPoints,
              CellShapeTag shape,
              vtkm::CellTopologicalDimensionsTag<NumDimensions>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    vtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    vtkm::IdComponent numEdges = vtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, workletProxy);
    VTKM_TEST_ASSERT(numEdges == 0, "0D or 1D shape should have no edges");

    vtkm::IdComponent numFaces = vtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    VTKM_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  template <typename CellShapeTag>
  void TryShapeWithNumPoints(vtkm::IdComponent numPoints, CellShapeTag) const
  {
    std::cout << "--- Test shape tag directly"
              << " (" << numPoints << " points)" << std::endl;
    this->DoTest(numPoints,
                 CellShapeTag(),
                 typename vtkm::CellTraits<CellShapeTag>::TopologicalDimensionsTag());

    std::cout << "--- Test generic shape tag"
              << " (" << numPoints << " points)" << std::endl;
    this->DoTest(numPoints,
                 vtkm::CellShapeTagGeneric(CellShapeTag::Id),
                 typename vtkm::CellTraits<CellShapeTag>::TopologicalDimensionsTag());
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    this->TryShapeWithNumPoints(vtkm::CellTraits<CellShapeTag>::NUM_POINTS, CellShapeTag());
  }

  void operator()(vtkm::CellShapeTagPolyLine) const
  {
    for (vtkm::IdComponent numPoints = 3; numPoints < 7; numPoints++)
    {
      this->TryShapeWithNumPoints(numPoints, vtkm::CellShapeTagPolyLine());
    }
  }

  void operator()(vtkm::CellShapeTagPolygon) const
  {
    for (vtkm::IdComponent numPoints = 3; numPoints < 7; numPoints++)
    {
      this->TryShapeWithNumPoints(numPoints, vtkm::CellShapeTagPolygon());
    }
  }
};

void TestAllShapes()
{
  vtkm::testing::Testing::TryAllCellShapes(TestCellFacesFunctor());
}

} // anonymous namespace

int UnitTestCellEdgeFace(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestAllShapes, argc, argv);
}
