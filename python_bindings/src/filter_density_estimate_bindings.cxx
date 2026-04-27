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

#if VISKORES_PYTHON_ENABLE_FILTER_DENSITY_ESTIMATE
void RegisterNanobindDensityEstimateClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("ContinuousScatterPlot");
  nb::class_<viskores::filter::density_estimate::ContinuousScatterPlot>(
    m, "ContinuousScatterPlot", doc::ClassDoc("ContinuousScatterPlot"))
    .def(nb::init<>())
    .def(
      "SetActiveFieldsPair",
      [](viskores::filter::density_estimate::ContinuousScatterPlot& self,
         const char* fieldName1,
         const char* fieldName2) { self.SetActiveFieldsPair(fieldName1, fieldName2); },
      nb::arg("field_name_1"),
      nb::arg("field_name_2"))
    .def("SetOutputFieldName",
         &viskores::filter::density_estimate::ContinuousScatterPlot::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::density_estimate::ContinuousScatterPlot::GetOutputFieldName)
    .def(
      "Execute",
      [](viskores::filter::density_estimate::ContinuousScatterPlot& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Entropy");
  nb::class_<viskores::filter::density_estimate::Entropy>(m, "Entropy", doc::ClassDoc("Entropy"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::density_estimate::Entropy& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::density_estimate::Entropy::GetActiveFieldName)
    .def("GetActiveFieldAssociation",
         &viskores::filter::density_estimate::Entropy::GetActiveFieldAssociation)
    .def("SetOutputFieldName", &viskores::filter::density_estimate::Entropy::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::density_estimate::Entropy::GetOutputFieldName)
    .def("SetNumberOfBins", &viskores::filter::density_estimate::Entropy::SetNumberOfBins)
    .def("GetNumberOfBins", &viskores::filter::density_estimate::Entropy::GetNumberOfBins)
    .def(
      "Execute",
      [](viskores::filter::density_estimate::Entropy& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Histogram");
  nb::class_<viskores::filter::density_estimate::Histogram>(
    m, "Histogram", doc::ClassDoc("Histogram"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::density_estimate::Histogram& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::density_estimate::Histogram::GetActiveFieldName)
    .def("GetActiveFieldAssociation",
         &viskores::filter::density_estimate::Histogram::GetActiveFieldAssociation)
    .def("SetOutputFieldName", &viskores::filter::density_estimate::Histogram::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::density_estimate::Histogram::GetOutputFieldName)
    .def("SetNumberOfBins", &viskores::filter::density_estimate::Histogram::SetNumberOfBins)
    .def("GetNumberOfBins", &viskores::filter::density_estimate::Histogram::GetNumberOfBins)
    .def("SetRange", &viskores::filter::density_estimate::Histogram::SetRange, nb::arg("range"))
    .def("GetRange", &viskores::filter::density_estimate::Histogram::GetRange)
    .def("GetBinDelta", &viskores::filter::density_estimate::Histogram::GetBinDelta)
    .def("GetComputedRange", &viskores::filter::density_estimate::Histogram::GetComputedRange)
    .def(
      "Execute",
      [](viskores::filter::density_estimate::Histogram& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("NDEntropy");
  nb::class_<viskores::filter::density_estimate::NDEntropy>(
    m, "NDEntropy", doc::ClassDoc("NDEntropy"))
    .def(nb::init<>())
    .def(
      "AddFieldAndBin",
      [](viskores::filter::density_estimate::NDEntropy& self,
         const char* fieldName,
         viskores::Id numBins) { self.AddFieldAndBin(fieldName, numBins); },
      nb::arg("field_name"),
      nb::arg("num_bins"))
    .def(
      "Execute",
      [](viskores::filter::density_estimate::NDEntropy& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("NDHistogram");
  nb::class_<viskores::filter::density_estimate::NDHistogram>(
    m, "NDHistogram", doc::ClassDoc("NDHistogram"))
    .def(nb::init<>())
    .def(
      "AddFieldAndBin",
      [](viskores::filter::density_estimate::NDHistogram& self,
         const char* fieldName,
         viskores::Id numBins) { self.AddFieldAndBin(fieldName, numBins); },
      nb::arg("field_name"),
      nb::arg("num_bins"))
    .def("GetBinDelta",
         &viskores::filter::density_estimate::NDHistogram::GetBinDelta,
         nb::arg("field_index"))
    .def("GetDataRange",
         &viskores::filter::density_estimate::NDHistogram::GetDataRange,
         nb::arg("field_index"))
    .def(
      "Execute",
      [](viskores::filter::density_estimate::NDHistogram& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Statistics");
  nb::class_<viskores::filter::density_estimate::Statistics>(
    m, "Statistics", doc::ClassDoc("Statistics"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::density_estimate::Statistics& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::density_estimate::Statistics::GetActiveFieldName)
    .def("GetActiveFieldAssociation",
         &viskores::filter::density_estimate::Statistics::GetActiveFieldAssociation)
    .def(
      "Execute",
      [](viskores::filter::density_estimate::Statistics& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ParticleDensityNearestGridPoint");
  nb::class_<viskores::filter::density_estimate::ParticleDensityNearestGridPoint>(
    m, "ParticleDensityNearestGridPoint", doc::ClassDoc("ParticleDensityNearestGridPoint"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::GetOutputFieldName)
    .def(
      "SetComputeNumberDensity",
      &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::SetComputeNumberDensity)
    .def(
      "GetComputeNumberDensity",
      &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::GetComputeNumberDensity)
    .def("SetDivideByVolume",
         &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::SetDivideByVolume)
    .def("GetDivideByVolume",
         &viskores::filter::density_estimate::ParticleDensityNearestGridPoint::GetDivideByVolume)
    .def(
      "SetDimension",
      [](viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self,
         nb::handle dimensionObject) { self.SetDimension(ParseDimensions(dimensionObject)); },
      nb::arg("dimension"))
    .def("GetDimension",
         [](const viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self)
         {
           auto dimension = self.GetDimension();
           return nb::make_tuple(dimension[0], dimension[1], dimension[2]);
         })
    .def(
      "SetOrigin",
      [](viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self,
         nb::handle originObject) { self.SetOrigin(ParseVec3(originObject, self.GetOrigin())); },
      nb::arg("origin"))
    .def("GetOrigin",
         [](const viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self)
         {
           auto origin = self.GetOrigin();
           return nb::make_tuple(origin[0], origin[1], origin[2]);
         })
    .def(
      "SetSpacing",
      [](viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self,
         nb::handle spacingObject)
      { self.SetSpacing(ParseVec3(spacingObject, self.GetSpacing())); },
      nb::arg("spacing"))
    .def("GetSpacing",
         [](const viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self)
         {
           auto spacing = self.GetSpacing();
           return nb::make_tuple(spacing[0], spacing[1], spacing[2]);
         })
    .def(
      "SetBounds",
      [](viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self,
         nb::handle boundsObject)
      {
        const auto boundsTuple = nb::cast<nb::tuple>(boundsObject);
        if (boundsTuple.size() != 6)
        {
          throw std::runtime_error("bounds must have six values");
        }
        self.SetBounds(viskores::Bounds(nb::cast<viskores::Float64>(boundsTuple[0]),
                                        nb::cast<viskores::Float64>(boundsTuple[1]),
                                        nb::cast<viskores::Float64>(boundsTuple[2]),
                                        nb::cast<viskores::Float64>(boundsTuple[3]),
                                        nb::cast<viskores::Float64>(boundsTuple[4]),
                                        nb::cast<viskores::Float64>(boundsTuple[5])));
      },
      nb::arg("bounds"))
    .def("GetBounds",
         [](const viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self)
         {
           const auto bounds = self.GetBounds();
           return nb::make_tuple(
             bounds.X.Min, bounds.X.Max, bounds.Y.Min, bounds.Y.Max, bounds.Z.Min, bounds.Z.Max);
         })
    .def(
      "Execute",
      [](viskores::filter::density_estimate::ParticleDensityNearestGridPoint& self,
         nb::handle dataObject) { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ParticleDensityCloudInCell");
  nb::class_<viskores::filter::density_estimate::ParticleDensityCloudInCell>(
    m, "ParticleDensityCloudInCell", doc::ClassDoc("ParticleDensityCloudInCell"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::density_estimate::ParticleDensityCloudInCell& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::GetOutputFieldName)
    .def("SetComputeNumberDensity",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::SetComputeNumberDensity)
    .def("GetComputeNumberDensity",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::GetComputeNumberDensity)
    .def("SetDivideByVolume",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::SetDivideByVolume)
    .def("GetDivideByVolume",
         &viskores::filter::density_estimate::ParticleDensityCloudInCell::GetDivideByVolume)
    .def(
      "SetDimension",
      [](viskores::filter::density_estimate::ParticleDensityCloudInCell& self,
         nb::handle dimensionObject) { self.SetDimension(ParseDimensions(dimensionObject)); },
      nb::arg("dimension"))
    .def("GetDimension",
         [](const viskores::filter::density_estimate::ParticleDensityCloudInCell& self)
         {
           auto dimension = self.GetDimension();
           return nb::make_tuple(dimension[0], dimension[1], dimension[2]);
         })
    .def(
      "SetOrigin",
      [](viskores::filter::density_estimate::ParticleDensityCloudInCell& self,
         nb::handle originObject) { self.SetOrigin(ParseVec3(originObject, self.GetOrigin())); },
      nb::arg("origin"))
    .def("GetOrigin",
         [](const viskores::filter::density_estimate::ParticleDensityCloudInCell& self)
         {
           auto origin = self.GetOrigin();
           return nb::make_tuple(origin[0], origin[1], origin[2]);
         })
    .def(
      "SetSpacing",
      [](viskores::filter::density_estimate::ParticleDensityCloudInCell& self,
         nb::handle spacingObject)
      { self.SetSpacing(ParseVec3(spacingObject, self.GetSpacing())); },
      nb::arg("spacing"))
    .def("GetSpacing",
         [](const viskores::filter::density_estimate::ParticleDensityCloudInCell& self)
         {
           auto spacing = self.GetSpacing();
           return nb::make_tuple(spacing[0], spacing[1], spacing[2]);
         })
    .def(
      "SetBounds",
      [](viskores::filter::density_estimate::ParticleDensityCloudInCell& self,
         nb::handle boundsObject)
      {
        const auto boundsTuple = nb::cast<nb::tuple>(boundsObject);
        if (boundsTuple.size() != 6)
        {
          throw std::runtime_error("bounds must have six values");
        }
        self.SetBounds(viskores::Bounds(nb::cast<viskores::Float64>(boundsTuple[0]),
                                        nb::cast<viskores::Float64>(boundsTuple[1]),
                                        nb::cast<viskores::Float64>(boundsTuple[2]),
                                        nb::cast<viskores::Float64>(boundsTuple[3]),
                                        nb::cast<viskores::Float64>(boundsTuple[4]),
                                        nb::cast<viskores::Float64>(boundsTuple[5])));
      },
      nb::arg("bounds"))
    .def("GetBounds",
         [](const viskores::filter::density_estimate::ParticleDensityCloudInCell& self)
         {
           const auto bounds = self.GetBounds();
           return nb::make_tuple(
             bounds.X.Min, bounds.X.Max, bounds.Y.Min, bounds.Y.Max, bounds.Z.Min, bounds.Z.Max);
         })
    .def(
      "Execute",
      [](viskores::filter::density_estimate::ParticleDensityCloudInCell& self,
         nb::handle dataObject) { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);
}
#else
void RegisterNanobindDensityEstimateClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
