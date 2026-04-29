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
  auto tetrahedralize =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::Tetrahedralize>(
      m, erase_existing_name, "Tetrahedralize");
  BindFilterFieldsToPassMethod<viskores::filter::geometry_refinement::Tetrahedralize>(
    tetrahedralize);
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::Tetrahedralize>(tetrahedralize);

  auto triangulate =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::Triangulate>(
      m, erase_existing_name, "Triangulate");
  BindFilterFieldsToPassMethod<viskores::filter::geometry_refinement::Triangulate>(triangulate);
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::Triangulate>(triangulate);

  auto shrink =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::Shrink>(
      m, erase_existing_name, "Shrink");
  shrink.def("SetShrinkFactor", &viskores::filter::geometry_refinement::Shrink::SetShrinkFactor)
    .def("GetShrinkFactor", &viskores::filter::geometry_refinement::Shrink::GetShrinkFactor);
  BindFilterFieldsToPassMethod<viskores::filter::geometry_refinement::Shrink>(shrink);
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::Shrink>(shrink);

  auto convertToPointCloud =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::ConvertToPointCloud>(
      m, erase_existing_name, "ConvertToPointCloud");
  convertToPointCloud
    .def("SetAssociateFieldsWithCells",
         &viskores::filter::geometry_refinement::ConvertToPointCloud::SetAssociateFieldsWithCells)
    .def("GetAssociateFieldsWithCells",
         &viskores::filter::geometry_refinement::ConvertToPointCloud::GetAssociateFieldsWithCells);
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::ConvertToPointCloud>(
    convertToPointCloud);

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

  auto splitSharpEdges =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::SplitSharpEdges>(
      m, erase_existing_name, "SplitSharpEdges");
  splitSharpEdges
    .def("SetFeatureAngle",
         &viskores::filter::geometry_refinement::SplitSharpEdges::SetFeatureAngle)
    .def("GetFeatureAngle",
         &viskores::filter::geometry_refinement::SplitSharpEdges::GetFeatureAngle);
  BindFilterActiveFieldMethods<viskores::filter::geometry_refinement::SplitSharpEdges>(
    splitSharpEdges);
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::SplitSharpEdges>(splitSharpEdges);

  auto tube =
    BindClassWithDefaultConstructor<viskores::filter::geometry_refinement::Tube>(
      m, erase_existing_name, "Tube");
  tube.def("SetRadius", &viskores::filter::geometry_refinement::Tube::SetRadius, nb::arg("radius"))
    .def(
      "SetNumberOfSides",
      [](viskores::filter::geometry_refinement::Tube& self, long long value)
      { self.SetNumberOfSides(static_cast<viskores::Id>(value)); },
      nb::arg("number_of_sides"))
    .def("SetCapping",
         &viskores::filter::geometry_refinement::Tube::SetCapping,
         nb::arg("enabled"));
  BindFilterExecuteMethod<viskores::filter::geometry_refinement::Tube>(tube);
}
#else
void RegisterNanobindGeometryRefinementClasses(nb::module_&,
                                               const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
