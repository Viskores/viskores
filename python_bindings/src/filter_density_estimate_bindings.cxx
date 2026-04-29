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

namespace density = viskores::filter::density_estimate;

namespace
{

template <typename FilterType, typename ClassType>
ClassType& BindDensityActiveFieldMethods(ClassType& cls)
{
  BindFilterActiveFieldMethods<FilterType>(cls);
  BindFilterActiveFieldAssociationMethod<FilterType>(cls);
  return cls;
}

template <typename FilterType, typename ClassType>
ClassType& BindParticleDensityGridMethods(ClassType& cls)
{
  cls
    .def("SetComputeNumberDensity", &FilterType::SetComputeNumberDensity)
    .def("GetComputeNumberDensity", &FilterType::GetComputeNumberDensity)
    .def("SetDivideByVolume", &FilterType::SetDivideByVolume)
    .def("GetDivideByVolume", &FilterType::GetDivideByVolume)
    .def(
      "SetBounds",
      [](FilterType& self, nb::handle boundsObject)
      { self.SetBounds(ParseBounds(boundsObject)); },
      nb::arg("bounds"))
    .def("GetBounds",
         [](const FilterType& self)
         { return BoundsToTuple(self.GetBounds()); });
  BindId3Property<FilterType>(
    cls, "SetDimension", "GetDimension", &FilterType::SetDimension, &FilterType::GetDimension);
  BindVec3Property<FilterType>(
    cls, "SetOrigin", "GetOrigin", &FilterType::SetOrigin, &FilterType::GetOrigin, "origin");
  BindVec3Property<FilterType>(
    cls, "SetSpacing", "GetSpacing", &FilterType::SetSpacing, &FilterType::GetSpacing, "spacing");
  return cls;
}

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_DENSITY_ESTIMATE
void RegisterNanobindDensityEstimateClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  auto continuousScatterPlot =
    BindClassWithDefaultConstructor<density::ContinuousScatterPlot>(
      m, erase_existing_name, "ContinuousScatterPlot");
  continuousScatterPlot
    .def(
      "SetActiveFieldsPair",
      [](density::ContinuousScatterPlot& self,
         const char* fieldName1,
         const char* fieldName2) { self.SetActiveFieldsPair(fieldName1, fieldName2); },
      nb::arg("field_name_1"),
      nb::arg("field_name_2"));
  BindFilterOutputFieldMethods<density::ContinuousScatterPlot>(
    continuousScatterPlot);
  BindFilterExecuteMethod<density::ContinuousScatterPlot>(
    continuousScatterPlot);

  auto entropy =
    BindClassWithDefaultConstructor<density::Entropy>(
      m, erase_existing_name, "Entropy");
  BindDensityActiveFieldMethods<density::Entropy>(entropy);
  BindFilterOutputFieldMethods<density::Entropy>(entropy);
  entropy
    .def("SetNumberOfBins", &density::Entropy::SetNumberOfBins)
    .def("GetNumberOfBins", &density::Entropy::GetNumberOfBins);
  BindFilterExecuteMethod<density::Entropy>(entropy);

  auto histogram =
    BindClassWithDefaultConstructor<density::Histogram>(
      m, erase_existing_name, "Histogram");
  BindDensityActiveFieldMethods<density::Histogram>(histogram);
  BindFilterOutputFieldMethods<density::Histogram>(histogram);
  histogram
    .def("SetNumberOfBins", &density::Histogram::SetNumberOfBins)
    .def("GetNumberOfBins", &density::Histogram::GetNumberOfBins)
    .def("SetRange", &density::Histogram::SetRange, nb::arg("range"))
    .def("GetRange", &density::Histogram::GetRange)
    .def("GetBinDelta", &density::Histogram::GetBinDelta)
    .def("GetComputedRange", &density::Histogram::GetComputedRange);
  BindFilterExecuteMethod<density::Histogram>(histogram);

  auto ndEntropy =
    BindClassWithDefaultConstructor<density::NDEntropy>(
      m, erase_existing_name, "NDEntropy");
  ndEntropy
    .def(
      "AddFieldAndBin",
      [](density::NDEntropy& self,
         const char* fieldName,
         viskores::Id numBins) { self.AddFieldAndBin(fieldName, numBins); },
      nb::arg("field_name"),
      nb::arg("num_bins"));
  BindFilterExecuteMethod<density::NDEntropy>(ndEntropy);

  auto ndHistogram =
    BindClassWithDefaultConstructor<density::NDHistogram>(
      m, erase_existing_name, "NDHistogram");
  ndHistogram
    .def(
      "AddFieldAndBin",
      [](density::NDHistogram& self,
         const char* fieldName,
         viskores::Id numBins) { self.AddFieldAndBin(fieldName, numBins); },
      nb::arg("field_name"),
      nb::arg("num_bins"))
    .def("GetBinDelta",
         &density::NDHistogram::GetBinDelta,
         nb::arg("field_index"))
    .def("GetDataRange",
         &density::NDHistogram::GetDataRange,
         nb::arg("field_index"));
  BindFilterExecuteMethod<density::NDHistogram>(ndHistogram);

  auto statistics =
    BindClassWithDefaultConstructor<density::Statistics>(
      m, erase_existing_name, "Statistics");
  BindDensityActiveFieldMethods<density::Statistics>(statistics);
  BindFilterExecuteMethod<density::Statistics>(statistics);

  auto particleDensityNearestGridPoint =
    BindClassWithDefaultConstructor<
      density::ParticleDensityNearestGridPoint>(
      m, erase_existing_name, "ParticleDensityNearestGridPoint");
  BindFilterActiveFieldMethods<
    density::ParticleDensityNearestGridPoint>(
    particleDensityNearestGridPoint);
  BindFilterOutputFieldMethods<
    density::ParticleDensityNearestGridPoint>(
    particleDensityNearestGridPoint);
  BindParticleDensityGridMethods<
    density::ParticleDensityNearestGridPoint>(
    particleDensityNearestGridPoint);
  BindFilterExecuteMethod<density::ParticleDensityNearestGridPoint>(
    particleDensityNearestGridPoint);

  auto particleDensityCloudInCell =
    BindClassWithDefaultConstructor<density::ParticleDensityCloudInCell>(
      m, erase_existing_name, "ParticleDensityCloudInCell");
  BindFilterActiveFieldMethods<density::ParticleDensityCloudInCell>(
    particleDensityCloudInCell);
  BindFilterOutputFieldMethods<density::ParticleDensityCloudInCell>(
    particleDensityCloudInCell);
  BindParticleDensityGridMethods<density::ParticleDensityCloudInCell>(
    particleDensityCloudInCell);
  BindFilterExecuteMethod<density::ParticleDensityCloudInCell>(
    particleDensityCloudInCell);
}
#else
void RegisterNanobindDensityEstimateClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
