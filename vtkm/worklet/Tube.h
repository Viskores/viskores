//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_tube_h
#define vtk_m_worklet_tube_h

#include <typeinfo>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{

class Tube
{
public:
  //Helper worklet to count various things in each polyline.
  class CountSegments : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    VTKM_CONT
    CountSegments(const bool& capping, const vtkm::Id& n)
      : Capping(capping)
      , NumSides(n)
      , NumVertsPerCell(3)
    {
    }

    using ControlSignature = void(CellSetIn,
                                  FieldOut ptsPerPolyline,
                                  FieldOut ptsPerTube,
                                  FieldOut numTubeConnIds);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 ptsPerPolyline,
                                    _3 ptsPerTube,
                                    _4 numTubeConnIds);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              vtkm::Id& ptsPerPolyline,
                              vtkm::Id& ptsPerTube,
                              vtkm::Id& numTubeConnIds) const
    {
      // We only support polylines that contain 2 or more points.
      if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE && numPoints > 1)
      {
        ptsPerPolyline = numPoints;
        ptsPerTube = this->NumSides * numPoints;
        // (two tris per segment) X (numSides) X numVertsPerCell
        numTubeConnIds = (numPoints - 1) * 2 * this->NumSides * this->NumVertsPerCell;

        //Capping adds center vertex in middle of cap, plus NumSides triangles for cap.
        if (this->Capping)
        {
          ptsPerTube += 2;
          numTubeConnIds += (2 * this->NumSides * this->NumVertsPerCell);
        }
      }
      else
      {
        ptsPerPolyline = 0;
        ptsPerTube = 0;
        numTubeConnIds = 0;
      }
    }

  private:
    bool Capping;
    vtkm::Id NumSides;
    vtkm::Id NumVertsPerCell;
  };

  //Helper worklet to generate normals at each point in the polyline.
  class GenerateNormals : public vtkm::worklet::WorkletMapPointToCell
  {
    static constexpr vtkm::FloatDefault vecMagnitudeEps = static_cast<vtkm::FloatDefault>(1e-3);

  public:
    VTKM_CONT
    GenerateNormals()
      : DefaultNorm(0, 0, 1)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  FieldInTo polylineOffset,
                                  WholeArrayOut newNormals);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 polylineOffset,
                                    _4 outNormals);
    using InputDomain = _1;


    template <typename InPointsType, typename PointIndexType>
    VTKM_EXEC vtkm::IdComponent FindValidSegment(const InPointsType& inPts,
                                                 const PointIndexType& ptIndices,
                                                 const vtkm::IdComponent& numPoints,
                                                 vtkm::IdComponent start) const
    {
      auto ps = inPts.Get(ptIndices[start]);
      vtkm::IdComponent end = start + 1;
      while (end < numPoints)
      {
        auto pe = inPts.Get(ptIndices[end]);
        if (vtkm::Magnitude(pe - ps) > 0)
          return end - 1;
        end++;
      }

      return numPoints;
    }

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename OutNormalType>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              const PointIndexType& ptIndices,
                              const InPointsType& inPts,
                              const vtkm::Id& polylineOffset,
                              OutNormalType& outNormals) const
    {
      //Ignore non-polyline and polyline with less than 2 points.
      if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        //The following follows the VTK implementation in:
        //vtkPolyLine::GenerateSlidingNormals
        vtkm::Vec3f sPrev, sNext, normal, p0, p1;
        vtkm::IdComponent sNextId = FindValidSegment(inPts, ptIndices, numPoints, 0);

        if (sNextId != numPoints) // at least one valid segment
        {
          p0 = inPts.Get(ptIndices[sNextId]);
          p1 = inPts.Get(ptIndices[sNextId + 1]);
          sPrev = vtkm::Normal(p1 - p0);
        }
        else // no valid segments. Set everything to the default normal.
        {
          for (vtkm::Id i = 0; i < numPoints; i++)
            outNormals.Set(polylineOffset + i, this->DefaultNorm);
          return;
        }

        // find the next valid, non-parallel segment
        while (++sNextId < numPoints)
        {
          sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
          if (sNextId != numPoints)
          {
            p0 = inPts.Get(ptIndices[sNextId]);
            p1 = inPts.Get(ptIndices[sNextId + 1]);
            sNext = vtkm::Normal(p1 - p0);

            // now the starting normal should simply be the cross product
            // in the following if statement we check for the case where
            // the two segments are parallel, in which case, continue searching
            // for the next valid segment
            auto n = vtkm::Cross(sPrev, sNext);
            if (vtkm::Magnitude(n) > vecMagnitudeEps)
            {
              normal = n;
              sPrev = sNext;
              break;
            }
          }
        }

        //only one valid segment...
        if (sNextId >= numPoints)
        {
          for (vtkm::IdComponent j = 0; j < 3; j++)
            if (sPrev[j] != 0)
            {
              normal[(j + 2) % 3] = 0;
              normal[(j + 1) % 3] = 1;
              normal[j] = -sPrev[(j + 1) % 3] / sPrev[j];
              break;
            }
        }

        vtkm::Normalize(normal);
        vtkm::Id lastNormalId = 0;
        while (++sNextId < numPoints)
        {
          sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
          if (sNextId == numPoints)
            break;

          p0 = inPts.Get(ptIndices[sNextId]);
          p1 = inPts.Get(ptIndices[sNextId + 1]);
          sNext = vtkm::Normal(p1 - p0);

          auto q = vtkm::Cross(sNext, sPrev);

          if (vtkm::Magnitude(q) <= vtkm::Epsilon<vtkm::FloatDefault>()) //can't use this segment
            continue;
          vtkm::Normalize(q);

          vtkm::FloatDefault f1 = vtkm::Dot(q, normal);
          vtkm::FloatDefault f2 = 1 - (f1 * f1);
          if (f2 > 0)
            f2 = vtkm::Sqrt(f2);
          else
            f2 = 0;

          auto c = vtkm::Normal(sNext + sPrev);
          auto w = vtkm::Cross(c, q);
          c = vtkm::Cross(sPrev, q);
          if ((vtkm::Dot(normal, c) * vtkm::Dot(w, c)) < 0)
            f2 = -f2;

          for (vtkm::Id i = lastNormalId; i < sNextId; i++)
            outNormals.Set(polylineOffset + i, normal);
          lastNormalId = sNextId;
          sPrev = sNext;
          normal = (f1 * q) + (f2 * w);
        }

        for (vtkm::Id i = lastNormalId; i < numPoints; i++)
          outNormals.Set(polylineOffset + i, normal);
      }
    }

  private:
    vtkm::Vec3f DefaultNorm;
  };

  //Helper worklet to generate the tube points
  class GeneratePoints : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    VTKM_CONT
    GeneratePoints(const bool& capping, const vtkm::Id& n, const vtkm::FloatDefault& r)
      : Capping(capping)
      , NumSides(n)
      , Radius(r)
      , Theta(2 * static_cast<vtkm::FloatDefault>(vtkm::Pi()) / static_cast<vtkm::FloatDefault>(n))
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn pointCoords,
                                  WholeArrayIn normals,
                                  FieldInTo tubePointOffsets,
                                  FieldInTo polylineOffset,
                                  WholeArrayOut newPointCoords);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 inNormals,
                                    _4 tubePointOffsets,
                                    _5 polylineOffset,
                                    _6 outPts);
    using InputDomain = _1;

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename InNormalsType,
              typename OutPointsType>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              const PointIndexType& ptIndices,
                              const InPointsType& inPts,
                              const InNormalsType& inNormals,
                              const vtkm::Id& tubePointOffsets,
                              const vtkm::Id& polylineOffset,
                              OutPointsType& outPts) const
    {
      if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        vtkm::Vec3f n, p, pNext, sNext, sPrev;
        vtkm::Id outIdx = tubePointOffsets;
        for (vtkm::IdComponent j = 0; j < numPoints; j++)
        {
          if (j == 0) //first point
          {
            p = inPts.Get(ptIndices[j]);
            pNext = inPts.Get(ptIndices[j + 1]);
            sNext = pNext - p;
            sPrev = sNext;
          }
          else if (j == numPoints - 1) //last point
          {
            sPrev = sNext;
            p = pNext;
          }
          else
          {
            p = pNext;
            pNext = inPts.Get(ptIndices[j + 1]);
            sPrev = sNext;
            sNext = pNext - p;
          }
          n = inNormals.Get(polylineOffset + j);

          //Coincident points.
          if (vtkm::Magnitude(sNext) <= vtkm::Epsilon<vtkm::FloatDefault>())
            this->RaiseError("Coincident points in Tube worklet.");

          vtkm::Normalize(sNext);
          auto s = (sPrev + sNext) / 2.;
          if (vtkm::Magnitude(s) <= vtkm::Epsilon<vtkm::FloatDefault>())
            s = vtkm::Cross(sPrev, n);
          vtkm::Normalize(s);

          auto w = vtkm::Cross(s, n);
          //Bad normal
          if (vtkm::Magnitude(w) <= vtkm::Epsilon<vtkm::FloatDefault>())
            this->RaiseError("Bad normal in Tube worklet.");
          vtkm::Normalize(w);

          //create orthogonal coordinate system.
          auto nP = vtkm::Cross(w, s);
          vtkm::Normalize(nP);

          //Add the start cap vertex. This is just a point at the center of the tube (on the polyline).
          if (this->Capping && j == 0)
          {
            outPts.Set(outIdx, p);
            outIdx++;
          }

          //this only implements the 'sides share vertices' line 476
          vtkm::Vec3f normal;
          for (vtkm::IdComponent k = 0; k < this->NumSides; k++)
          {
            vtkm::FloatDefault angle = static_cast<vtkm::FloatDefault>(k) * this->Theta;
            vtkm::FloatDefault cosValue = vtkm::Cos(angle);
            vtkm::FloatDefault sinValue = vtkm::Sin(angle);
            normal = w * cosValue + nP * sinValue;
            auto newPt = p + this->Radius * normal;
            outPts.Set(outIdx, newPt);
            outIdx++;
          }

          //Add the end cap vertex. This is just a point at the center of the tube (on the polyline).
          if (this->Capping && j == numPoints - 1)
          {
            outPts.Set(outIdx, p);
            outIdx++;
          }
        }
      }
    }

  private:
    bool Capping;
    vtkm::Id NumSides;
    vtkm::FloatDefault Radius;
    vtkm::FloatDefault Theta;
  };

  //Helper worklet to generate the tube cells
  class GenerateCells : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    VTKM_CONT
    GenerateCells(const bool& capping, const vtkm::Id& n)
      : Capping(capping)
      , NumSides(n)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  FieldInTo tubePointOffsets,
                                  FieldInTo tubeConnOffsets,
                                  WholeArrayOut outConnectivity);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 tubePointOffset,
                                    _3 tubeConnOffsets,
                                    _4 outConn);
    using InputDomain = _1;

    template <typename CellShapeTag, typename OutConnType>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              const vtkm::Id& tubePointOffset,
                              const vtkm::Id& tubeConnOffset,
                              OutConnType& outConn) const
    {
      if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        vtkm::Id outIdx = tubeConnOffset;
        vtkm::Id tubePtOffset = (this->Capping ? tubePointOffset + 1 : tubePointOffset);
        for (vtkm::IdComponent i = 0; i < numPoints - 1; i++)
        {
          for (vtkm::Id j = 0; j < this->NumSides; j++)
          {
            //Triangle 1: verts 0,1,2
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1, tubePtOffset + i * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outIdx += 3;

            //Triangle 2: verts 0,2,3
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2, tubePtOffset + (i + 1) * this->NumSides + j);
            outIdx += 3;
          }
        }

        if (this->Capping)
        {
          //start cap triangles
          vtkm::Id startCenterPt = 0 + tubePointOffset;
          for (vtkm::Id j = 0; j < this->NumSides; j++)
          {
            outConn.Set(outIdx + 0, startCenterPt);
            outConn.Set(outIdx + 1, startCenterPt + 1 + j);
            outConn.Set(outIdx + 2, startCenterPt + 1 + ((j + 1) % this->NumSides));
            outIdx += 3;
          }

          //end cap triangles
          vtkm::Id endCenterPt = (tubePointOffset + 1) + (numPoints * this->NumSides);
          vtkm::Id endOffsetPt = endCenterPt - this->NumSides;

          for (vtkm::Id j = 0; j < this->NumSides; j++)
          {
            outConn.Set(outIdx + 0, endCenterPt);
            outConn.Set(outIdx + 1, endOffsetPt + j);
            outConn.Set(outIdx + 2, endOffsetPt + ((j + 1) % this->NumSides));
            outIdx += 3;
          }
        }
      }
    }

  private:
    bool Capping;
    vtkm::Id NumSides;
  };

  VTKM_CONT
  Tube(const bool& capping, const vtkm::Id& n, const vtkm::FloatDefault& r)
    : Capping(capping)
    , NumSides(n)
    , Radius(r)

  {
  }

  VTKM_CONT
  void Run(const vtkm::cont::CoordinateSystem& coords,
           const vtkm::cont::DynamicCellSet& cellset,
           vtkm::cont::ArrayHandle<vtkm::Vec3f>& newPoints,
           vtkm::cont::CellSetSingleType<>& newCells)
  {
    using ExplCoordsType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
    using NormalsType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

    if (!(coords.GetData().IsType<ExplCoordsType>() &&
          (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()) ||
           cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))))
    {
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    }

    //Count number of polyline pts, tube pts and tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> ptsPerPolyline, ptsPerTube, numTubeConnIds;
    CountSegments countSegs(this->Capping, this->NumSides);
    vtkm::worklet::DispatcherMapTopology<CountSegments> countInvoker(countSegs);
    countInvoker.Invoke(cellset, ptsPerPolyline, ptsPerTube, numTubeConnIds);

    vtkm::Id totalPolylinePts = vtkm::cont::Algorithm::Reduce(ptsPerPolyline, vtkm::Id(0));
    if (totalPolylinePts == 0)
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    vtkm::Id totalTubePts = vtkm::cont::Algorithm::Reduce(ptsPerTube, vtkm::Id(0));
    vtkm::Id totalTubeConnIds = vtkm::cont::Algorithm::Reduce(numTubeConnIds, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> polylineOffset, tubePointOffsets, tubeConnOffsets;
    vtkm::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylineOffset);
    vtkm::cont::Algorithm::ScanExclusive(ptsPerTube, tubePointOffsets);
    vtkm::cont::Algorithm::ScanExclusive(numTubeConnIds, tubeConnOffsets);

    //Generate normals at each point on all polylines
    ExplCoordsType inCoords = coords.GetData().Cast<ExplCoordsType>();
    NormalsType normals;
    normals.Allocate(totalPolylinePts);
    vtkm::worklet::DispatcherMapTopology<GenerateNormals> genNormalsDisp;
    genNormalsDisp.Invoke(cellset, inCoords, polylineOffset, normals);

    //Generate the tube points
    newPoints.Allocate(totalTubePts);
    GeneratePoints genPts(this->Capping, this->NumSides, this->Radius);
    vtkm::worklet::DispatcherMapTopology<GeneratePoints> genPtsDisp(genPts);
    genPtsDisp.Invoke(cellset, inCoords, normals, tubePointOffsets, polylineOffset, newPoints);

    //Generate tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> newConnectivity;
    newConnectivity.Allocate(totalTubeConnIds);
    GenerateCells genCells(this->Capping, this->NumSides);
    vtkm::worklet::DispatcherMapTopology<GenerateCells> genCellsDisp(genCells);
    genCellsDisp.Invoke(cellset, tubePointOffsets, tubeConnOffsets, newConnectivity);
    newCells.Fill(totalTubePts, vtkm::CELL_SHAPE_TRIANGLE, 3, newConnectivity);
  }

private:
  bool Capping;
  vtkm::Id NumSides;
  vtkm::FloatDefault Radius;
};
}
}

#endif //  vtk_m_worklet_tube_h
