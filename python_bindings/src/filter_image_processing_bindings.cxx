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

#if VISKORES_PYTHON_ENABLE_FILTER_IMAGE_PROCESSING
void RegisterNanobindImageProcessingClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("ComputeMoments");
  nb::class_<viskores::filter::image_processing::ComputeMoments>(
    m, "ComputeMoments", doc::ClassDoc("ComputeMoments"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::image_processing::ComputeMoments& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::image_processing::ComputeMoments::GetActiveFieldName)
    .def("GetActiveFieldAssociation",
         &viskores::filter::image_processing::ComputeMoments::GetActiveFieldAssociation)
    .def("SetOutputFieldName",
         &viskores::filter::image_processing::ComputeMoments::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::image_processing::ComputeMoments::GetOutputFieldName)
    .def("SetRadius",
         &viskores::filter::image_processing::ComputeMoments::SetRadius,
         nb::arg("radius"))
    .def(
      "SetSpacing",
      [](viskores::filter::image_processing::ComputeMoments& self, nb::handle spacingObject)
      { self.SetSpacing(ParseVec3(spacingObject, viskores::Vec3f(1.0f, 1.0f, 1.0f))); },
      nb::arg("spacing"))
    .def(
      "SetOrder", &viskores::filter::image_processing::ComputeMoments::SetOrder, nb::arg("order"))
    .def(
      "Execute",
      [](viskores::filter::image_processing::ComputeMoments& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ImageDifference");
  nb::class_<viskores::filter::image_processing::ImageDifference>(
    m, "ImageDifference", doc::ClassDoc("ImageDifference"))
    .def(nb::init<>())
    .def(
      "SetPrimaryField",
      [](viskores::filter::image_processing::ImageDifference& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetPrimaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetPrimaryFieldName",
         &viskores::filter::image_processing::ImageDifference::GetPrimaryFieldName)
    .def("GetPrimaryFieldAssociation",
         &viskores::filter::image_processing::ImageDifference::GetPrimaryFieldAssociation)
    .def(
      "SetSecondaryField",
      [](viskores::filter::image_processing::ImageDifference& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetSecondaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetSecondaryFieldName",
         &viskores::filter::image_processing::ImageDifference::GetSecondaryFieldName)
    .def("GetSecondaryFieldAssociation",
         &viskores::filter::image_processing::ImageDifference::GetSecondaryFieldAssociation)
    .def("SetOutputFieldName",
         &viskores::filter::image_processing::ImageDifference::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::image_processing::ImageDifference::GetOutputFieldName)
    .def("SetThresholdFieldName",
         &viskores::filter::image_processing::ImageDifference::SetThresholdFieldName,
         nb::arg("name"))
    .def("GetThresholdFieldName",
         &viskores::filter::image_processing::ImageDifference::GetThresholdFieldName)
    .def("SetAverageRadius",
         &viskores::filter::image_processing::ImageDifference::SetAverageRadius,
         nb::arg("average_radius"))
    .def("GetAverageRadius", &viskores::filter::image_processing::ImageDifference::GetAverageRadius)
    .def("SetPixelShiftRadius",
         &viskores::filter::image_processing::ImageDifference::SetPixelShiftRadius,
         nb::arg("pixel_shift_radius"))
    .def("GetPixelShiftRadius",
         &viskores::filter::image_processing::ImageDifference::GetPixelShiftRadius)
    .def("SetAllowedPixelErrorRatio",
         &viskores::filter::image_processing::ImageDifference::SetAllowedPixelErrorRatio,
         nb::arg("pixel_error_ratio"))
    .def("GetAllowedPixelErrorRatio",
         &viskores::filter::image_processing::ImageDifference::GetAllowedPixelErrorRatio)
    .def("SetPixelDiffThreshold",
         &viskores::filter::image_processing::ImageDifference::SetPixelDiffThreshold,
         nb::arg("threshold"))
    .def("GetPixelDiffThreshold",
         &viskores::filter::image_processing::ImageDifference::GetPixelDiffThreshold)
    .def("GetImageDiffWithinThreshold",
         &viskores::filter::image_processing::ImageDifference::GetImageDiffWithinThreshold)
    .def(
      "Execute",
      [](viskores::filter::image_processing::ImageDifference& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ImageMedian");
  nb::class_<viskores::filter::image_processing::ImageMedian>(
    m, "ImageMedian", doc::ClassDoc("ImageMedian"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::image_processing::ImageMedian& self,
         const char* name,
         nb::handle associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::image_processing::ImageMedian::GetActiveFieldName)
    .def("GetActiveFieldAssociation",
         &viskores::filter::image_processing::ImageMedian::GetActiveFieldAssociation)
    .def("SetOutputFieldName", &viskores::filter::image_processing::ImageMedian::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::image_processing::ImageMedian::GetOutputFieldName)
    .def("Perform3x3", &viskores::filter::image_processing::ImageMedian::Perform3x3)
    .def("Perform5x5", &viskores::filter::image_processing::ImageMedian::Perform5x5)
    .def(
      "Execute",
      [](viskores::filter::image_processing::ImageMedian& self, nb::handle dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);
}
#else
void RegisterNanobindImageProcessingClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
