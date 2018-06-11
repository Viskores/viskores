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
#ifndef vtk_m_rendering_raytracing_RayTracingTypeDefs_h
#define vtk_m_rendering_raytracing_RayTracingTypeDefs_h

#include <type_traits>
#include <vtkm/ListTag.h>
#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/DynamicArrayHandle.h>

namespace vtkm
{
namespace rendering
{
// A more useful  bounds check that tells you where it happened
#ifndef NDEBUG
#define BOUNDS_CHECK(HANDLE, INDEX)                                                                \
  {                                                                                                \
    BoundsCheck((HANDLE), (INDEX), __FILE__, __LINE__);                                            \
  }
#else
#define BOUNDS_CHECK(HANDLE, INDEX)
#endif

template <typename ArrayHandleType>
VTKM_EXEC inline void BoundsCheck(const ArrayHandleType& handle,
                                  const vtkm::Id& index,
                                  const char* file,
                                  int line)
{
  if (index < 0 || index >= handle.GetNumberOfValues())
    printf("Bad Index %d  at file %s line %d\n", (int)index, file, line);
}

namespace raytracing
{
template <typename T>
VTKM_EXEC_CONT inline void GetInfinity(T& vtkmNotUsed(infinity));

template <>
VTKM_EXEC_CONT inline void GetInfinity<vtkm::Float32>(vtkm::Float32& infinity)
{
  infinity = vtkm::Infinity32();
}

template <>
VTKM_EXEC_CONT inline void GetInfinity<vtkm::Float64>(vtkm::Float64& infinity)
{
  infinity = vtkm::Infinity64();
}

template <typename Device>
inline std::string GetDeviceString(Device);

template <>
inline std::string GetDeviceString<vtkm::cont::DeviceAdapterTagSerial>(
  vtkm::cont::DeviceAdapterTagSerial)
{
  return "serial";
}

template <>
inline std::string GetDeviceString<vtkm::cont::DeviceAdapterTagTBB>(vtkm::cont::DeviceAdapterTagTBB)
{
  return "tbb";
}

template <>
inline std::string GetDeviceString<vtkm::cont::DeviceAdapterTagOpenMP>(
  vtkm::cont::DeviceAdapterTagOpenMP)
{
  return "openmp";
}

template <>
inline std::string GetDeviceString<vtkm::cont::DeviceAdapterTagCuda>(
  vtkm::cont::DeviceAdapterTagCuda)
{
  return "cuda";
}

using ColorBuffer4f = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>;
using ColorBuffer4b = vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>>;

//Defining types supported by the rendering

//vec3s
using Vec3F = vtkm::Vec<vtkm::Float32, 3>;
using Vec3D = vtkm::Vec<vtkm::Float64, 3>;
struct Vec3RenderingTypes : vtkm::ListTagBase<Vec3F, Vec3D>
{
};

// Scalars Types
using ScalarF = vtkm::Float32;
using ScalarD = vtkm::Float64;

struct RayStatusType : vtkm::ListTagBase<vtkm::UInt8>
{
};

struct ScalarRenderingTypes : vtkm::ListTagBase<ScalarF, ScalarD>
{
};

//Restrict Coordinate types to explicit for volume renderer
namespace detail
{

using ArrayHandleCompositeVectorFloat32_3Default =
  vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<vtkm::Float32>,
                                         vtkm::cont::ArrayHandle<vtkm::Float32>,
                                         vtkm::cont::ArrayHandle<vtkm::Float32>>;

using ArrayHandleCompositeVectorFloat64_3Default =
  vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<vtkm::Float64>,
                                         vtkm::cont::ArrayHandle<vtkm::Float64>,
                                         vtkm::cont::ArrayHandle<vtkm::Float64>>;

struct StructuredStorageListTagCoordinateSystem
  : vtkm::ListTagBase<vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                      vtkm::cont::ArrayHandleCartesianProduct<
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>>::StorageTag>
{
};
/*
 * This would be for support of curvilinear meshes
struct StructuredStorageListTagCoordinateSystem
    : vtkm::ListTagJoin<
        VTKM_DEFAULT_STORAGE_LIST_TAG,
    vtkm::ListTagBase<vtkm::cont::ArrayHandleUniformPointCoordinates::StorageTag,
                      vtkm::cont::ArrayHandleCartesianProduct<
                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                          vtkm::cont::ArrayHandle<vtkm::FloatDefault> >::StorageTag > >
{ };*/
} // namespace detail


/*
 * This is a less restrictive type list for Explicit meshes
struct ExplicitCoordinatesType : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32,3>,
                                                   vtkm::Vec<vtkm::Float64,3> > {};


struct StorageListTagExplicitCoordinateSystem
    : vtkm::ListTagBase<vtkm::cont::StorageTagBasic,
                        detail::ArrayHandleCompositeVectorFloat32_3Default::StorageTag,
                        detail::ArrayHandleCompositeVectorFloat64_3Default::StorageTag >{ };
*/
//Super restrictive

struct ExplicitCoordinatesType
  : vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>
{
};
struct StorageListTagExplicitCoordinateSystem : vtkm::ListTagBase<vtkm::cont::StorageTagBasic>
{
};


using StructuredStorage = detail::StructuredStorageListTagCoordinateSystem;

using DynamicArrayHandleStructuredCoordinateSystem =
  vtkm::cont::DynamicArrayHandleBase<ExplicitCoordinatesType, StructuredStorage>;

using DynamicArrayHandleExplicitCoordinateSystem =
  vtkm::cont::DynamicArrayHandleBase<ExplicitCoordinatesType,
                                     StorageListTagExplicitCoordinateSystem>;
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracingTypeDefs_h
