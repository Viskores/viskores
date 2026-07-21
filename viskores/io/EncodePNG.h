//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
