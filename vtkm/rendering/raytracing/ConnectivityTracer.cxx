//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/rendering/raytracing/ConnectivityTracer.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>

#include <vtkm/rendering/raytracing/Camera.h>
#include <vtkm/rendering/raytracing/CellIntersector.h>
#include <vtkm/rendering/raytracing/CellSampler.h>
#include <vtkm/rendering/raytracing/CellTables.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBase.h>
#include <vtkm/rendering/raytracing/MeshConnectivityBuilder.h>
#include <vtkm/rendering/raytracing/Ray.h>
#include <vtkm/rendering/raytracing/RayOperations.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <iomanip>

#ifndef CELL_SHAPE_ZOO
#define CELL_SHAPE_ZOO 255
#endif

#ifndef CELL_SHAPE_STRUCTURED
#define CELL_SHAPE_STRUCTURED 254
#endif

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
namespace detail
{

class AdjustSample : public vtkm::worklet::WorkletMapField
{
  vtkm::Float64 SampleDistance;

public:
  VTKM_CONT
  AdjustSample(const vtkm::Float64 sampleDistance)
    : SampleDistance(sampleDistance)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);
  template <typename FloatType>
  VTKM_EXEC inline void operator()(const vtkm::UInt8& status, FloatType& currentDistance) const
  {
    if (status != RAY_ACTIVE)
      return;

    currentDistance += FMod(currentDistance, (FloatType)SampleDistance);
  }
}; //class AdvanceRay

template <typename FloatType>
void RayTracking<FloatType>::Compact(vtkm::cont::ArrayHandle<FloatType>& compactedDistances,
                                     vtkm::cont::ArrayHandle<UInt8>& masks)
{
  //
  // These distances are stored in the rays, and it has
  // already been compacted.
  //
  CurrentDistance = compactedDistances;

  vtkm::cont::ArrayHandleCast<vtkm::Id, vtkm::cont::ArrayHandle<vtkm::UInt8>> castedMasks(masks);

  bool distance1IsEnter = EnterDist == &Distance1;

  vtkm::cont::ArrayHandle<FloatType> compactedDistance1;
  vtkm::cont::Algorithm::CopyIf(Distance1, masks, compactedDistance1);
  Distance1 = compactedDistance1;

  vtkm::cont::ArrayHandle<FloatType> compactedDistance2;
  vtkm::cont::Algorithm::CopyIf(Distance2, masks, compactedDistance2);
  Distance2 = compactedDistance2;

  vtkm::cont::ArrayHandle<vtkm::Int32> compactedExitFace;
  vtkm::cont::Algorithm::CopyIf(ExitFace, masks, compactedExitFace);
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
void RayTracking<FloatType>::Init(const vtkm::Id size,
                                  vtkm::cont::ArrayHandle<FloatType>& distances)
{

  ExitFace.Allocate(size);
  Distance1.Allocate(size);
  Distance2.Allocate(size);

  CurrentDistance = distances;
  //
  // Set the initial Distances
  //
  vtkm::worklet::DispatcherMapField<CopyAndOffset<FloatType>> resetDistancesDispatcher(
    CopyAndOffset<FloatType>(0.0f));
  resetDistancesDispatcher.Invoke(distances, *EnterDist);

  //
  // Init the exit faces. This value is used to load the next cell
  // base on the cell and face it left
  //
  vtkm::cont::ArrayHandleConstant<vtkm::Int32> negOne(-1, size);
  vtkm::cont::Algorithm::Copy(negOne, ExitFace);

  vtkm::cont::ArrayHandleConstant<FloatType> negOnef(-1.f, size);
  vtkm::cont::Algorithm::Copy(negOnef, *ExitDist);
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

void ConnectivityTracer::Init()
{
  //
  // Check to see if a sample distance was set
  //
  if (SampleDistance <= 0)
  {
    vtkm::Bounds coordsBounds = Coords.GetBounds();
    BoundingBox[0] = vtkm::Float32(coordsBounds.X.Min);
    BoundingBox[1] = vtkm::Float32(coordsBounds.X.Max);
    BoundingBox[2] = vtkm::Float32(coordsBounds.Y.Min);
    BoundingBox[3] = vtkm::Float32(coordsBounds.Y.Max);
    BoundingBox[4] = vtkm::Float32(coordsBounds.Z.Min);
    BoundingBox[5] = vtkm::Float32(coordsBounds.Z.Max);

    BackgroundColor[0] = 1.f;
    BackgroundColor[1] = 1.f;
    BackgroundColor[2] = 1.f;
    BackgroundColor[3] = 1.f;
    const vtkm::Float32 defaultSampleRate = 200.f;
    // We need to set some default sample distance
    vtkm::Vec3f_32 extent;
    extent[0] = BoundingBox[1] - BoundingBox[0];
    extent[1] = BoundingBox[3] - BoundingBox[2];
    extent[2] = BoundingBox[5] - BoundingBox[4];
    SampleDistance = vtkm::Magnitude(extent) / defaultSampleRate;
  }
}

vtkm::Id ConnectivityTracer::GetNumberOfMeshCells() const
{
  return CellSet.GetNumberOfCells();
}

void ConnectivityTracer::SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorMap)
{
  ColorMap = colorMap;
}

void ConnectivityTracer::SetVolumeData(const vtkm::cont::Field& scalarField,
                                       const vtkm::Range& scalarBounds,
                                       const vtkm::cont::DynamicCellSet& cellSet,
                                       const vtkm::cont::CoordinateSystem& coords)
{
  //TODO: Need a way to tell if we have been updated
  ScalarField = scalarField;
  ScalarBounds = scalarBounds;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;

  const bool isSupportedField = ScalarField.IsFieldCell() || ScalarField.IsFieldPoint();
  if (!isSupportedField)
  {
    throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  }
  FieldAssocPoints = ScalarField.IsFieldPoint();

  this->Integrator = Volume;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }
  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);
}

void ConnectivityTracer::SetEnergyData(const vtkm::cont::Field& absorption,
                                       const vtkm::Int32 numBins,
                                       const vtkm::cont::DynamicCellSet& cellSet,
                                       const vtkm::cont::CoordinateSystem& coords,
                                       const vtkm::cont::Field& emission)
{
  bool isSupportedField = absorption.GetAssociation() == vtkm::cont::Field::Association::CELL_SET;
  if (!isSupportedField)
    throw vtkm::cont::ErrorBadValue("Absorption Field '" + absorption.GetName() +
                                    "' not accociated with cells");
  ScalarField = absorption;
  CellSet = cellSet;
  Coords = coords;
  MeshConnIsConstructed = false;
  // Check for emission
  HasEmission = false;

  if (emission.GetAssociation() != vtkm::cont::Field::Association::ANY)
  {
    if (emission.GetAssociation() != vtkm::cont::Field::Association::CELL_SET)
      throw vtkm::cont::ErrorBadValue("Emission Field '" + emission.GetName() +
                                      "' not accociated with cells");
    HasEmission = true;
    EmissionField = emission;
  }
  // Do some basic range checking
  if (numBins < 1)
    throw vtkm::cont::ErrorBadValue("Number of energy bins is less than 1");
  vtkm::Id binCount = ScalarField.GetNumberOfValues();
  vtkm::Id cellCount = this->GetNumberOfMeshCells();
  if (cellCount != (binCount / vtkm::Id(numBins)))
  {
    std::stringstream message;
    message << "Invalid number of absorption bins\n";
    message << "Number of cells: " << cellCount << "\n";
    message << "Number of field values: " << binCount << "\n";
    message << "Number of bins: " << numBins << "\n";
    throw vtkm::cont::ErrorBadValue(message.str());
  }
  if (HasEmission)
  {
    binCount = EmissionField.GetNumberOfValues();
    if (cellCount != (binCount / vtkm::Id(numBins)))
    {
      std::stringstream message;
      message << "Invalid number of emission bins\n";
      message << "Number of cells: " << cellCount << "\n";
      message << "Number of field values: " << binCount << "\n";
      message << "Number of bins: " << numBins << "\n";
      throw vtkm::cont::ErrorBadValue(message.str());
    }
  }
  //TODO: Need a way to tell if we have been updated
  this->Integrator = Energy;

  if (MeshContainer == nullptr)
  {
    delete MeshContainer;
  }
  MeshConnectivityBuilder builder;
  MeshContainer = builder.BuildConnectivity(cellSet, coords);
}

void ConnectivityTracer::SetBackgroundColor(const vtkm::Vec4f_32& backgroundColor)
{
  BackgroundColor = backgroundColor;
}

void ConnectivityTracer::SetSampleDistance(const vtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}

void ConnectivityTracer::ResetTimers()
{
  IntersectTime = 0.;
  IntegrateTime = 0.;
  SampleTime = 0.;
  LostRayTime = 0.;
  MeshEntryTime = 0.;
}

void ConnectivityTracer::LogTimers()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("intersect ", IntersectTime);
  logger->AddLogData("integrate ", IntegrateTime);
  logger->AddLogData("sample_cells ", SampleTime);
  logger->AddLogData("lost_rays ", LostRayTime);
  logger->AddLogData("mesh_entry", LostRayTime);
}

template <typename FloatType>
void ConnectivityTracer::PrintRayStatus(Ray<FloatType>& rays)
{
  vtkm::Id raysExited = RayOperations::GetStatusCount(rays, RAY_EXITED_MESH);
  vtkm::Id raysActive = RayOperations::GetStatusCount(rays, RAY_ACTIVE);
  vtkm::Id raysAbandoned = RayOperations::GetStatusCount(rays, RAY_ABANDONED);
  vtkm::Id raysExitedDom = RayOperations::GetStatusCount(rays, RAY_EXITED_DOMAIN);
  std::cout << "\r Ray Status " << std::setw(10) << std::left << " Lost " << std::setw(10)
            << std::left << RaysLost << std::setw(10) << std::left << " Exited " << std::setw(10)
            << std::left << raysExited << std::setw(10) << std::left << " Active " << std::setw(10)
            << raysActive << std::setw(10) << std::left << " Abandoned " << std::setw(10)
            << raysAbandoned << " Exited Domain " << std::setw(10) << std::left << raysExitedDom
            << "\n";
}

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
  using ControlSignature = void(FieldIn, FieldInOut);
  using ExecutionSignature = void(_1, _2);

  VTKM_EXEC inline void operator()(const vtkm::UInt8& status, FloatType& distance) const
  {
    if (status == RAY_EXITED_MESH)
      distance += Offset;
  }
}; //class AdvanceRay

class LocateCell : public vtkm::worklet::WorkletMapField
{
private:
  CellIntersector<255> Intersector;

public:
  LocateCell() {}

  using ControlSignature = void(FieldInOut,
                                WholeArrayIn,
                                FieldIn,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldIn,
                                ExecObject meshConnectivity);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename FloatType, typename PointPortalType>
  VTKM_EXEC inline void operator()(vtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   const vtkm::Vec<FloatType, 3>& dir,
                                   FloatType& enterDistance,
                                   FloatType& exitDistance,
                                   vtkm::Int32& enterFace,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Vec<FloatType, 3>& origin,
                                   const MeshWrapper& meshConn) const
  {
    if (enterFace != -1 && rayStatus == RAY_ACTIVE)
    {
      currentCell = meshConn.GetConnectingCell(currentCell, enterFace);
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

    const vtkm::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
    //load local cell data
    for (int i = 0; i < numIndices; ++i)
    {
      BOUNDS_CHECK(vertices, cellConn[i]);
      vtkm::Vec<FloatType, 3> point = vtkm::Vec<FloatType, 3>(vertices.Get(cellConn[i]));
      xpoints[i] = point[0];
      ypoints[i] = point[1];
      zpoints[i] = point[2];
    }
    const vtkm::UInt8 cellShape = meshConn.GetCellShape(currentCell);
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

class RayBumper : public vtkm::worklet::WorkletMapField
{
private:
  CellIntersector<255> Intersector;
  const vtkm::UInt8 FailureStatus; // the status to assign ray if we fail to find the intersection
public:
  RayBumper(vtkm::UInt8 failureStatus = RAY_ABANDONED)
    : FailureStatus(failureStatus)
  {
  }


  using ControlSignature = void(FieldInOut,
                                WholeArrayIn,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldInOut,
                                FieldIn,
                                FieldInOut,
                                ExecObject meshConnectivity);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9);

  template <typename FloatType, typename PointPortalType>
  VTKM_EXEC inline void operator()(vtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   FloatType& enterDistance,
                                   FloatType& exitDistance,
                                   vtkm::Int32& enterFace,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Vec<FloatType, 3>& origin,
                                   vtkm::Vec<FloatType, 3>& rdir,
                                   const MeshWrapper& meshConn) const
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

    const vtkm::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
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

    vtkm::Vec<FloatType, 3> dir = rdir;
    vtkm::Vec<FloatType, 3> bump = toCentroid - dir;
    dir = dir + RAY_TUG_EPSILON * bump;

    vtkm::Normalize(dir);
    rdir = dir;

    const vtkm::UInt8 cellShape = meshConn.GetCellShape(currentCell);
    Intersector.IntersectCell(xpoints, ypoints, zpoints, rdir, origin, distances, cellShape);

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

class AddPathLengths : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  AddPathLengths() {}

  using ControlSignature = void(FieldIn,     // ray status
                                FieldIn,     // cell enter distance
                                FieldIn,     // cell exit distance
                                FieldInOut); // ray absorption data

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename FloatType>
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

class Integrate : public vtkm::worklet::WorkletMapField
{
private:
  const vtkm::Int32 NumBins;
  const vtkm::Float32 UnitScalar;

public:
  VTKM_CONT
  Integrate(const vtkm::Int32 numBins, const vtkm::Float32 unitScalar)
    : NumBins(numBins)
    , UnitScalar(unitScalar)
  {
  }

  using ControlSignature = void(FieldIn,         // ray status
                                FieldIn,         // cell enter distance
                                FieldIn,         // cell exit distance
                                FieldInOut,      // current distance
                                WholeArrayIn,    // cell absorption data array
                                WholeArrayInOut, // ray absorption data
                                FieldIn);        // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, WorkIndex);

  template <typename FloatType, typename CellDataPortalType, typename RayDataPortalType>
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
      absorb *= UnitScalar;
      absorb = vtkm::Exp(-absorb * segmentLength);
      BOUNDS_CHECK(energyBins, rayOffset + i);
      FloatType intensity = static_cast<FloatType>(energyBins.Get(rayOffset + i));
      energyBins.Set(rayOffset + i, intensity * absorb);
    }
    currentDistance = exitDistance;
  }
};

class IntegrateEmission : public vtkm::worklet::WorkletMapField
{
private:
  const vtkm::Int32 NumBins;
  const vtkm::Float32 UnitScalar;
  bool DivideEmisByAbsorb;

public:
  VTKM_CONT
  IntegrateEmission(const vtkm::Int32 numBins,
                    const vtkm::Float32 unitScalar,
                    const bool divideEmisByAbsorb)
    : NumBins(numBins)
    , UnitScalar(unitScalar)
    , DivideEmisByAbsorb(divideEmisByAbsorb)
  {
  }

  using ControlSignature = void(FieldIn,         // ray status
                                FieldIn,         // cell enter distance
                                FieldIn,         // cell exit distance
                                FieldInOut,      // current distance
                                WholeArrayIn,    // cell absorption data array
                                WholeArrayIn,    // cell emission data array
                                WholeArrayInOut, // ray absorption data
                                WholeArrayInOut, // ray emission data
                                FieldIn);        // current cell

  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, _9, WorkIndex);

  template <typename FloatType,
            typename CellAbsPortalType,
            typename CellEmisPortalType,
            typename RayDataPortalType>
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

      absorb *= UnitScalar;
      emission *= UnitScalar;

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
  vtkm::Vec4f_32 BGColor;
  IdentifyMissedRay(const vtkm::Id width, const vtkm::Id height, vtkm::Vec4f_32 bgcolor)
    : Width(width)
    , Height(height)
    , BGColor(bgcolor)
  {
  }
  using ControlSignature = void(FieldIn, WholeArrayIn);
  using ExecutionSignature = void(_1, _2);


  VTKM_EXEC inline bool IsBGColor(const vtkm::Vec4f_32 color) const
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
    vtkm::Vec4f_32 pixel;
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

template <typename FloatType>
class SampleCellAssocCells : public vtkm::worklet::WorkletMapField
{
private:
  CellSampler<255> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;

public:
  SampleCellAssocCells(const FloatType& sampleDistance,
                       const FloatType& minScalar,
                       const FloatType& maxScalar)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
  }


  using ControlSignature = void(FieldIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldIn,
                                FieldInOut,
                                FieldInOut,
                                WholeArrayIn,
                                WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex);

  template <typename ScalarPortalType, typename ColorMapType, typename FrameBufferType>
  VTKM_EXEC inline void operator()(const vtkm::Id& currentCell,
                                   ScalarPortalType& scalarPortal,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   vtkm::UInt8& rayStatus,
                                   const ColorMapType& colorMap,
                                   FrameBufferType& frameBuffer,
                                   const vtkm::Id& pixelIndex) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    vtkm::Vec4f_32 color;
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

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

    const vtkm::Id colorMapSize = colorMap.GetNumberOfValues();
    vtkm::Float32 lerpedScalar;
    lerpedScalar = static_cast<vtkm::Float32>((scalar - MinScalar) * InvDeltaScalar);
    vtkm::Id colorIndex = vtkm::Id(lerpedScalar * vtkm::Float32(colorMapSize));
    if (colorIndex < 0)
      colorIndex = 0;
    if (colorIndex >= colorMapSize)
      colorIndex = colorMapSize - 1;
    BOUNDS_CHECK(colorMap, colorIndex);
    vtkm::Vec4f_32 sampleColor = colorMap.Get(colorIndex);

    while (enterDistance <= currentDistance && currentDistance <= exitDistance)
    {
      //composite
      vtkm::Float32 alpha = sampleColor[3] * (1.f - color[3]);
      color[0] = color[0] + sampleColor[0] * alpha;
      color[1] = color[1] + sampleColor[1] * alpha;
      color[2] = color[2] + sampleColor[2] * alpha;
      color[3] = alpha + color[3];

      if (color[3] > 1.)
      {
        rayStatus = RAY_TERMINATED;
        break;
      }
      currentDistance += SampleDistance;
    }

    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <typename FloatType>
class SampleCellAssocPoints : public vtkm::worklet::WorkletMapField
{
private:
  CellSampler<255> Sampler;
  FloatType SampleDistance;
  FloatType MinScalar;
  FloatType InvDeltaScalar;

public:
  SampleCellAssocPoints(const FloatType& sampleDistance,
                        const FloatType& minScalar,
                        const FloatType& maxScalar)
    : SampleDistance(sampleDistance)
    , MinScalar(minScalar)
  {
    InvDeltaScalar = (minScalar == maxScalar) ? 1.f : 1.f / (maxScalar - minScalar);
  }


  using ControlSignature = void(FieldIn,
                                WholeArrayIn,
                                WholeArrayIn,
                                FieldIn,
                                FieldIn,
                                FieldInOut,
                                FieldIn,
                                FieldInOut,
                                FieldIn,
                                ExecObject meshConnectivity,
                                WholeArrayIn,
                                WholeArrayInOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7, _8, WorkIndex, _9, _10, _11, _12);

  template <typename PointPortalType,
            typename ScalarPortalType,
            typename ColorMapType,
            typename FrameBufferType>
  VTKM_EXEC inline void operator()(const vtkm::Id& currentCell,
                                   PointPortalType& vertices,
                                   ScalarPortalType& scalarPortal,
                                   const FloatType& enterDistance,
                                   const FloatType& exitDistance,
                                   FloatType& currentDistance,
                                   const vtkm::Vec3f_32& dir,
                                   vtkm::UInt8& rayStatus,
                                   const vtkm::Id& pixelIndex,
                                   const vtkm::Vec<FloatType, 3>& origin,
                                   MeshWrapper& meshConn,
                                   const ColorMapType& colorMap,
                                   FrameBufferType& frameBuffer) const
  {

    if (rayStatus != RAY_ACTIVE)
      return;

    vtkm::Vec4f_32 color;
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    color[0] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 0));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    color[1] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 1));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    color[2] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 2));
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    color[3] = static_cast<vtkm::Float32>(frameBuffer.Get(pixelIndex * 4 + 3));

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
    const vtkm::Int32 numIndices = meshConn.GetCellIndices(cellConn, currentCell);
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

    const vtkm::Id colorMapSize = colorMap.GetNumberOfValues();
    const vtkm::Int32 cellShape = meshConn.GetCellShape(currentCell);

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
      vtkm::Id colorIndex = vtkm::Id(lerpedScalar * vtkm::Float32(colorMapSize));

      colorIndex = vtkm::Min(vtkm::Max(colorIndex, vtkm::Id(0)), colorMapSize - 1);
      BOUNDS_CHECK(colorMap, colorIndex);
      vtkm::Vec4f_32 sampleColor = colorMap.Get(colorIndex);
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

    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 0);
    frameBuffer.Set(pixelIndex * 4 + 0, color[0]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 1);
    frameBuffer.Set(pixelIndex * 4 + 1, color[1]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 2);
    frameBuffer.Set(pixelIndex * 4 + 2, color[2]);
    BOUNDS_CHECK(frameBuffer, pixelIndex * 4 + 3);
    frameBuffer.Set(pixelIndex * 4 + 3, color[3]);
  }
}; //class Sample cell

template <typename FloatType>
void ConnectivityTracer::IntersectCell(Ray<FloatType>& rays,
                                       detail::RayTracking<FloatType>& tracker)
{
  vtkm::cont::Timer timer;
  timer.Start();
  vtkm::worklet::DispatcherMapField<LocateCell> locateDispatch;
  locateDispatch.Invoke(rays.HitIdx,
                        this->Coords,
                        rays.Dir,
                        *(tracker.EnterDist),
                        *(tracker.ExitDist),
                        tracker.ExitFace,
                        rays.Status,
                        rays.Origin,
                        MeshContainer);

  if (this->CountRayStatus)
    RaysLost = RayOperations::GetStatusCount(rays, RAY_LOST);
  this->IntersectTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::AccumulatePathLengths(Ray<FloatType>& rays,
                                               detail::RayTracking<FloatType>& tracker)
{
  vtkm::worklet::DispatcherMapField<AddPathLengths> dispatcher;
  dispatcher.Invoke(
    rays.Status, *(tracker.EnterDist), *(tracker.ExitDist), rays.GetBuffer("path_lengths").Buffer);
}

template <typename FloatType>
void ConnectivityTracer::FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker)
{
  vtkm::cont::Timer timer;
  timer.Start();

  vtkm::worklet::DispatcherMapField<RayBumper> bumpDispatch;
  bumpDispatch.Invoke(rays.HitIdx,
                      this->Coords,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.ExitFace,
                      rays.Status,
                      rays.Origin,
                      rays.Dir,
                      MeshContainer);

  this->LostRayTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker)
{
  using SampleP = SampleCellAssocPoints<FloatType>;
  using SampleC = SampleCellAssocCells<FloatType>;
  vtkm::cont::Timer timer;
  timer.Start();

  VTKM_ASSERT(rays.Buffers.at(0).GetNumChannels() == 4);

  if (FieldAssocPoints)
  {
    vtkm::worklet::DispatcherMapField<SampleP> dispatcher(
      SampleP(this->SampleDistance,
              vtkm::Float32(this->ScalarBounds.Min),
              vtkm::Float32(this->ScalarBounds.Max)));
    dispatcher.Invoke(rays.HitIdx,
                      this->Coords,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.CurrentDistance,
                      rays.Dir,
                      rays.Status,
                      rays.Origin,
                      MeshContainer,
                      this->ColorMap,
                      rays.Buffers.at(0).Buffer);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<SampleC> dispatcher(
      SampleC(this->SampleDistance,
              vtkm::Float32(this->ScalarBounds.Min),
              vtkm::Float32(this->ScalarBounds.Max)));

    dispatcher.Invoke(rays.HitIdx,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      tracker.CurrentDistance,
                      rays.Status,
                      this->ColorMap,
                      rays.Buffers.at(0).Buffer);
  }

  this->SampleTime += timer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::IntegrateCells(Ray<FloatType>& rays,
                                        detail::RayTracking<FloatType>& tracker)
{
  vtkm::cont::Timer timer;
  timer.Start();
  if (HasEmission)
  {
    bool divideEmisByAbsorp = false;
    vtkm::cont::ArrayHandle<FloatType> absorp = rays.Buffers.at(0).Buffer;
    vtkm::cont::ArrayHandle<FloatType> emission = rays.GetBuffer("emission").Buffer;
    vtkm::worklet::DispatcherMapField<IntegrateEmission> dispatcher(
      IntegrateEmission(rays.Buffers.at(0).GetNumChannels(), UnitScalar, divideEmisByAbsorp));
    dispatcher.Invoke(rays.Status,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      rays.Distance,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      this->EmissionField.GetData().ResetTypes(ScalarRenderingTypes()),
                      absorp,
                      emission,
                      rays.HitIdx);
  }
  else
  {
    vtkm::worklet::DispatcherMapField<Integrate> dispatcher(
      Integrate(rays.Buffers.at(0).GetNumChannels(), UnitScalar));
    dispatcher.Invoke(rays.Status,
                      *(tracker.EnterDist),
                      *(tracker.ExitDist),
                      rays.Distance,
                      this->ScalarField.GetData().ResetTypes(ScalarRenderingTypes()),
                      rays.Buffers.at(0).Buffer,
                      rays.HitIdx);
  }

  IntegrateTime += timer.GetElapsedTime();
}

// template <typename FloatType>
// void ConnectivityTracer<CellType>::PrintDebugRay(Ray<FloatType>& rays, vtkm::Id rayId)
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

template <typename FloatType>
void ConnectivityTracer::OffsetMinDistances(Ray<FloatType>& rays)
{
  vtkm::worklet::DispatcherMapField<AdvanceRay<FloatType>> dispatcher(
    AdvanceRay<FloatType>(FloatType(0.001)));
  dispatcher.Invoke(rays.Status, rays.MinDistance);
}

template <typename FloatType>
void ConnectivityTracer::FindMeshEntry(Ray<FloatType>& rays)
{
  vtkm::cont::Timer entryTimer;
  entryTimer.Start();
  //
  // if ray misses the external face it will be marked RAY_EXITED_MESH
  //
  MeshContainer->FindEntry(rays);
  MeshEntryTime += entryTimer.GetElapsedTime();
}

template <typename FloatType>
void ConnectivityTracer::IntegrateMeshSegment(Ray<FloatType>& rays)
{
  this->Init(); // sets sample distance
  detail::RayTracking<FloatType> rayTracker;
  rayTracker.Init(rays.NumRays, rays.Distance);

  bool hasPathLengths = rays.HasBuffer("path_lengths");

  if (this->Integrator == Volume)
  {
    vtkm::worklet::DispatcherMapField<detail::AdjustSample> adispatcher(SampleDistance);
    adispatcher.Invoke(rays.Status, rayTracker.CurrentDistance);
  }

  while (RayOperations::RaysInMesh(rays))
  {
    //
    // Rays the leave the mesh will be marked as RAYEXITED_MESH
    this->IntersectCell(rays, rayTracker);
    //
    // If the ray was lost due to precision issues, we find it.
    // If it is marked RAY_ABANDONED, then something went wrong.
    //
    this->FindLostRays(rays, rayTracker);
    //
    // integrate along the ray
    //
    if (this->Integrator == Volume)
      this->SampleCells(rays, rayTracker);
    else
      this->IntegrateCells(rays, rayTracker);

    if (hasPathLengths)
    {
      this->AccumulatePathLengths(rays, rayTracker);
    }
    //swap enter and exit distances
    rayTracker.Swap();
    if (this->CountRayStatus)
      this->PrintRayStatus(rays);
  } //for
}

template <typename FloatType>
void ConnectivityTracer::FullTrace(Ray<FloatType>& rays)
{

  this->RaysLost = 0;
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH);

  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays);
  }

  bool cullMissedRays = true;
  bool workRemaining = true;

  do
  {
    FindMeshEntry(rays);

    if (cullMissedRays)
    {
      vtkm::cont::ArrayHandle<UInt8> activeRays;
      activeRays = RayOperations::CompactActiveRays(rays);
      cullMissedRays = false;
    }

    IntegrateMeshSegment(rays);

    workRemaining = RayOperations::RaysProcessed(rays) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays);
      this->OffsetMinDistances(rays);
    }
  } while (workRemaining);
}

template <typename FloatType>
std::vector<PartialComposite<FloatType>> ConnectivityTracer::PartialTrace(Ray<FloatType>& rays)
{

  bool hasPathLengths = rays.HasBuffer("path_lengths");
  this->RaysLost = 0;
  RayOperations::ResetStatus(rays, RAY_EXITED_MESH);

  std::vector<PartialComposite<FloatType>> partials;

  if (this->CountRayStatus)
  {
    this->PrintRayStatus(rays);
  }

  bool workRemaining = true;

  do
  {
    FindMeshEntry(rays);

    vtkm::cont::ArrayHandle<UInt8> activeRays;
    activeRays = RayOperations::CompactActiveRays(rays);

    if (rays.NumRays == 0)
      break;

    IntegrateMeshSegment(rays);

    PartialComposite<FloatType> partial;
    partial.Buffer = rays.Buffers.at(0).Copy();
    vtkm::cont::Algorithm::Copy(rays.Distance, partial.Distances);
    vtkm::cont::Algorithm::Copy(rays.PixelIdx, partial.PixelIds);

    if (HasEmission && this->Integrator == Energy)
    {
      partial.Intensities = rays.GetBuffer("emission").Copy();
    }
    if (hasPathLengths)
    {
      partial.PathLengths = rays.GetBuffer("path_lengths").Copy().Buffer;
    }
    partials.push_back(partial);

    // reset buffers
    if (this->Integrator == Volume)
    {
      vtkm::cont::ArrayHandle<FloatType> signature;
      signature.Allocate(4);
      signature.GetPortalControl().Set(0, 0.f);
      signature.GetPortalControl().Set(1, 0.f);
      signature.GetPortalControl().Set(2, 0.f);
      signature.GetPortalControl().Set(3, 0.f);
      rays.Buffers.at(0).InitChannels(signature);
    }
    else
    {
      rays.Buffers.at(0).InitConst(1.f);
      if (HasEmission)
      {
        rays.GetBuffer("emission").InitConst(0.f);
      }
      if (hasPathLengths)
      {
        rays.GetBuffer("path_lengths").InitConst(0.f);
      }
    }

    workRemaining = RayOperations::RaysProcessed(rays) != rays.NumRays;
    //
    // Ensure that we move the current distance forward some
    // epsilon so we don't re-enter the cell we just left.
    //
    if (workRemaining)
    {
      RayOperations::CopyDistancesToMin(rays);
      this->OffsetMinDistances(rays);
    }
  } while (workRemaining);

  return partials;
}

template class detail::RayTracking<vtkm::Float32>;
template class detail::RayTracking<vtkm::Float64>;

template struct PartialComposite<vtkm::Float32>;
template struct PartialComposite<vtkm::Float64>;

template void ConnectivityTracer::FullTrace<vtkm::Float32>(Ray<vtkm::Float32>& rays);

template std::vector<PartialComposite<vtkm::Float32>>
ConnectivityTracer::PartialTrace<vtkm::Float32>(Ray<vtkm::Float32>& rays);

template void ConnectivityTracer::IntegrateMeshSegment<vtkm::Float32>(Ray<vtkm::Float32>& rays);

template void ConnectivityTracer::FindMeshEntry<vtkm::Float32>(Ray<vtkm::Float32>& rays);

template void ConnectivityTracer::FullTrace<vtkm::Float64>(Ray<vtkm::Float64>& rays);

template std::vector<PartialComposite<vtkm::Float64>>
ConnectivityTracer::PartialTrace<vtkm::Float64>(Ray<vtkm::Float64>& rays);

template void ConnectivityTracer::IntegrateMeshSegment<vtkm::Float64>(Ray<vtkm::Float64>& rays);

template void ConnectivityTracer::FindMeshEntry<vtkm::Float64>(Ray<vtkm::Float64>& rays);
}
}
} // namespace vtkm::rendering::raytracing
