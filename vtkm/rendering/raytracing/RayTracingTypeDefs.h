#ifndef vtk_m_rendering_raytracing_RayTracingTypeDefs_h
#define vtk_m_rendering_raytracing_RayTracingTypeDefs_h
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/ListTag.h>
namespace vtkm {
namespace rendering {
namespace raytracing {

typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorBuffer4f;
typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8,4> > ColorBuffer4b;

//Defining types supported by the rendering

//vec3s
typedef vtkm::Vec< vtkm::Float32, 3 > Vec3F;
typedef vtkm::Vec< vtkm::Float64, 3 > Vec3D;
struct Vec3RenderingTypes : vtkm::ListTagBase< Vec3F, Vec3D> { };

// Scalars Types
typedef vtkm::Float32 ScalarF;
typedef vtkm::Float64 ScalarD;

struct ScalarRenderingTypes : vtkm::ListTagBase<ScalarF, ScalarD> {};
}}}//namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracingTypeDefs_h