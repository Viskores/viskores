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

template <typename FilterType>
void SetSelectedFields(FilterType& self, nb::object fieldsObject)
{
  auto& selection = self.GetFieldsToPass();
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

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT
void RegisterNanobindGeometryRefinementClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Tetrahedralize");
  nb::class_<viskores::filter::geometry_refinement::Tetrahedralize>(
    m, "Tetrahedralize", doc::ClassDoc("Tetrahedralize"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::geometry_refinement::Tetrahedralize& self, nb::object fieldsObject)
      { SetSelectedFields(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::Tetrahedralize& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Triangulate");
  nb::class_<viskores::filter::geometry_refinement::Triangulate>(
    m, "Triangulate", doc::ClassDoc("Triangulate"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::geometry_refinement::Triangulate& self, nb::object fieldsObject)
      { SetSelectedFields(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::Triangulate& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Shrink");
  nb::class_<viskores::filter::geometry_refinement::Shrink>(m, "Shrink", doc::ClassDoc("Shrink"))
    .def(nb::init<>())
    .def("SetShrinkFactor", &viskores::filter::geometry_refinement::Shrink::SetShrinkFactor)
    .def("GetShrinkFactor", &viskores::filter::geometry_refinement::Shrink::GetShrinkFactor)
    .def(
      "SetFieldsToPass",
      [](viskores::filter::geometry_refinement::Shrink& self, nb::object fieldsObject)
      { SetSelectedFields(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::Shrink& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ConvertToPointCloud");
  nb::class_<viskores::filter::geometry_refinement::ConvertToPointCloud>(
    m, "ConvertToPointCloud", doc::ClassDoc("ConvertToPointCloud"))
    .def(nb::init<>())
    .def("SetAssociateFieldsWithCells",
         &viskores::filter::geometry_refinement::ConvertToPointCloud::SetAssociateFieldsWithCells)
    .def("GetAssociateFieldsWithCells",
         &viskores::filter::geometry_refinement::ConvertToPointCloud::GetAssociateFieldsWithCells)
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::ConvertToPointCloud& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("VertexClustering");
  nb::class_<viskores::filter::geometry_refinement::VertexClustering>(
    m, "VertexClustering", doc::ClassDoc("VertexClustering"))
    .def(nb::init<>())
    .def(
      "SetNumberOfDivisions",
      [](viskores::filter::geometry_refinement::VertexClustering& self, nb::object divisions)
      { self.SetNumberOfDivisions(ParseDimensions(divisions)); },
      nb::arg("divisions"))
    .def("GetNumberOfDivisions",
         [](const viskores::filter::geometry_refinement::VertexClustering& self)
         {
           const auto divisions = self.GetNumberOfDivisions();
           return nb::make_tuple(divisions[0], divisions[1], divisions[2]);
         })
    .def(
      "SetFieldsToPass",
      [](viskores::filter::geometry_refinement::VertexClustering& self, nb::object fieldsObject)
      { SetSelectedFields(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::VertexClustering& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("SplitSharpEdges");
  nb::class_<viskores::filter::geometry_refinement::SplitSharpEdges>(
    m, "SplitSharpEdges", doc::ClassDoc("SplitSharpEdges"))
    .def(nb::init<>())
    .def("SetFeatureAngle",
         &viskores::filter::geometry_refinement::SplitSharpEdges::SetFeatureAngle)
    .def("GetFeatureAngle",
         &viskores::filter::geometry_refinement::SplitSharpEdges::GetFeatureAngle)
    .def(
      "SetActiveField",
      [](viskores::filter::geometry_refinement::SplitSharpEdges& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::SplitSharpEdges& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Tube");
  nb::class_<viskores::filter::geometry_refinement::Tube>(m, "Tube", doc::ClassDoc("Tube"))
    .def(nb::init<>())
    .def("SetRadius", &viskores::filter::geometry_refinement::Tube::SetRadius, nb::arg("radius"))
    .def(
      "SetNumberOfSides",
      [](viskores::filter::geometry_refinement::Tube& self, long long value)
      { self.SetNumberOfSides(static_cast<viskores::Id>(value)); },
      nb::arg("number_of_sides"))
    .def("SetCapping", &viskores::filter::geometry_refinement::Tube::SetCapping, nb::arg("enabled"))
    .def(
      "Execute",
      [](viskores::filter::geometry_refinement::Tube& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);
}
#else
void RegisterNanobindGeometryRefinementClasses(nb::module_&,
                                               const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
