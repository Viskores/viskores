//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_ImageReaderBase_h
#define vtk_m_io_ImageReaderBase_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/io/vtkm_io_export.h>

namespace vtkm
{
namespace io
{

/// \brief Manages reading, and loading data from images
///
/// `ImageReaderBase` implements methods for loading imaging data from a canvas or
/// ArrayHandle and storing that data in a vtkm::cont::DataSet.  Image RGB values
/// are represented as a point field in a 2D uniform dataset.
///
/// `ImageReaderBase` implements virtual methods for reading files.  Ideally,
/// these methods will be overriden in various subclasses to implement specific
/// functionality for reading data to specific image file-types.
///
class VTKM_IO_EXPORT ImageReaderBase
{
public:
  using ColorArrayType = vtkm::cont::ArrayHandle<vtkm::Vec4f_32>;

  explicit VTKM_CONT ImageReaderBase(const char* filename);
  /// @brief Construct a reader to load data from the given file.
  explicit VTKM_CONT ImageReaderBase(const std::string& filename);
  virtual VTKM_CONT ~ImageReaderBase() noexcept;
  ImageReaderBase(const ImageReaderBase&) = delete;
  ImageReaderBase& operator=(const ImageReaderBase&) = delete;

  /// @brief Load data from the file and return it in a `DataSet` object.
  VTKM_CONT const vtkm::cont::DataSet& ReadDataSet();

  VTKM_CONT const vtkm::cont::DataSet& GetDataSet() const { return this->DataSet; }

  /// @brief Get the name of the output field that will be created to hold color data.
  VTKM_CONT const std::string& GetPointFieldName() const { return this->PointFieldName; }
  /// @brief Set the name of the output field that will be created to hold color data.
  VTKM_CONT void SetPointFieldName(const std::string& name) { this->PointFieldName = name; }

  VTKM_CONT const std::string& GetFileName() const { return this->FileName; }
  VTKM_CONT void SetFileName(const std::string& filename) { this->FileName = filename; }

protected:
  VTKM_CONT virtual void Read() = 0;

  /// Resets the `DataSet` to hold the given pixels.
  void InitializeImageDataSet(const vtkm::Id& width,
                              const vtkm::Id& height,
                              const ColorArrayType& pixels);

  std::string FileName;
  std::string PointFieldName = "color";
  vtkm::cont::DataSet DataSet;
};
}
} // namespace vtkm::io

#endif //vtk_m_io_ImageReaderBase_h
