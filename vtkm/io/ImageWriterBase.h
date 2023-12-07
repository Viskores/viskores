//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageWriterBase_h
#define vtk_m_io_ImageWriterBase_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/io/vtkm_io_export.h>

namespace vtkm
{
namespace io
{

/// \brief Manages writing, and loading data from images
///
/// `ImageWriterBase` implements methods for loading imaging data from a canvas or
/// ArrayHandle and storing that data in a vtkm::cont::DataSet.  Image RGB values
/// are represented as a point field in a 2D uniform dataset.
///
/// `ImageWriterBase` can be constructed from a file, canvas, or ArrayHandle.  It can
/// also be empy constructed and filled in with a dataset later.
///
/// `ImageWriterBase` implements virtual methods for writing files.  Ideally,
/// these methods will be overriden in various subclasses to implement specific
/// functionality for writing data to specific image file-types.
///
class VTKM_IO_EXPORT ImageWriterBase
{
public:
  using ColorArrayType = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;

  VTKM_CONT ImageWriterBase(const char* filename);
  /// @brief Construct a writer to save data to the given file.
  VTKM_CONT ImageWriterBase(const std::string& filename);
  VTKM_CONT virtual ~ImageWriterBase() noexcept;
  ImageWriterBase(const ImageWriterBase&) = delete;
  ImageWriterBase& operator=(const ImageWriterBase&) = delete;

  /// \brief Write the color field of a data set to an image file.
  ///
  /// The `DataSet` must have a 2D structured cell set.
  ///
  /// The specified color field must be of type `ColorArrayType` (a basic
  /// `ArrayHandle` of `vtkm::Vec4f_32`). If no color field name is given,
  /// the first point field that matches this criteria is written.
  ///
  VTKM_CONT virtual void WriteDataSet(const vtkm::cont::DataSet& dataSet,
                                      const std::string& colorField = {});

  enum class PixelDepth
  {
    PIXEL_8,
    PIXEL_16
  };

  /// @brief Specify the number of bits used by each color channel.
  VTKM_CONT PixelDepth GetPixelDepth() const { return this->Depth; }
  /// @brief Specify the number of bits used by each color channel.
  VTKM_CONT void SetPixelDepth(PixelDepth depth) { this->Depth = depth; }

  VTKM_CONT const std::string& GetFileName() const { return this->FileName; }
  VTKM_CONT void SetFileName(const std::string& filename) { this->FileName = filename; }

protected:
  std::string FileName;
  PixelDepth Depth = PixelDepth::PIXEL_8;

  VTKM_CONT virtual void Write(vtkm::Id width, vtkm::Id height, const ColorArrayType& pixels) = 0;
};
}
}

#endif //vtk_m_io_ImageWriterBase_h
