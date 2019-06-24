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
namespace detail
{
class CountSegments : public vtkm::worklet::WorkletMapPointToCell
{
public:
  VTKM_CONT
  CountSegments(const vtkm::Id& n)
    : NumSides(n)
    , NumVertsPerCell(3) //TRI
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
    if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE)
    {
      ptsPerPolyline = numPoints;
      ptsPerTube = this->NumSides * numPoints;
      //numTubeConnIds = this->NumVertsPerCell * (numPoints - 1) * this->NumSides;
      //two triangles per quad
      //TRI
      numTubeConnIds = this->NumVertsPerCell * (numPoints - 1) * this->NumSides * 2;
    }
    else
    {
      ptsPerPolyline = 0;
      ptsPerTube = 0;
      numTubeConnIds = 0;
    }
  }

private:
  vtkm::Id NumSides;
  vtkm::Id NumVertsPerCell;
};

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
  VTKM_EXEC vtkm::Id FindValidSegment(const InPointsType& inPts,
                                      const PointIndexType& ptIndices,
                                      const vtkm::Id& numPoints,
                                      vtkm::Id start) const
  {
    auto ps = inPts.Get(ptIndices[start]);
    vtkm::Id end = start + 1;
    while (end < numPoints)
    {
      auto pe = inPts.Get(ptIndices[end]);
      if (vtkm::Magnitude(pe - ps) > 0)
        return end - 1;
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
    //Ignore non-polyline and 0 point polylines.
    if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE || numPoints == 0)
      return;
    //Assign default for 1pt polylines.
    if (numPoints == 1)
    {
      outNormals.Set(ptIndices[0], this->DefaultNorm);
      return;
    }

    //The following follows the VTK implementation in:
    //vtkPolyLine::GenerateSlidingNormals
    vtkm::Vec<vtkm::FloatDefault, 3> sPrev, sNext, normal, p0, p1;
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
        //std::cout<<"  "<<sNextId<<"n= "<<n<<" "<<sPrev<<" x "<<sNext<<std::endl;
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
    /*
      std::cout<<"Normals: np= "<<numPoints<<" N= "<<normal<<std::endl;
      std::cout<<"ptIndices= {";
      for (int i = 0; i < numPoints; i++) std::cout<<ptIndices[i]<<" ";
      std::cout<<"}"<<std::endl;
      std::cout<<"polylineOffset= "<<polylineOffset<<std::endl;
      */
    //std::cout<<"LAST while loop: sNextId= "<<sNextId<<" normal= "<<normal<<std::endl;

    vtkm::Id lastNormalId = 0;
    while (++sNextId < numPoints)
    {
      sNextId = FindValidSegment(inPts, ptIndices, numPoints, sNextId);
      if (sNextId == numPoints)
        break;

      p0 = inPts.Get(ptIndices[sNextId]);
      p1 = inPts.Get(ptIndices[sNextId + 1]);
      sNext = vtkm::Normal(p1 - p0);
      auto w = vtkm::Cross(sPrev, normal);
      //std::cout<<std::endl;
      //std::cout<<"while_"<<sNextId<<" p0= "<<p0<<" p1= "<<p1<<std::endl;
      //std::cout<<"while_"<<sNextId<<" sPrev= "<<sPrev<<" "<<"sNext= "<<sNext<<" w= "<<w<<std::endl;

      if (vtkm::Magnitude(w) == 0) //can't use this segment
        continue;
      vtkm::Normalize(w);

      auto q = vtkm::Cross(sNext, sPrev);
      //std::cout<<"q= "<<q<<" "<<sNext<<" x "<<sPrev<<std::endl;

      if (vtkm::Magnitude(q) == 0) //can't use this segment
        continue;
      vtkm::Normalize(q);
      //std::cout<<"normal= "<<normal<<" sprev= "<<sPrev<<" sNext= "<<sNext<<" w= "<<w<<" q= "<<q<<" || "<<vtkm::Magnitude(q)<<std::endl;

      vtkm::FloatDefault f1 = vtkm::Dot(q, normal);
      vtkm::FloatDefault f2 = 1 - (f1 * f1);
      if (f2 > 0)
        f2 = vtkm::Sqrt(f2);
      else
        f2 = 0;

      auto c = vtkm::Normal(sNext + sPrev);
      //std::cout<<"c= "<<c<<" "<<sNext<<" x "<<sPrev<<std::endl;
      w = vtkm::Cross(c, q);
      c = vtkm::Cross(sPrev, q);
      if ((vtkm::Dot(normal, c) * vtkm::Dot(w, c)) < 0)
        f2 = -f2;

      //std::cout<<"round0 update: "<<lastNormalId<<" "<<sNextId<<std::endl;
      for (vtkm::Id i = lastNormalId; i < sNextId; i++)
      {
        //std::cout<<"round0: "<<polylineOffset+i<<" "<<normal<<std::endl;
        outNormals.Set(polylineOffset + i, normal);
      }
      lastNormalId = sNextId;
      sPrev = sNext;
      normal = (f1 * q) + (f2 * w);
      //std::cout<<"stuff: "<<c<<" "<<w<<" "<<q<<" "<<f1<<" "<<f2<<" normal= "<<normal<<std::endl;
    }

    for (vtkm::Id i = lastNormalId; i < numPoints; i++)
    {
      //std::cout<<"round1: "<<polylineOffset+i<<" "<<normal<<std::endl;
      outNormals.Set(polylineOffset + i, normal);
    }
  }

private:
  vtkm::Vec<vtkm::FloatDefault, 3> DefaultNorm;
};

class GeneratePoints : public vtkm::worklet::WorkletMapPointToCell
{
public:
  VTKM_CONT
  GeneratePoints(const bool& capping, const vtkm::Id& n, const vtkm::FloatDefault& r)
    : Capping(capping)
    , NumSides(n)
    , Radius(r)
    , Theta(2 * static_cast<vtkm::FloatDefault>(vtkm::Pi()) / n)
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
    //      std::cout<<std::endl;
    //      std::cout<<std::endl;
    //      std::cout<<"GeneratePoints: offset="<<tubePointOffsets<<" n="<<numPoints<<std::endl;

    if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE)
      return;

    vtkm::Vec<vtkm::FloatDefault, 3> n, p, pNext, sNext, sPrev, startCapNorm, endCapNorm;
    vtkm::Id outIdx = tubePointOffsets;
    for (vtkm::IdComponent j = 0; j < numPoints; j++)
    {
      if (j == 0) //first point
      {
        p = inPts.Get(ptIndices[j]);
        pNext = inPts.Get(ptIndices[j + 1]);
        sNext = pNext - p;
        sPrev = sNext;
        startCapNorm = -sPrev;
        vtkm::Normalize(startCapNorm);
      }
      else if (j == numPoints - 1) //last point
      {
        sPrev = sNext;
        p = pNext;
        endCapNorm = sNext;
        vtkm::Normalize(endCapNorm);
      }
      else
      {
        p = pNext;
        pNext = inPts.Get(ptIndices[j + 1]);
        sPrev = sNext;
        sNext = pNext - p;
      }
      n = inNormals.Get(polylineOffset + j);

      if (vtkm::Magnitude(sNext) == 0)
        throw vtkm::cont::ErrorBadValue("Coincident points.");
      vtkm::Normalize(sNext);

      auto s = (sPrev + sNext) / 2.;
      if (vtkm::Magnitude(s) == 0)
        s = vtkm::Cross(sPrev, n);
      vtkm::Normalize(s);

      auto w = vtkm::Cross(s, n);
      if (vtkm::Magnitude(w) == 0)
        throw vtkm::cont::ErrorBadValue("Bad normal in Tube worklet.");
      vtkm::Normalize(w);

      //create orthogonal coordinate system.
      auto nP = vtkm::Cross(w, s);
      vtkm::Normalize(nP);

      //      vtkm::Id outIdx = tubePointOffsets + this->NumSides * j;

      //Add the start cap vertex. This is just a point at the center of the tube (on the polyline).
      if (this->Capping && j == 0)
      {
        std::cout << "outPts: " << outIdx << " = " << p << std::endl;
        outPts.Set(outIdx, p);
        outIdx++;
      }

      //std::cout<<"normal_"<<j<<" "<<n<<std::endl;
      //std::cout<<"Vectors_"<<j<<" s= "<<s<<" w= "<<w<<" nP= "<<nP<<std::endl;
      //this only implements the 'sides share vertices' line 476
      vtkm::Vec<vtkm::FloatDefault, 3> normal;
      for (vtkm::IdComponent k = 0; k < this->NumSides; k++)
      {
        vtkm::FloatDefault angle = static_cast<vtkm::FloatDefault>(k) * this->Theta;
        vtkm::FloatDefault cosValue = vtkm::Cos(angle);
        vtkm::FloatDefault sinValue = vtkm::Sin(angle);
        normal = w * cosValue + nP * sinValue;
        auto newPt = p + this->Radius * normal;
        std::cout << "outPts: " << outIdx << " = " << newPt << std::endl;
        outPts.Set(outIdx, newPt);
        outIdx++;
        //std::cout<<"  outPt["<<outIdx+k<<"] = "<<newPt<<std::endl;
      }

      //Add the end cap vertex. This is just a point at the center of the tube (on the polyline).
      if (this->Capping && j == numPoints - 1)
      {
        std::cout << "outPts: " << outIdx << " = " << p << std::endl;
        outPts.Set(outIdx, p);
        outIdx++;
      }
    }
  }

private:
  bool Capping;
  vtkm::IdComponent NumSides;
  vtkm::FloatDefault Radius;
  vtkm::FloatDefault Theta;
};

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
    if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE)
      return;

    //      std::cout<<std::endl<<std::endl;
    //      std::cout<<"GenerateCells: ptOffset= "<<ptOffset<<" connOffset "<<connOffset<<std::endl;
    //      std::cout<<"  pointIndices: ";
    //      for (int i = 0; i < numPoints; i++) std::cout<<ptIndices[i]<<" "; std::cout<<std::endl;

    vtkm::Id outIdx = tubeConnOffset;
    vtkm::Id tubePtOffset = (this->Capping ? tubePointOffset + 1 : tubePointOffset);
    std::cout << "Tube conn" << std::endl;
    for (vtkm::IdComponent i = 0; i < numPoints - 1; i++)
    {
      for (vtkm::Id j = 0; j < this->NumSides; j++)
      {
        //We have a quad with vertices: 0,1,2,3
        //The vertex ids are:
        // 0: tubePointOffset+i*this->NumSides + j
        // 1: tubePointOffset+i*this->NumSides + (j+1) % this->NumSides
        // 2: tubePointOffset+(i+1)*this->NumSides + (j+1) % this->NumSides
        // 3: tubePointOffset + (i + 1) * this->NumSides + j
        //quad: 0123
        /*
        outConn.Set(outIdx+0, tubePointOffset+i*this->NumSides + j);
        outConn.Set(outIdx+1, tubePointOffset+i*this->NumSides + (j+1) % this->NumSides);
        outConn.Set(outIdx+2, tubePointOffset+(i+1)*this->NumSides + (j+1) % this->NumSides);
        outConn.Set(outIdx+3, tubePointOffset + (i + 1) * this->NumSides + j);
        outIdx += 4;
          */

        //Triangle 1: verts 0,1,2
        outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
        outConn.Set(outIdx + 1, tubePtOffset + i * this->NumSides + (j + 1) % this->NumSides);
        outConn.Set(outIdx + 2, tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);

        std::cout << "  " << outIdx + 0 << " " << tubePtOffset + i * this->NumSides + j
                  << std::endl;
        std::cout << "  " << outIdx + 1 << " "
                  << tubePtOffset + i * this->NumSides + (j + 1) % this->NumSides << std::endl;
        std::cout << "  " << outIdx + 2 << " "
                  << tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides
                  << std::endl;
        outIdx += 3;

        //Triangle 1: verts 0,2, 3
        outConn.Set(outIdx + 0, tubePtOffset + i * this->NumSides + j);
        outConn.Set(outIdx + 1, tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
        outConn.Set(outIdx + 2, tubePtOffset + (i + 1) * this->NumSides + j);
        std::cout << "  " << outIdx + 0 << " "
                  << tubePtOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides
                  << std::endl;
        std::cout << "  " << outIdx + 1 << " "
                  << tubePtOffset + i * this->NumSides + (j + 1) % this->NumSides << std::endl;
        std::cout << "  " << outIdx + 2 << " " << tubePtOffset + (i + 1) * this->NumSides + j
                  << std::endl;
        outIdx += 3;
      }
    }

    if (this->Capping)
    {
      //start cap triangles
      vtkm::Id startCenterPt = 0 + tubePointOffset;
      std::cout << "Start cap" << std::endl;
      for (vtkm::Id j = 0; j < this->NumSides; j++)
      {
        outConn.Set(outIdx + 0, startCenterPt);
        outConn.Set(outIdx + 1, startCenterPt + 1 + j);
        outConn.Set(outIdx + 2, startCenterPt + 1 + ((j + 1) % this->NumSides));
        std::cout << "  " << outIdx + 0 << ": " << outConn.Get(outIdx + 0) << std::endl;
        std::cout << "  " << outIdx + 1 << ": " << outConn.Get(outIdx + 1) << std::endl;
        std::cout << "  " << outIdx + 2 << ": " << outConn.Get(outIdx + 2) << std::endl;
        outIdx += 3;
      }

      //end cap triangles
      vtkm::Id endOffsetPt = 1 + (numPoints * this->NumSides - this->NumSides);
      vtkm::Id endCenterPt = 1 + numPoints * this->NumSides;
      std::cout << std::endl << "End cap lastpt= " << endCenterPt << std::endl;

      for (vtkm::Id j = 0; j < this->NumSides; j++)
      {
        outConn.Set(outIdx + 0, endCenterPt);
        outConn.Set(outIdx + 1, endOffsetPt + j);
        outConn.Set(outIdx + 2, endOffsetPt + ((j + 1) % this->NumSides));
        std::cout << "  " << outIdx + 0 << ": " << outConn.Get(outIdx + 0) << std::endl;
        std::cout << "  " << outIdx + 1 << ": " << outConn.Get(outIdx + 1) << std::endl;
        std::cout << "  " << outIdx + 2 << ": " << outConn.Get(outIdx + 2) << std::endl;
        outIdx += 3;
      }
    }
  }

private:
  bool Capping;
  vtkm::IdComponent NumSides;
};
}

class Tube
{
public:
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
           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& newPoints,
           vtkm::cont::CellSetSingleType<>& newCells)
  {
    using ExplCoordsType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;
    using NormalsType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;

    if (!(coords.GetData().IsType<ExplCoordsType>() &&
          (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()) ||
           cellset.IsSameType(vtkm::cont::CellSetSingleType<>()))))
    {
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    }

    //Count number of polyline pts, tube pts and tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> ptsPerPolyline, ptsPerTube, numTubeConnIds;
    detail::CountSegments countSegs(this->NumSides);
    vtkm::worklet::DispatcherMapTopology<detail::CountSegments> countInvoker(countSegs);
    countInvoker.Invoke(cellset, ptsPerPolyline, ptsPerTube, numTubeConnIds);

    vtkm::Id totalPolylinePts = vtkm::cont::Algorithm::Reduce(ptsPerPolyline, vtkm::Id(0));
    if (totalPolylinePts == 0)
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    vtkm::Id totalTubePts = vtkm::cont::Algorithm::Reduce(ptsPerTube, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> polylineOffset, tubePointOffsets, tubeConnOffsets;
    vtkm::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylineOffset);
    vtkm::cont::Algorithm::ScanExclusive(ptsPerTube, tubePointOffsets);
    vtkm::cont::Algorithm::ScanExclusive(numTubeConnIds, tubeConnOffsets);

    std::cout << "ptsPerPolyline: ";
    vtkm::cont::printSummary_ArrayHandle(ptsPerPolyline, std::cout, true);
    std::cout << "ptsPerTube: ";
    vtkm::cont::printSummary_ArrayHandle(ptsPerTube, std::cout, true);
    std::cout << "polylineOffset: ";
    vtkm::cont::printSummary_ArrayHandle(polylineOffset, std::cout, true);
    std::cout << "tubePointOffsets: ";
    vtkm::cont::printSummary_ArrayHandle(tubePointOffsets, std::cout, true);
    std::cout << "numTubeConnIds: ";
    vtkm::cont::printSummary_ArrayHandle(numTubeConnIds, std::cout, true);
    std::cout << "tubeConnOffsets: ";
    vtkm::cont::printSummary_ArrayHandle(tubeConnOffsets, std::cout, true);
    std::cout << "totalPolylinePts= " << totalPolylinePts << std::endl;

    //Generate normals at each point on all polylines
    ExplCoordsType inCoords = coords.GetData().Cast<ExplCoordsType>();
    NormalsType normals;
    normals.Allocate(totalPolylinePts);
    vtkm::worklet::DispatcherMapTopology<detail::GenerateNormals> genNormalsDisp;
    genNormalsDisp.Invoke(cellset, inCoords, polylineOffset, normals);

    //Generate the tube points
    if (this->Capping) //Capping needs center point at each end.
      totalTubePts += 2;
    newPoints.Allocate(totalTubePts);
    detail::GeneratePoints genPts(this->Capping, this->NumSides, this->Radius);
    vtkm::worklet::DispatcherMapTopology<detail::GeneratePoints> genPtsDisp(genPts);
    genPtsDisp.Invoke(cellset, inCoords, normals, tubePointOffsets, polylineOffset, newPoints);

    //Generate tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> newConnectivity;
    //TRI
    vtkm::Id numTubeCells;
    if (this->Capping)
      numTubeCells = (totalPolylinePts - 1) /** 3*/ * 2 + 2;
    else
      numTubeCells = (totalPolylinePts - 1) /** 3*/ * 2;

    std::cout << "newConn size= " << this->NumSides * numTubeCells << " = " << this->NumSides
              << " x " << numTubeCells << std::endl;
    newConnectivity.Allocate(this->NumSides * numTubeCells * 3);
    //newConnectivity.Allocate(this->NumSides * (totalPolylinePts - 1) * 4);
    detail::GenerateCells genCells(this->Capping, this->NumSides);
    vtkm::worklet::DispatcherMapTopology<detail::GenerateCells> genCellsDisp(genCells);
    genCellsDisp.Invoke(cellset, tubePointOffsets, tubeConnOffsets, newConnectivity);

    //TRI
    newCells.Fill(totalTubePts, vtkm::CELL_SHAPE_TRIANGLE, 3, newConnectivity);
    //newCells.Fill(totalTubePts, vtkm::CELL_SHAPE_QUAD, 4, newConnectivity);
  }

private:
  bool Capping;
  vtkm::Id NumSides;
  vtkm::FloatDefault Radius;
};
}
}

#endif //  vtk_m_worklet_tube_h
