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
#define vtk_m_rendering_raytracing_ConnectivityTracer_cxx

#include <vtkm/rendering/raytracing/ConnectivityTracer.h>
#include <vtkm/rendering/raytracing/ConnectivityTracer.hxx>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

namespace detail
{
struct RenderFunctor
{
  template <typename Device, typename Tracer, typename Rays>
  bool operator()(Device device, Tracer&& tracer, Rays&& rays) const
  {
    tracer.RenderOnDevice(rays, device);
    return true;
  }
};
} //namespace detail


template <vtkm::Int32 CellType, typename ConnectivityType>
void ConnectivityTracer<CellType, ConnectivityType>::Trace(Ray<vtkm::Float32>& rays)
{
  detail::RenderFunctor functor;
  vtkm::cont::TryExecute(functor, *this, rays);
}

template <vtkm::Int32 CellType, typename ConnectivityType>
void ConnectivityTracer<CellType, ConnectivityType>::Trace(Ray<vtkm::Float64>& rays)
{
  detail::RenderFunctor functor;
  vtkm::cont::TryExecute(functor, *this, rays);
}

//explicit construct all valid forms ofConnectivityTracer
template class detail::RayTracking<float>;
template class detail::RayTracking<double>;

template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_ZOO, UnstructuredMeshConn>;
template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_HEXAHEDRON, UnstructuredMeshConnSingleType>;
template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_WEDGE, UnstructuredMeshConnSingleType>;
template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_TETRA, UnstructuredMeshConnSingleType>;
template class VTKM_RENDERING_TEMPLATE_EXPORT
  ConnectivityTracer<CELL_SHAPE_STRUCTURED, StructuredMeshConn>;
}
}
} // namespace vtkm::rendering::raytracing
