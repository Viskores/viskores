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
