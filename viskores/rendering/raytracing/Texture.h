//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_rendering_raytracing_Texture_h
#define viskores_rendering_raytracing_Texture_h

#include <viskores/List.h>
#include <viskores/Range.h>
#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UncertainArrayHandle.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

using TextureRenderingTypes =
  viskores::List<viskores::Float32, viskores::Float64, viskores::Vec2f_32, viskores::Vec2f_64>;

VISKORES_CONT inline viskores::cont::UncertainArrayHandle<TextureRenderingTypes,
                                                          VISKORES_DEFAULT_STORAGE_LIST>
GetTextureFieldArray(const viskores::cont::Field& field)
{
  return field.GetData().ResetTypes(TextureRenderingTypes{}, VISKORES_DEFAULT_STORAGE_LIST{});
}

template <typename Precision, typename ComponentType>
VISKORES_EXEC inline viskores::Vec<Precision, 2> MakeTextureCoordinates(ComponentType value)
{
  return { Precision(value), Precision(0) };
}

template <typename Precision, typename ComponentType>
VISKORES_EXEC inline viskores::Vec<Precision, 2> MakeTextureCoordinates(
  const viskores::Vec<ComponentType, 2>& value)
{
  return { Precision(value[0]), Precision(value[1]) };
}

template <typename Precision>
class TextureCoordinateTransform
{
  viskores::Vec<Precision, 2> Minimum;
  viskores::Vec<Precision, 2> InverseDelta;
  viskores::Vec<bool, 2> Normalize;

public:
  VISKORES_CONT
  explicit TextureCoordinateTransform(const viskores::cont::ArrayHandle<viskores::Range>& ranges)
    : Minimum(Precision(0))
    , InverseDelta(Precision(0))
    , Normalize(false)
  {
    const auto portal = ranges.ReadPortal();
    const viskores::IdComponent numberOfRanges =
      static_cast<viskores::IdComponent>(portal.GetNumberOfValues());
    for (viskores::IdComponent component = 0; component < numberOfRanges && component < 2;
         ++component)
    {
      const viskores::Range range = portal.Get(component);
      if (range.Length() > 0.0)
      {
        this->Minimum[component] = Precision(range.Min);
        this->InverseDelta[component] = Precision(1) / Precision(range.Length());
        this->Normalize[component] = true;
      }
    }
  }

  VISKORES_EXEC viskores::Vec<Precision, 2> operator()(
    viskores::Vec<Precision, 2> coordinates) const
  {
    for (viskores::IdComponent component = 0; component < 2; ++component)
    {
      if (this->Normalize[component])
      {
        coordinates[component] =
          (coordinates[component] - this->Minimum[component]) * this->InverseDelta[component];
      }
    }
    return coordinates;
  }
};

}
}
} // namespace viskores::rendering::raytracing

#endif // viskores_rendering_raytracing_Texture_h
