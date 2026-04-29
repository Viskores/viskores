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
#include <viskores/cont/ArrayHandleSOA.h>

namespace viskores::python::bindings
{

namespace
{

nb::list FieldRangeList(const viskores::cont::Field& field)
{
  const auto ranges = field.GetRange();
  nb::list output;
  auto portal = ranges.ReadPortal();
  for (viskores::Id index = 0; index < ranges.GetNumberOfValues(); ++index)
  {
    const auto range = portal.Get(index);
    output.append(nb::make_tuple(range.Min, range.Max));
  }
  return output;
}

nb::list CoordinateSystemRangeList(const viskores::cont::CoordinateSystem& coordinateSystem)
{
  const auto ranges = coordinateSystem.GetRange();
  nb::list output;
  for (viskores::IdComponent index = 0; index < 3; ++index)
  {
    output.append(nb::make_tuple(ranges[index].Min, ranges[index].Max));
  }
  return output;
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
bool TrySetCoordinateSystemDataSOA(viskores::cont::CoordinateSystem& coordinateSystem,
                                   nb::handle values)
{
  using ArrayType =
    viskores::cont::ArrayHandleSOA<viskores::Vec<ComponentType, NumberOfComponents>>;
  ArrayType* typedValues = nullptr;
  if (!nb::try_cast(values, typedValues))
  {
    return false;
  }

  coordinateSystem.SetData(*typedValues);
  return true;
}

template <typename ComponentType>
struct TrySetCoordinateSystemDataSOAComponentCountFunctor
{
  viskores::cont::CoordinateSystem& CoordinateSystem;
  nb::handle Values;

  template <viskores::IdComponent NumberOfComponents>
  bool operator()() const
  {
    return TrySetCoordinateSystemDataSOA<ComponentType, NumberOfComponents>(
      this->CoordinateSystem, this->Values);
  }
};

template <typename ComponentType>
bool TrySetCoordinateSystemDataSpecialized(viskores::cont::CoordinateSystem& coordinateSystem,
                                           nb::handle values)
{
  return TryCompiledVecComponentCounts(
    TrySetCoordinateSystemDataSOAComponentCountFunctor<ComponentType>{ coordinateSystem, values });
}

struct TrySetCoordinateSystemDataSpecializedFunctor
{
  viskores::cont::CoordinateSystem& CoordinateSystem;
  nb::handle Values;

  template <typename ComponentType>
  bool operator()() const
  {
    return TrySetCoordinateSystemDataSpecialized<ComponentType>(
      this->CoordinateSystem, this->Values);
  }
};

bool TrySetCoordinateSystemDataFromPythonObject(viskores::cont::CoordinateSystem& coordinateSystem,
                                                nb::handle values)
{
  if (TryRegisteredScalarTypes(
        TrySetCoordinateSystemDataSpecializedFunctor{ coordinateSystem, values }))
  {
    return true;
  }

  return false;
}

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
  erase_existing_name("CellSet");
  nb::class_<viskores::cont::UnknownCellSet>(m, "CellSet", doc::ClassDoc("CellSet"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::UnknownCellSet& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.CellSet(type=\"" << self.GetCellSetName()
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

void RegisterNanobindFieldClasses(nb::module_& m,
                                  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Field");
  nb::class_<viskores::cont::Field>(m, "Field", doc::ClassDoc("Field"))
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::cont::Field* self,
         const std::string& name,
         nb::object associationObject,
         nb::object values)
      {
        new (self) viskores::cont::Field(
          name,
          ParseAssociation(associationObject, viskores::cont::Field::Association::Any),
          PythonObjectToUnknownArray(values));
      },
      nb::arg("name"),
      nb::arg("association"),
      nb::arg("values"))
    .def("__repr__",
         [](const viskores::cont::Field& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.Field(name=\"" << self.GetName()
                  << "\", association=" << static_cast<int>(self.GetAssociation()) << ")";
           return stream.str();
         })
    .def("GetName", &viskores::cont::Field::GetName)
    .def("GetAssociation",
         [](const viskores::cont::Field& self) { return static_cast<int>(self.GetAssociation()); })
    .def("GetNumberOfValues", &viskores::cont::Field::GetNumberOfValues)
    .def("IsCellField", &viskores::cont::Field::IsCellField)
    .def("IsPointField", &viskores::cont::Field::IsPointField)
    .def("IsWholeDataSetField", &viskores::cont::Field::IsWholeDataSetField)
    .def("IsPartitionsField", &viskores::cont::Field::IsPartitionsField)
    .def("IsGlobalField", &viskores::cont::Field::IsGlobalField)
    .def("GetData",
         [](const viskores::cont::Field& self)
         { return NumPyObjectFromUnknownArray(self.GetData()); })
    .def("GetRange", [](const viskores::cont::Field& self) { return FieldRangeList(self); });

  erase_existing_name("CoordinateSystem");
  nb::class_<viskores::cont::CoordinateSystem, viskores::cont::Field>(
    m, "CoordinateSystem", doc::ClassDoc("CoordinateSystem"))
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::cont::CoordinateSystem* self, const std::string& name, nb::object values)
      { new (self) viskores::cont::CoordinateSystem(name, PythonObjectToUnknownArray(values)); },
      nb::arg("name"),
      nb::arg("values"))
    .def(
      "__init__",
      [](viskores::cont::CoordinateSystem* self,
         const std::string& name,
         nb::object dimensionsObject,
         nb::object originObject,
         nb::object spacingObject)
      {
        new (self) viskores::cont::CoordinateSystem(
          name,
          ParseDimensions(dimensionsObject),
          ParseVec3(originObject, viskores::Vec3f(0.0f, 0.0f, 0.0f)),
          ParseVec3(spacingObject, viskores::Vec3f(1.0f, 1.0f, 1.0f)));
      },
      nb::arg("name"),
      nb::arg("dimensions"),
      nb::arg("origin") = nb::make_tuple(0.0f, 0.0f, 0.0f),
      nb::arg("spacing") = nb::make_tuple(1.0f, 1.0f, 1.0f))
    .def(
      "__init__",
      [](viskores::cont::CoordinateSystem* self, const viskores::cont::Field& field)
      { new (self) viskores::cont::CoordinateSystem(field); },
      nb::arg("field"))
    .def("__repr__",
         [](const viskores::cont::CoordinateSystem& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.CoordinateSystem(name=\"" << self.GetName()
                  << "\", points=" << self.GetNumberOfPoints() << ")";
           return stream.str();
         })
    .def("GetNumberOfPoints", &viskores::cont::CoordinateSystem::GetNumberOfPoints)
    .def("GetData",
         [](const viskores::cont::CoordinateSystem& self)
         { return viskores::cont::UnknownArrayHandle(self.GetData()); })
    .def(
      "SetData",
      [](viskores::cont::CoordinateSystem& self, nb::object values)
      {
        if (nb::isinstance<viskores::cont::UnknownArrayHandle>(values))
        {
          self.SetData(nb::cast<viskores::cont::UnknownArrayHandle>(values));
          return;
        }
        if (TrySetCoordinateSystemDataFromPythonObject(self, values))
        {
          return;
        }
        self.SetData(PythonObjectToUnknownArray(values));
      },
      nb::arg("values"))
    .def("GetRange",
         [](const viskores::cont::CoordinateSystem& self)
         { return CoordinateSystemRangeList(self); })
    .def("GetBounds",
         [](const viskores::cont::CoordinateSystem& self)
         { return BoundsToTuple(self.GetBounds()); });
}

void RegisterNanobindFieldFactoryFunctions(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("make_Field");
  m.attr("make_Field") = nb::cpp_function(
    [](const std::string& name, nb::object associationObject, nb::object values)
    {
      return viskores::cont::Field(
        name,
        ParseAssociation(associationObject, viskores::cont::Field::Association::Any),
        PythonObjectToUnknownArray(values));
    },
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
  RegisterSimpleFieldFactory(
    m,
    erase_existing_name,
    "make_FieldWholeDataSet",
    [](const std::string& name, const viskores::cont::UnknownArrayHandle& values)
    { return viskores::cont::Field(name, viskores::cont::Field::Association::WholeDataSet, values); },
    doc::MakeFieldWholeDataSet);
}

} // namespace viskores::python::bindings
