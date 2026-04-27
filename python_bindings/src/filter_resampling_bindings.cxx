//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

namespace
{

void SetFieldsToPassForResampling(viskores::filter::Filter& filter, nb::object fieldsObject)
{
  auto& selection = filter.GetFieldsToPass();
  selection.ClearFields();
  selection.SetMode(viskores::filter::FieldSelection::Mode::Select);

  if (nb::isinstance<nb::str>(fieldsObject))
  {
    selection.AddField(nb::cast<std::string>(fieldsObject));
    return;
  }

  if (!nb::isinstance<nb::sequence>(fieldsObject) || nb::isinstance<nb::str>(fieldsObject))
  {
    throw std::runtime_error("fields must be a string or sequence");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(fieldsObject);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  for (size_t index = 0; index < size; ++index)
  {
    nb::handle item = sequence[index];
    if (!nb::isinstance<nb::str>(item))
    {
      throw std::runtime_error("fields must contain only strings.");
    }
    selection.AddField(nb::cast<std::string>(item));
  }
}

viskores::cont::ArrayHandle<viskores::Vec3f> ParseProbeGeometryPoints(nb::handle object)
{
  auto unknown = NumPyArrayToUnknownArray(object);
  if (!unknown.IsType<viskores::cont::ArrayHandle<viskores::Vec3f>>())
  {
    throw std::runtime_error("Probe geometry points must be a NumPy-compatible Nx3 float array.");
  }
  viskores::cont::ArrayHandle<viskores::Vec3f> points;
  unknown.AsArrayHandle(points);
  return points;
}

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING
void RegisterNanobindResamplingClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Probe");
  nb::class_<viskores::filter::resampling::Probe>(m, "Probe", doc::ClassDoc("Probe"))
    .def(nb::init<>())
    .def(
      "SetGeometry",
      [](viskores::filter::resampling::Probe& self, nb::handle geometryObject)
      {
        viskores::cont::DataSet* geometry = nullptr;
        if (nb::try_cast(geometryObject, geometry))
        {
          self.SetGeometry(*geometry);
          return;
        }
        self.SetGeometry(ParseProbeGeometryPoints(geometryObject));
      },
      nb::arg("geometry"))
    .def("GetGeometry", &viskores::filter::resampling::Probe::GetGeometry)
    .def(
      "SetInvalidValue",
      [](viskores::filter::resampling::Probe& self, double invalidValue)
      { self.SetInvalidValue(invalidValue); },
      nb::arg("invalid_value"))
    .def("GetInvalidValue", &viskores::filter::resampling::Probe::GetInvalidValue)
    .def(
      "SetFieldsToPass",
      [](viskores::filter::resampling::Probe& self, nb::object fieldsObject)
      { SetFieldsToPassForResampling(self, fieldsObject); },
      nb::arg("fields"))
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::resampling::Probe>,
         nb::arg("data"),
         doc::ExecuteFilter);

  erase_existing_name("HistSampling");
  nb::class_<viskores::filter::resampling::HistSampling>(
    m, "HistSampling", doc::ClassDoc("HistSampling"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::resampling::HistSampling& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::resampling::HistSampling::GetActiveFieldName)
    .def("GetActiveFieldAssociation",
         &viskores::filter::resampling::HistSampling::GetActiveFieldAssociation)
    .def(
      "SetNumberOfBins",
      [](viskores::filter::resampling::HistSampling& self, long long numberOfBins)
      { self.SetNumberOfBins(static_cast<viskores::Id>(numberOfBins)); },
      nb::arg("number_of_bins"))
    .def("GetNumberOfBins", &viskores::filter::resampling::HistSampling::GetNumberOfBins)
    .def(
      "SetSampleFraction",
      [](viskores::filter::resampling::HistSampling& self, double fraction)
      { self.SetSampleFraction(static_cast<viskores::FloatDefault>(fraction)); },
      nb::arg("fraction"))
    .def("GetSampleFraction", &viskores::filter::resampling::HistSampling::GetSampleFraction)
    .def(
      "SetSeed",
      [](viskores::filter::resampling::HistSampling& self, unsigned long seed)
      { self.SetSeed(static_cast<viskores::UInt32>(seed)); },
      nb::arg("seed"))
    .def("GetSeed", &viskores::filter::resampling::HistSampling::GetSeed)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::resampling::HistSampling>,
         nb::arg("data"),
         doc::ExecuteFilter);
}
#else
void RegisterNanobindResamplingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
