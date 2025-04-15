//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/DataSet.h>

#include <viskores/exec/CellEdge.h>

#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

// This worklet computes the sum of the angles of all polygons connected
// to each point. This sum is related (but not equal to) the Gaussian
// curvature of the surface. A flat mesh will have a sum equal to 2 pi.
// A concave or convex surface will have a sum less than 2 pi. A saddle
// will have a sum greater than 2 pi. The actual Gaussian curvature is
// equal to (2 pi - angle sum)/A where A is an area of influence (which
// we are not calculating here). See
// http://computergraphics.stackexchange.com/questions/1718/what-is-the-simplest-way-to-compute-principal-curvature-for-a-mesh-triangle#1721
// or the publication "Discrete Differential-Geometry Operators for
// Triangulated 2-Manifolds" by Mayer et al. (Equation 9).
////
//// BEGIN-EXAMPLE SumOfAngles
////
struct SumOfAngles : viskores::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn inputCells,
                                WholeCellSetIn<>, // Same as inputCells
                                WholeArrayIn pointCoords,
                                FieldOutPoint angleSum);
  using ExecutionSignature = void(CellIndices incidentCells,
                                  InputIndex pointIndex,
                                  _2 cellSet,
                                  _3 pointCoordsPortal,
                                  _4 outSum);
  using InputDomain = _1;

  template<typename IncidentCellVecType,
           typename CellSetType,
           typename PointCoordsPortalType,
           typename SumType>
  VISKORES_EXEC void operator()(const IncidentCellVecType& incidentCells,
                                viskores::Id pointIndex,
                                const CellSetType& cellSet,
                                const PointCoordsPortalType& pointCoordsPortal,
                                SumType& outSum) const
  {
    using CoordType = typename PointCoordsPortalType::ValueType;

    CoordType thisPoint = pointCoordsPortal.Get(pointIndex);

    outSum = 0;
    for (viskores::IdComponent incidentCellIndex = 0;
         incidentCellIndex < incidentCells.GetNumberOfComponents();
         ++incidentCellIndex)
    {
      // Get information about incident cell.
      viskores::Id cellIndex = incidentCells[incidentCellIndex];
      typename CellSetType::CellShapeTag cellShape = cellSet.GetCellShape(cellIndex);
      typename CellSetType::IndicesType cellConnections = cellSet.GetIndices(cellIndex);
      viskores::IdComponent numPointsInCell = cellSet.GetNumberOfIndices(cellIndex);
      viskores::IdComponent numEdges;
      viskores::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);

      // Iterate over all edges and find the first one with pointIndex.
      // Use that to find the first vector.
      viskores::IdComponent edgeIndex = -1;
      CoordType vec1;
      while (true)
      {
        ++edgeIndex;
        if (edgeIndex >= numEdges)
        {
          this->RaiseError("Bad cell. Could not find two incident edges.");
          return;
        }
        viskores::IdComponent2 edge;
        viskores::exec::CellEdgeLocalIndex(
          numPointsInCell, 0, edgeIndex, cellShape, edge[0]);
        viskores::exec::CellEdgeLocalIndex(
          numPointsInCell, 1, edgeIndex, cellShape, edge[1]);
        if (cellConnections[edge[0]] == pointIndex)
        {
          vec1 = pointCoordsPortal.Get(cellConnections[edge[1]]) - thisPoint;
          break;
        }
        else if (cellConnections[edge[1]] == pointIndex)
        {
          vec1 = pointCoordsPortal.Get(cellConnections[edge[0]]) - thisPoint;
          break;
        }
        else
        {
          // Continue to next iteration of loop.
        }
      }

      // Continue iteration over remaining edges and find the second one with
      // pointIndex. Use that to find the second vector.
      CoordType vec2;
      while (true)
      {
        ++edgeIndex;
        if (edgeIndex >= numEdges)
        {
          this->RaiseError("Bad cell. Could not find two incident edges.");
          return;
        }
        viskores::IdComponent2 edge;
        viskores::exec::CellEdgeLocalIndex(
          numPointsInCell, 0, edgeIndex, cellShape, edge[0]);
        viskores::exec::CellEdgeLocalIndex(
          numPointsInCell, 1, edgeIndex, cellShape, edge[1]);
        if (cellConnections[edge[0]] == pointIndex)
        {
          vec2 = pointCoordsPortal.Get(cellConnections[edge[1]]) - thisPoint;
          break;
        }
        else if (cellConnections[edge[1]] == pointIndex)
        {
          vec2 = pointCoordsPortal.Get(cellConnections[edge[0]]) - thisPoint;
          break;
        }
        else
        {
          // Continue to next iteration of loop.
        }
      }

      // The dot product of two unit vectors is equal to the cosine of the
      // angle between them.
      viskores::Normalize(vec1);
      viskores::Normalize(vec2);
      SumType cosine = static_cast<SumType>(viskores::Dot(vec1, vec2));

      outSum += viskores::ACos(cosine);
    }
  }
};
////
//// END-EXAMPLE SumOfAngles
////

VISKORES_CONT
static void TrySumOfAngles()
{
  std::cout << "Read input data" << std::endl;
  viskores::io::VTKDataSetReader reader(
    viskores::cont::testing::Testing::GetTestDataBasePath() + "unstructured/cow.vtk");
  viskores::cont::DataSet dataSet = reader.ReadDataSet();

  std::cout << "Get information out of data" << std::endl;
  viskores::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  auto pointCoordinates = dataSet.GetCoordinateSystem().GetData();

  std::cout << "Run algorithm" << std::endl;
  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<viskores::FloatDefault> angleSums;
  invoker(SumOfAngles{}, cellSet, cellSet, pointCoordinates, angleSums);

  std::cout << "Add field to data set" << std::endl;
  dataSet.AddPointField("angle-sum", angleSums);

  std::cout << "Write result" << std::endl;
  viskores::io::VTKDataSetWriter writer("cow-curvature.vtk");
  writer.WriteDataSet(dataSet);
}

} // anonymous namespace

int GuideExampleSumOfAngles(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TrySumOfAngles, argc, argv);
}
