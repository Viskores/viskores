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
  auto computeMoments =
    BindClassWithDefaultConstructor<viskores::filter::image_processing::ComputeMoments>(
      m, erase_existing_name, "ComputeMoments");
  BindFilterActiveFieldMethods<viskores::filter::image_processing::ComputeMoments>(
    computeMoments);
  BindFilterActiveFieldAssociationMethod<viskores::filter::image_processing::ComputeMoments>(
    computeMoments);
  BindFilterOutputFieldMethods<viskores::filter::image_processing::ComputeMoments>(
    computeMoments);
  computeMoments
    .def("SetRadius",
         &viskores::filter::image_processing::ComputeMoments::SetRadius,
         nb::arg("radius"))
    .def(
      "SetSpacing",
      [](viskores::filter::image_processing::ComputeMoments& self, nb::handle spacingObject)
      { self.SetSpacing(ParseVec3(spacingObject, viskores::Vec3f(1.0f, 1.0f, 1.0f))); },
      nb::arg("spacing"))
    .def(
      "SetOrder", &viskores::filter::image_processing::ComputeMoments::SetOrder, nb::arg("order"));
  BindFilterExecuteMethod<viskores::filter::image_processing::ComputeMoments>(computeMoments);

  auto imageDifference =
    BindClassWithDefaultConstructor<viskores::filter::image_processing::ImageDifference>(
      m, erase_existing_name, "ImageDifference");
  BindFilterPrimaryFieldMethods<viskores::filter::image_processing::ImageDifference>(
    imageDifference);
  BindFilterSecondaryFieldMethods<viskores::filter::image_processing::ImageDifference>(
    imageDifference);
  BindFilterOutputFieldMethods<viskores::filter::image_processing::ImageDifference>(
    imageDifference);
  imageDifference
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
         &viskores::filter::image_processing::ImageDifference::GetImageDiffWithinThreshold);
  BindFilterExecuteMethod<viskores::filter::image_processing::ImageDifference>(imageDifference);

  auto imageMedian =
    BindClassWithDefaultConstructor<viskores::filter::image_processing::ImageMedian>(
      m, erase_existing_name, "ImageMedian");
  BindFilterActiveFieldMethods<viskores::filter::image_processing::ImageMedian>(imageMedian);
  BindFilterActiveFieldAssociationMethod<viskores::filter::image_processing::ImageMedian>(
    imageMedian);
  BindFilterOutputFieldMethods<viskores::filter::image_processing::ImageMedian>(imageMedian);
  imageMedian
    .def("Perform3x3", &viskores::filter::image_processing::ImageMedian::Perform3x3)
    .def("Perform5x5", &viskores::filter::image_processing::ImageMedian::Perform5x5);
  BindFilterExecuteMethod<viskores::filter::image_processing::ImageMedian>(imageMedian);
}
#else
void RegisterNanobindImageProcessingClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
