//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleSOA.h>

namespace viskores::python::bindings
{

namespace
{

nb::object NumPyObjectFromUnknownArray(const viskores::cont::UnknownArrayHandle& array)
{
  return UnknownArrayToNumPyArray(array);
}

nb::object NumPyObjectFromField(const viskores::cont::Field& field)
{
  return FieldToNumPyArray(field);
}

nb::object NumPyObjectFromCoordinateSystem(const viskores::cont::CoordinateSystem& coordinateSystem)
{
  return UnknownArrayToNumPyArray(coordinateSystem.GetData());
}

nb::tuple BoundsTuple(const viskores::Bounds& bounds)
{
  return nb::make_tuple(
    bounds.X.Min, bounds.X.Max, bounds.Y.Min, bounds.Y.Max, bounds.Z.Min, bounds.Z.Max);
}

std::string DataSetCellSetTypeName(const viskores::cont::DataSet& dataSet)
{
  const auto& cellSet = dataSet.GetCellSet();
  if (cellSet.IsType<viskores::cont::CellSetExplicit<>>())
  {
    return "explicit";
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
         << ", cells=" << dataSet.GetNumberOfCells()
         << ", fields=" << dataSet.GetNumberOfFields() << ")";
  return stream.str();
}

std::string PartitionedDataSetRepr(const viskores::cont::PartitionedDataSet& dataSet)
{
  std::ostringstream stream;
  stream << "viskores.PartitionedDataSet(partitions=" << dataSet.GetNumberOfPartitions() << ")";
  return stream.str();
}

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

std::vector<viskores::Vec3f> ParseVec3Sequence(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of coordinate tuples.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  std::vector<viskores::Vec3f> values;
  values.reserve(size);
  for (size_t index = 0; index < size; ++index)
  {
    values.push_back(ParseVec3(sequence[index], viskores::Vec3f(0.0f, 0.0f, 0.0f)));
  }
  return values;
}

std::vector<viskores::UInt8> ParseUInt8Sequence(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of integer values.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  std::vector<viskores::UInt8> values(size);
  for (size_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<viskores::UInt8>(nb::cast<unsigned long>(sequence[index]));
  }
  return values;
}

std::vector<viskores::IdComponent> ParseIdComponentSequence(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of integer values.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  std::vector<viskores::IdComponent> values(size);
  for (size_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<viskores::IdComponent>(nb::cast<long long>(sequence[index]));
  }
  return values;
}

std::vector<viskores::Id> ParseIdSequence(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("Expected a sequence of integer values.");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  std::vector<viskores::Id> values(size);
  for (size_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<viskores::Id>(nb::cast<long long>(sequence[index]));
  }
  return values;
}

} // namespace

void RegisterNanobindSharedDataClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("UnknownArrayHandle");
  nb::class_<viskores::cont::UnknownArrayHandle>(m, "UnknownArrayHandle")
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::UnknownArrayHandle& self) {
           std::ostringstream stream;
           stream << "viskores.cont.UnknownArrayHandle(type=\"" << self.GetArrayTypeName()
                  << "\", values=" << self.GetNumberOfValues()
                  << ", components=" << self.GetNumberOfComponentsFlat() << ")";
           return stream.str();
         })
    .def("IsValid", &viskores::cont::UnknownArrayHandle::IsValid)
    .def("GetArrayTypeName", &viskores::cont::UnknownArrayHandle::GetArrayTypeName)
    .def("GetNumberOfValues", &viskores::cont::UnknownArrayHandle::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat", &viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat)
    .def("AsNumPy",
         [](const viskores::cont::UnknownArrayHandle& self) {
           return NumPyObjectFromUnknownArray(self);
         })
    .def("IsStorageTypeSOA",
         [](const viskores::cont::UnknownArrayHandle& self) {
           return self.IsStorageType<viskores::cont::StorageTagSOA>();
         })
    .def("ExtractArrayFromComponents",
         [](const viskores::cont::UnknownArrayHandle& self) {
           return self.ExtractArrayFromComponents<viskores::FloatDefault>();
         });

  erase_existing_name("ArrayHandleSOAVec3f");
  nb::class_<viskores::cont::ArrayHandleSOA<viskores::Vec3f>>(m, "ArrayHandleSOAVec3f")
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::ArrayHandleSOA<viskores::Vec3f>& self) {
           std::ostringstream stream;
           stream << "viskores.cont.ArrayHandleSOAVec3f(values=" << self.GetNumberOfValues() << ")";
           return stream.str();
         })
    .def("GetNumberOfValues", &viskores::cont::ArrayHandleSOA<viskores::Vec3f>::GetNumberOfValues)
    .def("AsNumPy",
         [](const viskores::cont::ArrayHandleSOA<viskores::Vec3f>& self) {
           return NumPyObjectFromUnknownArray(viskores::cont::UnknownArrayHandle{ self });
         });

  erase_existing_name("ArrayHandleRecombineVecFloatDefault");
  nb::class_<viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>>(
    m, "ArrayHandleRecombineVecFloatDefault")
    .def("__repr__",
         [](const viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>& self) {
           std::ostringstream stream;
           stream << "viskores.cont.ArrayHandleRecombineVecFloatDefault(values="
                  << self.GetNumberOfValues() << ", components=" << self.GetNumberOfComponentsFlat()
                  << ")";
           return stream.str();
         })
    .def("GetNumberOfValues",
         &viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat",
         &viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>::GetNumberOfComponentsFlat)
    .def("AsNumPy",
         [](const viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>& self) {
           return NumPyObjectFromUnknownArray(viskores::cont::UnknownArrayHandle{ self });
         });

  erase_existing_name("ArrayCopy");
  m.attr("ArrayCopy") = nb::cpp_function(
    [](const viskores::cont::UnknownArrayHandle& source,
       viskores::cont::ArrayHandleSOA<viskores::Vec3f>& destination) {
      viskores::cont::ArrayCopy(source, destination);
    },
    nb::arg("source"),
    nb::arg("destination"));

  erase_existing_name("CellSet");
  nb::class_<viskores::cont::UnknownCellSet>(m, "CellSet")
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::UnknownCellSet& self) {
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
    .def("GetCellPointIds",
         [](const viskores::cont::UnknownCellSet& self, viskores::Id cellId) {
           const auto count = self.GetNumberOfPointsInCell(cellId);
           std::vector<viskores::Id> pointIds(static_cast<std::size_t>(count));
           self.GetCellPointIds(cellId, pointIds.data());
           return nb::cast(pointIds);
         },
         nb::arg("cell_id"));

  erase_existing_name("Field");
  nb::class_<viskores::cont::Field>(m, "Field")
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::cont::Field* self, const std::string& name, nb::object associationObject, nb::object values) {
        new (self) viskores::cont::Field(
          name,
          ParseAssociation(associationObject, viskores::cont::Field::Association::Any),
          NumPyArrayToUnknownArray(values));
      },
      nb::arg("name"),
      nb::arg("association"),
      nb::arg("values"))
    .def("__repr__",
         [](const viskores::cont::Field& self) {
           std::ostringstream stream;
           stream << "viskores.cont.Field(name=\"" << self.GetName() << "\", association="
                  << static_cast<int>(self.GetAssociation()) << ")";
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
    .def("GetData", [](const viskores::cont::Field& self) { return NumPyObjectFromUnknownArray(self.GetData()); })
    .def("GetRange", [](const viskores::cont::Field& self) { return FieldRangeList(self); });

  erase_existing_name("CoordinateSystem");
  nb::class_<viskores::cont::CoordinateSystem, viskores::cont::Field>(m, "CoordinateSystem")
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::cont::CoordinateSystem* self, const std::string& name, nb::object values) {
        new (self) viskores::cont::CoordinateSystem(
          name, NumPyArrayToUnknownArray(values));
      },
      nb::arg("name"),
      nb::arg("values"))
    .def(
      "__init__",
      [](viskores::cont::CoordinateSystem* self,
         const std::string& name,
         nb::object dimensionsObject,
         nb::object originObject,
         nb::object spacingObject) {
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
      [](viskores::cont::CoordinateSystem* self, const viskores::cont::Field& field) {
        new (self) viskores::cont::CoordinateSystem(field);
      },
      nb::arg("field"))
    .def("__repr__",
         [](const viskores::cont::CoordinateSystem& self) {
           std::ostringstream stream;
           stream << "viskores.cont.CoordinateSystem(name=\"" << self.GetName()
                  << "\", points=" << self.GetNumberOfPoints() << ")";
           return stream.str();
         })
    .def("GetNumberOfPoints", &viskores::cont::CoordinateSystem::GetNumberOfPoints)
    .def("GetData",
         [](const viskores::cont::CoordinateSystem& self) {
           return viskores::cont::UnknownArrayHandle(self.GetData());
         })
    .def("SetData",
         [](viskores::cont::CoordinateSystem& self, nb::object values) {
           if (nb::isinstance<viskores::cont::UnknownArrayHandle>(values))
           {
             self.SetData(nb::cast<viskores::cont::UnknownArrayHandle>(values));
             return;
           }
           if (nb::isinstance<viskores::cont::ArrayHandleSOA<viskores::Vec3f>>(values))
           {
             self.SetData(nb::cast<viskores::cont::ArrayHandleSOA<viskores::Vec3f>>(values));
             return;
           }
           if (nb::isinstance<viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>>(values))
           {
             self.SetData(
               nb::cast<viskores::cont::ArrayHandleRecombineVec<viskores::FloatDefault>>(values));
             return;
           }
           self.SetData(NumPyArrayToUnknownArray(values));
         },
         nb::arg("values"))
    .def("GetRange",
         [](const viskores::cont::CoordinateSystem& self) {
           return CoordinateSystemRangeList(self);
         })
    .def("GetBounds",
         [](const viskores::cont::CoordinateSystem& self) { return BoundsTuple(self.GetBounds()); });

  erase_existing_name("DataSet");
  nb::class_<viskores::cont::DataSet>(m, "DataSet")
    .def(nb::init<>())
    .def("__repr__", [](const viskores::cont::DataSet& self) { return DataSetRepr(self); })
    .def("AddField",
         [](viskores::cont::DataSet& self, const viskores::cont::Field& field) { self.AddField(field); },
         nb::arg("field"))
    .def(
      "AddPointField",
      [](viskores::cont::DataSet& self, const char* name, nb::object values) {
        self.AddPointField(name, NumPyArrayToUnknownArray(values));
      },
      nb::arg("field_name"),
      nb::arg("field"))
    .def(
      "AddCellField",
      [](viskores::cont::DataSet& self, const char* name, nb::object values) {
        self.AddCellField(name, NumPyArrayToUnknownArray(values));
      },
      nb::arg("field_name"),
      nb::arg("field"))
    .def(
      "SetGhostCellField",
      [](viskores::cont::DataSet& self, nb::args args) {
        if ((args.size() < 1) || (args.size() > 2))
        {
          throw std::runtime_error("SetGhostCellField expects values or (name, values).");
        }
        if (args.size() == 1)
        {
          self.SetGhostCellField(NumPyArrayToUnknownArray(args[0]));
        }
        else
        {
          if (!nb::isinstance<nb::str>(args[0]))
          {
            throw std::runtime_error("Ghost field name must be a string.");
          }
          self.SetGhostCellField(nb::cast<std::string>(args[0]),
                                 NumPyArrayToUnknownArray(args[1]));
        }
      })
    .def("GetGhostCellField",
         [](const viskores::cont::DataSet& self) { return NumPyObjectFromField(self.GetGhostCellField()); })
    .def("HasGhostCellField", &viskores::cont::DataSet::HasGhostCellField)
    .def("GetGhostCellFieldName", &viskores::cont::DataSet::GetGhostCellFieldName)
    .def("CellSetTypeName", [](const viskores::cont::DataSet& self) { return DataSetCellSetTypeName(self); })
    .def(
      "GetField",
      [](const viskores::cont::DataSet& self, const char* name, nb::object association) {
        return NumPyObjectFromField(
          self.GetField(name, ParseAssociation(association, viskores::cont::Field::Association::Any)));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "GetFieldObject",
      [](const viskores::cont::DataSet& self, const char* name, nb::object association) {
        return self.GetField(name, ParseAssociation(association, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "GetCoordinateSystem",
      [](const viskores::cont::DataSet& self, nb::object indexOrName) {
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
    .def("GetCellSet",
         [](const viskores::cont::DataSet& self) { return self.GetCellSet(); })
    .def(
      "AddCoordinateSystem",
      [](viskores::cont::DataSet& self, nb::args args) {
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
        return self.AddCoordinateSystem(
          nb::cast<std::string>(args[0]), NumPyArrayToUnknownArray(args[1]));
      })
    .def("HasCoordinateSystem", &viskores::cont::DataSet::HasCoordinateSystem)
    .def("GetCoordinateSystemName",
         [](const viskores::cont::DataSet& self, long long index) {
           return self.GetCoordinateSystemName(static_cast<viskores::Id>(index));
         },
         nb::arg("index") = 0)
    .def("GetNumberOfCoordinateSystems", &viskores::cont::DataSet::GetNumberOfCoordinateSystems)
    .def(
      "HasField",
      [](const viskores::cont::DataSet& self, const char* name, nb::object association) {
        return self.HasField(name, ParseAssociation(association, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("FieldNames", [](const viskores::cont::DataSet& self) { return DataSetFieldNames(self); })
    .def("GetNumberOfFields", &viskores::cont::DataSet::GetNumberOfFields)
    .def("GetNumberOfPoints", &viskores::cont::DataSet::GetNumberOfPoints)
    .def("GetNumberOfCells", &viskores::cont::DataSet::GetNumberOfCells);

  erase_existing_name("DataSetBuilderExplicit");
  nb::class_<viskores::cont::DataSetBuilderExplicit>(m, "DataSetBuilderExplicit")
    .def(nb::init<>())
    .def_static(
      "Create",
      [](nb::object coordsObject,
         nb::object shapesObject,
         nb::object numIndicesObject,
         nb::object connectivityObject,
         const std::string& coordName) {
        return WrapDataSet(viskores::cont::DataSetBuilderExplicit::Create(
          ParseVec3Sequence(coordsObject),
          ParseUInt8Sequence(shapesObject),
          ParseIdComponentSequence(numIndicesObject),
          ParseIdSequence(connectivityObject),
          coordName));
      },
      nb::arg("coords"),
      nb::arg("shapes"),
      nb::arg("num_indices"),
      nb::arg("connectivity"),
      nb::arg("coord_name") = "coords");

  erase_existing_name("DataSetBuilderExplicitIterative");
  nb::class_<viskores::cont::DataSetBuilderExplicitIterative>(m, "DataSetBuilderExplicitIterative")
    .def(nb::init<>())
    .def("Begin",
         &viskores::cont::DataSetBuilderExplicitIterative::Begin,
         nb::arg("coord_name") = "coords")
    .def("AddPoint",
         [](viskores::cont::DataSetBuilderExplicitIterative& self, nb::object pointObject) {
           return self.AddPoint(ParseVec3(pointObject, viskores::Vec3f(0.0f, 0.0f, 0.0f)));
         },
         nb::arg("point"))
    .def("AddCell",
         [](viskores::cont::DataSetBuilderExplicitIterative& self,
            unsigned long shape,
            nb::object connectivityObject) {
           self.AddCell(static_cast<viskores::UInt8>(shape), ParseIdSequence(connectivityObject));
         },
         nb::arg("shape"),
         nb::arg("connectivity"))
    .def("AddCellPoint",
         &viskores::cont::DataSetBuilderExplicitIterative::AddCellPoint,
         nb::arg("point_index"))
    .def("Create",
         [](viskores::cont::DataSetBuilderExplicitIterative& self) {
           return WrapDataSet(self.Create());
         });

  erase_existing_name("PartitionedDataSet");
  nb::class_<viskores::cont::PartitionedDataSet>(m, "PartitionedDataSet")
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::PartitionedDataSet& self) { return PartitionedDataSetRepr(self); })
    .def("AddField",
         [](viskores::cont::PartitionedDataSet& self, const viskores::cont::Field& field) {
           self.AddField(field);
         },
         nb::arg("field"))
    .def(
      "AppendPartition",
      [](viskores::cont::PartitionedDataSet& self,
         const std::shared_ptr<viskores::cont::DataSet>& dataset) { self.AppendPartition(*dataset); },
      nb::arg("dataset"))
    .def(
      "GetPartition",
      [](const viskores::cont::PartitionedDataSet& self, long long index) {
        return std::make_shared<viskores::cont::DataSet>(
          self.GetPartition(static_cast<viskores::Id>(index)));
      },
      nb::arg("index"))
    .def(
      "GetField",
      [](const viskores::cont::PartitionedDataSet& self, const char* name, nb::object association) {
        return NumPyObjectFromField(
          self.GetField(name, ParseAssociation(association, viskores::cont::Field::Association::Any)));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def(
      "HasField",
      [](const viskores::cont::PartitionedDataSet& self, const char* name, nb::object association) {
        return self.HasField(name, ParseAssociation(association, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetNumberOfFields", &viskores::cont::PartitionedDataSet::GetNumberOfFields)
    .def("GetNumberOfPartitions", &viskores::cont::PartitionedDataSet::GetNumberOfPartitions);

  erase_existing_name("make_Field");
  m.attr("make_Field") = nb::cpp_function(
    [](const std::string& name, nb::object associationObject, nb::object values) {
      return viskores::cont::Field(
        name,
        ParseAssociation(associationObject, viskores::cont::Field::Association::Any),
        NumPyArrayToUnknownArray(values));
    },
    nb::arg("name"),
    nb::arg("association"),
    nb::arg("values"));

  erase_existing_name("make_FieldPoint");
  m.attr("make_FieldPoint") = nb::cpp_function(
    [](const std::string& name, nb::object values) {
      return viskores::cont::make_FieldPoint(name, NumPyArrayToUnknownArray(values));
    },
    nb::arg("name"),
    nb::arg("values"));

  erase_existing_name("make_FieldCell");
  m.attr("make_FieldCell") = nb::cpp_function(
    [](const std::string& name, nb::object values) {
      return viskores::cont::make_FieldCell(name, NumPyArrayToUnknownArray(values));
    },
    nb::arg("name"),
    nb::arg("values"));

  erase_existing_name("make_FieldWholeDataSet");
  m.attr("make_FieldWholeDataSet") = nb::cpp_function(
    [](const std::string& name, nb::object values) {
      return viskores::cont::Field(
        name,
        viskores::cont::Field::Association::WholeDataSet,
        NumPyArrayToUnknownArray(values));
    },
    nb::arg("name"),
    nb::arg("values"));
}

} // namespace viskores::python::bindings
