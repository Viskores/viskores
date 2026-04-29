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
}
#else
void RegisterNanobindImageProcessingClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
