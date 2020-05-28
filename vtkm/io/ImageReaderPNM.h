//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReaderPNM_h
#define vtk_m_io_ImageReaderPNM_h

#include <vtkm/io/ImageReaderBase.h>

namespace vtkm
{
namespace io
{

/// \brief Manages reading images using the PNM format
///
/// `ImageReaderPNM` extends `ImageReaderBase`, and implements reading images from a
/// valid PNM format (for magic number P6). More details on the PNM
/// format can be found here: http://netpbm.sourceforge.net/doc/ppm.html
///
/// When a file is read the parsed MagicNumber and MaxColorSize provided
/// are utilized to correctly parse the bits from the file
class VTKM_IO_EXPORT ImageReaderPNM : public ImageReaderBase
{
  using Superclass = ImageReaderBase;

public:
  using Superclass::Superclass;
  VTKM_CONT ~ImageReaderPNM() noexcept override;
  ImageReaderPNM(const ImageReaderPNM&) = delete;
  ImageReaderPNM& operator=(const ImageReaderPNM&) = delete;

protected:
  VTKM_CONT void Read() override;

  /// Reads image data from the provided inStream with the supplied width/height
  /// Stores the data in a vector of PixelType which is converted to an DataSet
  ///
  template <typename PixelType>
  void DecodeFile(std::ifstream& inStream, const vtkm::Id& width, const vtkm::Id& height);
};
}
} // namespace vtkm::io

#endif //vtk_m_io_ImageReaderPNM_h
