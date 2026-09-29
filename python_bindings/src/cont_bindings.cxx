//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "viskores_python_bindings.h"

#include <viskores/interop/python/ArrayHandleToNumPy.h>
#include <viskores/interop/python/NumPyToArrayHandle.h>

#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <string>
#include <vector>

namespace vip = viskores::interop::python;

namespace viskores::python::bindings
{
namespace
{

std::string UnknownArrayHandleRepr(const viskores::cont::UnknownArrayHandle& self)
{
  return "viskores.cont.UnknownArrayHandle(values=" +
    std::to_string(self.GetNumberOfValues()) +
    ", components=" + std::to_string(self.GetNumberOfComponentsFlat()) + ")";
}

} // namespace

void BindCont(nb::module_& m)
{
  nb::class_<viskores::cont::UnknownArrayHandle>(m, "UnknownArrayHandle")
    .def(nb::init<>())
    .def("__len__", &viskores::cont::UnknownArrayHandle::GetNumberOfValues)
    .def("__repr__", &UnknownArrayHandleRepr)
    .def("GetNumberOfValues",
         &viskores::cont::UnknownArrayHandle::GetNumberOfValues,
         "Number of elements (tuples) in the array.")
    .def("GetNumberOfComponentsFlat",
         &viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat,
         "Number of scalar components per element, counting nested Vec components.")
    .def("NewInstanceBasic",
         &viskores::cont::UnknownArrayHandle::NewInstanceBasic,
         "Allocate a new Viskores-managed array of the same value type.")
    .def("DeepCopyFrom",
         [](viskores::cont::UnknownArrayHandle& self,
            const viskores::cont::UnknownArrayHandle& source) { self.DeepCopyFrom(source); },
         nb::arg("source"),
         "Deep copy data from source into this array, allocating as needed.")
    .def("asnumpy",
         &vip::ArrayHandleToNumPy,
         "Return a read-only NumPy view of this array. Zero-copy for basic and\n"
         "runtime-vec storage; other layouts are copied element-by-element.\n"
         "The view keeps the source array alive for its entire lifetime.");

  m.def("array_from_numpy",
        &vip::NumPyToArrayHandle,
        nb::arg("array"),
        nb::arg("allow_copy") = false,
        "Wrap a NumPy array as a Viskores UnknownArrayHandle. By default the\n"
        "binding shares storage with the NumPy buffer; this requires the input\n"
        "to be C-contiguous, aligned, and writable. Pass allow_copy=True to\n"
        "let the binding make a contiguous, writable copy when the input\n"
        "layout is not directly shareable. allow_copy is permission, not a\n"
        "command: a directly-shareable input still takes the zero-copy path.");
  m.def("asnumpy",
        &vip::ArrayHandleToNumPy,
        nb::arg("array"),
        "Return a read-only NumPy view of a Viskores UnknownArrayHandle.\n"
        "Equivalent to array.asnumpy().");

  // Field::Association enum
  nb::enum_<viskores::cont::Field::Association>(m, "FieldAssociation")
    .value("Any", viskores::cont::Field::Association::Any)
    .value("WholeDataSet", viskores::cont::Field::Association::WholeDataSet)
    .value("Points", viskores::cont::Field::Association::Points)
    .value("Cells", viskores::cont::Field::Association::Cells)
    .value("Partitions", viskores::cont::Field::Association::Partitions)
    .value("Global", viskores::cont::Field::Association::Global);

  nb::class_<viskores::cont::Field>(m, "Field")
    .def(nb::init<std::string, viskores::cont::Field::Association,
                  const viskores::cont::UnknownArrayHandle&>(),
         nb::arg("name"),
         nb::arg("association"),
         nb::arg("data"),
         "Construct a Field from a name, association, and UnknownArrayHandle.")
    .def("GetName", &viskores::cont::Field::GetName, "Field name.")
    .def("GetAssociation",
         &viskores::cont::Field::GetAssociation,
         "Field association (Points, Cells, etc.).")
    .def("GetNumberOfValues",
         &viskores::cont::Field::GetNumberOfValues,
         "Number of tuples in the field array.")
    .def("GetData",
         [](const viskores::cont::Field& self) { return self.GetData(); },
         "Return the field data as an UnknownArrayHandle.")
    .def("asnumpy",
         [](const viskores::cont::Field& self)
         { return vip::ArrayHandleToNumPy(self.GetData()); },
         "Return a read-only NumPy view of the field data.");

  nb::class_<viskores::cont::CoordinateSystem, viskores::cont::Field>(m, "CoordinateSystem")
    .def(nb::init<>())
    .def(nb::init<std::string, const viskores::cont::UnknownArrayHandle&>(),
         nb::arg("name"),
         nb::arg("data"),
         "Construct a CoordinateSystem from a name and an UnknownArrayHandle.")
    .def("GetBounds",
         &viskores::cont::CoordinateSystem::GetBounds,
         "Return the axis-aligned bounds of the coordinate data.");

  nb::class_<viskores::cont::DataSet>(m, "DataSet")
    .def(nb::init<>())
    .def("GetNumberOfFields",
         &viskores::cont::DataSet::GetNumberOfFields,
         "Number of fields attached to this DataSet.")
    .def("GetNumberOfPoints",
         &viskores::cont::DataSet::GetNumberOfPoints,
         "Number of points (vertices) in the DataSet.")
    .def("GetNumberOfCells",
         &viskores::cont::DataSet::GetNumberOfCells,
         "Number of cells in the DataSet.")
    .def("GetNumberOfCoordinateSystems",
         &viskores::cont::DataSet::GetNumberOfCoordinateSystems,
         "Number of coordinate systems attached to this DataSet.")
    .def("AddField",
         [](viskores::cont::DataSet& self, const viskores::cont::Field& field)
         { self.AddField(field); },
         nb::arg("field"),
         "Add a Field object to the DataSet.")
    .def("AddPointField",
         [](viskores::cont::DataSet& self,
            const std::string& name,
            const viskores::cont::UnknownArrayHandle& data)
         { self.AddPointField(name, data); },
         nb::arg("name"),
         nb::arg("data"),
         "Add a point-associated field from an UnknownArrayHandle.")
    .def("AddCellField",
         [](viskores::cont::DataSet& self,
            const std::string& name,
            const viskores::cont::UnknownArrayHandle& data)
         { self.AddCellField(name, data); },
         nb::arg("name"),
         nb::arg("data"),
         "Add a cell-associated field from an UnknownArrayHandle.")
    .def("HasField",
         &viskores::cont::DataSet::HasField,
         nb::arg("name"),
         nb::arg("association") = viskores::cont::Field::Association::Any,
         "Return True if a field with the given name (and optional association) exists.")
    .def("GetField",
         [](viskores::cont::DataSet& self,
            const std::string& name,
            viskores::cont::Field::Association assoc) -> viskores::cont::Field&
         { return self.GetField(name, assoc); },
         nb::arg("name"),
         nb::arg("association") = viskores::cont::Field::Association::Any,
         nb::rv_policy::reference_internal,
         "Retrieve a field by name. Throws if not found.\n"
         "The returned Field shares storage with the DataSet and keeps it alive.")
    .def("GetField",
         [](viskores::cont::DataSet& self, viskores::Id index) -> viskores::cont::Field&
         {
           if ((index < 0) || (index >= self.GetNumberOfFields()))
           {
             throw viskores::cont::ErrorBadValue("Field index out of range: " +
                                                 std::to_string(index));
           }
           return self.GetField(index);
         },
         nb::arg("index"),
         nb::rv_policy::reference_internal,
         "Retrieve a field by index (0 to GetNumberOfFields()-1). Throws if out of range.\n"
         "The returned Field shares storage with the DataSet and keeps it alive.")
    .def("AddCoordinateSystem",
         [](viskores::cont::DataSet& self, const viskores::cont::CoordinateSystem& cs)
         { self.AddCoordinateSystem(cs); },
         nb::arg("coordinate_system"),
         "Add a CoordinateSystem to the DataSet.")
    .def("GetCoordinateSystem",
         [](const viskores::cont::DataSet& self, viskores::Id index)
         {
           if ((index < 0) || (index >= self.GetNumberOfCoordinateSystems()))
           {
             throw viskores::cont::ErrorBadValue("Coordinate system index out of range: " +
                                                 std::to_string(index));
           }
           return self.GetCoordinateSystem(index);
         },
         nb::arg("index") = 0,
         "Return the CoordinateSystem at the given index (default 0). Throws if out of range.");

  nb::class_<viskores::cont::DataSetBuilderUniform>(m, "DataSetBuilderUniform")
    .def_static(
      "Create",
      [](std::vector<viskores::Id> dims) -> viskores::cont::DataSet
      {
        if ((dims.size() < 1) || (dims.size() > 3))
        {
          throw viskores::cont::ErrorBadValue(
            "DataSetBuilderUniform.Create() requires a list of 1, 2, or 3 dimensions.");
        }
        for (auto d : dims)
        {
          if (d < 1)
          {
            throw viskores::cont::ErrorBadValue(
              "DataSetBuilderUniform.Create() requires each dimension to be >= 1, got " +
              std::to_string(d));
          }
        }
        if (dims.size() == 1)
        {
          return viskores::cont::DataSetBuilderUniform::Create(dims[0]);
        }
        if (dims.size() == 2)
        {
          return viskores::cont::DataSetBuilderUniform::Create(
            viskores::Id2(dims[0], dims[1]));
        }
        return viskores::cont::DataSetBuilderUniform::Create(
          viskores::Id3(dims[0], dims[1], dims[2]));
      },
      nb::arg("dimensions"),
      "Create a uniform rectilinear DataSet. Pass a list of 1, 2, or 3 integer\n"
      "dimensions (number of points per axis), each >= 1. As in the C++ API, a\n"
      "dimension of 1 collapses that axis, lowering the dataset's dimensionality.\n"
      "Origin is [0,0,0] and spacing is [1,1,1] by default.");
}

} // namespace viskores::python::bindings
