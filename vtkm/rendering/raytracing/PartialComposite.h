//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_PartialComposite_h
#define vtk_m_rendering_raytracing_PartialComposite_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/rendering/raytracing/ChannelBuffer.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

template <typename FloatType>
struct PartialComposite
{
  vtkm::cont::ArrayHandle<vtkm::Id> PixelIds;   // pixel that owns composite
  vtkm::cont::ArrayHandle<FloatType> Distances; // distance of composite end
  ChannelBuffer<FloatType> Buffer;              // holds either color or absorption
  // (optional fields)
  ChannelBuffer<FloatType> Intensities;           // holds the intensity emerging from each ray
  vtkm::cont::ArrayHandle<FloatType> PathLengths; // Total distance traversed through the mesh
};
}
}
} // namespace vtkm::rendering::raytracing
#endif
