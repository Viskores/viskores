//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

namespace
{

std::string DataSetCellSetTypeName(const viskores::cont::DataSet& dataSet)
{
  const auto& cellSet = dataSet.GetCellSet();
  if (cellSet.IsType<viskores::cont::CellSetExplicit<>>())
  {
    return "explicit";
  }
  if (cellSet.CanConvert<viskores::cont::CellSetStructured<1>>())
  {
    return "structured1d";
  }
  if (cellSet.CanConvert<viskores::cont::CellSetStructured<2>>())
  {
    return "structured2d";
  }
  if (cellSet.CanConvert<viskores::cont::CellSetStructured<3>>())
  {
    return "structured3d";
  }
  return "other";
}

nb::list DataSetFieldNames(const viskores::cont::DataSet& dataSet)
{
  nb::list names;
  const viskores::IdComponent numberOfFields = dataSet.GetNumberOfFields();
  for (viskores::IdComponent index = 0; index < numberOfFields; ++index)
  {
    names.append(nb::str(dataSet.GetField(index).GetName().c_str()));
  }
  return names;
}

std::string DataSetRepr(const viskores::cont::DataSet& dataSet)
{
  std::ostringstream stream;
  stream << "viskores.DataSet(points=" << dataSet.GetNumberOfPoints()
         << ", cells=" << dataSet.GetNumberOfCells() << ", fields=" << dataSet.GetNumberOfFields()
         << ")";
  return stream.str();
}

std::string PartitionedDataSetRepr(const viskores::cont::PartitionedDataSet& dataSet)
{
  std::ostringstream stream;
  stream << "viskores.PartitionedDataSet(partitions=" << dataSet.GetNumberOfPartitions() << ")";
  return stream.str();
}

} // namespace

void RegisterNanobindDataSetClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("DataSet");
  nb::class_<viskores::cont::DataSet>(m, "DataSet", doc::ClassDoc("DataSet"))
    .def(nb::init<>())
    .def("__repr__", [](const viskores::cont::DataSet& self) { return DataSetRepr(self); })
    .def(
      "AddField",
      [](viskores::cont::DataSet& self, const viskores::cont::Field& field)
      { self.AddField(field); },
      nb::arg("field"))
    .def(
      "AddPointField",
      [](viskores::cont::DataSet& self, const char* name, nb::object values)
      { self.AddPointField(name, PythonObjectToUnknownArray(values)); },
      nb::arg("field_name"),
      nb::arg("field"))
    .def(
      "AddCellField",
      [](viskores::cont::DataSet& self, const char* name, nb::object values)
      { self.AddCellField(name, PythonObjectToUnknownArray(values)); },
      nb::arg("field_name"),
      nb::arg("field"))
    .def("SetGhostCellField",
         [](viskores::cont::DataSet& self, nb::args args)
         {
           if ((args.size() < 1) || (args.size() > 2))
           {
             throw std::runtime_error("SetGhostCellField expects values or (name, values).");
           }
           if (args.size() == 1)
           {
             self.SetGhostCellField(PythonObjectToUnknownArray(args[0]));
           }
           else
           {
             if (!nb::isinstance<nb::str>(args[0]))
             {
               throw std::runtime_error("Ghost field name must be a string.");
             }
             self.SetGhostCellField(nb::cast<std::string>(args[0]),
                                    PythonObjectToUnknownArray(args[1]));
           }
         })
    .def("GetGhostCellField",
         [](const viskores::cont::DataSet& self)
         { return NumPyObjectFromField(self.GetGhostCellField()); })
    .def("HasGhostCellField", &viskores::cont::DataSet::HasGhostCellField)
    .def("GetGhostCellFieldName", &viskores::cont::DataSet::GetGhostCellFieldName)
    .def("CellSetTypeName",
         [](const viskores::cont::DataSet& self) { return DataSetCellSetTypeName(self); })
    .def(
      "GetField",
      [](const viskores::cont::DataSet& self, const char* name, nb::object association)
      {
        return NumPyObjectFromField(self.GetField(
          name, ParseAssociation(association, viskores::cont::Field::Association::Any)));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "GetFieldObject",
      [](const viskores::cont::DataSet& self, const char* name, nb::object association)
      {
        return self.GetField(
          name, ParseAssociation(association, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "GetCoordinateSystem",
      [](const viskores::cont::DataSet& self, nb::object indexOrName)
      {
        if (indexOrName.is_none())
        {
          return self.GetCoordinateSystem();
        }
        if (nb::isinstance<nb::str>(indexOrName))
        {
          return self.GetCoordinateSystem(nb::cast<std::string>(indexOrName));
        }
        return self.GetCoordinateSystem(nb::cast<long long>(indexOrName));
      },
      nb::arg("index") = nb::none())
    .def("GetCellSet", [](const viskores::cont::DataSet& self) { return self.GetCellSet(); })
    .def("AddCoordinateSystem",
         [](viskores::cont::DataSet& self, nb::args args)
         {
           if ((args.size() < 1) || (args.size() > 2))
           {
             throw std::runtime_error(
               "AddCoordinateSystem expects coordinate_system, field_name, or (name, values).");
           }
           if (args.size() == 1)
           {
             if (nb::isinstance<viskores::cont::CoordinateSystem>(args[0]))
             {
               return self.AddCoordinateSystem(nb::cast<viskores::cont::CoordinateSystem>(args[0]));
             }
             if (!nb::isinstance<nb::str>(args[0]))
             {
               throw std::runtime_error("Coordinate system field name must be a string.");
             }
             return self.AddCoordinateSystem(nb::cast<std::string>(args[0]));
           }
           if (!nb::isinstance<nb::str>(args[0]))
           {
             throw std::runtime_error("Coordinate system name must be a string.");
           }
           return self.AddCoordinateSystem(nb::cast<std::string>(args[0]),
                                           PythonObjectToUnknownArray(args[1]));
         })
    .def("HasCoordinateSystem", &viskores::cont::DataSet::HasCoordinateSystem)
    .def(
      "GetCoordinateSystemName",
      [](const viskores::cont::DataSet& self, long long index)
      { return self.GetCoordinateSystemName(static_cast<viskores::Id>(index)); },
      nb::arg("index") = 0)
    .def("GetNumberOfCoordinateSystems", &viskores::cont::DataSet::GetNumberOfCoordinateSystems)
    .def(
      "HasField",
      [](const viskores::cont::DataSet& self, const char* name, nb::object association)
      {
        return self.HasField(
          name, ParseAssociation(association, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("FieldNames", [](const viskores::cont::DataSet& self) { return DataSetFieldNames(self); })
    .def("GetNumberOfFields", &viskores::cont::DataSet::GetNumberOfFields)
    .def("GetNumberOfPoints", &viskores::cont::DataSet::GetNumberOfPoints)
    .def("GetNumberOfCells", &viskores::cont::DataSet::GetNumberOfCells);
}

void RegisterNanobindPartitionedDataSetClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("PartitionedDataSet");
  nb::class_<viskores::cont::PartitionedDataSet>(
    m, "PartitionedDataSet", doc::ClassDoc("PartitionedDataSet"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::PartitionedDataSet& self)
         { return PartitionedDataSetRepr(self); })
    .def(
      "AddField",
      [](viskores::cont::PartitionedDataSet& self, const viskores::cont::Field& field)
      { self.AddField(field); },
      nb::arg("field"))
    .def(
      "AppendPartition",
      [](viskores::cont::PartitionedDataSet& self,
         const std::shared_ptr<viskores::cont::DataSet>& dataset)
      { self.AppendPartition(*dataset); },
      nb::arg("dataset"))
    .def(
      "GetPartition",
      [](const viskores::cont::PartitionedDataSet& self, long long index)
      {
        return std::make_shared<viskores::cont::DataSet>(
          self.GetPartition(static_cast<viskores::Id>(index)));
      },
      nb::arg("index"))
    .def(
      "GetField",
      [](const viskores::cont::PartitionedDataSet& self, const char* name, nb::object association)
      {
        return NumPyObjectFromField(self.GetField(
          name, ParseAssociation(association, viskores::cont::Field::Association::Any)));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "HasField",
      [](const viskores::cont::PartitionedDataSet& self, const char* name, nb::object association)
      {
        return self.HasField(
          name, ParseAssociation(association, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetNumberOfFields", &viskores::cont::PartitionedDataSet::GetNumberOfFields)
    .def("GetNumberOfPartitions", &viskores::cont::PartitionedDataSet::GetNumberOfPartitions);
}

} // namespace viskores::python::bindings
