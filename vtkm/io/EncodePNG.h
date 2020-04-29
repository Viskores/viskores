//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_EncodePNG_h
#define vtk_m_io_EncodePNG_h

#include <vtkm/Types.h>
#include <vtkm/io/vtkm_io_export.h>

#include <vector>

namespace vtkm
{
namespace io
{

VTKM_IO_EXPORT
vtkm::UInt32 EncodePNG(std::vector<unsigned char> const& image,
                       unsigned long width,
                       unsigned long height,
                       unsigned char* out_png,
                       std::size_t out_size);

VTKM_IO_EXPORT
vtkm::UInt32 SavePNG(std::string const& filename,
                     std::vector<unsigned char> const& image,
                     unsigned long width,
                     unsigned long height);
}
} // vtkm::io

#endif //vtk_m_io_EncodePNG_h
