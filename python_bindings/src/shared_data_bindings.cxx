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

viskores::cont::ArrayHandle<viskores::FloatDefault> ParseRectilinearAxis(nb::handle object,
                                                                         const char* name)
{
  if (!object.is_valid() || object.is_none())
  {
    std::ostringstream message;
    message << name << " coordinate values are required.";
    throw std::runtime_error(message.str());
  }

  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    std::ostringstream message;
    message << name << " coordinate values must be a one-dimensional numeric sequence.";
    throw std::runtime_error(message.str());
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(object);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  if (size == 0)
  {
    std::ostringstream message;
    message << name << " coordinate values must not be empty.";
    throw std::runtime_error(message.str());
  }

  std::vector<viskores::FloatDefault> values(size);
  for (size_t index = 0; index < size; ++index)
  {
    values[index] = static_cast<viskores::FloatDefault>(nb::cast<double>(sequence[index]));
  }
  return viskores::cont::make_ArrayHandle(values, viskores::CopyFlag::On);
}

viskores::IdComponent GetDimensionsRank(nb::handle object)
{
  if (!nb::isinstance<nb::sequence>(object) || nb::isinstance<nb::str>(object))
  {
    throw std::runtime_error("dimensions must be a sequence of 1, 2, or 3 integers.");
  }

  const size_t size = static_cast<size_t>(nb::len(nb::borrow<nb::sequence>(object)));
  if ((size < 1) || (size > 3))
  {
    throw std::runtime_error("dimensions must contain 1, 2, or 3 integers.");
  }
  return static_cast<viskores::IdComponent>(size);
}

template <typename ArrayType>
void RegisterArrayHandleSOAClass(nb::module_& m,
                                 const std::function<void(const char*)>& erase_existing_name,
                                 const char* name)
{
  erase_existing_name(name);
  nb::class_<ArrayType>(m, name)
    .def(nb::init<>())
    .def("__repr__",
         [name](const ArrayType& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont." << name << "(values=" << self.GetNumberOfValues() << ")";
           return stream.str();
         })
    .def("GetNumberOfValues", &ArrayType::GetNumberOfValues)
    .def(
      "AsNumPy",
      [](const ArrayType& self, bool copy)
      { return UnknownArrayToNumPyArray(viskores::cont::UnknownArrayHandle{ self }, copy); },
      nb::arg("copy") = true);
}

template <typename ComponentType>
void RegisterArrayHandleRecombineVecClass(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name,
  const char* name)
{
  using ArrayType = viskores::cont::ArrayHandleRecombineVec<ComponentType>;

  erase_existing_name(name);
  nb::class_<ArrayType>(m, name)
    .def("__repr__",
         [name](const ArrayType& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont." << name << "(values=" << self.GetNumberOfValues()
                  << ", components=" << self.GetNumberOfComponentsFlat() << ")";
           return stream.str();
         })
    .def("GetNumberOfValues", &ArrayType::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat", &ArrayType::GetNumberOfComponentsFlat)
    .def(
      "AsNumPy",
      [](const ArrayType& self, bool copy)
      { return UnknownArrayToNumPyArray(viskores::cont::UnknownArrayHandle{ self }, copy); },
      nb::arg("copy") = true);
}

template <typename ComponentType>
bool TryExtractArrayFromComponents(const viskores::cont::UnknownArrayHandle& array,
                                   nb::object& output)
{
  if (!array.IsBaseComponentType<ComponentType>())
  {
    return false;
  }

  output = nb::cast(array.ExtractArrayFromComponents<ComponentType>());
  return true;
}

nb::object ExtractArrayFromComponentsObject(const viskores::cont::UnknownArrayHandle& array)
{
  nb::object output;
  if (TryExtractArrayFromComponents<viskores::Float32>(array, output) ||
      TryExtractArrayFromComponents<viskores::Float64>(array, output) ||
      TryExtractArrayFromComponents<viskores::Int8>(array, output) ||
      TryExtractArrayFromComponents<viskores::UInt8>(array, output) ||
      TryExtractArrayFromComponents<viskores::Int16>(array, output) ||
      TryExtractArrayFromComponents<viskores::UInt16>(array, output) ||
      TryExtractArrayFromComponents<viskores::Int32>(array, output) ||
      TryExtractArrayFromComponents<viskores::UInt32>(array, output) ||
      TryExtractArrayFromComponents<viskores::Int64>(array, output) ||
      TryExtractArrayFromComponents<viskores::UInt64>(array, output))
  {
    return output;
  }
  throw std::runtime_error("Unsupported field component type for component extraction.");
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
bool TryArrayCopyToSOA(const viskores::cont::UnknownArrayHandle& source, nb::handle destination)
{
  using ArrayType =
    viskores::cont::ArrayHandleSOA<viskores::Vec<ComponentType, NumberOfComponents>>;
  ArrayType* typedDestination = nullptr;
  if (!nb::try_cast(destination, typedDestination))
  {
    return false;
  }

  viskores::cont::ArrayCopy(source, *typedDestination);
  return true;
}

template <typename ComponentType>
bool TryArrayCopyToSOAComponent(const viskores::cont::UnknownArrayHandle& source,
                                nb::handle destination)
{
  return TryArrayCopyToSOA<ComponentType, 2>(source, destination) ||
    TryArrayCopyToSOA<ComponentType, 3>(source, destination) ||
    TryArrayCopyToSOA<ComponentType, 4>(source, destination);
}

template <typename ComponentType, viskores::IdComponent NumberOfComponents>
bool TryPythonObjectToSOAUnknownArray(nb::handle object, viskores::cont::UnknownArrayHandle& array)
{
  using ArrayType =
    viskores::cont::ArrayHandleSOA<viskores::Vec<ComponentType, NumberOfComponents>>;
  ArrayType* typedArray = nullptr;
  if (!nb::try_cast(object, typedArray))
  {
    return false;
  }

  array = viskores::cont::UnknownArrayHandle(*typedArray);
  return true;
}

template <typename ComponentType>
bool TryPythonObjectToSOAUnknownArrayComponent(nb::handle object,
                                               viskores::cont::UnknownArrayHandle& array)
{
  return TryPythonObjectToSOAUnknownArray<ComponentType, 2>(object, array) ||
    TryPythonObjectToSOAUnknownArray<ComponentType, 3>(object, array) ||
    TryPythonObjectToSOAUnknownArray<ComponentType, 4>(object, array);
}

template <typename ComponentType>
bool TryPythonObjectToRecombineVecUnknownArray(nb::handle object,
                                               viskores::cont::UnknownArrayHandle& array)
{
  using ArrayType = viskores::cont::ArrayHandleRecombineVec<ComponentType>;
  ArrayType* typedArray = nullptr;
  if (!nb::try_cast(object, typedArray))
  {
    return false;
  }

  array = viskores::cont::UnknownArrayHandle(*typedArray);
  return true;
}

bool TryPythonObjectToRegisteredArray(nb::handle object, viskores::cont::UnknownArrayHandle& array)
{
  viskores::cont::UnknownArrayHandle* unknownArray = nullptr;
  if (nb::try_cast(object, unknownArray))
  {
    array = *unknownArray;
    return true;
  }

  if (TryPythonObjectToSOAUnknownArrayComponent<viskores::Float32>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::Float64>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::Int8>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::UInt8>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::Int16>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::UInt16>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::Int32>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::UInt32>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::Int64>(object, array) ||
      TryPythonObjectToSOAUnknownArrayComponent<viskores::UInt64>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::Float32>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::Float64>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::Int8>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::UInt8>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::Int16>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::UInt16>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::Int32>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::UInt32>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::Int64>(object, array) ||
      TryPythonObjectToRecombineVecUnknownArray<viskores::UInt64>(object, array))
  {
    return true;
  }

  return false;
}

viskores::cont::UnknownArrayHandle PythonObjectToUnknownArray(nb::handle object)
{
  viskores::cont::UnknownArrayHandle array;
  if (TryPythonObjectToRegisteredArray(object, array))
  {
    return array;
  }
  return NumPyArrayToUnknownArray(object);
}

void ArrayCopyToPythonDestination(nb::handle sourceObject, nb::handle destination)
{
  viskores::cont::UnknownArrayHandle source;
  if (!TryPythonObjectToRegisteredArray(sourceObject, source))
  {
    source = NumPyArrayToUnknownArray(sourceObject);
  }

  if (TryArrayCopyToSOAComponent<viskores::Float32>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::Float64>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::Int8>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::UInt8>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::Int16>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::UInt16>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::Int32>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::UInt32>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::Int64>(source, destination) ||
      TryArrayCopyToSOAComponent<viskores::UInt64>(source, destination))
  {
    return;
  }

  throw std::runtime_error("ArrayCopy destination must be a supported ArrayHandleSOA class.");
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
bool TrySetCoordinateSystemDataSpecialized(viskores::cont::CoordinateSystem& coordinateSystem,
                                           nb::handle values)
{
  return TrySetCoordinateSystemDataSOA<ComponentType, 2>(coordinateSystem, values) ||
    TrySetCoordinateSystemDataSOA<ComponentType, 3>(coordinateSystem, values) ||
    TrySetCoordinateSystemDataSOA<ComponentType, 4>(coordinateSystem, values);
}

bool TrySetCoordinateSystemDataFromPythonObject(viskores::cont::CoordinateSystem& coordinateSystem,
                                                nb::handle values)
{
  if (TrySetCoordinateSystemDataSpecialized<viskores::Float32>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::Float64>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::Int8>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::UInt8>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::Int16>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::UInt16>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::Int32>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::UInt32>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::Int64>(coordinateSystem, values) ||
      TrySetCoordinateSystemDataSpecialized<viskores::UInt64>(coordinateSystem, values))
  {
    return true;
  }

  return false;
}

template <typename CoordinateComponentType>
bool TryCreateExplicitDataSetWithCoordinateType(
  const viskores::cont::UnknownArrayHandle& coords,
  const std::vector<viskores::UInt8>& shapes,
  const std::vector<viskores::IdComponent>& numIndices,
  const std::vector<viskores::Id>& connectivity,
  const std::string& coordName,
  nb::object& output)
{
  using CoordinateArrayType =
    viskores::cont::ArrayHandle<viskores::Vec<CoordinateComponentType, 3>>;
  if (!coords.CanConvert<CoordinateArrayType>())
  {
    return false;
  }

  CoordinateArrayType coordsArray;
  coords.AsArrayHandle(coordsArray);
  auto shapesArray = viskores::cont::make_ArrayHandle(shapes, viskores::CopyFlag::On);
  auto numIndicesArray = viskores::cont::make_ArrayHandle(numIndices, viskores::CopyFlag::On);
  auto connectivityArray = viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On);

  output = WrapDataSet(viskores::cont::DataSetBuilderExplicit::Create(
    coordsArray, shapesArray, numIndicesArray, connectivityArray, coordName));
  return true;
}

nb::object CreateExplicitDataSetFromPythonObjects(nb::handle coordsObject,
                                                  nb::handle shapesObject,
                                                  nb::handle numIndicesObject,
                                                  nb::handle connectivityObject,
                                                  const std::string& coordName)
{
  const auto shapes = ParseUInt8Sequence(shapesObject);
  const auto numIndices = ParseIdComponentSequence(numIndicesObject);
  const auto connectivity = ParseIdSequence(connectivityObject);

  viskores::cont::UnknownArrayHandle coords;
  if (TryPythonObjectToRegisteredArray(coordsObject, coords))
  {
    nb::object output;
    if (TryCreateExplicitDataSetWithCoordinateType<viskores::Float32>(
          coords, shapes, numIndices, connectivity, coordName, output) ||
        TryCreateExplicitDataSetWithCoordinateType<viskores::Float64>(
          coords, shapes, numIndices, connectivity, coordName, output))
    {
      return output;
    }
    throw std::runtime_error(
      "Explicit coordinates must have shape (N, 3) and dtype float32 or float64.");
  }

  try
  {
    coords = NumPyArrayToUnknownArray(coordsObject);
    nb::object output;
    if (TryCreateExplicitDataSetWithCoordinateType<viskores::Float32>(
          coords, shapes, numIndices, connectivity, coordName, output) ||
        TryCreateExplicitDataSetWithCoordinateType<viskores::Float64>(
          coords, shapes, numIndices, connectivity, coordName, output))
    {
      return output;
    }
    throw std::runtime_error(
      "Explicit coordinates must have shape (N, 3) and dtype float32 or float64.");
  }
  catch (const nb::cast_error&)
  {
  }

  return WrapDataSet(viskores::cont::DataSetBuilderExplicit::Create(
    ParseVec3Sequence(coordsObject), shapes, numIndices, connectivity, coordName));
}

template <typename CoordinateComponentType>
bool TryCreateCurvilinearDataSetWithCoordinateType(const viskores::cont::UnknownArrayHandle& coords,
                                                   const viskores::Id3& dimensions,
                                                   viskores::IdComponent dimensionRank,
                                                   const std::string& coordName,
                                                   nb::object& output)
{
  using CoordinateArrayType =
    viskores::cont::ArrayHandle<viskores::Vec<CoordinateComponentType, 3>>;
  if (!coords.CanConvert<CoordinateArrayType>())
  {
    return false;
  }

  CoordinateArrayType coordsArray;
  coords.AsArrayHandle(coordsArray);
  if (dimensionRank == 1)
  {
    output = WrapDataSet(viskores::cont::DataSetBuilderCurvilinear::Create(coordsArray, coordName));
  }
  else if (dimensionRank == 2)
  {
    output = WrapDataSet(viskores::cont::DataSetBuilderCurvilinear::Create(
      coordsArray, viskores::Id2(dimensions[0], dimensions[1]), coordName));
  }
  else if (dimensionRank == 3)
  {
    output = WrapDataSet(
      viskores::cont::DataSetBuilderCurvilinear::Create(coordsArray, dimensions, coordName));
  }
  else
  {
    throw std::runtime_error("dimensions must contain 1, 2, or 3 integers.");
  }
  return true;
}

nb::object CreateCurvilinearDataSetFromPythonObjects(nb::handle coordsObject,
                                                     nb::handle dimensionsObject,
                                                     const std::string& coordName)
{
  const auto dimensions = ParseDimensions(dimensionsObject);
  const auto dimensionRank = GetDimensionsRank(dimensionsObject);
  const auto coords = PythonObjectToUnknownArray(coordsObject);

  viskores::Id expectedPoints = 1;
  for (viskores::IdComponent index = 0; index < dimensionRank; ++index)
  {
    if (dimensions[index] < 1)
    {
      throw std::runtime_error("Curvilinear dimensions must be positive.");
    }
    expectedPoints *= dimensions[index];
  }
  if (coords.GetNumberOfValues() != expectedPoints)
  {
    throw std::runtime_error(
      "Curvilinear coordinate count must match the product of the requested dimensions.");
  }

  nb::object output;
  if (TryCreateCurvilinearDataSetWithCoordinateType<viskores::Float32>(
        coords, dimensions, dimensionRank, coordName, output) ||
      TryCreateCurvilinearDataSetWithCoordinateType<viskores::Float64>(
        coords, dimensions, dimensionRank, coordName, output))
  {
    return output;
  }

  throw std::runtime_error(
    "Curvilinear coordinates must have shape (N, 3) and dtype float32 or float64.");
}

} // namespace

void RegisterNanobindSharedDataClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("UnknownArrayHandle");
  nb::class_<viskores::cont::UnknownArrayHandle>(m, "UnknownArrayHandle")
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::cont::UnknownArrayHandle& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.UnknownArrayHandle(type=\"" << self.GetArrayTypeName()
                  << "\", values=" << self.GetNumberOfValues()
                  << ", components=" << self.GetNumberOfComponentsFlat() << ")";
           return stream.str();
         })
    .def("IsValid", &viskores::cont::UnknownArrayHandle::IsValid)
    .def("GetArrayTypeName", &viskores::cont::UnknownArrayHandle::GetArrayTypeName)
    .def("GetNumberOfValues", &viskores::cont::UnknownArrayHandle::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat",
         &viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat)
    .def(
      "AsNumPy",
      [](const viskores::cont::UnknownArrayHandle& self, bool copy)
      { return UnknownArrayToNumPyArray(self, copy); },
      nb::arg("copy") = true)
    .def("IsStorageTypeSOA",
         [](const viskores::cont::UnknownArrayHandle& self)
         { return self.IsStorageType<viskores::cont::StorageTagSOA>(); })
    .def("ExtractArrayFromComponents",
         [](const viskores::cont::UnknownArrayHandle& self)
         { return ExtractArrayFromComponentsObject(self); });

  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2f_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec2f_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2f_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec2f_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3f_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec3f_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3f_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec3f_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4f_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec4f_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4f_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec4f_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2i_8>>(
    m, erase_existing_name, "ArrayHandleSOAVec2i_8");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2i_16>>(
    m, erase_existing_name, "ArrayHandleSOAVec2i_16");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2i_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec2i_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2i_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec2i_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3i_8>>(
    m, erase_existing_name, "ArrayHandleSOAVec3i_8");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3i_16>>(
    m, erase_existing_name, "ArrayHandleSOAVec3i_16");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3i_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec3i_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3i_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec3i_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4i_8>>(
    m, erase_existing_name, "ArrayHandleSOAVec4i_8");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4i_16>>(
    m, erase_existing_name, "ArrayHandleSOAVec4i_16");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4i_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec4i_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4i_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec4i_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2ui_8>>(
    m, erase_existing_name, "ArrayHandleSOAVec2ui_8");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2ui_16>>(
    m, erase_existing_name, "ArrayHandleSOAVec2ui_16");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2ui_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec2ui_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec2ui_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec2ui_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3ui_8>>(
    m, erase_existing_name, "ArrayHandleSOAVec3ui_8");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3ui_16>>(
    m, erase_existing_name, "ArrayHandleSOAVec3ui_16");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3ui_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec3ui_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec3ui_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec3ui_64");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4ui_8>>(
    m, erase_existing_name, "ArrayHandleSOAVec4ui_8");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4ui_16>>(
    m, erase_existing_name, "ArrayHandleSOAVec4ui_16");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4ui_32>>(
    m, erase_existing_name, "ArrayHandleSOAVec4ui_32");
  RegisterArrayHandleSOAClass<viskores::cont::ArrayHandleSOA<viskores::Vec4ui_64>>(
    m, erase_existing_name, "ArrayHandleSOAVec4ui_64");

  erase_existing_name("ArrayHandleSOAVec3f");
  m.attr("ArrayHandleSOAVec3f") = std::is_same<viskores::FloatDefault, viskores::Float32>::value
    ? m.attr("ArrayHandleSOAVec3f_32")
    : m.attr("ArrayHandleSOAVec3f_64");

  RegisterArrayHandleRecombineVecClass<viskores::Float32>(
    m, erase_existing_name, "ArrayHandleRecombineVecFloat32");
  RegisterArrayHandleRecombineVecClass<viskores::Float64>(
    m, erase_existing_name, "ArrayHandleRecombineVecFloat64");
  RegisterArrayHandleRecombineVecClass<viskores::Int8>(
    m, erase_existing_name, "ArrayHandleRecombineVecInt8");
  RegisterArrayHandleRecombineVecClass<viskores::UInt8>(
    m, erase_existing_name, "ArrayHandleRecombineVecUInt8");
  RegisterArrayHandleRecombineVecClass<viskores::Int16>(
    m, erase_existing_name, "ArrayHandleRecombineVecInt16");
  RegisterArrayHandleRecombineVecClass<viskores::UInt16>(
    m, erase_existing_name, "ArrayHandleRecombineVecUInt16");
  RegisterArrayHandleRecombineVecClass<viskores::Int32>(
    m, erase_existing_name, "ArrayHandleRecombineVecInt32");
  RegisterArrayHandleRecombineVecClass<viskores::UInt32>(
    m, erase_existing_name, "ArrayHandleRecombineVecUInt32");
  RegisterArrayHandleRecombineVecClass<viskores::Int64>(
    m, erase_existing_name, "ArrayHandleRecombineVecInt64");
  RegisterArrayHandleRecombineVecClass<viskores::UInt64>(
    m, erase_existing_name, "ArrayHandleRecombineVecUInt64");

  erase_existing_name("ArrayHandleRecombineVecFloatDefault");
  m.attr("ArrayHandleRecombineVecFloatDefault") =
    std::is_same<viskores::FloatDefault, viskores::Float32>::value
    ? m.attr("ArrayHandleRecombineVecFloat32")
    : m.attr("ArrayHandleRecombineVecFloat64");

  erase_existing_name("array_from_numpy");
  m.attr("array_from_numpy") = nb::cpp_function([](nb::object values, bool copy)
                                                { return NumPyArrayToUnknownArray(values, copy); },
                                                nb::arg("values"),
                                                nb::arg("copy") = true);

  erase_existing_name("asnumpy");
  m.attr("asnumpy") = nb::cpp_function(
    [](nb::handle object, bool copy) -> nb::object
    {
      viskores::cont::UnknownArrayHandle array;
      if (TryPythonObjectToRegisteredArray(object, array))
      {
        return UnknownArrayToNumPyArray(array, copy);
      }

      viskores::cont::Field* field = nullptr;
      if (nb::try_cast(object, field))
      {
        return UnknownArrayToNumPyArray(field->GetData(), copy);
      }

      return nb::module_::import_("numpy").attr("asarray")(object);
    },
    nb::arg("object"),
    nb::arg("copy") = true);

  erase_existing_name("ArrayCopy");
  m.attr("ArrayCopy") = nb::cpp_function([](nb::handle source, nb::handle destination)
                                         { ArrayCopyToPythonDestination(source, destination); },
                                         nb::arg("source"),
                                         nb::arg("destination"));

  erase_existing_name("CellSet");
  nb::class_<viskores::cont::UnknownCellSet>(m, "CellSet")
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

  erase_existing_name("Field");
  nb::class_<viskores::cont::Field>(m, "Field")
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
  nb::class_<viskores::cont::CoordinateSystem, viskores::cont::Field>(m, "CoordinateSystem")
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
         { return BoundsTuple(self.GetBounds()); });

  erase_existing_name("DataSet");
  nb::class_<viskores::cont::DataSet>(m, "DataSet")
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

  erase_existing_name("DataSetBuilderExplicit");
  nb::class_<viskores::cont::DataSetBuilderExplicit>(m, "DataSetBuilderExplicit")
    .def(nb::init<>())
    .def_static(
      "Create",
      [](nb::object coordsObject,
         nb::object shapesObject,
         nb::object numIndicesObject,
         nb::object connectivityObject,
         const std::string& coordName)
      {
        return CreateExplicitDataSetFromPythonObjects(
          coordsObject, shapesObject, numIndicesObject, connectivityObject, coordName);
      },
      nb::arg("coords"),
      nb::arg("shapes"),
      nb::arg("num_indices"),
      nb::arg("connectivity"),
      nb::arg("coord_name") = "coords");

  erase_existing_name("DataSetBuilderCurvilinear");
  nb::class_<viskores::cont::DataSetBuilderCurvilinear>(m, "DataSetBuilderCurvilinear")
    .def(nb::init<>())
    .def_static(
      "Create",
      [](nb::object coordsObject, nb::object dimensionsObject, const std::string& coordName)
      {
        return CreateCurvilinearDataSetFromPythonObjects(coordsObject, dimensionsObject, coordName);
      },
      nb::arg("coords"),
      nb::arg("dimensions"),
      nb::arg("coord_name") = "coords");

  erase_existing_name("DataSetBuilderUniform");
  nb::class_<viskores::cont::DataSetBuilderUniform>(m, "DataSetBuilderUniform")
    .def(nb::init<>())
    .def_static(
      "Create",
      [](nb::object dimensions, nb::object origin, nb::object spacing, const std::string& coordName)
      {
        const auto parsedDimensions = ParseDimensions(dimensions);
        const auto parsedOrigin = ParseVec3(origin, viskores::Vec3f(0.0f, 0.0f, 0.0f));
        const auto parsedSpacing = ParseVec3(spacing, viskores::Vec3f(1.0f, 1.0f, 1.0f));
        return WrapDataSet(viskores::cont::DataSetBuilderUniform::Create(
          parsedDimensions, parsedOrigin, parsedSpacing, coordName));
      },
      nb::arg("dimensions"),
      nb::arg("origin") = nb::none(),
      nb::arg("spacing") = nb::none(),
      nb::arg("coord_name") = "coords");

  erase_existing_name("DataSetBuilderRectilinear");
  nb::class_<viskores::cont::DataSetBuilderRectilinear>(m, "DataSetBuilderRectilinear")
    .def(nb::init<>())
    .def_static(
      "Create",
      [](nb::object xObject, nb::object yObject, nb::object zObject, const std::string& coordName)
      {
        auto actualCoordName = coordName;
        if (nb::isinstance<nb::str>(yObject) && zObject.is_none() && coordName == "coords")
        {
          actualCoordName = nb::cast<std::string>(yObject);
          yObject = nb::none();
        }
        if (nb::isinstance<nb::str>(zObject) && coordName == "coords")
        {
          actualCoordName = nb::cast<std::string>(zObject);
          zObject = nb::none();
        }

        auto x = ParseRectilinearAxis(xObject, "x");
        if (yObject.is_none())
        {
          return WrapDataSet(viskores::cont::DataSetBuilderRectilinear::Create(x, actualCoordName));
        }

        auto y = ParseRectilinearAxis(yObject, "y");
        if (zObject.is_none())
        {
          return WrapDataSet(
            viskores::cont::DataSetBuilderRectilinear::Create(x, y, actualCoordName));
        }

        auto z = ParseRectilinearAxis(zObject, "z");
        return WrapDataSet(
          viskores::cont::DataSetBuilderRectilinear::Create(x, y, z, actualCoordName));
      },
      nb::arg("x"),
      nb::arg("y") = nb::none(),
      nb::arg("z") = nb::none(),
      nb::arg("coord_name") = "coords");

  erase_existing_name("DataSetBuilderExplicitIterative");
  nb::class_<viskores::cont::DataSetBuilderExplicitIterative>(m, "DataSetBuilderExplicitIterative")
    .def(nb::init<>())
    .def("Begin",
         &viskores::cont::DataSetBuilderExplicitIterative::Begin,
         nb::arg("coord_name") = "coords")
    .def(
      "AddPoint",
      [](viskores::cont::DataSetBuilderExplicitIterative& self, nb::object pointObject)
      { return self.AddPoint(ParseVec3(pointObject, viskores::Vec3f(0.0f, 0.0f, 0.0f))); },
      nb::arg("point"))
    .def(
      "AddCell",
      [](viskores::cont::DataSetBuilderExplicitIterative& self,
         unsigned long shape,
         nb::object connectivityObject)
      { self.AddCell(static_cast<viskores::UInt8>(shape), ParseIdSequence(connectivityObject)); },
      nb::arg("shape"),
      nb::arg("connectivity"))
    .def("AddCellPoint",
         &viskores::cont::DataSetBuilderExplicitIterative::AddCellPoint,
         nb::arg("point_index"))
    .def("Create",
         [](viskores::cont::DataSetBuilderExplicitIterative& self)
         { return WrapDataSet(self.Create()); });

  erase_existing_name("PartitionedDataSet");
  nb::class_<viskores::cont::PartitionedDataSet>(m, "PartitionedDataSet")
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
    nb::arg("values"));

  erase_existing_name("make_FieldPoint");
  m.attr("make_FieldPoint") = nb::cpp_function(
    [](const std::string& name, nb::object values)
    { return viskores::cont::make_FieldPoint(name, PythonObjectToUnknownArray(values)); },
    nb::arg("name"),
    nb::arg("values"));

  erase_existing_name("make_FieldCell");
  m.attr("make_FieldCell") = nb::cpp_function(
    [](const std::string& name, nb::object values)
    { return viskores::cont::make_FieldCell(name, PythonObjectToUnknownArray(values)); },
    nb::arg("name"),
    nb::arg("values"));

  erase_existing_name("make_FieldWholeDataSet");
  m.attr("make_FieldWholeDataSet") = nb::cpp_function(
    [](const std::string& name, nb::object values)
    {
      return viskores::cont::Field(
        name, viskores::cont::Field::Association::WholeDataSet, PythonObjectToUnknownArray(values));
    },
    nb::arg("name"),
    nb::arg("values"));
}

} // namespace viskores::python::bindings
