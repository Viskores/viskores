//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_io_EncodePNG_h
#define viskores_io_EncodePNG_h

#include <viskores/Types.h>
#include <viskores/io/viskores_io_export.h>

#include <vector>

namespace viskores
{
namespace io
{

VISKORES_IO_EXPORT
viskores::UInt32 EncodePNG(std::vector<unsigned char> const& image,
                       unsigned long width,
                       unsigned long height,
                       unsigned char* out_png,
                       std::size_t out_size);

VISKORES_IO_EXPORT
viskores::UInt32 SavePNG(std::string const& filename,
                     std::vector<unsigned char> const& image,
                     unsigned long width,
                     unsigned long height);
}
} // viskores::io

#endif //viskores_io_EncodePNG_h
