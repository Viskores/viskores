//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace viskores::python::bindings
{

namespace
{

template <typename MakeField>
void RegisterSimpleFieldFactory(nb::module_& m,
                                const std::function<void(const char*)>& erase_existing_name,
                                const char* name,
                                MakeField makeField,
                                const char* docString)
{
  erase_existing_name(name);
  m.attr(name) = nb::cpp_function(
    [makeField](const std::string& fieldName, nb::object values)
    { return makeField(fieldName, PythonObjectToUnknownArray(values)); },
    nb::arg("name"),
    nb::arg("values"),
    docString);
}

} // namespace

void RegisterNanobindCellSetClass(nb::module_& m,
                                  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("UnknownCellSet");
  nb::class_<viskores::cont::UnknownCellSet>(m, "UnknownCellSet", doc::ClassDoc("UnknownCellSet"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::UnknownCellSet& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.UnknownCellSet(type=\"" << self.GetCellSetName()
                  << "\", points=" << self.GetNumberOfPoints()
                  << ", cells=" << self.GetNumberOfCells() << ")";
           return stream.str();
         })
    .def("IsValid", &viskores::cont::UnknownCellSet::IsValid)
    .def("GetCellSetName", &viskores::cont::UnknownCellSet::GetCellSetName)
    .def("GetNumberOfCells", &viskores::cont::UnknownCellSet::GetNumberOfCells)
    .def("GetNumberOfFaces", &viskores::cont::UnknownCellSet::GetNumberOfFaces)
    .def("GetNumberOfEdges", &viskores::cont::UnknownCellSet::GetNumberOfEdges)
    .def("GetNumberOfPoints", &viskores::cont::UnknownCellSet::GetNumberOfPoints)
    .def("GetCellShape", &viskores::cont::UnknownCellSet::GetCellShape, nb::arg("cell_id"))
    .def("GetNumberOfPointsInCell",
         &viskores::cont::UnknownCellSet::GetNumberOfPointsInCell,
         nb::arg("cell_id"))
    .def(
      "GetCellPointIds",
      [](const viskores::cont::UnknownCellSet& self, viskores::Id cellId)
      {
        const auto count = self.GetNumberOfPointsInCell(cellId);
        std::vector<viskores::Id> pointIds(static_cast<std::size_t>(count));
        self.GetCellPointIds(cellId, pointIds.data());
        return nb::cast(pointIds);
      },
      nb::arg("cell_id"));
}

void RegisterNanobindFieldFactoryFunctions(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("make_Field");
  m.attr("make_Field") = nb::cpp_function(
    [](const std::string& name, viskores::cont::Field::Association association, nb::object values)
    { return viskores::cont::Field(name, association, PythonObjectToUnknownArray(values)); },
    nb::arg("name"),
    nb::arg("association"),
    nb::arg("values"),
    doc::MakeField);

  RegisterSimpleFieldFactory(
    m,
    erase_existing_name,
    "make_FieldPoint",
    [](const std::string& name, const viskores::cont::UnknownArrayHandle& values)
    { return viskores::cont::make_FieldPoint(name, values); },
    doc::MakeFieldPoint);
  RegisterSimpleFieldFactory(
    m,
    erase_existing_name,
    "make_FieldCell",
    [](const std::string& name, const viskores::cont::UnknownArrayHandle& values)
    { return viskores::cont::make_FieldCell(name, values); },
    doc::MakeFieldCell);
}

} // namespace viskores::python::bindings
