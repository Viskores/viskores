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
    , NumVertsPerCell(4) //Quad
  {
  }

  using ControlSignature = void(CellSetIn,
                                FieldOut ptsPerPolyline,
                                FieldOut ptsPerTube,
                                FieldOut numTubeCells);
  using ExecutionSignature = void(CellShape shapeType,
                                  PointCount numPoints,
                                  _2 ptsPerPolyline,
                                  _3 ptsPerTube,
                                  _4 numTubeCells);
  using InputDomain = _1;

  template <typename CellShapeTag>
  VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                            const vtkm::IdComponent& numPoints,
                            vtkm::Id& ptsPerPolyline,
                            vtkm::Id& ptsPerTube,
                            vtkm::Id& numTubeCells) const
  {
    if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE)
    {
      ptsPerPolyline = numPoints;
      ptsPerTube = this->NumSides * numPoints;
      numTubeCells = this->NumVertsPerCell * (numPoints - 1) * this->NumSides;
    }
    else
    {
      ptsPerPolyline = 0;
      ptsPerTube = 0;
      numTubeCells = 0;
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
  GeneratePoints(const vtkm::Id& n, const vtkm::FloatDefault& r)
    : NumSides(n)
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

      //std::cout<<"normal_"<<j<<" "<<n<<std::endl;
      //std::cout<<"Vectors_"<<j<<" s= "<<s<<" w= "<<w<<" nP= "<<nP<<std::endl;
      //this only implements the 'sides share vertices' line 476
      vtkm::Vec<vtkm::FloatDefault, 3> normal;
      vtkm::Id outIdx = tubePointOffsets + this->NumSides * j;
      for (vtkm::IdComponent k = 0; k < this->NumSides; k++)
      {
        vtkm::FloatDefault angle = static_cast<vtkm::FloatDefault>(k) * this->Theta;
        vtkm::FloatDefault cosValue = vtkm::Cos(angle);
        vtkm::FloatDefault sinValue = vtkm::Sin(angle);
        normal = w * cosValue + nP * sinValue;
        auto newPt = p + this->Radius * normal;
        //std::cout<<"outPts: "<<outIdx+k<<" = "<<newPt<<std::endl;
        outPts.Set(outIdx + k, newPt);
        //std::cout<<"  outPt["<<outIdx+k<<"] = "<<newPt<<std::endl;
      }
    }
  }

private:
  vtkm::IdComponent NumSides;
  vtkm::FloatDefault Radius;
  vtkm::FloatDefault Theta;
};

class GenerateCells : public vtkm::worklet::WorkletMapPointToCell
{
public:
  VTKM_CONT
  GenerateCells(const vtkm::Id& n)
    : NumSides(n)
  {
  }

  using ControlSignature = void(CellSetIn cellset,
                                FieldInTo tubePointOffsets,
                                FieldInTo tubeCellOffsets,
                                WholeArrayOut outConnectivity);
  using ExecutionSignature = void(CellShape shapeType,
                                  PointCount numPoints,
                                  _2 tubePointOffset,
                                  _3 tubeCellOffsets,
                                  _4 outConn);
  using InputDomain = _1;

  template <typename CellShapeTag, typename OutConnType>
  VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                            const vtkm::IdComponent& numPoints,
                            const vtkm::Id& tubePointOffset,
                            const vtkm::Id& tubeCellOffset,
                            OutConnType& outConn) const
  {
    if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE)
      return;

    //      std::cout<<std::endl<<std::endl;
    //      std::cout<<"GenerateCells: ptOffset= "<<ptOffset<<" connOffset "<<connOffset<<std::endl;
    //      std::cout<<"  pointIndices: ";
    //      for (int i = 0; i < numPoints; i++) std::cout<<ptIndices[i]<<" "; std::cout<<std::endl;
    vtkm::Id outIdx = tubeCellOffset;
    for (vtkm::IdComponent i = 0; i < numPoints - 1; i++)
    {
      for (vtkm::Id j = 0; j < this->NumSides; j++)
      {
        outConn.Set(outIdx + 0, tubePointOffset + i * this->NumSides + j);
        outConn.Set(outIdx + 1, tubePointOffset + i * this->NumSides + (j + 1) % this->NumSides);
        outConn.Set(outIdx + 2,
                    tubePointOffset + (i + 1) * this->NumSides + (j + 1) % this->NumSides);
        outConn.Set(outIdx + 3, tubePointOffset + (i + 1) * this->NumSides + j);
        //              for (int k = 0; k < 4; k++)
        //                  std::cout<<"  outConn["<<outIdx+k<<"] = "<<outConn.Get(outIdx+k)<<std::endl;
        outIdx += 4;
      }
    }
    //      std::cout<<"*********** gen cells done ********"<<std::endl;
  }

private:
  vtkm::IdComponent NumSides;
};
}

class Tube
{
public:
  VTKM_CONT
  Tube(const vtkm::Id& n, const vtkm::FloatDefault& r)
    : NumSides(n)
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
    vtkm::cont::ArrayHandle<vtkm::Id> ptsPerPolyline, ptsPerTube, numTubeCells;
    detail::CountSegments countSegs(this->NumSides);
    vtkm::worklet::DispatcherMapTopology<detail::CountSegments> countInvoker(countSegs);
    countInvoker.Invoke(cellset, ptsPerPolyline, ptsPerTube, numTubeCells);

    vtkm::Id totalPolylinePts = vtkm::cont::Algorithm::Reduce(ptsPerPolyline, vtkm::Id(0));
    if (totalPolylinePts == 0)
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
    vtkm::Id totalTubePts = vtkm::cont::Algorithm::Reduce(ptsPerTube, vtkm::Id(0));

    vtkm::cont::ArrayHandle<vtkm::Id> polylineOffset, tubePointOffsets, tubeCellOffsets;
    vtkm::cont::Algorithm::ScanExclusive(ptsPerPolyline, polylineOffset);
    vtkm::cont::Algorithm::ScanExclusive(ptsPerTube, tubePointOffsets);
    vtkm::cont::Algorithm::ScanExclusive(numTubeCells, tubeCellOffsets);

    /*
    std::cout<<"ptsPerPolyline: ";
    vtkm::cont::printSummary_ArrayHandle(ptsPerPolyline, std::cout, true);
    std::cout<<"totalPtsPerCell: ";
    vtkm::cont::printSummary_ArrayHandle(ptsPerTube, std::cout, true);
    std::cout<<"polylineOffset: ";
    vtkm::cont::printSummary_ArrayHandle(polylineOffset, std::cout, true);
    std::cout<<"tubePointOffsets: ";
    vtkm::cont::printSummary_ArrayHandle(tubePointOffsets, std::cout, true);
    std::cout<<"numTubeCells: ";
    vtkm::cont::printSummary_ArrayHandle(numTubeCells, std::cout, true);
    std::cout<<"tubeCellOffsets: ";
    vtkm::cont::printSummary_ArrayHandle(tubeCellOffsets, std::cout, true);
    std::cout<<"totalPolylinePts= "<<totalPolylinePts<<std::endl;
    */

    //Generate normals at each point on all polylines
    ExplCoordsType inCoords = coords.GetData().Cast<ExplCoordsType>();
    NormalsType normals;
    normals.Allocate(totalPolylinePts);
    vtkm::worklet::DispatcherMapTopology<detail::GenerateNormals> genNormalsDisp;
    genNormalsDisp.Invoke(cellset, inCoords, polylineOffset, normals);

    //Generate the tube points
    newPoints.Allocate(totalTubePts);
    detail::GeneratePoints genPts(this->NumSides, this->Radius);
    vtkm::worklet::DispatcherMapTopology<detail::GeneratePoints> genPtsDisp(genPts);
    genPtsDisp.Invoke(cellset, inCoords, normals, tubePointOffsets, polylineOffset, newPoints);

    //Generate tube cells
    vtkm::cont::ArrayHandle<vtkm::Id> newConnectivity;
    newConnectivity.Allocate(this->NumSides * (totalPolylinePts - 1) * 4);
    detail::GenerateCells genCells(this->NumSides);
    vtkm::worklet::DispatcherMapTopology<detail::GenerateCells> genCellsDisp(genCells);
    genCellsDisp.Invoke(cellset, tubePointOffsets, tubeCellOffsets, newConnectivity);

    newCells.Fill(totalTubePts, vtkm::CELL_SHAPE_QUAD, 4, newConnectivity);
  }

private:
  vtkm::Id NumSides;
  vtkm::FloatDefault Radius;
};
}
}

#endif //  vtk_m_worklet_tube_h
