//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "ArrayConversion.h"
#include "anari/frontend/type_utility.h"
// Viskores
#include <viskores/TypeList.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>
// C++
#include <limits>
#include <type_traits>


namespace
{

template <typename BaseType, int NumComponents>
struct ViskoresTypeImpl
{
  using type = viskores::Vec<BaseType, static_cast<viskores::IdComponent>(NumComponents)>;
};
template <typename BaseType>
struct ViskoresTypeImpl<BaseType, 1>
{
  using type = BaseType;
};

template <int ANARIDataTypeId>
struct ANARIToViskoresTypeImpl
{
  using properties = anari::ANARITypeProperties<ANARIDataTypeId>;
  using type =
    typename ViskoresTypeImpl<typename properties::base_type, properties::components>::type;
};

template <int ANARIDataTypeId>
using ANARIToViskoresType = typename ANARIToViskoresTypeImpl<ANARIDataTypeId>::type;

template <int ANARIDataTypeId>
struct ConstructArrayHandle
{
  using T = ANARIToViskoresType<ANARIDataTypeId>;
  viskores::cont::UnknownArrayHandle operator()(const void* memory, viskores::Id numValues)
  {
    return this->DoIt(memory, numValues, std::is_pointer<T>{});
  }

  viskores::cont::UnknownArrayHandle DoIt(const void* memory,
                                          viskores::Id numValues,
                                          std::false_type /*is_pointer*/)
  {
    // TODO: The ANARI interface generally passes arrays in the host memory space.
    // This can be really inefficient as data is pulled from GPU to CPU, passed to
    // ANARI, and then copied right back to the GPU. Need an extension to allow
    // client applications to pass device memory directly.
    return viskores::cont::make_ArrayHandle(
      reinterpret_cast<const T*>(memory), numValues, viskores::CopyFlag::Off);
  }

  viskores::cont::UnknownArrayHandle DoIt(const void*, viskores::Id, std::true_type /*is_pointer*/)
  {
    std::cout << "Cannot convert an array of type "
              << anari::ANARITypeProperties<ANARIDataTypeId>::type_name << " to Viskores.\n";
    return viskores::cont::UnknownArrayHandle{};
  }
};
template <>
struct ConstructArrayHandle<ANARI_UNKNOWN>
{
  viskores::cont::UnknownArrayHandle operator()(const void*, viskores::Id)
  {
    std::cout << "Cannot convert an array of type ANARI_UNKNOWN to Viskores.\n";
    return viskores::cont::UnknownArrayHandle{};
  }
};


struct ConvertColorValues : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  template <typename InType, typename OutType>
  VISKORES_EXEC void operator()(const InType& inValue, OutType& outValue) const
  {
    using InComponentType = typename InType::ComponentType;
    using OutComponentType = typename OutType::ComponentType;

    constexpr OutComponentType scale = OutComponentType{ 1 } /
      static_cast<OutComponentType>(std::numeric_limits<InComponentType>::max());
    for (viskores::IdComponent index = 0; index < inValue.GetNumberOfComponents(); ++index)
    {
      outValue[index] = static_cast<OutComponentType>(inValue[index]) * scale;
    }
  }
};

template <typename T>
void ChannelsToFloat(const viskores::cont::UnknownArrayHandle& originalChannels,
                     viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>& floatChannels,
                     viskores::TypeTraitsIntegerTag)
{
  floatChannels = viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>(
    originalChannels.GetNumberOfComponentsFlat());
  viskores::cont::Invoker invoke;
  invoke(ConvertColorValues{}, originalChannels.ExtractArrayFromComponents<T>(), floatChannels);
}

template <typename T>
void ChannelsToFloat(const viskores::cont::UnknownArrayHandle& originalChannels,
                     viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>& floatChannels,
                     viskores::TypeTraitsRealTag)
{
  floatChannels = viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>(
    originalChannels.GetNumberOfComponentsFlat());
  viskores::cont::ArrayCopyShallowIfPossible(originalChannels, floatChannels);
}

template <typename T>
void ChannelsToFloat(const viskores::cont::UnknownArrayHandle& originalChannels,
                     viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>& floatChannels,
                     bool& converted)
{
  if (!converted && originalChannels.IsBaseComponentType<T>())
  {
    ChannelsToFloat<T>(
      originalChannels, floatChannels, typename viskores::TypeTraits<T>::NumericTag{});
    converted = true;
  }
}

viskores::cont::ArrayHandleRuntimeVec<viskores::Float32> ChannelsToFloat(
  const viskores::cont::UnknownArrayHandle& originalChannels)
{
  viskores::cont::ArrayHandleRuntimeVec<viskores::Float32> floatChannels;
  bool converted = false;
  viskores::ListForEach(
    [&](auto type) { ChannelsToFloat<decltype(type)>(originalChannels, floatChannels, converted); },
    viskores::TypeListScalarAll{});
  if (!converted)
  {
    throw viskores::cont::ErrorBadType("Could not identify type for color array.");
  }
  return floatChannels;
}

struct GammaCorrection : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);

  template <typename ChannelType>
  VISKORES_EXEC void operator()(ChannelType& channels) const
  {
    // Only adjust RGB channels, not alpha.
    // We may need to unpremultiply the color channels before linearizing the channels, but
    // I am not handling that right now.
    viskores::IdComponent numComponents = viskores::Min(channels.GetNumberOfComponents(), 3);
    for (viskores::IdComponent i = 0; i < numComponents; ++i)
    {
      channels[i] = viskores::Pow(channels[i], 2.2f);
    }
  }
};

viskores::cont::ArrayHandle<viskores::Vec4f_32> ExpandColorChannels(
  const viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>& inputChannels)
{
  viskores::cont::ArrayHandleConstant<viskores::Float32> ones(1.0f,
                                                              inputChannels.GetNumberOfValues());
  viskores::cont::ArrayHandleRecombineVec<viskores::Float32> combinedChannels;

  // The ANARI specification of color (as of version 1.1,
  // https://registry.khronos.org/ANARI/specs/1.1/ANARI-1.1.html#color) is
  // remarkably unclear on how to interpret the channels of the color. It is
  // well implied that 3 channels specify RGB and 4 channels specify RGBA. We
  // interpret 1 channel as a grayscale/luminance and 2 channels as grayscale +
  // opacity. In these later two cases, all three RGB channels in the output are
  // set to the value in the input. Note that this is different than the Helide
  // reference implementation, but that implementation makes little sense. It
  // just sets the respective R and G channels of the output.
  viskores::IdComponent numComponents = inputChannels.GetNumberOfComponentsFlat();
  if ((numComponents < 1) || (numComponents > 4))
  {
    throw viskores::cont::ErrorBadType("Colors have invalid number of components: " +
                                       std::to_string(numComponents));
  }
  if (numComponents == 4)
  {
    // Special case: Colors already in expected RGBA.
    return inputChannels.AsArrayHandleBasic<viskores::cont::ArrayHandle<viskores::Vec4f_32>>();
  }
  if (numComponents <= 2)
  {
    // First component is luminance.
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 0));
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 0));
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 0));
  }
  else
  {
    // First 3 components are RGB
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 0));
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 1));
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 2));
  }

  if (numComponents == 2)
  {
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(inputChannels, 1));
  }
  else
  {
    combinedChannels.AppendComponentArray(viskores::cont::ArrayExtractComponent(ones, 0));
  }

  viskores::cont::ArrayHandle<viskores::Vec4f_32> outputChannels;
  viskores::cont::ArrayCopy(combinedChannels, outputChannels);
  return outputChannels;
}

viskores::cont::ArrayHandle<viskores::Vec4f_32> ANARIColorsToViskoresColorsImpl(
  const viskores::cont::UnknownArrayHandle& anariColors,
  ANARIDataType anariType)
{
  viskores::cont::ArrayHandleRuntimeVec<viskores::Float32> floatChannels =
    ChannelsToFloat(anariColors);

  if ((anariType == ANARI_UFIXED8_R_SRGB) || (anariType == ANARI_UFIXED8_RA_SRGB) ||
      (anariType == ANARI_UFIXED8_RGB_SRGB) || (anariType == ANARI_UFIXED8_RGBA_SRGB))
  {
    viskores::cont::Invoker invoke;
    invoke(GammaCorrection{}, floatChannels);
  }

  return ExpandColorChannels(floatChannels);
}

template <typename ArrayType>
viskores::cont::ArrayHandle<viskores::Vec4f_32> ANARIColorsToViskoresColorsImpl(
  const ArrayType& anariColors)
{
  try
  {
    return ANARIColorsToViskoresColorsImpl(anariColors.dataAsViskoresArray(),
                                           anariColors.elementType());
  }
  catch (viskores::cont::Error error)
  {
    anariColors.reportMessage(
      ANARI_SEVERITY_ERROR, "Failed to read color array: %s", error.GetMessage());
    return viskores::cont::make_ArrayHandle({ viskores::Vec4f_32{ 1.0f, 0.8f, 0.0f, 1.0f } });
  }
}

} // anonymous namespace

namespace viskores_device
{

viskores::cont::UnknownArrayHandle ANARIArrayToViskoresArray(const helium::Array* anariArray)
{
  viskores::Id numValues = anariArray->totalSize();
  const void* memory = anariArray->data();

  return anari::anariTypeInvoke<viskores::cont::UnknownArrayHandle, ConstructArrayHandle>(
    anariArray->elementType(), memory, numValues);
}

viskores::cont::ArrayHandle<viskores::Vec4f_32> ANARIColorsToViskoresColors(
  const Array1D& anariColors)
{
  return ANARIColorsToViskoresColorsImpl(anariColors);
}

viskores::cont::ArrayHandle<viskores::Vec4f_32> ANARIColorsToViskoresColors(
  const Array2D& anariColors)
{
  return ANARIColorsToViskoresColorsImpl(anariColors);
}

viskores::cont::ArrayHandle<viskores::Vec4f_32> ANARIColorsToViskoresColors(
  const Array3D& anariColors)
{
  return ANARIColorsToViskoresColorsImpl(anariColors);
}

} // namespace viskores_device
