//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageWriter_h
#define vtk_m_io_ImageWriter_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace io
{

/// \brief Manages writing, and loading data from images
///
/// \c BaseImageWriter implements methods for loading imaging data from a canvas or
/// ArrayHandle and storing that data in a vtkm::cont::DataSet.  Image rgb values
/// are represented as a point field in a 2D uniform dataset.
///
/// \c BaseImageWriter can be constructed from a file, canvas, or ArrayHandle.  It can
/// also be empy constructed and filled in with a dataset later.
///
/// \c BaseImageWriter implements virtual methods for writing files.  Ideally,
/// these methods will be overriden in various subclasses to implement specific
/// functionality for writing data to specific image file-types.
///
class BaseImageWriter
{
public:
  /// Constructs an emtpy BaseImageWriter.
  ///
  BaseImageWriter() = default;
  explicit BaseImageWriter(const vtkm::Id& maxColorValue)
    : MaxColorValue(maxColorValue)
  {
  }
  ~BaseImageWriter() noexcept = default;

  /// Write and store ImageDataSet to a file.  Meant to be implemented in
  /// overriden image-specific classes
  ///
  virtual void WriteToFile(const std::string& fileName,
                           const vtkm::cont::DataSet& dataSet) const = 0;

  vtkm::Id GetImageWidth(vtkm::cont::DataSet dataSet) const;
  vtkm::Id GetImageHeight(vtkm::cont::DataSet dataSet) const;

  const std::string& GetPointFieldName() const { return this->PointFieldName; }
  void SetMaxColorValue(const vtkm::Id& maxColorValue) { this->MaxColorValue = maxColorValue; }

protected:
  std::string PointFieldName = "pixel-data";
  vtkm::Id MaxColorValue{ 0 };
};

/// \brief Manages writing images using the PNM format
///
/// \c PNMWriter extends BaseImageWriter, and implements writing images in a
/// valid PNM format (for magic number P6). More details on the PNM
/// format can be found here: http://netpbm.sourceforge.net/doc/ppm.html
///
/// When a file is writen the MaxColorValue found in the file is used to
/// determine the PixelType required to stored PixelType is instead dependent
/// upon the read MaxColorValue obtained from the file
class PNMWriter : public BaseImageWriter
{
  using Superclass = BaseImageWriter;

public:
  using Superclass::Superclass;
  PNMWriter() = default;
  ~PNMWriter() noexcept = default;

  /// Attempts to write the ImageDataSet to a PNM file. The MaxColorValue
  /// set in the file with either be selected from the stored MaxColorValue
  /// member variable, or from the templated type if MaxColorValue hasn't been
  /// set from a read file.
  ///
  void WriteToFile(const std::string& fileName, const vtkm::cont::DataSet& dataSet) const override;

protected:
  /// Writes image data stored in ImageDataSet to the provided outStream
  /// Casts the data to the provided PixelType
  ///
  template <typename PixelType>
  void EncodeFile(std::ofstream& outStream, const vtkm::cont::DataSet& dataSet) const;

  // Currently only works with P6 PNM files (PPM)
  std::string MagicNumber{ "P6" };
};

/// \brief Manages writing images using the PNG format via lodepng
///
/// \c PNGWriter extends BaseImageWriter and implements writing images in a valid
/// PNG format.  It utilizes lodepng's encode file functions to write
/// PNG images that are automatically compressed to optimal sizes relative to
/// the actual bit complexity of the image.
///
/// \c PNGImage will automatically upsample/downsample written image data
/// to the supplied templated PixelType.  For example, it is possible to write
/// a 1-bit greyscale image into a 16bit RGB PNG object. It is up to the user to
/// decide the pixel format for output PNGs
class PNGWriter : public BaseImageWriter
{
  using Superclass = BaseImageWriter;

public:
  using Superclass::Superclass;
  PNGWriter() = default;
  ~PNGWriter() noexcept = default;

  /// Writes stored data matched to the class's templated type
  /// to a file in PNG format. Relies upon the lodepng encoding
  /// method to optimize compression and choose the best storage format.
  ///
  void WriteToFile(const std::string& fileName, const vtkm::cont::DataSet& dataSet) const override;

  /// Writes stored data matched to the method's templated type
  /// to a file in PNG format. Relies upon the lodepng encoding
  /// method to optimize compression and choose the best storage format.
  ///
  template <typename PixelType>
  void WriteToFile(const std::string& fileName, const vtkm::cont::DataSet& dataSet) const;
};


} // namespace io
} // namespace vtkm

#include <vtkm/io/ImageWriter.hxx>

#endif
