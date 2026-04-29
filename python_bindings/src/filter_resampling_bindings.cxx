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
  auto probe = BindClassWithDefaultConstructor<viskores::filter::resampling::Probe>(
    m, erase_existing_name, "Probe");
  probe
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
    .def("GetInvalidValue", &viskores::filter::resampling::Probe::GetInvalidValue);
  BindFilterFieldsToPassMethod<viskores::filter::resampling::Probe>(probe);
  BindFilterExecuteMethod<viskores::filter::resampling::Probe>(probe);

  auto histSampling = BindClassWithDefaultConstructor<viskores::filter::resampling::HistSampling>(
    m, erase_existing_name, "HistSampling");
  BindFilterActiveFieldMethods<viskores::filter::resampling::HistSampling>(histSampling);
  histSampling
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
    .def("GetSeed", &viskores::filter::resampling::HistSampling::GetSeed);
  BindFilterExecuteMethod<viskores::filter::resampling::HistSampling>(histSampling);
}
#else
void RegisterNanobindResamplingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
