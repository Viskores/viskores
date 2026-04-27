//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_RENDERING
viskores::rendering::Camera* RequireCamera(nb::handle object)
{
  viskores::rendering::Camera* camera = nullptr;
  if (nb::try_cast(object, camera))
  {
    return camera;
  }
  throw std::runtime_error("Expected a viskores.rendering.Camera instance.");
}

viskores::rendering::Mapper* RequireMapper(nb::handle object)
{
  viskores::rendering::MapperVolume* mapperVolume = nullptr;
  if (nb::try_cast(object, mapperVolume))
  {
    return mapperVolume;
  }
  viskores::rendering::MapperPoint* mapperPoint = nullptr;
  if (nb::try_cast(object, mapperPoint))
  {
    return mapperPoint;
  }
  viskores::rendering::MapperQuad* mapperQuad = nullptr;
  if (nb::try_cast(object, mapperQuad))
  {
    return mapperQuad;
  }
  viskores::rendering::MapperConnectivity* mapperConnectivity = nullptr;
  if (nb::try_cast(object, mapperConnectivity))
  {
    return mapperConnectivity;
  }
  viskores::rendering::MapperCylinder* mapperCylinder = nullptr;
  if (nb::try_cast(object, mapperCylinder))
  {
    return mapperCylinder;
  }
  viskores::rendering::MapperGlyphScalar* mapperGlyphScalar = nullptr;
  if (nb::try_cast(object, mapperGlyphScalar))
  {
    return mapperGlyphScalar;
  }
  viskores::rendering::MapperGlyphVector* mapperGlyphVector = nullptr;
  if (nb::try_cast(object, mapperGlyphVector))
  {
    return mapperGlyphVector;
  }
  viskores::rendering::MapperWireframer* mapperWireframer = nullptr;
  if (nb::try_cast(object, mapperWireframer))
  {
    return mapperWireframer;
  }
  viskores::rendering::MapperRayTracer* mapperRayTracer = nullptr;
  if (nb::try_cast(object, mapperRayTracer))
  {
    return mapperRayTracer;
  }
  throw std::runtime_error(
    "Expected a viskores.rendering mapper instance.");
}

viskores::rendering::Color ParseColor(
  nb::handle object,
  const viskores::rendering::Color& defaultValue)
{
  if (!object.is_valid() || object.is_none())
  {
    return defaultValue;
  }

  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of 3 or 4 floats.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  if ((size != 3) && (size != 4))
  {
    throw std::runtime_error("Expected a sequence of 3 or 4 floats.");
  }

  float values[4] = { defaultValue.Components[0],
                      defaultValue.Components[1],
                      defaultValue.Components[2],
                      defaultValue.Components[3] };
  for (size_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<float>(nb::cast<double>(sequence[index]));
  }
  return viskores::rendering::Color(values[0], values[1], values[2], values[3]);
}
#endif

} // namespace viskores::python::bindings
