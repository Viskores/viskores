//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "shared_data_helpers.h"

#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

namespace
{

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

struct TryCreateExplicitDataSetFunctor
{
  const viskores::cont::UnknownArrayHandle& Coords;
  const std::vector<viskores::UInt8>& Shapes;
  const std::vector<viskores::IdComponent>& NumIndices;
  const std::vector<viskores::Id>& Connectivity;
  const std::string& CoordName;
  nb::object& Output;

  template <typename CoordinateComponentType>
  bool operator()() const
  {
    return TryCreateExplicitDataSetWithCoordinateType<CoordinateComponentType>(
      this->Coords, this->Shapes, this->NumIndices, this->Connectivity, this->CoordName, this->Output);
  }
};

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
    if (TryRegisteredFloatTypes(
          TryCreateExplicitDataSetFunctor{ coords, shapes, numIndices, connectivity, coordName, output }))
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
    if (TryRegisteredFloatTypes(
          TryCreateExplicitDataSetFunctor{ coords, shapes, numIndices, connectivity, coordName, output }))
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

struct TryCreateCurvilinearDataSetFunctor
{
  const viskores::cont::UnknownArrayHandle& Coords;
  const viskores::Id3& Dimensions;
  viskores::IdComponent DimensionRank;
  const std::string& CoordName;
  nb::object& Output;

  template <typename CoordinateComponentType>
  bool operator()() const
  {
    return TryCreateCurvilinearDataSetWithCoordinateType<CoordinateComponentType>(
      this->Coords, this->Dimensions, this->DimensionRank, this->CoordName, this->Output);
  }
};

nb::object CreateCurvilinearDataSetFromPythonObjects(nb::handle coordsObject,
                                                     nb::handle dimensionsObject,
                                                     const std::string& coordName)
{
  const auto [dimensions, dimensionRank] = ParseDimensionsAndRank(dimensionsObject);
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
  if (TryRegisteredFloatTypes(
        TryCreateCurvilinearDataSetFunctor{ coords, dimensions, dimensionRank, coordName, output }))
  {
    return output;
  }

  throw std::runtime_error(
    "Curvilinear coordinates must have shape (N, 3) and dtype float32 or float64.");
}

} // namespace

void RegisterNanobindDataSetBuilderClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("DataSetBuilderExplicit");
  nb::class_<viskores::cont::DataSetBuilderExplicit>(
    m, "DataSetBuilderExplicit", doc::ClassDoc("DataSetBuilderExplicit"))
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
  nb::class_<viskores::cont::DataSetBuilderCurvilinear>(
    m, "DataSetBuilderCurvilinear", doc::ClassDoc("DataSetBuilderCurvilinear"))
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
  nb::class_<viskores::cont::DataSetBuilderUniform>(
    m, "DataSetBuilderUniform", doc::ClassDoc("DataSetBuilderUniform"))
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
  nb::class_<viskores::cont::DataSetBuilderRectilinear>(
    m, "DataSetBuilderRectilinear", doc::ClassDoc("DataSetBuilderRectilinear"))
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
  nb::class_<viskores::cont::DataSetBuilderExplicitIterative>(
    m, "DataSetBuilderExplicitIterative", doc::ClassDoc("DataSetBuilderExplicitIterative"))
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
}

} // namespace viskores::python::bindings
