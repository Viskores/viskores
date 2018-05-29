//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/CellIntersector.h>
#include <vtkm/rendering/raytracing/CellSampler.h>
#include <vtkm/rendering/raytracing/CellTables.h>
#include <vtkm/rendering/raytracing/ConnectivityBase.h>
#include <vtkm/rendering/raytracing/MeshConnectivityStructures.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{
template <typename FloatType>
template <typename Device>
void RayTracking<FloatType>::Compact(vtkm::cont::ArrayHandle<FloatType>& compactedDistances,
                                     vtkm::cont::ArrayHandle<UInt8>& masks,
                                     Device)
{
  //
  // These distances are stored in the rays, and it has
  // already been compacted.
  //
  CurrentDistance = compactedDistances;

  vtkm::cont::ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::UInt8>> castedMasks(masks);

  bool distance1IsEnter = EnterDist == &Distance1;

  vtkm::cont::ArrayHandle<FloatType> compactedDistance1;
  vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(Distance1, masks, compactedDistance1);
  Distance1 = compactedDistance1;

  vtkm::cont::ArrayHandle<FloatType> compactedDistance2;
  vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(Distance2, masks, compactedDistance2);
  Distance2 = compactedDistance2;

  vtkm::cont::ArrayHandle<vtkm::Int32> compactedExitFace;
  vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(ExitFace, masks, compactedExitFace);
  ExitFace = compactedExitFace;

  if (distance1IsEnter)
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }
  else
  {
    EnterDist = &Distance2;
    ExitDist = &Distance1;
  }
}

template <typename FloatType>
template <typename Device>
void RayTracking<FloatType>::Init(const vtkm::Id size,
                                  vtkm::cont::ArrayHandle<FloatType>& distances,
                                  Device)
{

  ExitFace.PrepareForOutput(size, Device());
  Distance1.PrepareForOutput(size, Device());
  Distance2.PrepareForOutput(size, Device());

  CurrentDistance = distances;
  //
  // Set the initial Distances
  //
  vtkm::worklet::DispatcherMapField<CopyAndOffset<FloatType>, Device>(
    CopyAndOffset<FloatType>(0.0f))
    .Invoke(distances, *EnterDist);

  //
  // Init the exit faces. This value is used to load the next cell
  // base on the cell and face it left
  //
  vtkm::worklet::DispatcherMapField<MemSet<vtkm::Int32>, Device>(MemSet<vtkm::Int32>(-1))
    .Invoke(ExitFace);

  vtkm::worklet::DispatcherMapField<MemSet<FloatType>, Device>(MemSet<FloatType>(-1))
    .Invoke(*ExitDist);
}

template <typename FloatType>
void RayTracking<FloatType>::Swap()
{
  vtkm::cont::ArrayHandle<FloatType>* tmpPtr;
  tmpPtr = EnterDist;
  EnterDist = ExitDist;
  ExitDist = tmpPtr;
}
} //namespace detail


//
//  Advance Ray
//      After a ray leaves the mesh, we need to check to see
//      of the ray re-enters the mesh within this domain. This
//      function moves the ray forward some offset to prevent
//      "shadowing" and hitting the same exit point.
//
template <typename FloatType>
class AdvanceRay : public vtkm::worklet::WorkletMapField
{
  FloatType Offset;

public:
  VTKM_CONT
  AdvanceRay(const FloatType offset = 0.00001)
    : Offset(offset)
  {
  }
  using ControlSignature = void(FieldIn<>, FieldInOut<>);
  using ExecutionSignature = void(_1, _2);

  VTKM_EXEC inline void operator()(const vtkm::UInt8& status, FloatType& distance) const
  {
    if (status == RAY_EXITED_MESH)
      distance += Offset;
  }
}; //class AdvanceRay

template <vtkm::Int32 CellType, typename FloatType, typename MeshType>
class LocateCell : public vtkm::worklet::WorkletMapField
{
private:
  MeshType MeshConn;
  CellIntersector<CellType> Intersector;

public:
  template <typename ConnectivityType>
  LocateCell(ConnectivityType& meshConn)
    : MeshConn(meshConn)
  {
  }

  using ControlSignature = void(FieldInOut<>,
                                WholeArrayIn<>,
                                FieldIn<>,
                                FieldInOut<>,
                                FieldInOut<>,
                                FieldInOut<>,
                                FieldInOut<>,
                                FieldIn<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8);

  template <typename PointPortalType>
  VTKM_EXEC inline void operator()(vtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   const vtkm::Vec<FloatType, 3>& dir,
                                   FloatType& enterDistance,
                                   FloatType& exitDistance,
                                   vtkm::Int32& enterFace,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Vec<FloatType, 3>& origin) const
  {
    if (enterFace != -1 && rayStatus == RAY_ACTIVE)
    {
      currentCell = MeshConn.GetConnectingCell(currentCell, enterFace);
      if (currentCell == -1)
        rayStatus = RAY_EXITED_MESH;
      enterFace = -1;
    }
    //This ray is dead or exited the mesh and needs re-entry
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    FloatType xpoints[8];
    FloatType ypoints[8];
    FloatType zpoints[8];
    vtkm::Id cellConn[8];
    FloatType distances[6];

    const vtkm::Int32 numIndices = MeshConn.GetCellIndices(cellConn, currentCell);
    //load local cell data
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(vertices, cellConn[i]);
      vtkm::Vec<FloatType, 3> point = vtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
      xpoints[i] = point[0];
      ypoints[i] = point[1];
      zpoints[i] = point[2];
    }
    const vtkm::UInt8 cellShape = MeshConn.GetCellShape(currentCell);
    Intersector.IntersectCell(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);

    CellTables tables;
    const vtkm::Int32 numFaces = tables.FaceLookUp(tables.CellTypeLookUp(cellShape), 1);
    //vtkm::Int32 minFace = 6;
    vtkm::Int32 maxFace = -1;

    FloatType minDistance = static_cast<FloatType>(1e32);
    FloatType maxDistance = static_cast<FloatType>(-1);
    int hitCount = 0;
    for (vtkm::Int32 i = 0; i < numFaces; ++i)
    {
      FloatType dist = distances[i];

      if (dist != -1)
      {
        hitCount++;
        if (dist < minDistance)
        {
          minDistance = dist;
          //minFace = i;
        }
        if (dist > maxDistance)
        {
          maxDistance = dist;
          maxFace = i;
        }
      }
    }

    if (maxDistance <= enterDistance || minDistance == maxDistance)
    {
      rayStatus = RAY_LOST;
    }
    else
    {
      enterDistance = minDistance;
      exitDistance = maxDistance;
      enterFace = maxFace;
    }

  } //operator
};  //class LocateCell

template <vtkm::Int32 CellType, typename FloatType, typename Device, typename MeshType>
class RayBumper : public vtkm::worklet::WorkletMapField
{
private:
  using FloatTypeHandle = typename vtkm::cont::ArrayHandle<FloatType>;
  using FloatTypePortal = typename FloatTypeHandle::template ExecutionTypes<Device>::Portal;
  FloatTypePortal DirectionsX;
  FloatTypePortal DirectionsY;
  FloatTypePortal DirectionsZ;
  MeshType MeshConn;
  CellIntersector<CellType> Intersector;
  const vtkm::UInt8 FailureStatus; // the status to assign ray if we fail to find the intersection
public:
  template <typename ConnectivityType>
  RayBumper(FloatTypeHandle dirsx,
            FloatTypeHandle dirsy,
            FloatTypeHandle dirsz,
            ConnectivityType meshConn,
            vtkm::UInt8 failureStatus = RAY_ABANDONED)
    : DirectionsX(dirsx.PrepareForInPlace(Device()))
    , DirectionsY(dirsy.PrepareForInPlace(Device()))
    , DirectionsZ(dirsz.PrepareForInPlace(Device()))
    , MeshConn(meshConn)
    , FailureStatus(failureStatus)
  {
  }


  using ControlSignature = void(FieldInOut<>,
                                WholeArrayIn<>,
                                FieldInOut<>,
                                FieldInOut<>,
                                FieldInOut<>,
                                FieldInOut<>,
                                FieldIn<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, WorkIndex, _6, _7);

  template <typename PointPortalType>
  VTKM_EXEC inline void operator()(vtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   FloatType& enterDistance,
                                   FloatType& exitDistance,
                                   vtkm::Int32& enterFace,
                                   const vtkm::Id& pixelIndex,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Vec<FloatType, 3>& origin) const
  {
    // We only process lost rays
    if (rayStatus != RAY_LOST)
    {
      return;
    }

    FloatType xpoints[8];
    FloatType ypoints[8];
    FloatType zpoints[8];
    vtkm::Id cellConn[8];
    FloatType distances[6];

    vtkm::Vec<FloatType, 3> centroid(0., 0., 0.);
    const vtkm::Int32 numIndices = MeshConn.GetCellIndices(cellConn, currentCell);
    //load local cell data
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(vertices, cellConn[i]);
      vtkm::Vec<FloatType, 3> point = vtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
      centroid = centroid + point;
      xpoints[i] = point[0];
      ypoints[i] = point[1];
      zpoints[i] = point[2];
    }

    FloatType invNumIndices = static_cast<FloatType>(1.) / static_cast<FloatType>(numIndices);
    centroid[0] = centroid[0] * invNumIndices;
    centroid[1] = centroid[1] * invNumIndices;
    centroid[2] = centroid[2] * invNumIndices;

    vtkm::Vec<FloatType, 3> toCentroid = centroid - origin;
    vtkm::Normalize(toCentroid);

    vtkm::Vec<FloatType, 3> dir(
      DirectionsX.Get(pixelIndex), DirectionsY.Get(pixelIndex), DirectionsZ.Get(pixelIndex));
    vtkm::Vec<FloatType, 3> bump = toCentroid - dir;
    dir = dir + RAY_TUG_EPSILON * bump;

    vtkm::Normalize(dir);

    DirectionsX.Set(pixelIndex, dir[0]);
    DirectionsY.Set(pixelIndex, dir[1]);
    DirectionsZ.Set(pixelIndex, dir[2]);

    const vtkm::UInt8 cellShape = MeshConn.GetCellShape(currentCell);
    Intersector.IntersectCell(xpoints, ypoints, zpoints, dir, origin, distances, cellShape);

    CellTables tables;
    const vtkm::Int32 numFaces = tables.FaceLookUp(tables.CellTypeLookUp(cellShape), 1);

    //vtkm::Int32 minFace = 6;
    vtkm::Int32 maxFace = -1;
    FloatType minDistance = static_cast<FloatType>(1e32);
    FloatType maxDistance = static_cast<FloatType>(-1);
    int hitCount = 0;
    for (int i = 0; i < numFaces; ++i)
    {
      FloatType dist = distances[i];

      if (dist != -1)
      {
        hitCount++;
        if (dist < minDistance)
        {
          minDistance = dist;
          //minFace = i;
        }
        if (dist >= maxDistance)
        {
          maxDistance = dist;
          maxFace = i;
        }
      }
    }
    if (minDistance >= maxDistance)
    {
      rayStatus = FailureStatus;
    }
    else
    {
      enterDistance = minDistance;
      exitDistance = maxDistance;
      enterFace = maxFace;
      rayStatus = RAY_ACTIVE; //re-activate ray
    }

  } //operator
};  //class RayBumper

template <typename FloatType>
class AddPathLengths : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  AddPathLengths() {}

  using ControlSignature = void(FieldIn<RayStatusType>,            // ray status
                                FieldIn<ScalarRenderingTypes>,     // cell enter distance
                                FieldIn<ScalarRenderingTypes>,     // cell exit distance
                                FieldInOut<ScalarRenderingTypes>); // ray absorption data

  using ExecutionSignature = void(_1, _2, _3, _4);

  VTKM_EXEC inline void operator()(const vtkm::UInt8& rayStatus,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& distance) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }

    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;
    distance += segmentLength;
  }
};

template <typename FloatType>
class Integrate : public vtkm::worklet::WorkletMapField
{
private:
  const vtkm::Int32 NumBins;

public:
  VTKM_CONT
  Integrate(const vtkm::Int32 numBins)
    : NumBins(numBins)
  {
  }

  using ControlSignature = void(FieldIn<RayStatusType>,                // ray status
                                FieldIn<ScalarRenderingTypes>,         // cell enter distance
                                FieldIn<ScalarRenderingTypes>,         // cell exit distance
                                FieldInOut<ScalarRenderingTypes>,      // current distance
                                WholeArrayIn<ScalarRenderingTypes>,    // cell absorption data array
                                WholeArrayInOut<ScalarRenderingTypes>, // ray absorption data
                                FieldIn<IdType>);                      // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);

  template <typename CellDataPortalType, typename RayDataPortalType>
  VTKM_EXEC inline void operator()(const vtkm::UInt8& rayStatus,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const CellDataPortalType& cellData,
                                   RayDataPortalType& energyBins,
                                   const vtkm::Id& currentCell,
                                   const vtkm::Id& rayIndex) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;

    vtkm::Id rayOffset = NumBins * rayIndex;
    vtkm::Id cellOffset = NumBins * currentCell;
    for (vtkm::Int32 i = 0; i < NumBins; ++i)
    {
      BOUNDS_CHECK(cellData, cellOffset + i);
      FloatType absorb = static_cast<FloatType>(cellData.Get(cellOffset + i));

      absorb = vtkm::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(energyBins, rayOffset + i);
      FloatType intensity = static_cast<FloatType>(energyBins.Get(rayOffset + i));
      energyBins.Set(rayOffset + i, intensity * absorb);
    }
    currentDistance = exitDistance;
  }
};

template <typename FloatType>
class IntegrateEmission : public vtkm::worklet::WorkletMapField
{
private:
  const vtkm::Int32 NumBins;
  bool DivideEmisByAbsorb;

public:
  VTKM_CONT
  IntegrateEmission(const vtkm::Int32 numBins, const bool divideEmisByAbsorb)
    : NumBins(numBins)
    , DivideEmisByAbsorb(divideEmisByAbsorb)
  {
  }

  using ControlSignature = void(FieldIn<>,                          // ray status
                                FieldIn<>,                          // cell enter distance
                                FieldIn<>,                          // cell exit distance
                                FieldInOut<>,                       // current distance
                                WholeArrayIn<ScalarRenderingTypes>, // cell absorption data array
                                WholeArrayIn<ScalarRenderingTypes>, // cell emission data array
                                WholeArrayInOut<>,                  // ray absorption data
                                WholeArrayInOut<>,                  // ray emission data
                                FieldIn<>);                         // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, WorkIndex);

  template <typename CellAbsPortalType, typename CellEmisPortalType, typename RayDataPortalType>
  VTKM_EXEC inline void operator()(const vtkm::UInt8& rayStatus,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const CellAbsPortalType& absorptionData,
                                   const CellEmisPortalType& emissionData,
                                   RayDataPortalType& absorptionBins,
                                   RayDataPortalType& emissionBins,
                                   const vtkm::Id& currentCell,
                                   const vtkm::Id& rayIndex) const
  {
    if (rayStatus != RAY_ACTIVE)
    {
      return;
    }
    if (exitDistance <= enterDistance)
    {
      return;
    }

    FloatType segmentLength = exitDistance - enterDistance;

    vtkm::Id rayOffset = NumBins * rayIndex;
    vtkm::Id cellOffset = NumBins * currentCell;
    for (vtkm::Int32 i = 0; i < NumBins; ++i)
    {
      BOUNDS_CHECK(absorptionData, cellOffset + i);
      FloatType absorb = static_cast<FloatType>(absorptionData.Get(cellOffset + i));
      BOUNDS_CHECK(emissionData, cellOffset + i);
      FloatType emission = static_cast<FloatType>(emissionData.Get(cellOffset + i));

      if (DivideEmisByAbsorb)
      {
        emission /= absorb;
      }

      FloatType tmp = vtkm::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(absorptionBins, rayOffset + i);

      //
      // Traditionally, we would only keep track of a single intensity value per ray
      // per bin and we would integrate from the beginning to end of the ray. In a
      // distributed memory setting, we would move cell data around so that the
      // entire ray could be traced, but in situ, moving that much cell data around
      // could blow memory. Here we are keeping track of two values. Total absorption
      // through this contiguous segment of the mesh, and the amount of emitted energy
      // that makes it out of this mesh segment. If this is really run on a single node,
      // we can get the final energy value by multiplying the background intensity by
      // the total absorption of the mesh segment and add in the amount of emitted
      // energy that escapes.
      //
      FloatType absorbIntensity = static_cast<FloatType>(absorptionBins.Get(rayOffset + i));
      FloatType emissionIntensity = static_cast<FloatType>(emissionBins.Get(rayOffset + i));

      absorptionBins.Set(rayOffset + i, absorbIntensity * tmp);

      emissionIntensity = emissionIntensity * tmp + emission * (1.f - tmp);

      BOUNDS_CHECK(emissionBins, rayOffset + i);
      emissionBins.Set(rayOffset + i, emissionIntensity);
    }
    currentDistance = exitDistance;
  }
};
//
//  IdentifyMissedRay is a debugging routine that detects
//  rays that fail to have any value because of a external
//  intersection and cell intersection mismatch
//
//
class IdentifyMissedRay : public vtkm::worklet::WorkletMapField
{
public:
  vtkm::Id Width;
  vtkm::Id Height;
  vtkm::Vec<vtkm::Float32, 4> BGColor;
  IdentifyMissedRay(const vtkm::Id width,
                    const vtkm::Id height,
                    vtkm::Vec<vtkm::Float32, 4> bgcolor)
    : Width(width)
    , Height(height)
    , BGColor(bgcolor)
  {
  }
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>);
  using ExecutionSignature = void(_1, _2);


  VTKM_EXEC inline bool IsBGColor(const vtkm::Vec<vtkm::Float32, 4> color) const
  {
    bool isBG = false;

    if (color[0] == BGColor[0] && color[1] == BGColor[1] && color[2] == BGColor[2] &&
        color[3] == BGColor[3])
      isBG = true;
    return isBG;
  }

  template <typename ColorBufferType>
  VTKM_EXEC inline void operator()(const vtkm::Id& pixelId, ColorBufferType& buffer) const
  {
    vtkm::Id x = pixelId % Width;
    vtkm::Id y = pixelId / Width;

    // Conservative check, we only want to check pixels in the middle
    if (x <= 0 || y <= 0)
      return;
    if (x >= Width - 1 || y >= Height - 1)
      return;
    vtkm::Vec<vtkm::Float32, 4> pixel;
    pixel[0] = static_cast<vtkm::Float32>(buffer.Get(pixelId * 4 + 0));
    pixel[1] = static_cast<vtkm::Float32>(buffer.Get(pixelId * 4 + 1));
    pixel[2] = static_cast<vtkm::Float32>(buffer.Get(pixelId * 4 + 2));
    pixel[3] = static_cast<vtkm::Float32>(buffer.Get(pixelId * 4 + 3));
    if (!IsBGColor(pixel))
      return;
    vtkm::Id p0 = (y)*Width + (x + 1);
    vtkm::Id p1 = (y)*Width + (x - 1);
    vtkm::Id p2 = (y + 1) * Width + (x);
    vtkm::Id p3 = (y - 1) * Width + (x);
    pixel[0] = static_cast<vtkm::Float32>(buffer.Get(p0 * 4 + 0));
    pixel[1] = static_cast<vtkm::Float32>(buffer.Get(p0 * 4 + 1));
    pixel[2] = static_cast<vtkm::Float32>(buffer.Get(p0 * 4 + 2));
    pixel[3] = static_cast<vtkm::Float32>(buffer.Get(p0 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<vtkm::Float32>(buffer.Get(p1 * 4 + 0));
    pixel[1] = static_cast<vtkm::Float32>(buffer.Get(p1 * 4 + 1));
    pixel[2] = static_cast<vtkm::Float32>(buffer.Get(p1 * 4 + 2));
    pixel[3] = static_cast<vtkm::Float32>(buffer.Get(p1 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<vtkm::Float32>(buffer.Get(p2 * 4 + 0));
    pixel[1] = static_cast<vtkm::Float32>(buffer.Get(p2 * 4 + 1));
    pixel[2] = static_cast<vtkm::Float32>(buffer.Get(p2 * 4 + 2));
    pixel[3] = static_cast<vtkm::Float32>(buffer.Get(p2 * 4 + 3));
    if (IsBGColor(pixel))
      return;
    pixel[0] = static_cast<vtkm::Float32>(buffer.Get(p3 * 4 + 0));
    pixel[1] = static_cast<vtkm::Float32>(buffer.Get(p3 * 4 + 1));
    pixel[2] = static_cast<vtkm::Float32>(buffer.Get(p3 * 4 + 2));
    pixel[3] = static_cast<vtkm::Float32>(buffer.Get(p3 * 4 + 3));
    if (IsBGColor(pixel))
      return;

    printf("Possible error ray missed ray %d\n", (int)pixelId);
  }
};

template <vtkm::Int32 CellType, typename FloatType, typename Device, typename MeshType>
class SampleCellAssocCells : public vtkm::worklet::WorkletMapField
{
private:
  using ColorHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
  using ColorBuffer = typename vtkm::cont::ArrayHandle<FloatType>;
  using ColorConstPortal = typename ColorHandle::ExecutionTypes<Device>::PortalConst;
  using ColorPortal = typename ColorBuffer::template ExecutionTypes<Device>::Portal;

  CellSampler<CellType> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;
  ColorPortal FrameBuffer;
  ColorConstPortal ColorMap;
  MeshType MeshConn;
  vtkm::Int32 ColorMapSize;

public:
  template <typename ConnectivityType>
  SampleCellAssocCells(const FloatType& sampleDistance,
                       const FloatType& minScalar,
                       const FloatType& maxScalar,
                       ColorHandle& colorMap,
                       ColorBuffer& frameBuffer,
                       ConnectivityType& meshConn)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
    , ColorMap(colorMap.PrepareForInput(Device()))
    , MeshConn(meshConn)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
    ColorMapSize = static_cast<vtkm::Int32>(ColorMap.GetNumberOfValues());
    this->FrameBuffer = frameBuffer.PrepareForOutput(frameBuffer.GetNumberOfValues(), Device());
  }


  using ControlSignature = void(FieldIn<>,
                                WholeArrayIn<ScalarRenderingTypes>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldInOut<>,
                                FieldInOut<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, WorkIndex);

  template <typename ScalarPortalType>
  VTKM_EXEC inline void operator()(const vtkm::Id& currentCell,
                                   ScalarPortalType& scalarPortal,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Id& pixelIndex) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    vtkm::Vec<vtkm::Float32, 4> color;
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 3));

    vtkm::Float32 scalar;
    BOUNDS_CHECK(scalarPortal, currentCell);
    scalar = vtkm::Float32(scalarPortal.Get(currentCell));
    //
    // There can be mismatches in the initial enter distance and the current distance
    // due to lost rays at cell borders. For now,
    // we will just advance the current position to the enter distance, since otherwise,
    // the pixel would never be sampled.
    //
    if (currentDistance < enterDistance)
      currentDistance = enterDistance;

    vtkm::Float32 lerpedScalar;
    lerpedScalar = static_cast<vtkm::Float32>((scalar - MinScalar) * InvDeltaScalar);
    vtkm::Id colorIndex = vtkm::Id(lerpedScalar * vtkm::Float32(ColorMapSize));
    if (colorIndex < 0)
      colorIndex = 0;
    if (colorIndex >= ColorMapSize)
      colorIndex = ColorMapSize - 1;
    BOUNDS_CHECK(ColorMap, colorIndex);
    vtkm::Vec<vtkm::Float32, 4> sampleColor = ColorMap.Get(colorIndex);

    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      //composite
      sampleColor[3] *= (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];

      if (color[3] > 1.)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
      currentDistance += SampleDistance;
    }

    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 0);
    FrameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 1);
    FrameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 2);
    FrameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 3);
    FrameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <vtkm::Int32 CellType, typename FloatType, typename Device, typename MeshType>
class SampleCellAssocPoints : public vtkm::worklet::WorkletMapField
{
private:
  using ColorHandle = typename vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
  using ColorBuffer = typename vtkm::cont::ArrayHandle<FloatType>;
  using ColorConstPortal = typename ColorHandle::ExecutionTypes<Device>::PortalConst;
  using ColorPortal = typename ColorBuffer::template ExecutionTypes<Device>::Portal;

  CellSampler<CellType> Sampler;
  FloatType SampleDistance;
  MeshType MeshConn;
  FloatType MinScalar;
  FloatType InvDeltaScalar;
  ColorPortal FrameBuffer;
  ColorConstPortal ColorMap;
  vtkm::Id ColorMapSize;

public:
  template <typename ConnectivityType>
  SampleCellAssocPoints(const FloatType& sampleDistance,
                        const FloatType& minScalar,
                        const FloatType& maxScalar,
                        ColorHandle& colorMap,
                        ColorBuffer& frameBuffer,
                        ConnectivityType& meshConn)
    : SampleDistance(sampleDistance)
    , MeshConn(meshConn)
    , MinScalar(minScalar)
    , ColorMap(colorMap.PrepareForInput(Device()))
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
    ColorMapSize = ColorMap.GetNumberOfValues();
    this->FrameBuffer = frameBuffer.PrepareForOutput(frameBuffer.GetNumberOfValues(), Device());
  }


  using ControlSignature = void(FieldIn<>,
                                WholeArrayIn<Vec3>,
                                WholeArrayIn<ScalarRenderingTypes>,
                                FieldIn<>,
                                FieldIn<>,
                                FieldInOut<>,
                                FieldIn<>,
                                FieldInOut<>,
                                FieldIn<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex, _9);

  template <typename PointPortalType, typename ScalarPortalType>
  VTKM_EXEC inline void operator()(const vtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   ScalarPortalType& scalarPortal,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const vtkm::Vec<vtkm::Float32, 3>& dir,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Id& pixelIndex,
                                   const vtkm::Vec<FloatType, 3>& origin) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    vtkm::Vec<vtkm::Float32, 4> color;
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<vtkm::Float32>(FrameBuffer.Get(pixelIndex * 4 + 3));

    if (color[3] >= 1.f)
    {
      rayStatus = RAY_TERMINATED;
      return;
    }
    vtkm::Vec<vtkm::Float32, 8> scalars;
    vtkm::Vec<vtkm::Vec<FloatType, 3>, 8> points;
    // silence "may" be uninitialized warning
    for (vtkm::Int32 i = 0; i < 8; ++i)
    {
      scalars[i] = 0.f;
      points[i] = vtkm::Vec<FloatType, 3>(0.f, 0.f, 0.f);
    }
    //load local scalar cell data
    vtkm::Id cellConn[8];
    const vtkm::Int32 numIndices = MeshConn.GetCellIndices(cellConn, currentCell);
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(scalarPortal, cellConn[i]);
      scalars[i] = static_cast<vtkm::Float32>(scalarPortal.Get(cellConn[i]));
      BOUNDS_CHECK(vertices, cellConn[i]);
      points[i] = vtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
    }
    //
    // There can be mismatches in the initial enter distance and the current distance
    // due to lost rays at cell borders. For now,
    // we will just advance the current position to the enter distance, since otherwise,
    // the pixel would never be sampled.
    //
    if (currentDistance < enterDistance)
    {
      currentDistance = enterDistance;
    }

    const vtkm::Int32 cellShape = MeshConn.GetCellShape(currentCell);
    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      vtkm::Vec<FloatType, 3> sampleLoc = origin + currentDistance * dir;
      vtkm::Float32 lerpedScalar;
      bool validSample =
        Sampler.SampleCell(points, scalars, sampleLoc, lerpedScalar, *this, cellShape);
      if (!validSample)
      {
        //
        // There is a slight mismatch between intersections and parametric coordinates
        // which results in a invalid sample very close to the cell edge. Just throw
        // this sample away, and move to the next sample.
        //

        //There should be a sample here, so offset and try again.

        currentDistance += 0.00001f;
        continue;
      }
      lerpedScalar = static_cast<vtkm::Float32>((lerpedScalar - MinScalar) * InvDeltaScalar);
      vtkm::Id colorIndex = vtkm::Id(lerpedScalar * vtkm::Float32(ColorMapSize));

      colorIndex = vtkm::Min(vtkm::Max(colorIndex, vtkm::Id(0)), ColorMapSize - 1);
      BOUNDS_CHECK(ColorMap, colorIndex);
      vtkm::Vec<vtkm::Float32, 4> sampleColor = ColorMap.Get(colorIndex);
      //composite
      sampleColor[3] *= (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * sampleColor[3];
      color[1] = color[1] + sampleColor[1] * sampleColor[3];
      color[2] = color[2] + sampleColor[2] * sampleColor[3];
      color[3] = sampleColor[3] + color[3];

      if (color[3] >= 1.0)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
      currentDistance += SampleDistance;
    }

    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 0);
    FrameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 1);
    FrameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 2);
    FrameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(FrameBuffer, pixelIndex * 4 + 3);
    FrameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename FloatType, typename Device>
void ConnectivityTracer<CellType, ConnectivityType>::IntersectCell(
  Ray<FloatType>& rays,
  detail::RayTracking<FloatType>& tracker,
  Device)
{
  using LocateC = LocateCell<CellType, FloatType, MeshConnExec<ConnectivityType, Device>>;
  vtkm::cont::Timer<Device> timer;
  vtkm::worklet::DispatcherMapField<LocateC, Device>(LocateC(MeshConn))
    .Invoke(rays.HitIdx,
            this->MeshConn.GetCoordinates(),
            rays.Dir,
            *(tracker.EnterDist),
            *(tracker.ExitDist),
            tracker.ExitFace,
            rays.Status,
            rays.Origin);

  if (this->CountRayStatus)
    RaysLost = RayOperations::GetStatusCount(rays, RAY_LOST, Device());
  this->IntersectTime += timer.GetElapsedTime();
}

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename FloatType, typename Device>
void ConnectivityTracer<CellType, ConnectivityType>::AccumulatePathLengths(
  Ray<FloatType>& rays,
  detail::RayTracking<FloatType>& tracker,
  Device)
{
  vtkm::worklet::DispatcherMapField<AddPathLengths<FloatType>, Device>(AddPathLengths<FloatType>())
    .Invoke(rays.Status,
            *(tracker.EnterDist),
            *(tracker.ExitDist),
            rays.GetBuffer("path_lengths").Buffer);
}

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename FloatType, typename Device>
void ConnectivityTracer<CellType, ConnectivityType>::FindLostRays(
  Ray<FloatType>& rays,
  detail::RayTracking<FloatType>& tracker,
  Device)
{
  using RayB = RayBumper<CellType, FloatType, Device, MeshConnExec<ConnectivityType, Device>>;
  vtkm::cont::Timer<Device> timer;

  vtkm::worklet::DispatcherMapField<RayB, Device>(
    RayB(rays.DirX, rays.DirY, rays.DirZ, this->MeshConn))
    .Invoke(rays.HitIdx,
            this->MeshConn.GetCoordinates(),
            *(tracker.EnterDist),
            *(tracker.ExitDist),
            tracker.ExitFace,
            rays.Status,
            rays.Origin);

  this->LostRayTime += timer.GetElapsedTime();
}

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename FloatType, typename Device>
void ConnectivityTracer<CellType, ConnectivityType>::SampleCells(
  Ray<FloatType>& rays,
  detail::RayTracking<FloatType>& tracker,
  Device)
{
  using SampleP =
    SampleCellAssocPoints<CellType, FloatType, Device, MeshConnExec<ConnectivityType, Device>>;
  using SampleC =
    SampleCellAssocCells<CellType, FloatType, Device, MeshConnExec<ConnectivityType, Device>>;
  vtkm::cont::Timer<Device> timer;

  VTKM_ASSERT(rays.Buffers.at(0).GetNumChannels() == 4);

  if (FieldAssocPoints)
  {
    vtkm::worklet::DispatcherMapField<SampleP, Device>(
      SampleP(this->SampleDistance,
              vtkm::Float32(this->ScalarBounds.Min),
              vtkm::Float32(this->ScalarBounds.Max),
              this->ColorMap,
              rays.Buffers.at(0).Buffer,
              this->MeshConn))
      .Invoke(rays.HitIdx,
              this->MeshConn.GetCoordinates(),
              this->ScalarField.GetData(),
              *(tracker.EnterDist),
              *(tracker.ExitDist),
              tracker.CurrentDistance,
              rays.Dir,
              rays.Status,
              rays.Origin);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<SampleC, Device>(
      SampleC(this->SampleDistance,
              vtkm::Float32(this->ScalarBounds.Min),
              vtkm::Float32(this->ScalarBounds.Max),
              this->ColorMap,
              rays.Buffers.at(0).Buffer,
              this->MeshConn))
      .Invoke(rays.HitIdx,
              this->ScalarField.GetData(),
              *(tracker.EnterDist),
              *(tracker.ExitDist),
              tracker.CurrentDistance,
              rays.Status);
  }

  this->SampleTime += timer.GetElapsedTime();
}

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename FloatType, typename Device>
void ConnectivityTracer<CellType, ConnectivityType>::IntegrateCells(
  Ray<FloatType>& rays,
  detail::RayTracking<FloatType>& tracker,
  Device)
{
  vtkm::cont::Timer<Device> timer;
  if (HasEmission)
  {
    bool divideEmisByAbsorp = false;
    vtkm::cont::ArrayHandle<FloatType> absorp = rays.Buffers.at(0).Buffer;
    vtkm::cont::ArrayHandle<FloatType> emission = rays.GetBuffer("emission").Buffer;
    vtkm::worklet::DispatcherMapField<IntegrateEmission<FloatType>, Device>(
      IntegrateEmission<FloatType>(rays.Buffers.at(0).GetNumChannels(), divideEmisByAbsorp))
      .Invoke(rays.Status,
              *(tracker.EnterDist),
              *(tracker.ExitDist),
              rays.Distance,
              this->ScalarField.GetData(),
              this->EmissionField.GetData(),
              absorp,
              emission,
              rays.HitIdx);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<Integrate<FloatType>, Device>(
      Integrate<FloatType>(rays.Buffers.at(0).GetNumChannels()))
      .Invoke(rays.Status,
              *(tracker.EnterDist),
              *(tracker.ExitDist),
              rays.Distance,
              this->ScalarField.GetData(),
              rays.Buffers.at(0).Buffer,
              rays.HitIdx);
  }

  IntegrateTime += timer.GetElapsedTime();
}

// template <vtkm::Int32 CellType, typename ConnectivityType>
// template <typename FloatType>
// void ConnectivityTracer<CellType,ConnectivityType>::PrintDebugRay(Ray<FloatType>& rays, vtkm::Id rayId)
// {
//   vtkm::Id index = -1;
//   for (vtkm::Id i = 0; i < rays.NumRays; ++i)
//   {
//     if (rays.PixelIdx.GetPortalControl().Get(i) == rayId)
//     {
//       index = i;
//       break;
//     }
//   }
//   if (index == -1)
//   {
//     return;
//   }

//   std::cout << "++++++++RAY " << rayId << "++++++++\n";
//   std::cout << "Status: " << (int)rays.Status.GetPortalControl().Get(index) << "\n";
//   std::cout << "HitIndex: " << rays.HitIdx.GetPortalControl().Get(index) << "\n";
//   std::cout << "Dist " << rays.Distance.GetPortalControl().Get(index) << "\n";
//   std::cout << "MinDist " << rays.MinDistance.GetPortalControl().Get(index) << "\n";
//   std::cout << "Origin " << rays.Origin.GetPortalConstControl().Get(index) << "\n";
//   std::cout << "Dir " << rays.Dir.GetPortalConstControl().Get(index) << "\n";
//   std::cout << "+++++++++++++++++++++++++\n";
// }

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename FloatType, typename Device>
void ConnectivityTracer<CellType, ConnectivityType>::OffsetMinDistances(Ray<FloatType>& rays,
                                                                        Device)
{
  vtkm::worklet::DispatcherMapField<AdvanceRay<FloatType>, Device>(
    AdvanceRay<FloatType>(FloatType(0.001)))
    .Invoke(rays.Status, rays.MinDistance);
}

template <vtkm::Int32 CellType, typename ConnectivityType>
template <typename Device, typename FloatType>
void ConnectivityTracer<CellType, ConnectivityType>::RenderOnDevice(Ray<FloatType>& rays, Device)
{
  Logger* logger = Logger::GetInstance();
  logger->OpenLogEntry("conn_tracer");
  logger->AddLogData("device", GetDeviceString(Device()));
  this->ResetTimers();
  vtkm::cont::Timer<Device> renderTimer;

  this->SetBoundingBox(Device());

  bool hasPathLengths = rays.HasBuffer("path_lengths");

  vtkm::cont::Timer<Device> timer;
  this->Init();
  //
  // All Rays begin as exited to force intersection
  //
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH, Device());

  detail::RayTracking<FloatType> rayTracker;

  rayTracker.Init(rays.NumRays, rays.Distance, Device());
  vtkm::Float64 time = timer.GetElapsedTime();
  logger->AddLogData("init", time);

  MeshConn.Construct(Device());


  bool cullMissedRays = true;
  bool workRemaining = true;
  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays, Device());
  }

  do
  {
    {
      vtkm::cont::Timer<Device> entryTimer;
      //
      // if ray misses the external face it will be marked RAY_EXITED_MESH
      //
      MeshConn.FindEntry(rays, Device());
      MeshEntryTime += entryTimer.GetElapsedTime();
    }

    if (this->CountRayStatus)
    {
      this->PrintRayStatus(rays, Device());
    }
    if (cullMissedRays)
    {
      //TODO: if we always call this after intersection, then
      //      we could make a specialized version that only compacts
      //      hitIdx distance and status, resizing everything else.
      vtkm::cont::ArrayHandle<UInt8> activeRays;
      activeRays = RayOperations::CompactActiveRays(rays, Device());
      rayTracker.Compact(rays.Distance, activeRays, Device());
      cullMissedRays = false;
    }

    if (this->CountRayStatus)
    {
      this->PrintRayStatus(rays, Device());
    }
    // TODO: we should compact out exited rays once below a threshold
    while (RayOperations::RaysInMesh(rays, Device()))
    {
      //
      // Rays the leave the mesh will be marked as RAYEXITED_MESH
      this->IntersectCell(rays, rayTracker, Device());
      //
      // If the ray was lost due to precision issues, we find it.
      // If it is marked RAY_ABANDONED, then something went wrong.
      //
      this->FindLostRays(rays, rayTracker, Device());
      //
      // integrate along the ray
      //
      if (this->Integrator == Volume)
        this->SampleCells(rays, rayTracker, Device());
      else
        this->IntegrateCells(rays, rayTracker, Device());

      if (hasPathLengths)
      {
        this->AccumulatePathLengths(rays, rayTracker, Device());
      }
      //swap enter and exit distances
      rayTracker.Swap();
      if (this->CountRayStatus)
        this->PrintRayStatus(rays, Device());
    } //for

    workRemaining = RayOperations::RaysProcessed(rays, Device()) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays, Device());
      this->OffsetMinDistances(rays, Device());
    }
  } while (workRemaining);

  if (rays.DebugWidth != -1 && this->Integrator == Volume)
  {

    vtkm::cont::ArrayHandleCounting<vtkm::Id> pCounter(0, 1, rays.NumRays);
    vtkm::worklet::DispatcherMapField<IdentifyMissedRay, Device>(
      IdentifyMissedRay(rays.DebugWidth, rays.DebugHeight, this->BackgroundColor))
      .Invoke(pCounter, rays.Buffers.at(0).Buffer);
  }
  vtkm::Float64 renderTime = renderTimer.GetElapsedTime();
  this->LogTimers();
  logger->AddLogData("active_pixels", rays.NumRays);
  logger->CloseLogEntry(renderTime);
} //Render
}
}
} // namespace vtkm::rendering::raytracing
