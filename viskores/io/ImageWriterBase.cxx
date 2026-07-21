//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#include <viskores/io/FileUtils.h>
#include <viskores/io/ImageWriterBase.h>

#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Logging.h>

#include <viskores/worklet/WorkletMapField.h>

namespace
{

struct NormalizeColorsWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inColor, FieldOut outColor);
  using ExecutionSignature = _2(_1);
  using InputDomain = _1;

  template <typename InColorVec>
  VISKORES_EXEC viskores::Vec4f_32 operator()(const InColorVec& inColor) const
  {
    viskores::Vec4f_32 outColor{ 1.0f };
    for (viskores::IdComponent compIdx = 0; compIdx < inColor.GetNumberOfComponents(); ++compIdx)
    {
      outColor[compIdx] = this->NormalizeChannel(inColor[compIdx]);
    }
    return outColor;
  }

  VISKORES_EXEC constexpr viskores::Float32 NormalizeChannel(viskores::UInt8 channel) const
  {
    return static_cast<viskores::Float32>(channel) / 255.0f;
  }
  VISKORES_EXEC constexpr viskores::Float32 NormalizeChannel(viskores::Float32 channel) const
  {
    return channel;
  }
  VISKORES_EXEC constexpr viskores::Float32 NormalizeChannel(viskores::Float64 channel) const
  {
    return static_cast<viskores::Float32>(channel);
  }
};

VISKORES_CONT viskores::io::ImageWriterBase::ColorArrayType NormalizeColors(
  const viskores::cont::UnknownArrayHandle& colorArray)
{
  viskores::io::ImageWriterBase::ColorArrayType result;
  viskores::cont::Invoker invoke;

  if (colorArray.CanConvert<viskores::io::ImageWriterBase::ColorArrayType>())
  {
    colorArray.AsArrayHandle(result);
  }
  else if (colorArray.IsBaseComponentType<viskores::UInt8>())
  {
    invoke(
      NormalizeColorsWorklet{}, colorArray.ExtractArrayFromComponents<viskores::UInt8>(), result);
  }
  else if (colorArray.IsBaseComponentType<viskores::Float32>())
  {
    invoke(
      NormalizeColorsWorklet{}, colorArray.ExtractArrayFromComponents<viskores::Float32>(), result);
  }
  else if (colorArray.IsBaseComponentType<viskores::Float64>())
  {
    invoke(
      NormalizeColorsWorklet{}, colorArray.ExtractArrayFromComponents<viskores::Float64>(), result);
  }
  else
  {
    throw viskores::cont::ErrorBadType("Image writers only support UInt8 or Float channels.");
  }

  return result;
}

} // anonymous namespace

namespace viskores
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

void ImageWriterBase::WriteDataSet(const viskores::cont::DataSet& dataSet,
                                   const std::string& colorFieldName)
{
  using CellSetType = viskores::cont::CellSetStructured<2>;
  if (!dataSet.GetCellSet().IsType<CellSetType>())
  {
    throw viskores::cont::ErrorBadType(
      "Image writers can only write data sets with 2D structured data.");
  }
  CellSetType cellSet = dataSet.GetCellSet().AsCellSet<CellSetType>();
  viskores::Id2 cellDimensions = cellSet.GetCellDimensions();
  // Number of points is one more in each dimension than number of cells
  viskores::Id width = cellDimensions[0] + 1;
  viskores::Id height = cellDimensions[1] + 1;

  viskores::cont::Field colorField;
  if (!colorFieldName.empty())
  {
    if (!dataSet.HasPointField(colorFieldName))
    {
      throw viskores::cont::ErrorBadValue("Data set does not have requested field " +
                                          colorFieldName);
    }
    colorField = dataSet.GetPointField(colorFieldName);
  }
  else
  {
    // Find a field of the correct type.
    viskores::Id numFields = dataSet.GetNumberOfFields();
    bool foundField = false;
    for (viskores::Id fieldId = 0; fieldId < numFields; ++fieldId)
    {
      colorField = dataSet.GetField(fieldId);
      if ((colorField.GetAssociation() == viskores::cont::Field::Association::Points) &&
          (colorField.GetData().IsType<ColorArrayType>()))
      {
        foundField = true;
        break;
      }
    }
    if (!foundField)
    {
      throw viskores::cont::ErrorBadValue(
        "Data set does not have any fields that look like color data.");
    }
  }

  if (CreateDirectoriesFromFilePath(this->FileName))
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                   "Created output directory: " << ParentPath(this->FileName));
  }
  this->Write(width, height, NormalizeColors(colorField.GetData()));
}

} // namespace viskores::io
} // namespace viskores
