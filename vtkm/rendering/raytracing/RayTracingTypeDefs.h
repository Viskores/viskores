//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_RayTracingTypeDefs_h
#define vtk_m_rendering_raytracing_RayTracingTypeDefs_h
#include <vtkm/ListTag.h>
#include <vtkm/cont/ArrayHandle.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>> ColorBuffer4f;
typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>> ColorBuffer4b;

//Defining types supported by the rendering

//vec3s
typedef vtkm::Vec<vtkm::Float32, 3> Vec3F;
typedef vtkm::Vec<vtkm::Float64, 3> Vec3D;
struct Vec3RenderingTypes : vtkm::ListTagBase<Vec3F, Vec3D>
{
};

// Scalars Types
typedef vtkm::Float32 ScalarF;
typedef vtkm::Float64 ScalarD;

struct ScalarRenderingTypes : vtkm::ListTagBase<ScalarF, ScalarD>
{
};
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_RayTracingTypeDefs_h
