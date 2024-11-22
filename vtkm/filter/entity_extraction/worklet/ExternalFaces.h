//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_ExternalFaces_h
#define vtk_m_worklet_ExternalFaces_h

#include <vtkm/CellShape.h>
#include <vtkm/Hash.h>
#include <vtkm/Math.h>
#include <vtkm/Swap.h>

#include <vtkm/exec/CellFace.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{

struct ExternalFaces
{
  //Worklet that returns the number of external faces for each structured cell
  class NumExternalFacesPerStructuredCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  FieldOut numFacesInCell,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = _2(CellShape, _3);
    using InputDomain = _1;

    VTKM_CONT
    NumExternalFacesPerStructuredCell(const vtkm::Vec3f_64& min_point,
                                      const vtkm::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    VTKM_EXEC
    static inline vtkm::IdComponent CountExternalFacesOnDimension(vtkm::Float64 grid_min,
                                                                  vtkm::Float64 grid_max,
                                                                  vtkm::Float64 cell_min,
                                                                  vtkm::Float64 cell_max)
    {
      vtkm::IdComponent count = 0;

      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
      {
        count++;
      }
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        count++;
      }
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        count += 2;
      }

      return count;
    }

    template <typename CellShapeTag, typename PointCoordVecType>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape,
                                           const PointCoordVecType& pointCoordinates) const
    {
      (void)shape; // C4100 false positive workaround
      VTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      vtkm::IdComponent count = 0;

      count += CountExternalFacesOnDimension(
        MinPoint[0], MaxPoint[0], pointCoordinates[0][0], pointCoordinates[1][0]);

      count += CountExternalFacesOnDimension(
        MinPoint[1], MaxPoint[1], pointCoordinates[0][1], pointCoordinates[3][1]);

      count += CountExternalFacesOnDimension(
        MinPoint[2], MaxPoint[2], pointCoordinates[0][2], pointCoordinates[4][2]);

      return count;
    }

  private:
    vtkm::Vec3f_64 MinPoint;
    vtkm::Vec3f_64 MaxPoint;
  };


  //Worklet that finds face connectivity for each structured cell
  class BuildConnectivityStructured : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  WholeCellSetIn<> inputCell,
                                  FieldOut faceShapes,
                                  FieldOut facePointCount,
                                  FieldOut faceConnectivity,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = void(CellShape, VisitIndex, InputIndex, _2, _3, _4, _5, _6);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    BuildConnectivityStructured(const vtkm::Vec3f_64& min_point, const vtkm::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    enum FaceType
    {
      FACE_GRID_MIN,
      FACE_GRID_MAX,
      FACE_GRID_MIN_AND_MAX,
      FACE_NONE
    };

    VTKM_EXEC
    static inline bool FoundFaceOnDimension(vtkm::Float64 grid_min,
                                            vtkm::Float64 grid_max,
                                            vtkm::Float64 cell_min,
                                            vtkm::Float64 cell_max,
                                            vtkm::IdComponent& faceIndex,
                                            vtkm::IdComponent& count,
                                            vtkm::IdComponent dimensionFaceOffset,
                                            vtkm::IdComponent visitIndex)
    {
      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      FaceType Faces = FaceType::FACE_NONE;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MIN;
      }
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MAX;
      }
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
      {
        Faces = FaceType::FACE_GRID_MIN_AND_MAX;
      }

      if (Faces == FaceType::FACE_NONE)
        return false;

      if (Faces == FaceType::FACE_GRID_MIN)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        else
        {
          count++;
        }
      }
      else if (Faces == FaceType::FACE_GRID_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        else
        {
          count++;
        }
      }
      else if (Faces == FaceType::FACE_GRID_MIN_AND_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        count++;
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        count++;
      }

      return false;
    }

    template <typename PointCoordVecType>
    VTKM_EXEC inline vtkm::IdComponent FindFaceIndexForVisit(
      vtkm::IdComponent visitIndex,
      const PointCoordVecType& pointCoordinates) const
    {
      vtkm::IdComponent count = 0;
      vtkm::IdComponent faceIndex = 0;
      // Search X dimension
      if (!FoundFaceOnDimension(MinPoint[0],
                                MaxPoint[0],
                                pointCoordinates[0][0],
                                pointCoordinates[1][0],
                                faceIndex,
                                count,
                                0,
                                visitIndex))
      {
        // Search Y dimension
        if (!FoundFaceOnDimension(MinPoint[1],
                                  MaxPoint[1],
                                  pointCoordinates[0][1],
                                  pointCoordinates[3][1],
                                  faceIndex,
                                  count,
                                  2,
                                  visitIndex))
        {
          // Search Z dimension
          FoundFaceOnDimension(MinPoint[2],
                               MaxPoint[2],
                               pointCoordinates[0][2],
                               pointCoordinates[4][2],
                               faceIndex,
                               count,
                               4,
                               visitIndex);
        }
      }
      return faceIndex;
    }

    template <typename CellShapeTag,
              typename CellSetType,
              typename PointCoordVecType,
              typename ConnectivityType>
    VTKM_EXEC void operator()(CellShapeTag shape,
                              vtkm::IdComponent visitIndex,
                              vtkm::Id inputIndex,
                              const CellSetType& cellSet,
                              vtkm::UInt8& shapeOut,
                              vtkm::IdComponent& numFacePointsOut,
                              ConnectivityType& faceConnectivity,
                              const PointCoordVecType& pointCoordinates) const
    {
      VTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      vtkm::IdComponent faceIndex = FindFaceIndexForVisit(visitIndex, pointCoordinates);

      vtkm::IdComponent numFacePoints;
      vtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, numFacePoints);
      VTKM_ASSERT(numFacePoints == faceConnectivity.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(inputIndex);

      shapeOut = vtkm::CELL_SHAPE_QUAD;
      numFacePointsOut = 4;

      for (vtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        vtkm::IdComponent localFaceIndex;
        vtkm::ErrorCode status =
          vtkm::exec::CellFaceLocalIndex(facePointIndex, faceIndex, shape, localFaceIndex);
        if (status == vtkm::ErrorCode::Success)
        {
          faceConnectivity[facePointIndex] = inCellIndices[localFaceIndex];
        }
        else
        {
          // An error condition, but do we want to crash the operation?
          faceConnectivity[facePointIndex] = 0;
        }
      }
    }

  private:
    vtkm::Vec3f_64 MinPoint;
    vtkm::Vec3f_64 MaxPoint;
  };

  // Worklet that returns the number of faces for each cell/shape
  class NumFacesPerCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut numFacesInCell);
    using ExecutionSignature = void(CellShape, _2);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VTKM_EXEC void operator()(CellShapeTag shape, vtkm::IdComponent& numFacesInCell) const
    {
      vtkm::exec::CellFaceNumberOfFaces(shape, numFacesInCell);
    }
  };

  // Worklet that identifies a cell face by a hash value. Not necessarily completely unique.
  class FaceHash : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldOutCell cellFaceHashes);
    using ExecutionSignature = void(CellShape, PointIndices, _2);
    using InputDomain = _1;

    template <typename CellShapeTag, typename CellNodeVecType, typename CellFaceHashes>
    VTKM_EXEC void operator()(const CellShapeTag shape,
                              const CellNodeVecType& cellNodeIds,
                              CellFaceHashes& cellFaceHashes) const
    {
      const vtkm::IdComponent numFaces = cellFaceHashes.GetNumberOfComponents();
      for (vtkm::IdComponent faceIndex = 0; faceIndex < numFaces; ++faceIndex)
      {
        vtkm::Id minFacePointId;
        vtkm::exec::CellFaceMinPointId(faceIndex, shape, cellNodeIds, minFacePointId);
        cellFaceHashes[faceIndex] = static_cast<vtkm::HashType>(minFacePointId);
      }
    }
  };

  // Worklet that identifies the number of faces per hash.
  class NumFacesPerHash : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn faceHashes, AtomicArrayInOut numFacesPerHash);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    template <typename NumFacesPerHashArray>
    VTKM_EXEC void operator()(const vtkm::HashType& faceHash,
                              NumFacesPerHashArray& numFacesPerHash) const
    {
      // MemoryOrder::Relaxed is safe here, since we're not using the atomics for synchronization.
      numFacesPerHash.Add(faceHash, 1, vtkm::MemoryOrder::Relaxed);
    }
  };

  /// Class to pack and unpack cell and face indices to/from a single integer.
  class CellFaceIdPacker
  {
  public:
    using CellAndFaceIdType = vtkm::UInt64;
    using CellIdType = vtkm::Id;
    using FaceIdType = vtkm::Int8;

    static constexpr CellAndFaceIdType GetNumFaceIdBits()
    {
      static_assert(vtkm::exec::detail::CellFaceTables::MAX_NUM_FACES == 6,
                    "MAX_NUM_FACES must be 6, otherwise, update GetNumFaceIdBits");
      return 3;
    }
    static constexpr CellAndFaceIdType GetFaceMask() { return (1ULL << GetNumFaceIdBits()) - 1; }

    /// Pack function for both cellIndex and faceIndex
    VTKM_EXEC inline static constexpr CellAndFaceIdType Pack(const CellIdType& cellIndex,
                                                             const FaceIdType& faceIndex)
    {
      // Pack the cellIndex in the higher bits, leaving FACE_INDEX_BITS bits for faceIndex
      return static_cast<CellAndFaceIdType>(cellIndex << GetNumFaceIdBits()) |
        static_cast<CellAndFaceIdType>(faceIndex);
    }

    /// Unpacking function for both cellIndex and faceIndex
    /// This is templated because we don't want to create a copy of the packedCellAndFaceId value.
    template <typename TCellAndFaceIdType>
    VTKM_EXEC inline static constexpr void Unpack(const TCellAndFaceIdType& packedCellAndFaceId,
                                                  CellIdType& cellIndex,
                                                  FaceIdType& faceIndex)
    {
      // Extract faceIndex from the lower GetNumFaceIdBits bits
      faceIndex = static_cast<FaceIdType>(packedCellAndFaceId & GetFaceMask());
      // Extract cellIndex by shifting back
      cellIndex = static_cast<CellIdType>(packedCellAndFaceId >> GetNumFaceIdBits());
    }
  };

  // Worklet that writes out the cell and face ids of each face per hash.
  class BuildFacesPerHash : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellFaceHashes,
                                  AtomicArrayInOut numFacesPerHash,
                                  WholeArrayOut cellAndFaceIdOfFacesPerHash);
    using ExecutionSignature = void(InputIndex, _1, _2, _3);
    using InputDomain = _1;

    template <typename CellFaceHashes,
              typename NumFacesPerHashArray,
              typename CellAndFaceIdOfFacePerHashArray>
    VTKM_EXEC void operator()(vtkm::Id inputIndex,
                              const CellFaceHashes& cellFaceHashes,
                              NumFacesPerHashArray& numFacesPerHash,
                              CellAndFaceIdOfFacePerHashArray& cellAndFaceIdOfFacesPerHash) const
    {
      const vtkm::IdComponent numFaces = cellFaceHashes.GetNumberOfComponents();
      for (vtkm::IdComponent faceIndex = 0; faceIndex < numFaces; ++faceIndex)
      {
        const auto& faceHash = cellFaceHashes[faceIndex];
        // MemoryOrder::Relaxed is safe here, since we're not using the atomics for synchronization.
        const vtkm::IdComponent hashFaceIndex =
          numFacesPerHash.Add(faceHash, -1, vtkm::MemoryOrder::Relaxed) - 1;
        cellAndFaceIdOfFacesPerHash.Get(faceHash)[hashFaceIndex] =
          CellFaceIdPacker::Pack(inputIndex, static_cast<CellFaceIdPacker::FaceIdType>(faceIndex));
      }
    }
  };

  // Worklet that identifies the number of external faces per Hash.
  // Because there can be collisions in the hash, this instance hash might
  // represent multiple faces, which have to be checked. The resulting
  // number is the total number of external faces. It also reorders the
  // faces so that the external faces are first, followed by the internal faces.
  class FaceCounts : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldInOut cellAndFaceIdOfFacesInHash,
                                  WholeCellSetIn<> inputCells,
                                  FieldOut externalFacesInHash);
    using ExecutionSignature = _3(_1, _2);
    using InputDomain = _1;

    template <typename CellAndFaceIdOfFacesInHash, typename CellSetType>
    VTKM_EXEC vtkm::IdComponent operator()(CellAndFaceIdOfFacesInHash& cellAndFaceIdOfFacesInHash,
                                           const CellSetType& cellSet) const
    {
      const vtkm::IdComponent numFacesInHash = cellAndFaceIdOfFacesInHash.GetNumberOfComponents();

      static constexpr vtkm::IdComponent FACE_CANONICAL_IDS_CACHE_SIZE = 100;
      if (numFacesInHash <= 1)
      {
        // Either one or zero faces. If there is one, it's external, In either case, do nothing.
        return numFacesInHash;
      }
      else if (numFacesInHash <= FACE_CANONICAL_IDS_CACHE_SIZE) // Fast path with caching
      {
        CellFaceIdPacker::CellIdType myCellId;
        CellFaceIdPacker::FaceIdType myFaceId;
        vtkm::Vec<vtkm::Id3, FACE_CANONICAL_IDS_CACHE_SIZE> faceCanonicalIds;
        for (vtkm::IdComponent faceIndex = 0; faceIndex < numFacesInHash; ++faceIndex)
        {
          CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[faceIndex], myCellId, myFaceId);
          vtkm::exec::CellFaceCanonicalId(myFaceId,
                                          cellSet.GetCellShape(myCellId),
                                          cellSet.GetIndices(myCellId),
                                          faceCanonicalIds[faceIndex]);
        }
        // Start by assuming all faces are duplicate, then remove two for each duplicate pair found.
        vtkm::IdComponent numExternalFaces = 0;
        // Iterate over the faces in the hash in reverse order (to minimize the swaps being
        // performed) and find duplicates faces. Put duplicates at the end and unique faces
        // at the beginning. Narrow this range until all unique/duplicate are found.
        for (vtkm::IdComponent myIndex = numFacesInHash - 1; myIndex >= numExternalFaces;)
        {
          bool isInternal = false;
          const vtkm::Id3& myFace = faceCanonicalIds[myIndex];
          vtkm::IdComponent otherIndex;
          for (otherIndex = myIndex - 1; otherIndex >= numExternalFaces; --otherIndex)
          {
            const vtkm::Id3& otherFace = faceCanonicalIds[otherIndex];
            // The first id of the canonical face id is the minimum point id of the face. Since that
            // is the hash function, we already know that all faces have the same minimum point id.
            if (/*myFace[0] == otherFace[0] && */ myFace[1] == otherFace[1] &&
                myFace[2] == otherFace[2])
            {
              // Faces are the same. Must be internal. We don't have to worry about otherFace
              // matching anything else because a proper topology will have at most 2 cells sharing
              // a face, so there should be no more matches.
              isInternal = true;
              break;
            }
          }
          if (isInternal) // If two faces are internal,
          {               // swap them to the end of the list to avoid revisiting them.
            --myIndex;    // decrement for the first duplicate face, which is at the end
            if (myIndex != otherIndex)
            {
              FaceCounts::SwapFace<CellFaceIdPacker::CellAndFaceIdType>(
                cellAndFaceIdOfFacesInHash[otherIndex], cellAndFaceIdOfFacesInHash[myIndex]);
              vtkm::Swap(faceCanonicalIds[otherIndex], faceCanonicalIds[myIndex]);
            }
            --myIndex; // decrement for the second duplicate face
          }
          else // If the face is external
          {    // swap it to the front of the list, to avoid revisiting it.
            if (myIndex != numExternalFaces)
            {
              FaceCounts::SwapFace<CellFaceIdPacker::CellAndFaceIdType>(
                cellAndFaceIdOfFacesInHash[myIndex], cellAndFaceIdOfFacesInHash[numExternalFaces]);
              vtkm::Swap(faceCanonicalIds[myIndex], faceCanonicalIds[numExternalFaces]);
            }
            ++numExternalFaces; // increment for the new external face
            // myIndex remains the same, since we have a new face to check at the same myIndex.
            // However, numExternalFaces has incremented, so the loop could still terminate.
          }
        }
        return numExternalFaces;
      }
      else // Slow path without caching
      {
        CellFaceIdPacker::CellIdType myCellId, otherCellId;
        CellFaceIdPacker::FaceIdType myFaceId, otherFaceId;
        vtkm::Id3 myFace, otherFace;
        // Start by assuming all faces are duplicate, then remove two for each duplicate pair found.
        vtkm::IdComponent numExternalFaces = 0;
        // Iterate over the faces in the hash in reverse order (to minimize the swaps being
        // performed) and find duplicates faces. Put duplicates at the end and unique faces
        // at the beginning. Narrow this range until all unique/duplicate are found.
        for (vtkm::IdComponent myIndex = numFacesInHash - 1; myIndex >= numExternalFaces;)
        {
          bool isInternal = false;
          CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[myIndex], myCellId, myFaceId);
          vtkm::exec::CellFaceCanonicalId(
            myFaceId, cellSet.GetCellShape(myCellId), cellSet.GetIndices(myCellId), myFace);
          vtkm::IdComponent otherIndex;
          for (otherIndex = myIndex - 1; otherIndex >= numExternalFaces; --otherIndex)
          {
            CellFaceIdPacker::Unpack(
              cellAndFaceIdOfFacesInHash[otherIndex], otherCellId, otherFaceId);
            vtkm::exec::CellFaceCanonicalId(otherFaceId,
                                            cellSet.GetCellShape(otherCellId),
                                            cellSet.GetIndices(otherCellId),
                                            otherFace);
            // The first id of the canonical face id is the minimum point id of the face. Since that
            // is the hash function, we already know that all faces have the same minimum point id.
            if (/*myFace[0] == otherFace[0] && */ myFace[1] == otherFace[1] &&
                myFace[2] == otherFace[2])
            {
              // Faces are the same. Must be internal. We don't have to worry about otherFace
              // matching anything else because a proper topology will have at most 2 cells sharing
              // a face, so there should be no more matches.
              isInternal = true;
              break;
            }
          }
          if (isInternal) // If two faces are internal,
          {               // swap them to the end of the list to avoid revisiting them.
            --myIndex;    // decrement for the first duplicate face, which is at the end
            if (myIndex != otherIndex)
            {
              FaceCounts::SwapFace<CellFaceIdPacker::CellAndFaceIdType>(
                cellAndFaceIdOfFacesInHash[otherIndex], cellAndFaceIdOfFacesInHash[myIndex]);
            }
            --myIndex; // decrement for the second duplicate face
          }
          else // If the face is external
          {    // swap it to the front of the list, to avoid revisiting it.
            if (myIndex != numExternalFaces)
            {
              FaceCounts::SwapFace<CellFaceIdPacker::CellAndFaceIdType>(
                cellAndFaceIdOfFacesInHash[myIndex], cellAndFaceIdOfFacesInHash[numExternalFaces]);
            }
            ++numExternalFaces; // increment for the new external face
            // myIndex remains the same, since we have a new face to check at the same myIndex.
            // However, numExternalFaces has incremented, so the loop could still terminate.
          }
        }
        return numExternalFaces;
      }
    }

  private:
    template <typename FaceT, typename FaceRefT>
    VTKM_EXEC inline static void SwapFace(FaceRefT&& cellAndFace1, FaceRefT&& cellAndFace2)
    {
      const FaceT tmpCellAndFace = cellAndFace1;
      cellAndFace1 = cellAndFace2;
      cellAndFace2 = tmpCellAndFace;
    }
  };

public:
  // Worklet that returns the number of points for each outputted face.
  // Have to manage the case where multiple faces have the same hash.
  class NumPointsPerFace : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellAndFaceIdOfFacesInHash,
                                  WholeCellSetIn<> inputCells,
                                  FieldOut numPointsInExternalFace);
    using ExecutionSignature = void(_1, _2, VisitIndex, _3);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CellAndFaceIdOfFacesInHash, typename CellSetType>
    VTKM_EXEC void operator()(const CellAndFaceIdOfFacesInHash& cellAndFaceIdOfFacesInHash,
                              const CellSetType& cellSet,
                              vtkm::IdComponent visitIndex,
                              vtkm::IdComponent& numPointsInExternalFace) const
    {
      // external faces are first, so we can use the visit index directly
      CellFaceIdPacker::CellIdType myCellId;
      CellFaceIdPacker::FaceIdType myFaceId;
      CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[visitIndex], myCellId, myFaceId);

      vtkm::exec::CellFaceNumberOfPoints(
        myFaceId, cellSet.GetCellShape(myCellId), numPointsInExternalFace);
    }
  };

  // Worklet that returns the shape and connectivity for each external face
  class BuildConnectivity : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellAndFaceIdOfFacesInHash,
                                  WholeCellSetIn<> inputCells,
                                  FieldOut shapesOut,
                                  FieldOut connectivityOut,
                                  FieldOut cellIdMapOut);
    using ExecutionSignature = void(_1, _2, VisitIndex, _3, _4, _5);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CellAndFaceIdOfFacesInHash, typename CellSetType, typename ConnectivityType>
    VTKM_EXEC void operator()(const CellAndFaceIdOfFacesInHash& cellAndFaceIdOfFacesInHash,
                              const CellSetType& cellSet,
                              vtkm::IdComponent visitIndex,
                              vtkm::UInt8& shapeOut,
                              ConnectivityType& connectivityOut,
                              vtkm::Id& cellIdMapOut) const
    {
      // external faces are first, so we can use the visit index directly
      CellFaceIdPacker::CellIdType myCellId;
      CellFaceIdPacker::FaceIdType myFaceId;
      CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[visitIndex], myCellId, myFaceId);

      const typename CellSetType::CellShapeTag shapeIn = cellSet.GetCellShape(myCellId);
      vtkm::exec::CellFaceShape(myFaceId, shapeIn, shapeOut);
      cellIdMapOut = myCellId;

      vtkm::IdComponent numFacePoints;
      vtkm::exec::CellFaceNumberOfPoints(myFaceId, shapeIn, numFacePoints);
      VTKM_ASSERT(numFacePoints == connectivityOut.GetNumberOfComponents());

      const typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(myCellId);
      for (vtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; ++facePointIndex)
      {
        vtkm::IdComponent localFaceIndex;
        const vtkm::ErrorCode status =
          vtkm::exec::CellFaceLocalIndex(facePointIndex, myFaceId, shapeIn, localFaceIndex);
        if (status == vtkm::ErrorCode::Success)
        {
          connectivityOut[facePointIndex] = inCellIndices[localFaceIndex];
        }
        else
        {
          // An error condition, but do we want to crash the operation?
          connectivityOut[facePointIndex] = 0;
        }
      }
    }
  };

  class IsPolyDataCell : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut isPolyDataCell);
    using ExecutionSignature = _2(CellShape);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape) const
    {
      vtkm::IdComponent numFaces;
      vtkm::exec::CellFaceNumberOfFaces(shape, numFaces);
      return !numFaces;
    }
  };

  class CountPolyDataCellPoints : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = vtkm::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inCellSet, FieldOut numPoints);
    using ExecutionSignature = _2(PointCount);
    using InputDomain = _1;

    VTKM_EXEC vtkm::Id operator()(vtkm::Id count) const { return count; }
  };

  class PassPolyDataCells : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = vtkm::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inputTopology,
                                  FieldOut shapes,
                                  FieldOut pointIndices,
                                  FieldOut cellIdMapOut);
    using ExecutionSignature = void(CellShape, PointIndices, InputIndex, _2, _3, _4);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    VTKM_EXEC void operator()(const CellShape& inShape,
                              const InPointIndexType& inPoints,
                              vtkm::Id inputIndex,
                              vtkm::UInt8& outShape,
                              OutPointIndexType& outPoints,
                              vtkm::Id& cellIdMapOut) const
    {
      cellIdMapOut = inputIndex;
      outShape = inShape.Id;

      vtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
      VTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename T>
  struct BiasFunctor
  {
    VTKM_EXEC_CONT
    explicit BiasFunctor(T bias = T(0))
      : Bias(bias)
    {
    }

    VTKM_EXEC_CONT
    T operator()(T x) const { return x + this->Bias; }

    T Bias;
  };

public:
  VTKM_CONT
  ExternalFaces()
    : PassPolyData(true)
  {
  }

  VTKM_CONT
  void SetPassPolyData(bool flag) { this->PassPolyData = flag; }

  VTKM_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }

  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }


  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry for regular grids.
  ///
  /// Faster Run() method for uniform and rectilinear grid types.
  /// Uses grid extents to find cells on the boundaries of the grid.
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VTKM_CONT void Run(
    const vtkm::cont::CellSetStructured<3>& inCellSet,
    const vtkm::cont::CoordinateSystem& coord,
    vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    // create an invoker
    vtkm::cont::Invoker invoke;

    vtkm::Vec3f_64 MinPoint;
    vtkm::Vec3f_64 MaxPoint;

    vtkm::Id3 PointDimensions = inCellSet.GetPointDimensions();

    using DefaultHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
    using CartesianArrayHandle =
      vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

    auto coordData = coord.GetData();
    if (coordData.CanConvert<CartesianArrayHandle>())
    {
      const auto vertices = coordData.AsArrayHandle<CartesianArrayHandle>();
      const auto vertsSize = vertices.GetNumberOfValues();
      const auto tmp = vtkm::cont::ArrayGetValues({ 0, vertsSize - 1 }, vertices);
      MinPoint = tmp[0];
      MaxPoint = tmp[1];
    }
    else
    {
      auto vertices = coordData.AsArrayHandle<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      auto Coordinates = vertices.ReadPortal();

      MinPoint = Coordinates.GetOrigin();
      vtkm::Vec3f_64 spacing = Coordinates.GetSpacing();

      vtkm::Vec3f_64 unitLength;
      unitLength[0] = static_cast<vtkm::Float64>(PointDimensions[0] - 1);
      unitLength[1] = static_cast<vtkm::Float64>(PointDimensions[1] - 1);
      unitLength[2] = static_cast<vtkm::Float64>(PointDimensions[2] - 1);
      MaxPoint = MinPoint + spacing * unitLength;
    }

    // Count the number of external faces per cell
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numExternalFaces;
    invoke(NumExternalFacesPerStructuredCell(MinPoint, MaxPoint),
           inCellSet,
           numExternalFaces,
           coordData);

    vtkm::Id numberOfExternalFaces =
      vtkm::cont::Algorithm::Reduce(numExternalFaces, 0, vtkm::Sum());

    vtkm::worklet::ScatterCounting scatterCellToExternalFace(numExternalFaces);

    // Maps output cells to input cells. Store this for cell field mapping.
    this->CellIdMap = scatterCellToExternalFace.GetOutputToInputMap();

    numExternalFaces.ReleaseResources();

    vtkm::Id connectivitySize = 4 * numberOfExternalFaces;
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> faceConnectivity;
    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> faceShapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> facePointCount;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    // Build connectivity for external faces
    invoke(BuildConnectivityStructured(MinPoint, MaxPoint),
           scatterCellToExternalFace,
           inCellSet,
           inCellSet,
           faceShapes,
           facePointCount,
           vtkm::cont::make_ArrayHandleGroupVec<4>(faceConnectivity),
           coordData);

    auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(facePointCount);

    outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, faceConnectivity, offsets);
  }

  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry
  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  VTKM_CONT void Run(
    const InCellSetType& inCellSet,
    vtkm::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    using PointCountArrayType = vtkm::cont::ArrayHandle<vtkm::IdComponent>;
    using ShapeArrayType = vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage>;
    using OffsetsArrayType = vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>;
    using ConnectivityArrayType = vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage>;

    // create an invoker
    vtkm::cont::Invoker invoke;

    // Create an array to store the number of faces per cell
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numFacesPerCell;

    // Compute the number of faces per cell
    invoke(NumFacesPerCell(), inCellSet, numFacesPerCell);

    // Compute the offsets into a packed array holding face information for each cell.
    vtkm::Id totalNumberOfFaces;
    vtkm::cont::ArrayHandle<vtkm::Id> facesPerCellOffsets;
    vtkm::cont::ConvertNumComponentsToOffsets(
      numFacesPerCell, facesPerCellOffsets, totalNumberOfFaces);
    // Release the resources of numFacesPerCell that is not needed anymore
    numFacesPerCell.ReleaseResources();

    PointCountArrayType polyDataPointCount;
    ShapeArrayType polyDataShapes;
    OffsetsArrayType polyDataOffsets;
    ConnectivityArrayType polyDataConnectivity;
    vtkm::cont::ArrayHandle<vtkm::Id> polyDataCellIdMap;
    vtkm::Id polyDataConnectivitySize = 0;
    if (this->PassPolyData)
    {
      vtkm::cont::ArrayHandle<vtkm::IdComponent> isPolyDataCell;

      invoke(IsPolyDataCell(), inCellSet, isPolyDataCell);

      vtkm::worklet::ScatterCounting scatterPolyDataCells(isPolyDataCell);

      isPolyDataCell.ReleaseResources();

      if (scatterPolyDataCells.GetOutputRange(inCellSet.GetNumberOfCells()) != 0)
      {
        invoke(CountPolyDataCellPoints(), scatterPolyDataCells, inCellSet, polyDataPointCount);

        vtkm::cont::ConvertNumComponentsToOffsets(
          polyDataPointCount, polyDataOffsets, polyDataConnectivitySize);

        polyDataConnectivity.Allocate(polyDataConnectivitySize);

        invoke(PassPolyDataCells(),
               scatterPolyDataCells,
               inCellSet,
               polyDataShapes,
               vtkm::cont::make_ArrayHandleGroupVecVariable(polyDataConnectivity, polyDataOffsets),
               polyDataCellIdMap);
      }
    }

    if (totalNumberOfFaces == 0)
    {
      if (!polyDataConnectivitySize)
      {
        // Data has no faces. Output is empty.
        outCellSet.PrepareToAddCells(0, 0);
        outCellSet.CompleteAddingCells(inCellSet.GetNumberOfPoints());
        return;
      }
      else
      {
        // Pass only input poly data to output
        outCellSet.Fill(
          inCellSet.GetNumberOfPoints(), polyDataShapes, polyDataConnectivity, polyDataOffsets);
        this->CellIdMap = polyDataCellIdMap;
        return;
      }
    }

    // Create an array to store the hash values of the faces
    vtkm::cont::ArrayHandle<vtkm::HashType> faceHashes;
    faceHashes.Allocate(totalNumberOfFaces);

    // Create a group vec array to access the faces of each cell conveniently
    auto faceHashesGroupVec =
      vtkm::cont::make_ArrayHandleGroupVecVariable(faceHashes, facesPerCellOffsets);

    // Compute the hash values of the faces
    invoke(FaceHash(), inCellSet, faceHashesGroupVec);

    // Create an array to store the number of faces per hash
    const vtkm::Id numberOfHashes = inCellSet.GetNumberOfPoints();
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numFacesPerHash;
    numFacesPerHash.AllocateAndFill(numberOfHashes, 0);

    // Count the number of faces per hash
    invoke(NumFacesPerHash(), faceHashes, numFacesPerHash);

    // Compute the offsets for a packed array holding face information for each hash.
    vtkm::cont::ArrayHandle<vtkm::Id> facesPerHashOffsets;
    vtkm::cont::ConvertNumComponentsToOffsets(numFacesPerHash, facesPerHashOffsets);

    // Create an array to store the cell and face ids of each face per hash
    vtkm::cont::ArrayHandle<CellFaceIdPacker::CellAndFaceIdType> cellAndFaceIdOfFacesPerHash;
    cellAndFaceIdOfFacesPerHash.Allocate(totalNumberOfFaces);

    // Create a group vec array to access/write the cell and face ids of each face per hash
    auto cellAndFaceIdOfFacesPerHashGroupVec = vtkm::cont::make_ArrayHandleGroupVecVariable(
      cellAndFaceIdOfFacesPerHash, facesPerHashOffsets);

    // Build the cell and face ids of all faces per hash
    invoke(BuildFacesPerHash(),
           faceHashesGroupVec,
           numFacesPerHash,
           cellAndFaceIdOfFacesPerHashGroupVec);
    // Release the resources of the arrays that are not needed anymore
    facesPerCellOffsets.ReleaseResources();
    faceHashes.ReleaseResources();
    numFacesPerHash.ReleaseResources();

    // Create an array to count the number of external faces per hash
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numExternalFacesPerHash;
    numExternalFacesPerHash.Allocate(numberOfHashes);

    // Compute the number of external faces per hash
    invoke(FaceCounts(), cellAndFaceIdOfFacesPerHashGroupVec, inCellSet, numExternalFacesPerHash);

    // Create a scatter counting object to only access the hashes with external faces
    vtkm::worklet::ScatterCounting scatterCullInternalFaces(numExternalFacesPerHash);
    const vtkm::Id numberOfExternalFaces = scatterCullInternalFaces.GetOutputRange(numberOfHashes);
    // Release the resources of externalFacesPerHash that is not needed anymore
    numExternalFacesPerHash.ReleaseResources();

    // Create an array to store the number of points of the external faces
    PointCountArrayType numPointsPerExternalFace;
    numPointsPerExternalFace.Allocate(numberOfExternalFaces);

    // Compute the number of points of the external faces
    invoke(NumPointsPerFace(),
           scatterCullInternalFaces,
           cellAndFaceIdOfFacesPerHashGroupVec,
           inCellSet,
           numPointsPerExternalFace);

    // Compute the offsets for a packed array holding the point connections for each external face.
    OffsetsArrayType pointsPerExternalFaceOffsets;
    vtkm::Id connectivitySize;
    vtkm::cont::ConvertNumComponentsToOffsets(
      numPointsPerExternalFace, pointsPerExternalFaceOffsets, connectivitySize);

    // Create an array to connectivity of the external faces
    ConnectivityArrayType externalFacesConnectivity;
    externalFacesConnectivity.Allocate(connectivitySize);

    // Create a group vec array to access the connectivity of each external face
    auto externalFacesConnectivityGroupVec = vtkm::cont::make_ArrayHandleGroupVecVariable(
      externalFacesConnectivity, pointsPerExternalFaceOffsets);

    // Create an array to store the shape of the external faces
    ShapeArrayType externalFacesShapes;
    externalFacesShapes.Allocate(numberOfExternalFaces);

    // Create an array to store the cell id of the external faces
    vtkm::cont::ArrayHandle<vtkm::Id> faceToCellIdMap;
    faceToCellIdMap.Allocate(numberOfExternalFaces);

    // Build the connectivity of the external faces
    invoke(BuildConnectivity(),
           scatterCullInternalFaces,
           cellAndFaceIdOfFacesPerHashGroupVec,
           inCellSet,
           externalFacesShapes,
           externalFacesConnectivityGroupVec,
           faceToCellIdMap);

    if (!polyDataConnectivitySize)
    {
      outCellSet.Fill(inCellSet.GetNumberOfPoints(),
                      externalFacesShapes,
                      externalFacesConnectivity,
                      pointsPerExternalFaceOffsets);
      this->CellIdMap = faceToCellIdMap;
    }
    else
    {
      // Create a view that doesn't have the last offset:
      auto pointsPerExternalFaceOffsetsTrim = vtkm::cont::make_ArrayHandleView(
        pointsPerExternalFaceOffsets, 0, pointsPerExternalFaceOffsets.GetNumberOfValues() - 1);

      // Join poly data to face data output
      vtkm::cont::ArrayHandleConcatenate<ShapeArrayType, ShapeArrayType> faceShapesArray(
        externalFacesShapes, polyDataShapes);
      ShapeArrayType joinedShapesArray;
      vtkm::cont::ArrayCopy(faceShapesArray, joinedShapesArray);

      vtkm::cont::ArrayHandleConcatenate<PointCountArrayType, PointCountArrayType> pointCountArray(
        numPointsPerExternalFace, polyDataPointCount);
      PointCountArrayType joinedPointCountArray;
      vtkm::cont::ArrayCopy(pointCountArray, joinedPointCountArray);

      vtkm::cont::ArrayHandleConcatenate<ConnectivityArrayType, ConnectivityArrayType>
        connectivityArray(externalFacesConnectivity, polyDataConnectivity);
      ConnectivityArrayType joinedConnectivity;
      vtkm::cont::ArrayCopy(connectivityArray, joinedConnectivity);

      // Adjust poly data offsets array with face connectivity size before join
      auto adjustedPolyDataOffsets = vtkm::cont::make_ArrayHandleTransform(
        polyDataOffsets, BiasFunctor<vtkm::Id>(externalFacesConnectivity.GetNumberOfValues()));

      auto offsetsArray = vtkm::cont::make_ArrayHandleConcatenate(pointsPerExternalFaceOffsetsTrim,
                                                                  adjustedPolyDataOffsets);
      OffsetsArrayType joinedOffsets;
      // Need to compile a special device copy because the precompiled ArrayCopy does not
      // know how to copy the ArrayHandleTransform.
      vtkm::cont::ArrayCopyDevice(offsetsArray, joinedOffsets);

      vtkm::cont::ArrayHandleConcatenate<vtkm::cont::ArrayHandle<vtkm::Id>,
                                         vtkm::cont::ArrayHandle<vtkm::Id>>
        cellIdMapArray(faceToCellIdMap, polyDataCellIdMap);
      vtkm::cont::ArrayHandle<vtkm::Id> joinedCellIdMap;
      vtkm::cont::ArrayCopy(cellIdMapArray, joinedCellIdMap);

      outCellSet.Fill(
        inCellSet.GetNumberOfPoints(), joinedShapesArray, joinedConnectivity, joinedOffsets);
      this->CellIdMap = joinedCellIdMap;
    }
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetCellIdMap() const { return this->CellIdMap; }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
  bool PassPolyData;

}; //struct ExternalFaces
}
} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
