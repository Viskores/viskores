//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/FileUtils.h>
#include <vtkm/io/ImageWriterBase.h>

#include <vtkm/cont/Logging.h>

namespace vtkm
{
namespace io
{

ImageWriterBase::ImageWriterBase(const char* filename)
  : FileName(filename)
{
}

ImageWriterBase::ImageWriterBase(const std::string& filename)
  : FileName(filename)
{
}

ImageWriterBase::~ImageWriterBase() noexcept {}

void ImageWriterBase::WriteDataSet(const vtkm::cont::DataSet& dataSet,
                                   const std::string& colorFieldName)
{
  using CellSetType = vtkm::cont::CellSetStructured<2>;
  if (!dataSet.GetCellSet().IsType<CellSetType>())
  {
    throw vtkm::cont::ErrorBadType(
      "Image writers can only write data sets with 2D structured data.");
  }
  CellSetType cellSet = dataSet.GetCellSet().AsCellSet<CellSetType>();
  vtkm::Id2 cellDimensions = cellSet.GetCellDimensions();
  // Number of points is one more in each dimension than number of cells
  vtkm::Id width = cellDimensions[0] + 1;
  vtkm::Id height = cellDimensions[1] + 1;

  vtkm::cont::Field colorField;
  if (!colorFieldName.empty())
  {
    if (!dataSet.HasPointField(colorFieldName))
    {
      throw vtkm::cont::ErrorBadValue("Data set does not have requested field " + colorFieldName);
    }
    colorField = dataSet.GetPointField(colorFieldName);
  }
  else
  {
    // Find a field of the correct type.
    vtkm::Id numFields = dataSet.GetNumberOfFields();
    bool foundField = false;
    for (vtkm::Id fieldId = 0; fieldId < numFields; ++fieldId)
    {
      colorField = dataSet.GetField(fieldId);
      if ((colorField.GetAssociation() == vtkm::cont::Field::Association::Points) &&
          (colorField.GetData().IsType<ColorArrayType>()))
      {
        foundField = true;
        break;
      }
    }
    if (!foundField)
    {
      throw vtkm::cont::ErrorBadValue(
        "Data set does not have any fields that look like color data.");
    }
  }

  if (CreateDirectoriesFromFilePath(this->FileName))
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Created output directory: " << ParentPath(this->FileName));
  }
  this->Write(width, height, colorField.GetData().AsArrayHandle<ColorArrayType>());
}

} // namespace vtkm::io
} // namespace vtkm
