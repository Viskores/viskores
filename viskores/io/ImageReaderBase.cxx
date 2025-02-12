//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/io/ImageReaderBase.h>

#include <viskores/cont/DataSetBuilderUniform.h>

namespace viskores
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

const viskores::cont::DataSet& ImageReaderBase::ReadDataSet()
{
  this->Read();
  return this->DataSet;
}

void ImageReaderBase::InitializeImageDataSet(const viskores::Id& width,
                                             const viskores::Id& height,
                                             const ColorArrayType& pixels)
{
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::Id2 dimensions(width, height);
  this->DataSet = dsb.Create(dimensions);
  this->DataSet.AddPointField(this->PointFieldName, pixels);
}
}
} // namespace viskores::io
