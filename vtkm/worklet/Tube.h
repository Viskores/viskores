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
  class CountSegments : public vtkm::worklet::WorkletVisitCellsWithPoints
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
                                  FieldOut numTubeConnIds,
                                  FieldOut linesPerPolyline);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 ptsPerPolyline,
                                    _3 ptsPerTube,
                                    _4 numTubeConnIds,
                                    _5 linesPerPolyline);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              vtkm::Id& ptsPerPolyline,
                              vtkm::Id& ptsPerTube,
                              vtkm::Id& numTubeConnIds,
                              vtkm::Id& linesPerPolyline) const
    {
      // We only support polylines that contain 2 or more points.
      if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE && numPoints > 1)
      {
        ptsPerPolyline = numPoints;
        ptsPerTube = this->NumSides * numPoints;
        // (two tris per segment) X (numSides) X numVertsPerCell
        numTubeConnIds = (numPoints - 1) * 2 * this->NumSides * this->NumVertsPerCell;
        linesPerPolyline = numPoints - 1;

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
        linesPerPolyline = 0;
      }
    }

  private:
    bool Capping;
    vtkm::Id NumSides;
    vtkm::Id NumVertsPerCell;
  };

  //Helper worklet to generate normals at each point in the polyline.
  class GenerateNormals : public vtkm::worklet::WorkletVisitCellsWithPoints
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
                                  FieldInCell polylineOffset,
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
  class GeneratePoints : public vtkm::worklet::WorkletVisitCellsWithPoints
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
                                  FieldInCell tubePointOffsets,
                                  FieldInCell polylineOffset,
                                  WholeArrayOut newPointCoords,
                                  WholeArrayOut outPointSrcIdx);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    PointIndices ptIndices,
                                    _2 inPts,
                                    _3 inNormals,
                                    _4 tubePointOffsets,
                                    _5 polylineOffset,
                                    _6 outPts,
                                    _7 outPointSrcIdx);
    using InputDomain = _1;

    template <typename CellShapeTag,
              typename PointIndexType,
              typename InPointsType,
              typename InNormalsType,
              typename OutPointsType,
              typename OutPointSrcIdxType>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              const PointIndexType& ptIndices,
                              const InPointsType& inPts,
                              const InNormalsType& inNormals,
                              const vtkm::Id& tubePointOffsets,
                              const vtkm::Id& polylineOffset,
                              OutPointsType& outPts,
                              OutPointSrcIdxType& outPointSrcIdx) const
    {
      if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE || numPoints < 2)
        return;
      else
      {
        vtkm::Id outIdx = tubePointOffsets;
        vtkm::Id pIdx = ptIndices[0];
        vtkm::Id pNextIdx = ptIndices[1];
        vtkm::Vec3f p = inPts.Get(pIdx);
        vtkm::Vec3f pNext = inPts.Get(pNextIdx);
        vtkm::Vec3f sNext = pNext - p;
        vtkm::Vec3f sPrev = sNext;
        for (vtkm::IdComponent j = 0; j < numPoints; j++)
        {
          if (j == 0) //first point
          {
            //Variables initialized before loop started.
          }
          else if (j == numPoints - 1) //last point
          {
            sPrev = sNext;
            p = pNext;
            pIdx = pNextIdx;
          }
          else
          {
            p = pNext;
            pIdx = pNextIdx;
            pNextIdx = ptIndices[j + 1];
            pNext = inPts.Get(pNextIdx);
            sPrev = sNext;
            sNext = pNext - p;
          }
          vtkm::Vec3f n = inNormals.Get(polylineOffset + j);

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
            outPointSrcIdx.Set(outIdx, pIdx);
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
            outPointSrcIdx.Set(outIdx, pIdx);
            outIdx++;
          }

          //Add the end cap vertex. This is just a point at the center of the tube (on the polyline).
          if (this->Capping && j == numPoints - 1)
          {
            outPts.Set(outIdx, p);
            outPointSrcIdx.Set(outIdx, pIdx);
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
  class GenerateCells : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    VTKM_CONT
    GenerateCells(const bool& capping, const vtkm::Id& n)
      : Capping(capping)
      , NumSides(n)
    {
    }

    using ControlSignature = void(CellSetIn cellset,
                                  FieldInCell tubePointOffsets,
                                  FieldInCell tubeConnOffsets,
                                  FieldInCell segOffset,
                                  WholeArrayOut outConnectivity,
                                  WholeArrayOut outCellSrcIdx);
    using ExecutionSignature = void(CellShape shapeType,
                                    PointCount numPoints,
                                    _2 tubePointOffset,
                                    _3 tubeConnOffsets,
                                    _4 segOffset,
                                    _5 outConn,
                                    _6 outCellSrcIdx);
    using InputDomain = _1;

    template <typename CellShapeTag, typename OutConnType, typename OutCellSrcIdxType>
    VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                              const vtkm::IdComponent& numPoints,
                              const vtkm::Id& tubePointOffset,
                              const vtkm::Id& tubeConnOffset,
                              const vtkm::Id& segOffset,
                              OutConnType& outConn,
                              OutCellSrcIdxType& outCellSrcIdx) const
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
            outCellSrcIdx.Set(outIdx / 3, segOffset + static_cast<vtkm::Id>(i));
            outIdx += 3;

            //Triangle 2: verts 0,2,3
            outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
            outConn.Set(outIdx + 1,
                        tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
            outConn.Set(outIdx + 2, tubePtOffset + (i + 1) * this->NumSides + j);
            outCellSrcIdx.Set(outIdx / 3, segOffset + static_cast<vtkm::Id>(i));
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
            outCellSrcIdx.Set(outIdx / 3, segOffset);
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
            outCellSrcIdx.Set(outIdx / 3, segOffset + static_cast<vtkm::Id>(numPoints - 2));
            outIdx += 3;
          }
        }
      }
    }

  private:
    bool Capping;
    vtkm::Id NumSides;
  };


  class MapField : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn sourceIdx, WholeArrayIn sourceArray, FieldOut output);
    using ExecutionSignature = void(_1 sourceIdx, _2 sourceArray, _3 output);
    using InputDomain = _1;

    VTKM_CONT
    MapField() {}

    template <typename SourceArrayType, typename T>
    VTKM_EXEC void operator()(const vtkm::Id& sourceIdx,
                              const SourceArrayType& sourceArray,
                              T& output) const
    {
      output = sourceArray.Get(sourceIdx);
    }
  };

  VTKM_CONT
  Tube()
    : Capping(false)
    , NumSides(0)
    , Radius(0)
  {
  }

  VTKM_CONT
  Tube(const bool& capping, const vtkm::Id& n, const vtkm::FloatDefault& r)
    : Capping(capping)
    , NumSides(n)
    , Radius(r)

  {
  }

  VTKM_CONT
  void SetCapping(bool v) { this->Capping = v; }
  VTKM_CONT
  void SetNumberOfSides(vtkm::Id n) { this->NumSides = n; }
  VTKM_CONT
  void SetRadius(vtkm::FloatDefault r) { this->Radius = r; }

  template <typename Storage>
  VTKM_CONT void Run(const vtkm::cont::ArrayHandle<vtkm::Vec3f, Storage>& coords,
                     const vtkm::cont::DynamicCellSet& cellset,
                     vtkm::cont::ArrayHandle<vtkm::Vec3f>& newPoints,
                     vtkm::cont::CellSetSingleType<>& newCells)
  {
    using NormalsType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

    if (!cellset.IsSameType(vtkm::cont::CellSetExplicit<>()) &&
        !cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))
    {
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    }

    //Count number of polyline pts, tube pts and tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> ptsPerPolyline, ptsPerTube, numTubeConnIds, segPerPolyline;
    CountSegments countSegs(this->Capping, this->NumSides);
    vtkm::worklet::DispatcherMapTopology<CountSegments> countInvoker(countSegs);
    countInvoker.Invoke(cellset, ptsPerPolyline, ptsPerTube, numTubeConnIds, segPerPolyline);

    vtkm::Id totalPolylinePts = vtkm::cont::Algorithm::Reduce(ptsPerPolyline, vtkm::Id(0));
    if (totalPolylinePts == 0)
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    vtkm::Id totalTubePts = vtkm::cont::Algorithm::Reduce(ptsPerTube, vtkm::Id(0));
    vtkm::Id totalTubeConnIds = vtkm::cont::Algorithm::Reduce(numTubeConnIds, vtkm::Id(0));
    //All cells are triangles, so cell count is simple to compute.
    vtkm::Id totalTubeCells = totalTubeConnIds / 3;

    vtkm::cont::ArrayHandle<vtkm::Id> polylinePtOffset, tubePointOffsets, tubeConnOffsets,
      segOffset;
    vtkm::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylinePtOffset);
    vtkm::cont::Algorithm::ScanExclusive(ptsPerTube, tubePointOffsets);
    vtkm::cont::Algorithm::ScanExclusive(numTubeConnIds, tubeConnOffsets);
    vtkm::cont::Algorithm::ScanExclusive(segPerPolyline, segOffset);

    //Generate normals at each point on all polylines
    NormalsType normals;
    normals.Allocate(totalPolylinePts);
    vtkm::worklet::DispatcherMapTopology<GenerateNormals> genNormalsDisp;
    genNormalsDisp.Invoke(cellset, coords, polylinePtOffset, normals);

    //Generate the tube points
    newPoints.Allocate(totalTubePts);
    this->OutputPointSourceIndex.Allocate(totalTubePts);
    GeneratePoints genPts(this->Capping, this->NumSides, this->Radius);
    vtkm::worklet::DispatcherMapTopology<GeneratePoints> genPtsDisp(genPts);
    genPtsDisp.Invoke(cellset,
                      coords,
                      normals,
                      tubePointOffsets,
                      polylinePtOffset,
                      newPoints,
                      this->OutputPointSourceIndex);

    //Generate tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> newConnectivity;
    newConnectivity.Allocate(totalTubeConnIds);
    this->OutputCellSourceIndex.Allocate(totalTubeCells);
    GenerateCells genCells(this->Capping, this->NumSides);
    vtkm::worklet::DispatcherMapTopology<GenerateCells> genCellsDisp(genCells);
    genCellsDisp.Invoke(cellset,
                        tubePointOffsets,
                        tubeConnOffsets,
                        segOffset,
                        newConnectivity,
                        this->OutputCellSourceIndex);
    newCells.Fill(totalTubePts, vtkm::CELL_SHAPE_TRIANGLE, 3, newConnectivity);
  }

  template <typename T, typename StorageType>
  vtkm::cont::ArrayHandle<T> ProcessPointField(
    const vtkm::cont::ArrayHandle<T, StorageType>& input) const
  {
    vtkm::cont::ArrayHandle<T> output;
    vtkm::worklet::DispatcherMapField<MapField> mapFieldDisp;

    output.Allocate(this->OutputPointSourceIndex.GetNumberOfValues());
    mapFieldDisp.Invoke(this->OutputPointSourceIndex, input, output);
    return output;
  }

  template <typename T, typename StorageType>
  vtkm::cont::ArrayHandle<T> ProcessCellField(
    const vtkm::cont::ArrayHandle<T, StorageType>& input) const
  {
    vtkm::cont::ArrayHandle<T> output;
    vtkm::worklet::DispatcherMapField<MapField> mapFieldDisp;

    output.Allocate(this->OutputCellSourceIndex.GetNumberOfValues());
    mapFieldDisp.Invoke(this->OutputCellSourceIndex, input, output);
    return output;
  }

private:
  bool Capping;
  vtkm::Id NumSides;
  vtkm::FloatDefault Radius;
  vtkm::cont::ArrayHandle<vtkm::Id> OutputCellSourceIndex;
  vtkm::cont::ArrayHandle<vtkm::Id> OutputPointSourceIndex;
};
}
}

#endif //  vtk_m_worklet_tube_h
