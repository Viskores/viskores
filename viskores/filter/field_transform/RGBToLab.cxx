//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/List.h>
#include <viskores/Math.h>
#include <viskores/StaticAssert.h>
#include <viskores/TypeTraits.h>
#include <viskores/filter/field_transform/RGBToLab.h>
#include <viskores/worklet/WorkletMapField.h>

namespace
{

class RGBToLabWorklet : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  template <typename T>
  VISKORES_EXEC viskores::Vec3f operator()(const viskores::Vec<T, 3>& rgb) const
  {
    const viskores::FloatDefault r = LinearizeSRGB(NormalizeRGBComponent(rgb[0]));
    const viskores::FloatDefault g = LinearizeSRGB(NormalizeRGBComponent(rgb[1]));
    const viskores::FloatDefault b = LinearizeSRGB(NormalizeRGBComponent(rgb[2]));

    const viskores::FloatDefault x =
      (viskores::FloatDefault(0.4124564) * r + viskores::FloatDefault(0.3575761) * g +
       viskores::FloatDefault(0.1804375) * b) /
      viskores::FloatDefault(0.95047);
    const viskores::FloatDefault y = viskores::FloatDefault(0.2126729) * r +
      viskores::FloatDefault(0.7151522) * g + viskores::FloatDefault(0.0721750) * b;
    const viskores::FloatDefault z =
      (viskores::FloatDefault(0.0193339) * r + viskores::FloatDefault(0.1191920) * g +
       viskores::FloatDefault(0.9503041) * b) /
      viskores::FloatDefault(1.08883);

    const viskores::FloatDefault fx = LabFunction(x);
    const viskores::FloatDefault fy = LabFunction(y);
    const viskores::FloatDefault fz = LabFunction(z);

    return viskores::Vec3f(viskores::FloatDefault(116) * fy - viskores::FloatDefault(16),
                           viskores::FloatDefault(500) * (fx - fy),
                           viskores::FloatDefault(200) * (fy - fz));
  }

private:
  VISKORES_EXEC static viskores::FloatDefault NormalizeRGBComponent(viskores::UInt8 value)
  {
    return static_cast<viskores::FloatDefault>(value) / viskores::FloatDefault(255);
  }

  template <typename T>
  VISKORES_EXEC static viskores::FloatDefault NormalizeRGBComponent(T value)
  {
    VISKORES_STATIC_ASSERT_MSG(
      (std::is_same_v<viskores::TypeTraitsRealTag, typename viskores::TypeTraits<T>::NumericTag>),
      "Trying to convert color that is not byte or float.");
    const viskores::FloatDefault normalized = static_cast<viskores::FloatDefault>(value);
    return viskores::Clamp(normalized, viskores::FloatDefault{ 0 }, viskores::FloatDefault{ 1 });
  }

  VISKORES_EXEC static viskores::FloatDefault LinearizeSRGB(viskores::FloatDefault value)
  {
    if (value <= viskores::FloatDefault(0.04045))
    {
      return value / viskores::FloatDefault(12.92);
    }
    return viskores::Pow((value + viskores::FloatDefault(0.055)) / viskores::FloatDefault(1.055),
                         viskores::FloatDefault(2.4));
  }

  VISKORES_EXEC static viskores::FloatDefault LabFunction(viskores::FloatDefault value)
  {
    constexpr viskores::FloatDefault epsilon =
      viskores::FloatDefault(216) / viskores::FloatDefault(24389);
    constexpr viskores::FloatDefault kappa =
      viskores::FloatDefault(24389) / viskores::FloatDefault(27);

    if (value > epsilon)
    {
      return viskores::Cbrt(value);
    }
    return (kappa * value + viskores::FloatDefault(16)) / viskores::FloatDefault(116);
  }
};

} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace field_transform
{

VISKORES_CONT viskores::cont::DataSet RGBToLab::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  using RGBFieldTypes = viskores::List<viskores::Vec3ui_8, viskores::Vec3f_32, viskores::Vec3f_64>;

  const auto& inField = this->GetFieldFromDataSet(inDataSet);
  viskores::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete)
  {
    viskores::cont::ArrayHandle<viskores::Vec3f> result;
    this->Invoke(RGBToLabWorklet{}, concrete, result);
    outArray = result;
  };
  inField.GetData().CastAndCallForTypes<RGBFieldTypes, VISKORES_DEFAULT_STORAGE_LIST>(resolveType);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    outputName = inField.GetName() + "_lab";
  }

  return this->CreateResultField(inDataSet, outputName, inField.GetAssociation(), outArray);
}

} // namespace field_transform
} // namespace filter
} // namespace viskores
