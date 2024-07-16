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

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>

#include <vtkm/exec/CellEdge.h>

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/cont/testing/Testing.h>

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
struct SumOfAngles : vtkm::worklet::WorkletVisitPointsWithCells
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
  VTKM_EXEC void operator()(const IncidentCellVecType& incidentCells,
                            vtkm::Id pointIndex,
                            const CellSetType& cellSet,
                            const PointCoordsPortalType& pointCoordsPortal,
                            SumType& outSum) const
  {
    using CoordType = typename PointCoordsPortalType::ValueType;

    CoordType thisPoint = pointCoordsPortal.Get(pointIndex);

    outSum = 0;
    for (vtkm::IdComponent incidentCellIndex = 0;
         incidentCellIndex < incidentCells.GetNumberOfComponents();
         ++incidentCellIndex)
    {
      // Get information about incident cell.
      vtkm::Id cellIndex = incidentCells[incidentCellIndex];
      typename CellSetType::CellShapeTag cellShape = cellSet.GetCellShape(cellIndex);
      typename CellSetType::IndicesType cellConnections = cellSet.GetIndices(cellIndex);
      vtkm::IdComponent numPointsInCell = cellSet.GetNumberOfIndices(cellIndex);
      vtkm::IdComponent numEdges;
      vtkm::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);

      // Iterate over all edges and find the first one with pointIndex.
      // Use that to find the first vector.
      vtkm::IdComponent edgeIndex = -1;
      CoordType vec1;
      while (true)
      {
        ++edgeIndex;
        if (edgeIndex >= numEdges)
        {
          this->RaiseError("Bad cell. Could not find two incident edges.");
          return;
        }
        vtkm::IdComponent2 edge;
        vtkm::exec::CellEdgeLocalIndex(
          numPointsInCell, 0, edgeIndex, cellShape, edge[0]);
        vtkm::exec::CellEdgeLocalIndex(
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
        vtkm::IdComponent2 edge;
        vtkm::exec::CellEdgeLocalIndex(
          numPointsInCell, 0, edgeIndex, cellShape, edge[0]);
        vtkm::exec::CellEdgeLocalIndex(
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
      vtkm::Normalize(vec1);
      vtkm::Normalize(vec2);
      SumType cosine = static_cast<SumType>(vtkm::Dot(vec1, vec2));

      outSum += vtkm::ACos(cosine);
    }
  }
};
////
//// END-EXAMPLE SumOfAngles
////

VTKM_CONT
static void TrySumOfAngles()
{
  std::cout << "Read input data" << std::endl;
  vtkm::io::VTKDataSetReader reader(vtkm::cont::testing::Testing::GetTestDataBasePath() +
                                    "unstructured/cow.vtk");
  vtkm::cont::DataSet dataSet = reader.ReadDataSet();

  std::cout << "Get information out of data" << std::endl;
  vtkm::cont::CellSetExplicit<> cellSet;
  dataSet.GetCellSet().AsCellSet(cellSet);

  auto pointCoordinates = dataSet.GetCoordinateSystem().GetData();

  std::cout << "Run algorithm" << std::endl;
  vtkm::cont::Invoker invoker;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> angleSums;
  invoker(SumOfAngles{}, cellSet, cellSet, pointCoordinates, angleSums);

  std::cout << "Add field to data set" << std::endl;
  dataSet.AddPointField("angle-sum", angleSums);

  std::cout << "Write result" << std::endl;
  vtkm::io::VTKDataSetWriter writer("cow-curvature.vtk");
  writer.WriteDataSet(dataSet);
}

} // anonymous namespace

int GuideExampleSumOfAngles(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TrySumOfAngles, argc, argv);
}
