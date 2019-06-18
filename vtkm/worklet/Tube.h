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
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>
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
  CountSegments(vtkm::Id& n)
    : NumSides(n)
  {
  }

  using ControlSignature = void(CellSetIn, FieldOut, FieldOut);
  using ExecutionSignature = void(CellShape, PointCount, PointIndices, _2, _3);
  using InputDomain = _1;

  template <typename CellShapeTag, typename PointIndexType>
  VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                            const vtkm::IdComponent& numPoints,
                            const PointIndexType& ptIndices,
                            vtkm::Id& ptsPerSegment,
                            vtkm::Id& totalPoints) const
  {
    if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE)
    {
      ptsPerSegment = numPoints;
      totalPoints = this->NumSides * numPoints;
      std::cout << "Pt indices: ";
      for (int i = 0; i < numPoints; i++)
        std::cout << ptIndices[i] << " ";
      std::cout << std::endl;
    }
    else
    {
      ptsPerSegment = 0;
      totalPoints = 0;
    }
  }

private:
  vtkm::Id NumSides;
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
                                FieldInTo pointOffset,
                                WholeArrayOut newNormals);
  using ExecutionSignature = void(CellShape shapeType,
                                  PointCount numPoints,
                                  PointIndices ptIndices,
                                  _2 inPts,
                                  _3 pointOffset,
                                  _4 outNormals);
  using InputDomain = _1;

  template <typename CellShapeTag,
            typename PointIndexType,
            typename InPointsType,
            typename OutNormalType>
  VTKM_EXEC void operator()(const CellShapeTag& shapeType,
                            const vtkm::IdComponent& numPoints,
                            const PointIndexType& ptIndices,
                            const InPointsType& inPts,
                            const vtkm::Id& ptOffset,
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

    //Find the first normal.
    // Find two non-parallel segments and compute the normal.
    vtkm::Vec<vtkm::FloatDefault, 3> sPrev, sNext, normal;
    vtkm::IdComponent sNextId = 0;
    auto p0 = inPts.Get(ptIndices[sNextId]);
    auto p1 = inPts.Get(ptIndices[sNextId + 1]);
    std::cout << sNextId << ": P01= " << p0 << " " << p1 << std::endl;
    sPrev = vtkm::Normal(p1 - p0);
    p0 = p1;
    sNextId = 1;
    while (++sNextId < numPoints)
    {
      p1 = inPts.Get(ptIndices[sNextId]);
      std::cout << sNextId << ": P01= " << p0 << " " << p1 << std::endl;

      sNext = vtkm::Normal(p1 - p0);
      std::cout << "  pn = " << sPrev << " " << sNext << std::endl;
      auto n = vtkm::Cross(sPrev, sNext);
      if (vtkm::Magnitude(n) > vecMagnitudeEps)
      {
        sPrev = sNext;
        normal = n;
        break;
      }

      p0 = p1;
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
      std::cout << "Only one valid segment.... " << normal << std::endl;
    }

    vtkm::Normalize(normal);
    std::cout << "Normals: np= " << numPoints << " N= " << normal << std::endl;
    std::cout << "ptIndices= {";
    for (int i = 0; i < numPoints; i++)
      std::cout << ptIndices[i] << " ";
    std::cout << "}" << std::endl;

    std::cout << "ptOffset= " << ptOffset << std::endl;

    vtkm::Id lastNormalId = 0;
    while (++sNextId < numPoints)
    {
      if (sNextId == numPoints - 1)
        break;

      p0 = inPts.Get(ptIndices[sNextId]);
      p1 = inPts.Get(ptIndices[sNextId + 1]);
      sNext = vtkm::Normal(p1 - p0);
      auto w = vtkm::Cross(sPrev, normal);
      if (vtkm::Magnitude(w) == 0) //can't use this segment
        continue;
      auto q = vtkm::Cross(sNext, sPrev);
      if (vtkm::Magnitude(q) == 0) //can't use this segment
        continue;
      vtkm::FloatDefault f1 = vtkm::Dot(q, normal);
      vtkm::FloatDefault f2 = 1 - (f1 * f1);
      if (f2 > 0)
        f2 = vtkm::Sqrt(f2);
      else
        f2 = 0;

      auto c = vtkm::Normal(sNext + sPrev);
      w = vtkm::Cross(c, q);
      if ((vtkm::Dot(normal, c) * vtkm::Dot(w, c)) < 0)
        f2 = -f2;

      for (vtkm::Id i = lastNormalId; i < sNextId; i++)
      {
        std::cout << "round0: " << ptOffset + i << " " << normal << std::endl;
        outNormals.Set(ptOffset + i, normal);
      }
      lastNormalId = sNextId;
      sPrev = sNext;
      normal = (f1 * q) + (f2 * w);
    }
    for (vtkm::Id i = lastNormalId; i < numPoints; i++)
    {
      std::cout << "round1: " << ptOffset + i << " " << normal << std::endl;
      outNormals.Set(ptOffset + i, normal);
    }

    /*
      for (int i = 0; i < numPoints; i++)
          outNormals.Set(ptOffset+i, vtkm::Vec<vtkm::FloatDefault, 3>(ptOffset, i, 0));
      */
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
                                FieldInTo pointOffset,
                                WholeArrayOut newPointCoords);
  using ExecutionSignature = void(CellShape shapeType,
                                  PointCount numPoints,
                                  PointIndices ptIndices,
                                  _2 inPts,
                                  _3 inNormals,
                                  _4 ptOffset,
                                  _5 outPts);
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
                            const vtkm::Id& ptOffset,
                            OutPointsType& outPts) const
  {
    if (shapeType.Id != vtkm::CELL_SHAPE_POLY_LINE)
      return;

    //std::cout<<"inPts.sz() "<<inPts.GetNumberOfValues()<<std::endl;
    //std::cout<<"outPts.sz() "<<outPts.GetNumberOfValues()<<std::endl;
    std::cout << "ptOffset= " << ptOffset << std::endl;

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
      n = inNormals.Get(ptOffset + j);

      if (vtkm::Magnitude(sNext) == 0)
        throw vtkm::cont::ErrorBadValue("Coincident points.");

      auto s = (sPrev + sNext) / 2.;
      if (vtkm::Magnitude(s) == 0)
        s = vtkm::Cross(sPrev, n);

      auto w = vtkm::Cross(s, n);
      if (vtkm::Magnitude(w) == 0)
        throw vtkm::cont::ErrorBadValue("Bad normal in Tube worklet.");

      //create orthogonal coordinate system.
      auto nP = vtkm::Normal(vtkm::Cross(w, s));

      vtkm::Vec<vtkm::FloatDefault, 3> normal;
      for (vtkm::IdComponent k = 0; k < this->NumSides; k++)
      {
        vtkm::FloatDefault cosValue = vtkm::Cos(static_cast<vtkm::FloatDefault>(k * this->Theta));
        vtkm::FloatDefault sinValue = vtkm::Sin(static_cast<vtkm::FloatDefault>(k * this->Theta));
        normal = w * cosValue + nP * sinValue;
      }
    }




#if 0
      for (vtkm::IdComponent i = 0; i < numPoints; i++)
      {
        vtkm::Id pidx = ptIndices[i];
        std::cout<<"GeneratePoints: "<<pidx<<" --> ";
        auto pt = inPts.Get(pidx);

        vtkm::Id outIdx = ptOffset + this->NumSides*i;
        for (vtkm::Id j = 0; j < this->NumSides; j++)
        {
            std::cout<<outIdx+j<<" ";
            pt[0] = -1;
            pt[1] = -1;
            pt[2] = -1;
            outPts.Set(outIdx+j, pt);
        }
        std::cout<<std::endl;
      }
#endif
  }

private:
  vtkm::IdComponent NumSides;
  vtkm::FloatDefault Radius;
  vtkm::FloatDefault Theta;
};

#if 0
  class GeneratePoints : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
  using ControlSignature = void(CellSetIn cellSet, FieldOut);
                                  /*
                                  WholeArrayIn pointCoords,
                                  FieldInTo cellOffset,
                                  FieldInTo ptOffset,
                                  WholeArrayOut pts);
                                  */

//   using ExecutionSignature = void(CellShape, PointCount, PointIndices); //, _2, _3, _4, _5);
   using ExecutionSignature = void(CellShape, PointCount, PointIndices, _2);
   using InputDomain = _1;

   VTKM_CONT
   GeneratePoints(const vtkm::Id& n, const vtkm::FloatDefault& r)
       : NumSides(n)
       , Radius(r)
   {
   }

//    template <typename CellShapeTag, typename PointIndexType, typename PointPortal>
    template <typename CellShapeTag, typename PointIndexType>
    VTKM_EXEC
    void operator()(const CellShapeTag &shapeType,
                    const vtkm::IdComponent& numPoints,
                    const PointIndexType& ptIndices,
                    vtkm::Id &idx) const
    //                    const vtkm::Id& cellOffset,
    //                    const vtkm::Id& ptOffset,
    //                    const PointPortal& inPointsPortal,
    //                    PointPortal& outPointsPortal) const
    {
    }


  private:
      vtkm::Id NumSides;
      vtkm::FloatDefault Radius;
  };
#endif

#if 0
  class MeowMeow : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    VTKM_CONT
    MeowMeow() {}

    using ControlSignature = void(CellSetIn, WholeArrayIn);
    using ExecutionSignature = void(InputIndex pointIndex,
                                    CellIndices incidentCells,
                                    _1 cellSet,
                                    _2 pointCoordsPortal);
    using InputDomain = _1;

    template <typename IncidentCellVecType, typename PointType>
    VTKM_EXEC
    void operator()(const vtkm::Id &idx,
                    const IncidentCellVecType& indices,
                    const PointType &pt) const
    {
      vtkm::IdComponent ncells = indices.GetNumberOfComponents();
    }
  };
  class GeneratePoints : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn, FieldOut);
    using ExecutionSignature = void(InputIndex, _1, _2);
    using InputDomain = _1;

    VTKM_CONT
    GeneratePoints(vtkm::FloatDefault rad, vtkm::Id numSides)
      : Radius(rad)
      , NumSides(numSides)
    {
    }

    VTKM_CONT
    GeneratePoints()
    {
    }

    template <typename T>
    VTKM_EXEC
    void operator()(const vtkm::Id& idx,
                    const vtkm::Vec<T,3>& pt,
                    vtkm::Vec<T,3>& newPt) const
    {
        newPt[0] = pt[0];
    }

  private:
    vtkm::FloatDefault Radius;
    vtkm::FloatDefault NumSides;
  };
#endif
}

#define SEG_PER_TRI 3
//CSS is CellSetStructured
#define TRI_PER_CSS 12

class Tube
{
public:
  template <int DIM>
  class SegmentedStructured : public vtkm::worklet::WorkletMapPointToCell
  {

  public:
    typedef void ControlSignature(CellSetIn cellset, FieldInTo, WholeArrayOut);
    typedef void ExecutionSignature(FromIndices, _2, _3);
    //typedef _1 InputDomain;
    VTKM_CONT
    SegmentedStructured() {}

#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
    template <typename CellNodeVecType, typename OutIndicesPortal>
    VTKM_EXEC void cell2seg(vtkm::Id3 idx,
                            vtkm::Vec<Id, 3>& segment,
                            const vtkm::Id offset,
                            const CellNodeVecType& cellIndices,
                            OutIndicesPortal& outputIndices) const
    {

      segment[1] = cellIndices[vtkm::IdComponent(idx[0])];
      segment[2] = cellIndices[vtkm::IdComponent(idx[1])];
      outputIndices.Set(offset, segment);

      segment[1] = cellIndices[vtkm::IdComponent(idx[1])];
      segment[2] = cellIndices[vtkm::IdComponent(idx[2])];
      outputIndices.Set(offset + 1, segment);

      segment[1] = cellIndices[vtkm::IdComponent(idx[2])];
      segment[2] = cellIndices[vtkm::IdComponent(idx[0])];
      outputIndices.Set(offset + 2, segment);
    }
    template <typename CellNodeVecType, typename OutIndicesPortal>
    VTKM_EXEC void operator()(const CellNodeVecType& cellIndices,
                              const vtkm::Id& cellIndex,
                              OutIndicesPortal& outputIndices) const
    {
      if (DIM == 2)
      {
        // Do nothing mark says
      }
      else if (DIM == 3)
      {
        vtkm::Id offset = cellIndex * TRI_PER_CSS * SEG_PER_TRI;
        vtkm::Vec<vtkm::Id, 3> segment;
        segment[0] = cellIndex;
        vtkm::Id3 idx;
        idx[0] = 0;
        idx[1] = 1;
        idx[2] = 5;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 5;
        idx[2] = 4;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 2;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 1;
        idx[1] = 6;
        idx[2] = 5;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 7;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 3;
        idx[1] = 6;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 4;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 7;
        idx[2] = 3;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 3;
        idx[2] = 2;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 0;
        idx[1] = 2;
        idx[2] = 1;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 5;
        idx[2] = 6;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
        idx[0] = 4;
        idx[1] = 6;
        idx[2] = 7;
        offset += 3;
        cell2seg(idx, segment, offset, cellIndices, outputIndices);
      }
    }
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
  };


  class Cylinderize : public vtkm::worklet::WorkletMapPointToCell
  {

  public:
    VTKM_CONT
    Cylinderize() {}
    typedef void ControlSignature(CellSetIn cellset, FieldInCell, WholeArrayOut);
    typedef void ExecutionSignature(_2, CellShape, PointCount, PointIndices, WorkIndex, _3);

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void tri2seg(vtkm::Id& offset,
                           const VecType& cellIndices,
                           const vtkm::Id& cellId,
                           const vtkm::Id Id0,
                           const vtkm::Id Id1,
                           const vtkm::Id Id2,
                           OutputPortal& outputIndices) const
    {
      vtkm::Id3 segment;
      segment[0] = cellId;
      segment[1] = vtkm::Id(cellIndices[vtkm::IdComponent(Id0)]);
      segment[2] = vtkm::Id(cellIndices[vtkm::IdComponent(Id1)]);
      outputIndices.Set(offset++, segment);

      segment[1] = vtkm::Id(cellIndices[vtkm::IdComponent(Id1)]);
      segment[2] = vtkm::Id(cellIndices[vtkm::IdComponent(Id2)]);
      outputIndices.Set(offset++, segment);

      segment[1] = vtkm::Id(cellIndices[vtkm::IdComponent(Id2)]);
      segment[2] = vtkm::Id(cellIndices[vtkm::IdComponent(Id0)]);
      outputIndices.Set(offset++, segment);
    }


    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& offset,
                              vtkm::CellShapeTagQuad shapeType,
                              const vtkm::IdComponent& numPoints,
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {
      if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(offset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(offset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(offset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(offset + 3, segment);
      }
    }

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagHexahedron vtkmNotUsed(shapeType),
                              const vtkm::IdComponent& vtkmNotUsed(numPoints),
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const

    {
      vtkm::Id offset = pointOffset;
      tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
      tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
      tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
    }

    template <typename VecType, typename OutputPortal>
    VTKM_EXEC void operator()(const vtkm::Id& pointOffset,
                              vtkm::CellShapeTagGeneric shapeType,
                              const vtkm::IdComponent& numPoints,
                              const VecType& cellIndices,
                              const vtkm::Id& cellId,
                              OutputPortal& outputIndices) const
    {

      if (shapeType.Id == vtkm::CELL_SHAPE_LINE)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;

        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_POLY_LINE)
      {
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_TRIANGLE)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 2, segment);
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_QUAD)
      {
        vtkm::Id3 segment;
        segment[0] = cellId;
        segment[1] = cellIndices[0];
        segment[2] = cellIndices[1];
        outputIndices.Set(pointOffset, segment);

        segment[1] = cellIndices[1];
        segment[2] = cellIndices[2];
        outputIndices.Set(pointOffset + 1, segment);

        segment[1] = cellIndices[2];
        segment[2] = cellIndices[3];
        outputIndices.Set(pointOffset + 2, segment);

        segment[1] = cellIndices[3];
        segment[2] = cellIndices[0];
        outputIndices.Set(pointOffset + 3, segment);
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_TETRA)
      {
        vtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 3, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
      {
        vtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 6, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 7, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 6, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 7, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 7, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 5, 6, outputIndices);
        tri2seg(offset, cellIndices, cellId, 4, 6, 7, outputIndices);
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_WEDGE)
      {
        vtkm::Id offset = pointOffset;
        tri2seg(offset, cellIndices, cellId, 0, 1, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 5, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 0, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 4, 5, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 5, 2, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
      }
      else if (shapeType.Id == vtkm::CELL_SHAPE_PYRAMID)
      {
        vtkm::Id offset = pointOffset;

        tri2seg(offset, cellIndices, cellId, 0, 4, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 1, 2, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 2, 3, 4, outputIndices);
        tri2seg(offset, cellIndices, cellId, 0, 4, 3, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 2, 1, outputIndices);
        tri2seg(offset, cellIndices, cellId, 3, 1, 0, outputIndices);
      }
    }

  }; //class cylinderize

public:
  VTKM_CONT
  Tube(const vtkm::Id& n, const vtkm::FloatDefault& r)
    : NumSides(n)
    , Radius(r)

  {
  }

  VTKM_CONT
  void Run(const vtkm::cont::CoordinateSystem& coords, const vtkm::cont::DynamicCellSet& cellset)
  {
    using ExplCoordsType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;
    using NormalsType = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>;

    if (coords.GetData().IsType<ExplCoordsType>() &&
        (cellset.IsSameType(vtkm::cont::CellSetExplicit<>()) ||
         cellset.IsSameType(vtkm::cont::CellSetSingleType<>())))
    {
      vtkm::cont::ArrayHandle<vtkm::Id> vertsPerCell, totalPointsPerCell;
      detail::CountSegments countSegs(this->NumSides);
      vtkm::worklet::DispatcherMapTopology<detail::CountSegments> countInvoker(countSegs);
      countInvoker.Invoke(cellset, vertsPerCell, totalPointsPerCell);

      vtkm::Id totalVerts = vtkm::cont::Algorithm::Reduce(vertsPerCell, vtkm::Id(0));
      if (totalVerts == 0)
        throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");

      vtkm::Id numPoints = vtkm::cont::Algorithm::Reduce(totalPointsPerCell, vtkm::Id(0));

      ExplCoordsType inCoords = coords.GetData().Cast<ExplCoordsType>();
      ExplCoordsType newPts;
      newPts.Allocate(numPoints);

      vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets, pointOffsets;
      vtkm::cont::Algorithm::ScanExclusive(vertsPerCell, cellOffsets);
      vtkm::cont::Algorithm::ScanExclusive(totalPointsPerCell, pointOffsets);

      NormalsType normals;
      normals.Allocate(totalVerts);
      vtkm::worklet::DispatcherMapTopology<detail::GenerateNormals> genNormalsDisp;

      std::cout << std::endl << std::endl;
      std::cout << "------------ GenerateNormals --------------------" << std::endl;
      genNormalsDisp.Invoke(cellset, inCoords, cellOffsets, normals);

      detail::GeneratePoints genPts(this->NumSides, this->Radius);
      vtkm::worklet::DispatcherMapTopology<detail::GeneratePoints> genPtsDisp(genPts);
      genPtsDisp.Invoke(cellset, inCoords, normals, pointOffsets, newPts);

      /*
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3>> outputIndices;
      outputIndices.Allocate(total);
      */

      vtkm::cont::printSummary_ArrayHandle(inCoords, std::cout, true);
      vtkm::cont::printSummary_ArrayHandle(newPts, std::cout, true);
      std::cout << "Normals: ";
      vtkm::cont::printSummary_ArrayHandle(normals, std::cout, true);
      vtkm::cont::printSummary_ArrayHandle(vertsPerCell, std::cout, true);
      std::cout << "totalPtsPerCell: ";
      vtkm::cont::printSummary_ArrayHandle(totalPointsPerCell, std::cout, true);
      std::cout << "cellOffset: ";
      vtkm::cont::printSummary_ArrayHandle(cellOffsets, std::cout, true);
      std::cout << "pointOffsets: ";
      vtkm::cont::printSummary_ArrayHandle(pointOffsets, std::cout, true);
      std::cout << "totalVerts= " << totalVerts << std::endl;



      std::cout << std::endl << std::endl << std::endl;
      std::cout << "CellOffsets: " << std::endl;
      vtkm::cont::printSummary_ArrayHandle(cellOffsets, std::cout, true);
      std::cout << "VertsPerCell: " << std::endl;
      vtkm::cont::printSummary_ArrayHandle(vertsPerCell, std::cout, true);
    }
    else
      throw vtkm::cont::ErrorBadValue("Tube filter only supported for polyline data.");
  }

private:
  vtkm::Id NumSides;
  vtkm::FloatDefault Radius;
};
}
}

#endif //  vtk_m_worklet_tube_h
