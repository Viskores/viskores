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

#if VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT
void RegisterNanobindGeometryRefinementClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  auto vertexClustering =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::VertexClustering>(
      m, erase_existing_name, "VertexClustering");
  vertexClustering
    .def(
      "SetNumberOfDivisions",
      [](viskores::filter::geometry_refinement::VertexClustering& self, nb::object divisions)
      { self.SetNumberOfDivisions(ParseDimensions(divisions)); },
      nb::arg("divisions"))
    .def("GetNumberOfDivisions",
         [](const viskores::filter::geometry_refinement::VertexClustering& self)
         { return Vec3ToTuple(self.GetNumberOfDivisions()); });
  BindFilterFieldsToPassMethod<viskores::filter::geometry_refinement::VertexClustering>(
    vertexClustering);
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::VertexClustering>(
    vertexClustering);
}
#else
void RegisterNanobindGeometryRefinementClasses(nb::module_&,
                                               const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
