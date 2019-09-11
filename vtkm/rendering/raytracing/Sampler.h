//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Sampler_h
#define vtk_m_rendering_raytracing_Sampler_h
#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <vtkm::Int32 Base>
VTKM_EXEC void Halton2D(const vtkm::Int32& sampleNum, vtkm::Vec2f_32& coord)
{
  //generate base2 halton
  vtkm::Float32 x = 0.0f;
  vtkm::Float32 xadd = 1.0f;
  vtkm::UInt32 b2 = 1 + static_cast<vtkm::UInt32>(sampleNum);
  while (b2 != 0)
  {
    xadd *= 0.5f;
    if ((b2 & 1) != 0)
      x += xadd;
    b2 >>= 1;
  }

  vtkm::Float32 y = 0.0f;
  vtkm::Float32 yadd = 1.0f;
  vtkm::Int32 bn = 1 + sampleNum;
  while (bn != 0)
  {
    yadd *= 1.0f / (vtkm::Float32)Base;
    y += (vtkm::Float32)(bn % Base) * yadd;
    bn /= Base;
  }

  coord[0] = x;
  coord[1] = y;
} // Halton2D

VTKM_EXEC
vtkm::Vec3f_32 CosineWeightedHemisphere(const vtkm::Int32& sampleNum, const vtkm::Vec3f_32& normal)
{
  //generate orthoganal basis about normal
  int kz = 0;
  if (vtkm::Abs(normal[0]) > vtkm::Abs(normal[1]))
  {
    if (vtkm::Abs(normal[0]) > vtkm::Abs(normal[2]))
      kz = 0;
    else
      kz = 2;
  }
  else
  {
    if (vtkm::Abs(normal[1]) > vtkm::Abs(normal[2]))
      kz = 1;
    else
      kz = 2;
  }
  vtkm::Vec3f_32 notNormal;
  notNormal[0] = 0.f;
  notNormal[1] = 0.f;
  notNormal[2] = 0.f;
  notNormal[kz] = 1.f;

  vtkm::Vec3f_32 xAxis = vtkm::Cross(normal, notNormal);
  vtkm::Normalize(xAxis);
  vtkm::Vec3f_32 yAxis = vtkm::Cross(normal, xAxis);
  vtkm::Normalize(yAxis);

  vtkm::Vec2f_32 xy;
  Halton2D<3>(sampleNum, xy);
  const vtkm::Float32 r = Sqrt(xy[0]);
  const vtkm::Float32 theta = 2 * static_cast<vtkm::Float32>(vtkm::Pi()) * xy[1];

  vtkm::Vec3f_32 direction(0.f, 0.f, 0.f);
  direction[0] = r * vtkm::Cos(theta);
  direction[1] = r * vtkm::Sin(theta);
  direction[2] = vtkm::Sqrt(vtkm::Max(0.0f, 1.f - xy[0]));

  vtkm::Vec3f_32 sampleDir;
  sampleDir[0] = vtkm::dot(direction, xAxis);
  sampleDir[1] = vtkm::dot(direction, yAxis);
  sampleDir[2] = vtkm::dot(direction, normal);
  return sampleDir;
}
}
}
} // namespace vtkm::rendering::raytracing
#endif
