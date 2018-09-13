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
#ifndef vtk_m_rendering_raytracing_ConnectivityTracer_h
#define vtk_m_rendering_raytracing_ConnectivityTracer_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <iomanip>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/rendering/raytracing/MeshConnectivityContainers.h>

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

//forward declare so we can be friends
struct RenderFunctor;

//
//  Ray tracker manages memory and pointer
//  swapping for current cell intersection data
//
template <typename FloatType>
class RayTracking
{
public:
  vtkm::cont::ArrayHandle<vtkm::Int32> ExitFace;
  vtkm::cont::ArrayHandle<FloatType> CurrentDistance;
  vtkm::cont::ArrayHandle<FloatType> Distance1;
  vtkm::cont::ArrayHandle<FloatType> Distance2;
  vtkm::cont::ArrayHandle<FloatType>* EnterDist;
  vtkm::cont::ArrayHandle<FloatType>* ExitDist;

  RayTracking()
  {
    EnterDist = &Distance1;
    ExitDist = &Distance2;
  }

  template <typename Device>
  void Compact(vtkm::cont::ArrayHandle<FloatType>& compactedDistances,
               vtkm::cont::ArrayHandle<UInt8>& masks,
               Device);

  template <typename Device>
  void Init(const vtkm::Id size, vtkm::cont::ArrayHandle<FloatType>& distances, Device);

  void Swap();
};

} //namespace detail


class ConnectivityTracer
{
public:
  ConnectivityTracer()
    : MeshContainer(nullptr)
  {
  }

  ~ConnectivityTracer()
  {
    if (MeshContainer != nullptr)
    {
      delete MeshContainer;
    }
  }

  enum IntegrationMode
  {
    Volume,
    Energy
  };

  void SetVolumeData(const vtkm::cont::Field& scalarField,
                     const vtkm::Range& scalarBounds,
                     const vtkm::cont::DynamicCellSet& cellSet,
                     const vtkm::cont::CoordinateSystem& coords);

  void SetEnergyData(const vtkm::cont::Field& absorption,
                     const vtkm::Int32 numBins,
                     const vtkm::cont::DynamicCellSet& cellSet,
                     const vtkm::cont::CoordinateSystem& coords,
                     const vtkm::cont::Field& emission);

  void SetBackgroundColor(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor);
  void SetSampleDistance(const vtkm::Float32& distance);
  void SetColorMap(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap);
  // template <typename Device>
  // VTKM_CONT void SetBoundingBox(Device)
  // {
  //   vtkm::Bounds coordsBounds = MeshConn->GetCoordinateBounds(Device());
  //   BoundingBox[0] = vtkm::Float32(coordsBounds.X.Min);
  //   BoundingBox[1] = vtkm::Float32(coordsBounds.X.Max);
  //   BoundingBox[2] = vtkm::Float32(coordsBounds.Y.Min);
  //   BoundingBox[3] = vtkm::Float32(coordsBounds.Y.Max);
  //   BoundingBox[4] = vtkm::Float32(coordsBounds.Z.Min);
  //   BoundingBox[5] = vtkm::Float32(coordsBounds.Z.Max);
  //   BackgroundColor[0] = 1.f;
  //   BackgroundColor[1] = 1.f;
  //   BackgroundColor[2] = 1.f;
  //   BackgroundColor[3] = 1.f;
  // }

  void Trace(Ray<vtkm::Float32>& rays);

  void Trace(Ray<vtkm::Float64>& rays);

  MeshConnContainer* GetMeshContainer() { return MeshContainer; }

  void Init();

  vtkm::Id GetNumberOfMeshCells() const;

  void ResetTimers();
  void LogTimers();

  //vtkm::Id GetNumberOfMeshCells() override { return MeshConn.GetNumberOfCells(); }
private:
  friend struct detail::RenderFunctor;
  template <typename FloatType, typename Device>
  void IntersectCell(Ray<FloatType>& rays,
                     detail::RayTracking<FloatType>& tracker,
                     const MeshConnectivityBase* meshConn,
                     Device);

  template <typename FloatType, typename Device>
  void AccumulatePathLengths(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void FindLostRays(Ray<FloatType>& rays,
                    detail::RayTracking<FloatType>& tracker,
                    const MeshConnectivityBase* meshConn,
                    Device);

  template <typename FloatType, typename Device>
  void SampleCells(Ray<FloatType>& rays,
                   detail::RayTracking<FloatType>& tracker,
                   const MeshConnectivityBase* meshConn,
                   Device);

  template <typename FloatType, typename Device>
  void IntegrateCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void OffsetMinDistances(Ray<FloatType>& rays, Device);

  template <typename FloatType, typename Device>
  void PrintRayStatus(Ray<FloatType>& rays, Device);

protected:
  template <typename Device, typename FloatType>
  void RenderOnDevice(Ray<FloatType>& rays, Device);

  // Data set info
  vtkm::cont::Field ScalarField;
  vtkm::cont::Field EmissionField;
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  vtkm::Range ScalarBounds;
  vtkm::Float32 BoundingBox[6];

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorMap;
  vtkm::cont::ArrayHandle<vtkm::Id> PreviousCellIds;

  vtkm::Vec<vtkm::Float32, 4> BackgroundColor;
  vtkm::Float32 SampleDistance;
  vtkm::Id RaysLost;
  IntegrationMode Integrator;
  //
  // flags
  bool CountRayStatus;
  bool MeshConnIsConstructed;
  bool DebugFiltersOn;
  bool ReEnterMesh; // Do not try to re-enter the mesh
  bool CreatePartialComposites;
  bool FieldAssocPoints;
  bool HasEmission; // Mode for integrating through energy bins

  // timers
  vtkm::Float64 IntersectTime;
  vtkm::Float64 IntegrateTime;
  vtkm::Float64 SampleTime;
  vtkm::Float64 LostRayTime;
  vtkm::Float64 MeshEntryTime;

  MeshConnContainer* MeshContainer;
}; // class ConnectivityTracer<CellType,ConnectivityType>

#ifndef vtk_m_rendering_raytracing_ConnectivityTracer_cxx
//extern explicit instantiations of ConnectivityTracer
//extern template class VTKM_RENDERING_TEMPLATE_EXPORT
//  ConnectivityTracer<CELL_SHAPE_ZOO>;
//extern template class VTKM_RENDERING_TEMPLATE_EXPORT
//  ConnectivityTracer<CELL_SHAPE_HEXAHEDRON>;
//extern template class VTKM_RENDERING_TEMPLATE_EXPORT
//  ConnectivityTracer<CELL_SHAPE_WEDGE>;
//extern template class VTKM_RENDERING_TEMPLATE_EXPORT
//  ConnectivityTracer<CELL_SHAPE_TETRA>;
//extern template class VTKM_RENDERING_TEMPLATE_EXPORT
//  ConnectivityTracer<CELL_SHAPE_STRUCTURED>;
#endif
}
}
} // namespace vtkm::rendering::raytracing
#endif
