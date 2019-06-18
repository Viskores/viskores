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

#include <vtkm/Types.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vector>

namespace vtkm
{
namespace rendering
{

/// Decodes a PNG file buffer in memory, into a raw pixel buffer
/// Output is RGBA 32-bit (8 bit per channel) color format
/// no matter what color type the original PNG image had. This gives predictable,
/// usable data from any random input PNG.
///
VTKM_RENDERING_EXPORT
vtkm::UInt32 DecodePNG(std::vector<unsigned char>& out_image,
                       unsigned long& image_width,
                       unsigned long& image_height,
                       const unsigned char* in_png,
                       std::size_t in_size);
}
} // vtkm::rendering

#endif //vtk_m_rendering_DecodePNG_h
