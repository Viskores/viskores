//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/ImageReaderBase.h>

#include <vtkm/cont/DataSetBuilderUniform.h>

namespace vtkm
{
namespace io
{

ImageReaderBase::ImageReaderBase(const char* filename)
  : FileName(filename)
{
}

ImageReaderBase::ImageReaderBase(const std::string& filename)
  : FileName(filename)
{
}

ImageReaderBase::~ImageReaderBase() noexcept {}

const vtkm::cont::DataSet& ImageReaderBase::ReadDataSet()
{
  this->Read();
  return this->DataSet;
}

void ImageReaderBase::InitializeImageDataSet(const vtkm::Id& width,
                                             const vtkm::Id& height,
                                             const ColorArrayType& pixels)
{
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::Id2 dimensions(width, height);
  this->DataSet = dsb.Create(dimensions);
  this->DataSet.AddPointField(this->PointFieldName, pixels);
}
}
} // namespace vtkm::io
