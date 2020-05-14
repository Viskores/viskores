//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReader_h
#define vtk_m_io_ImageReader_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{

// Forward Declare
namespace rendering
{
class Canvas;
} // namespace rendering

namespace io
{

/// \brief Manages reading, and loading data from images
///
/// \c BaseImageReader implements methods for loading imaging data from a canvas or
/// ArrayHandle and storing that data in a vtkm::cont::DataSet.  Image rgb values
/// are represented as a point field in a 2D uniform dataset.
///
/// \c BaseImageReader can be constructed from a file, canvas, or ArrayHandle.  It can
/// also be empy constructed and filled in with a dataset later.
///
/// \c BaseImageReader implements virtual methods for reading files.  Ideally,
/// these methods will be overriden in various subclasses to implement specific
/// functionality for reading data to specific image file-types.
///
/// \c The templated type is used when Filling the ImageDataSet.
///
class BaseImageReader
{
public:
  /// Constructs an emtpy BaseImageReader.
  ///
  BaseImageReader() = default;
  explicit BaseImageReader(const vtkm::Id& maxColorValue)
    : MaxColorValue(maxColorValue)
  {
  }
  ~BaseImageReader() noexcept = default;

  /// Reads image data from a file.  Meant to be implemented in overriden
  /// image-specific classes
  ///
  virtual vtkm::cont::DataSet ReadFromFile(const std::string& fileName) = 0;

  /// Creates an ImageDataSet from a Canvas object
  ///
  vtkm::cont::DataSet CreateImageDataSet(const vtkm::rendering::Canvas& canvas);

  /// Creates an ImageDataSet from a RGBA 32bit float color buffer
  /// Assumes the color buffer is stored in row-major ordering
  ///
  vtkm::cont::DataSet CreateImageDataSet(const vtkm::cont::ArrayHandle<vtkm::Vec4f_32>& colorBuffer,
                                         const vtkm::Id& width,
                                         const vtkm::Id& height);

  const std::string& GetPointFieldName() const { return this->PointFieldName; }

protected:
  vtkm::cont::DataSet InitializeImageDataSet(const vtkm::Id& width, const vtkm::Id& height);

  std::string PointFieldName = "pixel-data";
  vtkm::Id MaxColorValue{ 0 };
};

/// \brief Manages reading images using the PNG format via lodepng
///
/// \c PNGReader extends BaseImageReader and implements reading images in a valid
/// PNG format.  It utilizes lodepng's decode file functions to read
/// PNG images that are automatically compressed to optimal sizes relative to
/// the actual bit complexity of the image.
///
/// \c PNGReader will automatically upsample/downsample read image data
/// to a 16 bit RGB no matter how the image is compressed. It is up to the user to
/// decide the pixel format for input PNGs
class PNGReader : public BaseImageReader
{
  using Superclass = BaseImageReader;

public:
  using Superclass::Superclass;
  PNGReader() = default;
  ~PNGReader() noexcept = default;

  /// Reads PNG data from the provided file and stores it
  /// as a 16bit RGB value
  ///
  vtkm::cont::DataSet ReadFromFile(const std::string& fileName) override;

  /// Reads PNG data from the provided file and stores it
  /// according to the method's templated PixelType
  ///
  template <typename PixelType>
  vtkm::cont::DataSet ReadFromFile(const std::string& fileName);
};


/// \brief Manages reading images using the PNM format
///
/// \c PNMImage extends BaseImage, and implements reading images from a
/// valid PNM format (for magic number P6). More details on the PNM
/// format can be found here: http://netpbm.sourceforge.net/doc/ppm.html
///
/// When a file is read the parsed MagicNumber and MaxColorSize provided
/// are utilized to correctly parse the bits from the file
class PNMReader : public BaseImageReader
{
  using Superclass = BaseImageReader;

public:
  using Superclass::Superclass;
  PNMReader() = default;
  ~PNMReader() noexcept = default;

  /// Attempts to read the provided file into a DataSet object.
  /// Will pull the image's MaxColorValue from the file and then Decode
  /// with the appropriate RGB PixelType bit depth.
  ///
  vtkm::cont::DataSet ReadFromFile(const std::string& fileName) override;

protected:
  /// Reads image data from the provided inStream with the supplied width/height
  /// Stores the data in a vector of PixelType which is converted to an DataSet
  ///
  template <typename PixelType>
  vtkm::cont::DataSet DecodeFile(std::ifstream& inStream,
                                 const vtkm::Id& width,
                                 const vtkm::Id& height);

  // This is set to only work for P6 pnm image types for now (ie ppm)
  std::string MagicNumber{ "P6" };
};


} // namespace io
} // namespace vtkm

#include <vtkm/io/ImageReader.hxx>

#endif
