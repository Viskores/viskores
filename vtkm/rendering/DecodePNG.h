//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_DecodePNG_h
#define vtk_m_rendering_DecodePNG_h
#include <vtkm/Deprecated.h>
#include <vtkm/io/DecodePNG.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{

VTKM_RENDERING_EXPORT
vtkm::UInt32 DecodePNG(std::vector<unsigned char>& out_image,
                       unsigned long& image_width,
                       unsigned long& image_height,
                       const unsigned char* in_png,
                       std::size_t in_size) VTKM_DEPRECATED(1.6, "Please use vtkm::io::DecodePNG")
{
  return vtkm::io::DecodePNG(out_image, image_width, image_height, in_png, in_size);
}
}
} // vtkm::rendering


#endif
