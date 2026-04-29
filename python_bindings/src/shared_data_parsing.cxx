//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

namespace
{

template <typename ValueType, typename CastType>
std::vector<ValueType> ParseNumericSequence(nb::handle object, const char* message)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error(message);
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  std::vector<ValueType> values(size);
  for (size_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<ValueType>(nb::cast<CastType>(sequence[index]));
  }
  return values;
}

} // namespace

std::vector<viskores::Vec3f> ParseVec3Sequence(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of coordinate tuples.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  std::vector<viskores::Vec3f> values;
  values.reserve(size);
  for (size_t index = 0; index < size; ++index)
  {
    values.push_back(ParseVec3(sequence[index], viskores::Vec3f(0.0f, 0.0f, 0.0f)));
  }
  return values;
}

std::vector<viskores::UInt8> ParseUInt8Sequence(nb::handle object)
{
  return ParseNumericSequence<viskores::UInt8, unsigned long>(
    object, "Expected a sequence of integer values.");
}

std::vector<viskores::IdComponent> ParseIdComponentSequence(nb::handle object)
{
  return ParseNumericSequence<viskores::IdComponent, long long>(
    object, "Expected a sequence of integer values.");
}

std::vector<viskores::Id> ParseIdSequence(nb::handle object)
{
  return ParseNumericSequence<viskores::Id, long long>(
    object, "Expected a sequence of integer values.");
}

viskores::cont::ArrayHandle<viskores::FloatDefault> ParseRectilinearAxis(nb::handle object,
                                                                         const char* name)
{
  if (!object.is_valid() || object.is_none())
  {
    std::ostringstream message;
    message << name << " coordinate values are required.";
    throw std::runtime_error(message.str());
  }

  std::ostringstream message;
  message << name << " coordinate values must be a one-dimensional numeric sequence.";
  const std::string sequenceMessage = message.str();
  auto values =
    ParseNumericSequence<viskores::FloatDefault, double>(object, sequenceMessage.c_str());
  if (values.empty())
  {
    std::ostringstream emptyMessage;
    emptyMessage << name << " coordinate values must not be empty.";
    throw std::runtime_error(emptyMessage.str());
  }
  return viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
}

} // namespace viskores::python::bindings
