//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_worklet_ExternalFaces_h
#define viskores_worklet_ExternalFaces_h

#include <viskores/CellShape.h>
#include <viskores/Hash.h>
#include <viskores/Math.h>
#include <viskores/Swap.h>

#include <viskores/exec/CellFace.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayCopyDevice.h>
#include <viskores/cont/ArrayGetValues.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConcatenate.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/ArrayHandleView.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>
#include <viskores/cont/Field.h>

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

struct ExternalFaces
{
  //Worklet that returns the number of external faces for each structured cell
  class NumExternalFacesPerStructuredCell : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet,
                                  FieldOut numFacesInCell,
                                  FieldInPoint pointCoordinates);
    using ExecutionSignature = _2(CellShape, _3);
    using InputDomain = _1;

    VISKORES_CONT
    NumExternalFacesPerStructuredCell(const viskores::Vec3f_64& min_point,
                                      const viskores::Vec3f_64& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    VISKORES_EXEC
    static inline viskores::IdComponent CountExternalFacesOnDimension(viskores::Float64 grid_min,
                                                                      viskores::Float64 grid_max,
                                                                      viskores::Float64 cell_min,
                                                                      viskores::Float64 cell_max)
    {
      viskores::IdComponent count = 0;

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
    VISKORES_EXEC viskores::IdComponent operator()(CellShapeTag shape,
                                                   const PointCoordVecType& pointCoordinates) const
    {
      (void)shape; // C4100 false positive workaround
      VISKORES_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      viskores::IdComponent count = 0;

      count += CountExternalFacesOnDimension(
        MinPoint[0], MaxPoint[0], pointCoordinates[0][0], pointCoordinates[1][0]);

      count += CountExternalFacesOnDimension(
        MinPoint[1], MaxPoint[1], pointCoordinates[0][1], pointCoordinates[3][1]);

      count += CountExternalFacesOnDimension(
        MinPoint[2], MaxPoint[2], pointCoordinates[0][2], pointCoordinates[4][2]);

      return count;
    }

  private:
    viskores::Vec3f_64 MinPoint;
    viskores::Vec3f_64 MaxPoint;
  };


  //Worklet that finds face connectivity for each structured cell
  class BuildConnectivityStructured : public viskores::worklet::WorkletVisitCellsWithPoints
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

    using ScatterType = viskores::worklet::ScatterCounting;

    VISKORES_CONT
    BuildConnectivityStructured(const viskores::Vec3f_64& min_point,
                                const viskores::Vec3f_64& max_point)
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

    VISKORES_EXEC
    static inline bool FoundFaceOnDimension(viskores::Float64 grid_min,
                                            viskores::Float64 grid_max,
                                            viskores::Float64 cell_min,
                                            viskores::Float64 cell_max,
                                            viskores::IdComponent& faceIndex,
                                            viskores::IdComponent& count,
                                            viskores::IdComponent dimensionFaceOffset,
                                            viskores::IdComponent visitIndex)
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
    VISKORES_EXEC inline viskores::IdComponent FindFaceIndexForVisit(
      viskores::IdComponent visitIndex,
      const PointCoordVecType& pointCoordinates) const
    {
      viskores::IdComponent count = 0;
      viskores::IdComponent faceIndex = 0;
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
    VISKORES_EXEC void operator()(CellShapeTag shape,
                                  viskores::IdComponent visitIndex,
                                  viskores::Id inputIndex,
                                  const CellSetType& cellSet,
                                  viskores::UInt8& shapeOut,
                                  viskores::IdComponent& numFacePointsOut,
                                  ConnectivityType& faceConnectivity,
                                  const PointCoordVecType& pointCoordinates) const
    {
      VISKORES_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      viskores::IdComponent faceIndex = FindFaceIndexForVisit(visitIndex, pointCoordinates);

      viskores::IdComponent numFacePoints;
      viskores::exec::CellFaceNumberOfPoints(faceIndex, shape, numFacePoints);
      VISKORES_ASSERT(numFacePoints == faceConnectivity.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(inputIndex);

      shapeOut = viskores::CELL_SHAPE_QUAD;
      numFacePointsOut = 4;

      for (viskores::IdComponent facePointIndex = 0; facePointIndex < numFacePoints;
           facePointIndex++)
      {
        viskores::IdComponent localFaceIndex;
        viskores::ErrorCode status =
          viskores::exec::CellFaceLocalIndex(facePointIndex, faceIndex, shape, localFaceIndex);
        if (status == viskores::ErrorCode::Success)
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
    viskores::Vec3f_64 MinPoint;
    viskores::Vec3f_64 MaxPoint;
  };

  // Worklet that returns the number of faces for each cell/shape
  class NumFacesPerCell : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut numFacesInCell);
    using ExecutionSignature = void(CellShape, _2);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VISKORES_EXEC void operator()(CellShapeTag shape, viskores::IdComponent& numFacesInCell) const
    {
      viskores::exec::CellFaceNumberOfFaces(shape, numFacesInCell);
    }
  };

  // Worklet that identifies a cell face by a hash value. Not necessarily completely unique.
  class FaceHash : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldOutCell cellFaceHashes);
    using ExecutionSignature = void(CellShape, PointIndices, _2);
    using InputDomain = _1;

    template <typename CellShapeTag, typename CellNodeVecType, typename CellFaceHashes>
    VISKORES_EXEC void operator()(const CellShapeTag shape,
                                  const CellNodeVecType& cellNodeIds,
                                  CellFaceHashes& cellFaceHashes) const
    {
      const viskores::IdComponent numFaces = cellFaceHashes.GetNumberOfComponents();
      for (viskores::IdComponent faceIndex = 0; faceIndex < numFaces; ++faceIndex)
      {
        viskores::Id minFacePointId;
        viskores::exec::CellFaceMinPointId(faceIndex, shape, cellNodeIds, minFacePointId);
        cellFaceHashes[faceIndex] = static_cast<viskores::HashType>(minFacePointId);
      }
    }
  };

  // Worklet that identifies the number of faces per hash.
  class NumFacesPerHash : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn faceHashes, AtomicArrayInOut numFacesPerHash);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    template <typename NumFacesPerHashArray>
    VISKORES_EXEC void operator()(const viskores::HashType& faceHash,
                                  NumFacesPerHashArray& numFacesPerHash) const
    {
      // MemoryOrder::Relaxed is safe here, since we're not using the atomics for synchronization.
      numFacesPerHash.Add(faceHash, 1, viskores::MemoryOrder::Relaxed);
    }
  };

  /// Class to pack and unpack cell and face indices to/from a single integer.
  class CellFaceIdPacker
  {
  public:
    using CellAndFaceIdType = viskores::UInt64;
    using CellIdType = viskores::Id;
    using FaceIdType = viskores::Int8;

    static constexpr CellAndFaceIdType GetNumFaceIdBits()
    {
      static_assert(viskores::exec::detail::CellFaceTables::MAX_NUM_FACES == 6,
                    "MAX_NUM_FACES must be 6, otherwise, update GetNumFaceIdBits");
      return 3;
    }
    static constexpr CellAndFaceIdType GetFaceMask() { return (1ULL << GetNumFaceIdBits()) - 1; }

    /// Pack function for both cellIndex and faceIndex
    VISKORES_EXEC inline static constexpr CellAndFaceIdType Pack(const CellIdType& cellIndex,
                                                                 const FaceIdType& faceIndex)
    {
      // Pack the cellIndex in the higher bits, leaving FACE_INDEX_BITS bits for faceIndex
      return static_cast<CellAndFaceIdType>(cellIndex << GetNumFaceIdBits()) |
        static_cast<CellAndFaceIdType>(faceIndex);
    }

    /// Unpacking function for both cellIndex and faceIndex
    /// This is templated because we don't want to create a copy of the packedCellAndFaceId value.
    template <typename TCellAndFaceIdType>
    VISKORES_EXEC inline static constexpr void Unpack(const TCellAndFaceIdType& packedCellAndFaceId,
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
  class BuildFacesPerHash : public viskores::worklet::WorkletMapField
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
    VISKORES_EXEC void operator()(
      viskores::Id inputIndex,
      const CellFaceHashes& cellFaceHashes,
      NumFacesPerHashArray& numFacesPerHash,
      CellAndFaceIdOfFacePerHashArray& cellAndFaceIdOfFacesPerHash) const
    {
      const viskores::IdComponent numFaces = cellFaceHashes.GetNumberOfComponents();
      for (viskores::IdComponent faceIndex = 0; faceIndex < numFaces; ++faceIndex)
      {
        const auto& faceHash = cellFaceHashes[faceIndex];
        // MemoryOrder::Relaxed is safe here, since we're not using the atomics for synchronization.
        const viskores::IdComponent hashFaceIndex =
          numFacesPerHash.Add(faceHash, -1, viskores::MemoryOrder::Relaxed) - 1;
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
  class FaceCounts : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldInOut cellAndFaceIdOfFacesInHash,
                                  WholeCellSetIn<> inputCells,
                                  FieldOut externalFacesInHash);
    using ExecutionSignature = _3(_1, _2);
    using InputDomain = _1;

    template <typename CellAndFaceIdOfFacesInHash, typename CellSetType>
    VISKORES_EXEC viskores::IdComponent operator()(
      CellAndFaceIdOfFacesInHash& cellAndFaceIdOfFacesInHash,
      const CellSetType& cellSet) const
    {
      const viskores::IdComponent numFacesInHash =
        cellAndFaceIdOfFacesInHash.GetNumberOfComponents();

      static constexpr viskores::IdComponent FACE_CANONICAL_IDS_CACHE_SIZE = 100;
      if (numFacesInHash <= 1)
      {
        // Either one or zero faces. If there is one, it's external, In either case, do nothing.
        return numFacesInHash;
      }
      else if (numFacesInHash <= FACE_CANONICAL_IDS_CACHE_SIZE) // Fast path with caching
      {
        CellFaceIdPacker::CellIdType myCellId;
        CellFaceIdPacker::FaceIdType myFaceId;
        viskores::Vec<viskores::Id3, FACE_CANONICAL_IDS_CACHE_SIZE> faceCanonicalIds;
        for (viskores::IdComponent faceIndex = 0; faceIndex < numFacesInHash; ++faceIndex)
        {
          CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[faceIndex], myCellId, myFaceId);
          viskores::exec::CellFaceCanonicalId(myFaceId,
                                              cellSet.GetCellShape(myCellId),
                                              cellSet.GetIndices(myCellId),
                                              faceCanonicalIds[faceIndex]);
        }
        // Start by assuming all faces are duplicate, then remove two for each duplicate pair found.
        viskores::IdComponent numExternalFaces = 0;
        // Iterate over the faces in the hash in reverse order (to minimize the swaps being
        // performed) and find duplicates faces. Put duplicates at the end and unique faces
        // at the beginning. Narrow this range until all unique/duplicate are found.
        for (viskores::IdComponent myIndex = numFacesInHash - 1; myIndex >= numExternalFaces;)
        {
          bool isInternal = false;
          const viskores::Id3& myFace = faceCanonicalIds[myIndex];
          viskores::IdComponent otherIndex;
          for (otherIndex = myIndex - 1; otherIndex >= numExternalFaces; --otherIndex)
          {
            const viskores::Id3& otherFace = faceCanonicalIds[otherIndex];
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
              viskores::Swap(faceCanonicalIds[otherIndex], faceCanonicalIds[myIndex]);
            }
            --myIndex; // decrement for the second duplicate face
          }
          else // If the face is external
          {    // swap it to the front of the list, to avoid revisiting it.
            if (myIndex != numExternalFaces)
            {
              FaceCounts::SwapFace<CellFaceIdPacker::CellAndFaceIdType>(
                cellAndFaceIdOfFacesInHash[myIndex], cellAndFaceIdOfFacesInHash[numExternalFaces]);
              viskores::Swap(faceCanonicalIds[myIndex], faceCanonicalIds[numExternalFaces]);
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
        viskores::Id3 myFace, otherFace;
        // Start by assuming all faces are duplicate, then remove two for each duplicate pair found.
        viskores::IdComponent numExternalFaces = 0;
        // Iterate over the faces in the hash in reverse order (to minimize the swaps being
        // performed) and find duplicates faces. Put duplicates at the end and unique faces
        // at the beginning. Narrow this range until all unique/duplicate are found.
        for (viskores::IdComponent myIndex = numFacesInHash - 1; myIndex >= numExternalFaces;)
        {
          bool isInternal = false;
          CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[myIndex], myCellId, myFaceId);
          viskores::exec::CellFaceCanonicalId(
            myFaceId, cellSet.GetCellShape(myCellId), cellSet.GetIndices(myCellId), myFace);
          viskores::IdComponent otherIndex;
          for (otherIndex = myIndex - 1; otherIndex >= numExternalFaces; --otherIndex)
          {
            CellFaceIdPacker::Unpack(
              cellAndFaceIdOfFacesInHash[otherIndex], otherCellId, otherFaceId);
            viskores::exec::CellFaceCanonicalId(otherFaceId,
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
    VISKORES_EXEC inline static void SwapFace(FaceRefT&& cellAndFace1, FaceRefT&& cellAndFace2)
    {
      const FaceT tmpCellAndFace = cellAndFace1;
      cellAndFace1 = cellAndFace2;
      cellAndFace2 = tmpCellAndFace;
    }
  };

public:
  // Worklet that returns the number of points for each outputted face.
  // Have to manage the case where multiple faces have the same hash.
  class NumPointsPerFace : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellAndFaceIdOfFacesInHash,
                                  WholeCellSetIn<> inputCells,
                                  FieldOut numPointsInExternalFace);
    using ExecutionSignature = void(_1, _2, VisitIndex, _3);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template <typename CellAndFaceIdOfFacesInHash, typename CellSetType>
    VISKORES_EXEC void operator()(const CellAndFaceIdOfFacesInHash& cellAndFaceIdOfFacesInHash,
                                  const CellSetType& cellSet,
                                  viskores::IdComponent visitIndex,
                                  viskores::IdComponent& numPointsInExternalFace) const
    {
      // external faces are first, so we can use the visit index directly
      CellFaceIdPacker::CellIdType myCellId;
      CellFaceIdPacker::FaceIdType myFaceId;
      CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[visitIndex], myCellId, myFaceId);

      viskores::exec::CellFaceNumberOfPoints(
        myFaceId, cellSet.GetCellShape(myCellId), numPointsInExternalFace);
    }
  };

  // Worklet that returns the shape and connectivity for each external face
  class BuildConnectivity : public viskores::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn cellAndFaceIdOfFacesInHash,
                                  WholeCellSetIn<> inputCells,
                                  FieldOut shapesOut,
                                  FieldOut connectivityOut,
                                  FieldOut cellIdMapOut);
    using ExecutionSignature = void(_1, _2, VisitIndex, _3, _4, _5);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template <typename CellAndFaceIdOfFacesInHash, typename CellSetType, typename ConnectivityType>
    VISKORES_EXEC void operator()(const CellAndFaceIdOfFacesInHash& cellAndFaceIdOfFacesInHash,
                                  const CellSetType& cellSet,
                                  viskores::IdComponent visitIndex,
                                  viskores::UInt8& shapeOut,
                                  ConnectivityType& connectivityOut,
                                  viskores::Id& cellIdMapOut) const
    {
      // external faces are first, so we can use the visit index directly
      CellFaceIdPacker::CellIdType myCellId;
      CellFaceIdPacker::FaceIdType myFaceId;
      CellFaceIdPacker::Unpack(cellAndFaceIdOfFacesInHash[visitIndex], myCellId, myFaceId);

      const typename CellSetType::CellShapeTag shapeIn = cellSet.GetCellShape(myCellId);
      viskores::exec::CellFaceShape(myFaceId, shapeIn, shapeOut);
      cellIdMapOut = myCellId;

      viskores::IdComponent numFacePoints;
      viskores::exec::CellFaceNumberOfPoints(myFaceId, shapeIn, numFacePoints);
      VISKORES_ASSERT(numFacePoints == connectivityOut.GetNumberOfComponents());

      const typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(myCellId);
      for (viskores::IdComponent facePointIndex = 0; facePointIndex < numFacePoints;
           ++facePointIndex)
      {
        viskores::IdComponent localFaceIndex;
        const viskores::ErrorCode status =
          viskores::exec::CellFaceLocalIndex(facePointIndex, myFaceId, shapeIn, localFaceIndex);
        if (status == viskores::ErrorCode::Success)
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

  class IsPolyDataCell : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn inCellSet, FieldOut isPolyDataCell);
    using ExecutionSignature = _2(CellShape);
    using InputDomain = _1;

    template <typename CellShapeTag>
    VISKORES_EXEC viskores::IdComponent operator()(CellShapeTag shape) const
    {
      viskores::IdComponent numFaces;
      viskores::exec::CellFaceNumberOfFaces(shape, numFaces);
      return !numFaces;
    }
  };

  class CountPolyDataCellPoints : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = viskores::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inCellSet, FieldOut numPoints);
    using ExecutionSignature = _2(PointCount);
    using InputDomain = _1;

    VISKORES_EXEC viskores::Id operator()(viskores::Id count) const { return count; }
  };

  class PassPolyDataCells : public viskores::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ScatterType = viskores::worklet::ScatterCounting;

    using ControlSignature = void(CellSetIn inputTopology,
                                  FieldOut shapes,
                                  FieldOut pointIndices,
                                  FieldOut cellIdMapOut);
    using ExecutionSignature = void(CellShape, PointIndices, InputIndex, _2, _3, _4);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    VISKORES_EXEC void operator()(const CellShape& inShape,
                                  const InPointIndexType& inPoints,
                                  viskores::Id inputIndex,
                                  viskores::UInt8& outShape,
                                  OutPointIndexType& outPoints,
                                  viskores::Id& cellIdMapOut) const
    {
      cellIdMapOut = inputIndex;
      outShape = inShape.Id;

      viskores::IdComponent numPoints = inPoints.GetNumberOfComponents();
      VISKORES_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }
  };

  template <typename T>
  struct BiasFunctor
  {
    VISKORES_EXEC_CONT
    explicit BiasFunctor(T bias = T(0))
      : Bias(bias)
    {
    }

    VISKORES_EXEC_CONT
    T operator()(T x) const { return x + this->Bias; }

    T Bias;
  };

public:
  VISKORES_CONT
  ExternalFaces()
    : PassPolyData(true)
  {
  }

  VISKORES_CONT
  void SetPassPolyData(bool flag) { this->PassPolyData = flag; }

  VISKORES_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }

  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }


  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry for regular grids.
  ///
  /// Faster Run() method for uniform and rectilinear grid types.
  /// Uses grid extents to find cells on the boundaries of the grid.
  template <typename ShapeStorage, typename ConnectivityStorage, typename OffsetsStorage>
  VISKORES_CONT void Run(
    const viskores::cont::CellSetStructured<3>& inCellSet,
    const viskores::cont::CoordinateSystem& coord,
    viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    // create an invoker
    viskores::cont::Invoker invoke;

    viskores::Vec3f_64 MinPoint;
    viskores::Vec3f_64 MaxPoint;

    viskores::Id3 PointDimensions = inCellSet.GetPointDimensions();

    using DefaultHandle = viskores::cont::ArrayHandle<viskores::FloatDefault>;
    using CartesianArrayHandle =
      viskores::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>;

    auto coordData = coord.GetData();
    if (coordData.CanConvert<CartesianArrayHandle>())
    {
      const auto vertices = coordData.AsArrayHandle<CartesianArrayHandle>();
      const auto vertsSize = vertices.GetNumberOfValues();
      const auto tmp = viskores::cont::ArrayGetValues({ 0, vertsSize - 1 }, vertices);
      MinPoint = tmp[0];
      MaxPoint = tmp[1];
    }
    else
    {
      auto vertices = coordData.AsArrayHandle<viskores::cont::ArrayHandleUniformPointCoordinates>();
      auto Coordinates = vertices.ReadPortal();

      MinPoint = Coordinates.GetOrigin();
      viskores::Vec3f_64 spacing = Coordinates.GetSpacing();

      viskores::Vec3f_64 unitLength;
      unitLength[0] = static_cast<viskores::Float64>(PointDimensions[0] - 1);
      unitLength[1] = static_cast<viskores::Float64>(PointDimensions[1] - 1);
      unitLength[2] = static_cast<viskores::Float64>(PointDimensions[2] - 1);
      MaxPoint = MinPoint + spacing * unitLength;
    }

    // Count the number of external faces per cell
    viskores::cont::ArrayHandle<viskores::IdComponent> numExternalFaces;
    invoke(NumExternalFacesPerStructuredCell(MinPoint, MaxPoint),
           inCellSet,
           numExternalFaces,
           coordData);

    viskores::Id numberOfExternalFaces =
      viskores::cont::Algorithm::Reduce(numExternalFaces, 0, viskores::Sum());

    viskores::worklet::ScatterCounting scatterCellToExternalFace(numExternalFaces);

    // Maps output cells to input cells. Store this for cell field mapping.
    this->CellIdMap = scatterCellToExternalFace.GetOutputToInputMap();

    numExternalFaces.ReleaseResources();

    viskores::Id connectivitySize = 4 * numberOfExternalFaces;
    viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorage> faceConnectivity;
    viskores::cont::ArrayHandle<viskores::UInt8, ShapeStorage> faceShapes;
    viskores::cont::ArrayHandle<viskores::IdComponent> facePointCount;
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
           viskores::cont::make_ArrayHandleGroupVec<4>(faceConnectivity),
           coordData);

    auto offsets = viskores::cont::ConvertNumComponentsToOffsets(facePointCount);

    outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, faceConnectivity, offsets);
  }

  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry
  template <typename InCellSetType,
            typename ShapeStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage>
  VISKORES_CONT void Run(
    const InCellSetType& inCellSet,
    viskores::cont::CellSetExplicit<ShapeStorage, ConnectivityStorage, OffsetsStorage>& outCellSet)
  {
    using PointCountArrayType = viskores::cont::ArrayHandle<viskores::IdComponent>;
    using ShapeArrayType = viskores::cont::ArrayHandle<viskores::UInt8, ShapeStorage>;
    using OffsetsArrayType = viskores::cont::ArrayHandle<viskores::Id, OffsetsStorage>;
    using ConnectivityArrayType = viskores::cont::ArrayHandle<viskores::Id, ConnectivityStorage>;

    // create an invoker
    viskores::cont::Invoker invoke;

    // Create an array to store the number of faces per cell
    viskores::cont::ArrayHandle<viskores::IdComponent> numFacesPerCell;

    // Compute the number of faces per cell
    invoke(NumFacesPerCell(), inCellSet, numFacesPerCell);

    // Compute the offsets into a packed array holding face information for each cell.
    viskores::Id totalNumberOfFaces;
    viskores::cont::ArrayHandle<viskores::Id> facesPerCellOffsets;
    viskores::cont::ConvertNumComponentsToOffsets(
      numFacesPerCell, facesPerCellOffsets, totalNumberOfFaces);
    // Release the resources of numFacesPerCell that is not needed anymore
    numFacesPerCell.ReleaseResources();

    PointCountArrayType polyDataPointCount;
    ShapeArrayType polyDataShapes;
    OffsetsArrayType polyDataOffsets;
    ConnectivityArrayType polyDataConnectivity;
    viskores::cont::ArrayHandle<viskores::Id> polyDataCellIdMap;
    viskores::Id polyDataConnectivitySize = 0;
    if (this->PassPolyData)
    {
      viskores::cont::ArrayHandle<viskores::IdComponent> isPolyDataCell;

      invoke(IsPolyDataCell(), inCellSet, isPolyDataCell);

      viskores::worklet::ScatterCounting scatterPolyDataCells(isPolyDataCell);

      isPolyDataCell.ReleaseResources();

      if (scatterPolyDataCells.GetOutputRange(inCellSet.GetNumberOfCells()) != 0)
      {
        invoke(CountPolyDataCellPoints(), scatterPolyDataCells, inCellSet, polyDataPointCount);

        viskores::cont::ConvertNumComponentsToOffsets(
          polyDataPointCount, polyDataOffsets, polyDataConnectivitySize);

        polyDataConnectivity.Allocate(polyDataConnectivitySize);

        invoke(
          PassPolyDataCells(),
          scatterPolyDataCells,
          inCellSet,
          polyDataShapes,
          viskores::cont::make_ArrayHandleGroupVecVariable(polyDataConnectivity, polyDataOffsets),
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
    viskores::cont::ArrayHandle<viskores::HashType> faceHashes;
    faceHashes.Allocate(totalNumberOfFaces);

    // Create a group vec array to access the faces of each cell conveniently
    auto faceHashesGroupVec =
      viskores::cont::make_ArrayHandleGroupVecVariable(faceHashes, facesPerCellOffsets);

    // Compute the hash values of the faces
    invoke(FaceHash(), inCellSet, faceHashesGroupVec);

    // Create an array to store the number of faces per hash
    const viskores::Id numberOfHashes = inCellSet.GetNumberOfPoints();
    viskores::cont::ArrayHandle<viskores::IdComponent> numFacesPerHash;
    numFacesPerHash.AllocateAndFill(numberOfHashes, 0);

    // Count the number of faces per hash
    invoke(NumFacesPerHash(), faceHashes, numFacesPerHash);

    // Compute the offsets for a packed array holding face information for each hash.
    viskores::cont::ArrayHandle<viskores::Id> facesPerHashOffsets;
    viskores::cont::ConvertNumComponentsToOffsets(numFacesPerHash, facesPerHashOffsets);

    // Create an array to store the cell and face ids of each face per hash
    viskores::cont::ArrayHandle<CellFaceIdPacker::CellAndFaceIdType> cellAndFaceIdOfFacesPerHash;
    cellAndFaceIdOfFacesPerHash.Allocate(totalNumberOfFaces);

    // Create a group vec array to access/write the cell and face ids of each face per hash
    auto cellAndFaceIdOfFacesPerHashGroupVec = viskores::cont::make_ArrayHandleGroupVecVariable(
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
    viskores::cont::ArrayHandle<viskores::IdComponent> numExternalFacesPerHash;
    numExternalFacesPerHash.Allocate(numberOfHashes);

    // Compute the number of external faces per hash
    invoke(FaceCounts(), cellAndFaceIdOfFacesPerHashGroupVec, inCellSet, numExternalFacesPerHash);

    // Create a scatter counting object to only access the hashes with external faces
    viskores::worklet::ScatterCounting scatterCullInternalFaces(numExternalFacesPerHash);
    const viskores::Id numberOfExternalFaces =
      scatterCullInternalFaces.GetOutputRange(numberOfHashes);
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
    viskores::Id connectivitySize;
    viskores::cont::ConvertNumComponentsToOffsets(
      numPointsPerExternalFace, pointsPerExternalFaceOffsets, connectivitySize);

    // Create an array to connectivity of the external faces
    ConnectivityArrayType externalFacesConnectivity;
    externalFacesConnectivity.Allocate(connectivitySize);

    // Create a group vec array to access the connectivity of each external face
    auto externalFacesConnectivityGroupVec = viskores::cont::make_ArrayHandleGroupVecVariable(
      externalFacesConnectivity, pointsPerExternalFaceOffsets);

    // Create an array to store the shape of the external faces
    ShapeArrayType externalFacesShapes;
    externalFacesShapes.Allocate(numberOfExternalFaces);

    // Create an array to store the cell id of the external faces
    viskores::cont::ArrayHandle<viskores::Id> faceToCellIdMap;
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
      auto pointsPerExternalFaceOffsetsTrim = viskores::cont::make_ArrayHandleView(
        pointsPerExternalFaceOffsets, 0, pointsPerExternalFaceOffsets.GetNumberOfValues() - 1);

      // Join poly data to face data output
      viskores::cont::ArrayHandleConcatenate<ShapeArrayType, ShapeArrayType> faceShapesArray(
        externalFacesShapes, polyDataShapes);
      ShapeArrayType joinedShapesArray;
      viskores::cont::ArrayCopy(faceShapesArray, joinedShapesArray);

      viskores::cont::ArrayHandleConcatenate<PointCountArrayType, PointCountArrayType>
        pointCountArray(numPointsPerExternalFace, polyDataPointCount);
      PointCountArrayType joinedPointCountArray;
      viskores::cont::ArrayCopy(pointCountArray, joinedPointCountArray);

      viskores::cont::ArrayHandleConcatenate<ConnectivityArrayType, ConnectivityArrayType>
        connectivityArray(externalFacesConnectivity, polyDataConnectivity);
      ConnectivityArrayType joinedConnectivity;
      viskores::cont::ArrayCopy(connectivityArray, joinedConnectivity);

      // Adjust poly data offsets array with face connectivity size before join
      auto adjustedPolyDataOffsets = viskores::cont::make_ArrayHandleTransform(
        polyDataOffsets, BiasFunctor<viskores::Id>(externalFacesConnectivity.GetNumberOfValues()));

      auto offsetsArray = viskores::cont::make_ArrayHandleConcatenate(
        pointsPerExternalFaceOffsetsTrim, adjustedPolyDataOffsets);
      OffsetsArrayType joinedOffsets;
      // Need to compile a special device copy because the precompiled ArrayCopy does not
      // know how to copy the ArrayHandleTransform.
      viskores::cont::ArrayCopyDevice(offsetsArray, joinedOffsets);

      viskores::cont::ArrayHandleConcatenate<viskores::cont::ArrayHandle<viskores::Id>,
                                             viskores::cont::ArrayHandle<viskores::Id>>
        cellIdMapArray(faceToCellIdMap, polyDataCellIdMap);
      viskores::cont::ArrayHandle<viskores::Id> joinedCellIdMap;
      viskores::cont::ArrayCopy(cellIdMapArray, joinedCellIdMap);

      outCellSet.Fill(
        inCellSet.GetNumberOfPoints(), joinedShapesArray, joinedConnectivity, joinedOffsets);
      this->CellIdMap = joinedCellIdMap;
    }
  }

  viskores::cont::ArrayHandle<viskores::Id> GetCellIdMap() const { return this->CellIdMap; }

private:
  viskores::cont::ArrayHandle<viskores::Id> CellIdMap;
  bool PassPolyData;

}; //struct ExternalFaces
}
} //namespace viskores::worklet

#endif //viskores_worklet_ExternalFaces_h
