//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
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
