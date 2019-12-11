//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_RayTracingTypeDefs_h
#define vtk_m_rendering_raytracing_RayTracingTypeDefs_h

#include <type_traits>
#include <vtkm/List.h>
#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/VariantArrayHandle.h>

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

struct DeviceStringFunctor
{
  std::string result;
  DeviceStringFunctor()
    : result("")
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = GetDeviceString(Device());
    return true;
  }
};

inline std::string GetDeviceString()
{
  DeviceStringFunctor functor;
  vtkm::cont::TryExecute(functor);
  return functor.result;
}

using ColorBuffer4f = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;
using ColorBuffer4b = vtkm::cont::ArrayHandle<vtkm::Vec4ui_8>;

//Defining types supported by the rendering

//vec3s
using Vec3F = vtkm::Vec3f_32;
using Vec3D = vtkm::Vec3f_64;
using Vec3RenderingTypes = vtkm::List<Vec3F, Vec3D>;

// Scalars Types
using ScalarF = vtkm::Float32;
using ScalarD = vtkm::Float64;

using RayStatusType = vtkm::List<vtkm::UInt8>;

using ScalarRenderingTypes = vtkm::List<ScalarF, ScalarD>;
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracingTypeDefs_h
