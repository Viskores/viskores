//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#ifndef viskores_io_ImageUtils_h
#define viskores_io_ImageUtils_h

#include <viskores/cont/DataSet.h>
#include <viskores/io/viskores_io_export.h>

namespace viskores
{
namespace io
{

VISKORES_IO_EXPORT
void WriteImageFile(const viskores::cont::DataSet& dataSet,
                    const std::string& fullPath,
                    const std::string& fieldName);

VISKORES_IO_EXPORT
viskores::cont::DataSet ReadImageFile(const std::string& fullPath, const std::string& fieldName);

} // namespace viskores::io
} // namespace viskores:

#endif //viskores_io_ImageUtils_h
