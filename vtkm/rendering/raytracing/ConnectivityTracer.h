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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/rendering/raytracing/ConnectivityTracerBase.h>
#include <vtkm/rendering/raytracing/MeshConnectivityStructures.h>

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


template <vtkm::Int32 CellType, typename ConnectivityType>
class ConnectivityTracer : public ConnectivityTracerBase
{
public:
  ConnectivityTracer(ConnectivityType& meshConn)
    : ConnectivityTracerBase()
    , MeshConn(meshConn)
  {
  }

  template <typename Device>
  VTKM_CONT void SetBoundingBox(Device)
  {
    vtkm::Bounds coordsBounds = MeshConn.GetCoordinateBounds(Device());
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
  }

  void Trace(Ray<vtkm::Float32>& rays) override;

  void Trace(Ray<vtkm::Float64>& rays) override;

  ConnectivityType GetMeshConn() { return MeshConn; }

  vtkm::Id GetNumberOfMeshCells() override { return MeshConn.GetNumberOfCells(); }

private:
  friend struct detail::RenderFunctor;
  template <typename FloatType, typename Device>
  void IntersectCell(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void AccumulatePathLengths(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void FindLostRays(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void SampleCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void IntegrateCells(Ray<FloatType>& rays, detail::RayTracking<FloatType>& tracker, Device);

  template <typename FloatType, typename Device>
  void OffsetMinDistances(Ray<FloatType>& rays, Device);

  template <typename Device, typename FloatType>
  void RenderOnDevice(Ray<FloatType>& rays, Device);

  ConnectivityType MeshConn;
}; // class ConnectivityTracer<CellType,ConnectivityType>

#ifndef vtk_m_rendering_raytracing_ConnectivityTracer_cxx
//extern explicit instantiations of ConnectivityTracer
extern template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_ZOO, UnstructuredMeshConn>;
extern template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_HEXAHEDRON, UnstructuredMeshConnSingleType>;
extern template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_WEDGE, UnstructuredMeshConnSingleType>;
extern template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_TETRA, UnstructuredMeshConnSingleType>;
extern template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_STRUCTURED, StructuredMeshConn>;
#endif
}
}
} // namespace vtkm::rendering::raytracing
#endif
