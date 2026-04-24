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

std::vector<viskores::Float64> ParseIsoValues(nb::handle object)
{
  if (!object.is_valid() || object.is_none())
  {
    return {};
  }

  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of floating-point values.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t count = static_cast<size_t>(nb::len(sequence));
  std::vector<viskores::Float64> values(count);
  for (size_t index = 0; index < count; ++index)
  {
    values[index] = nb::cast<viskores::Float64>(sequence[index]);
  }
  return values;
}

} // namespace viskores::python::bindings
