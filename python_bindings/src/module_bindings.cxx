//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <nanobind/operators.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <iomanip>

namespace viskores::python::bindings
{

namespace
{

std::string DeviceAdapterIdRepr(const viskores::cont::DeviceAdapterId& device)
{
  std::ostringstream stream;
  stream << "viskores.cont.DeviceAdapterId(\"" << device.GetName() << "\")";
  return stream.str();
}

viskores::cont::InitializeResult InitializeFromArgv(
  nb::list argv,
  viskores::cont::InitializeOptions opts = viskores::cont::InitializeOptions::None)
{
  if (nb::len(argv) == 0)
  {
    throw std::runtime_error("argv must include the program name as argv[0].");
  }

  std::vector<std::string> storage;
  storage.reserve(static_cast<size_t>(nb::len(argv)));
  for (nb::handle value : argv)
  {
    storage.push_back(nb::cast<std::string>(value));
  }

  std::vector<char*> rawArgv;
  rawArgv.reserve(storage.size());
  for (std::string& arg : storage)
  {
    rawArgv.push_back(arg.data());
  }

  int argc = static_cast<int>(rawArgv.size());
  auto result = viskores::cont::Initialize(argc, rawArgv.data(), opts);

  argv.attr("clear")();
  for (int index = 0; index < argc; ++index)
  {
    argv.append(nb::str(rawArgv[static_cast<size_t>(index)]));
  }

  return result;
}

void RegisterNanobindInitializeClasses(nb::module_& m,
                                       const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("DeviceAdapterId");
  nb::class_<viskores::cont::DeviceAdapterId>(m, "DeviceAdapterId")
    .def("__init__",
         [](viskores::cont::DeviceAdapterId* self, int value) {
           new (self)
             viskores::cont::DeviceAdapterId(
               viskores::cont::make_DeviceAdapterId(static_cast<viskores::Int8>(value)));
         },
         nb::arg("value"))
    .def("__init__",
         [](viskores::cont::DeviceAdapterId* self, const std::string& name) {
           new (self) viskores::cont::DeviceAdapterId(viskores::cont::make_DeviceAdapterId(name));
         },
         nb::arg("name"))
    .def("__repr__", &DeviceAdapterIdRepr)
    .def("IsValueValid", &viskores::cont::DeviceAdapterId::IsValueValid)
    .def("GetValue", &viskores::cont::DeviceAdapterId::GetValue)
    .def("GetName", &viskores::cont::DeviceAdapterId::GetName)
    .def(nb::self == nb::self)
    .def(nb::self != nb::self)
    .def(nb::self < nb::self);

  erase_existing_name("InitializeResult");
  nb::class_<viskores::cont::InitializeResult>(m, "InitializeResult")
    .def(nb::init<>())
    .def_rw("Device", &viskores::cont::InitializeResult::Device)
    .def_rw("Usage", &viskores::cont::InitializeResult::Usage)
    .def("__repr__",
         [](const viskores::cont::InitializeResult& self) {
           std::ostringstream stream;
           stream << "viskores.cont.InitializeResult(Device=" << DeviceAdapterIdRepr(self.Device)
                  << ", Usage=" << std::quoted(self.Usage) << ")";
           return stream.str();
         });

  erase_existing_name("InitializeOptions");
  nb::enum_<viskores::cont::InitializeOptions>(m,
                                               "InitializeOptions",
                                               nb::is_arithmetic(),
                                               nb::is_flag())
    .value("None_", viskores::cont::InitializeOptions::None)
    .value("RequireDevice", viskores::cont::InitializeOptions::RequireDevice)
    .value("DefaultAnyDevice", viskores::cont::InitializeOptions::DefaultAnyDevice)
    .value("AddHelp", viskores::cont::InitializeOptions::AddHelp)
    .value("ErrorOnBadOption", viskores::cont::InitializeOptions::ErrorOnBadOption)
    .value("ErrorOnBadArgument", viskores::cont::InitializeOptions::ErrorOnBadArgument)
    .value("Strict", viskores::cont::InitializeOptions::Strict);

  erase_existing_name("Initialize");
  m.def("Initialize", []() { return viskores::cont::Initialize(); });
  m.def("Initialize",
        &InitializeFromArgv,
        nb::arg("argv"),
        nb::arg("opts") = viskores::cont::InitializeOptions::None,
        nb::sig("def Initialize(argv: list[str], opts: InitializeOptions = "
                "InitializeOptions.None_) -> InitializeResult"));

  erase_existing_name("IsInitialized");
  m.def("IsInitialized", &viskores::cont::IsInitialized);

  erase_existing_name("make_DeviceAdapterId");
  m.def("make_DeviceAdapterId",
        nb::overload_cast<const viskores::cont::DeviceAdapterNameType&>(
          &viskores::cont::make_DeviceAdapterId),
        nb::arg("name"));
  m.def("make_DeviceAdapterId",
        nb::overload_cast<viskores::Int8>(&viskores::cont::make_DeviceAdapterId),
        nb::arg("value"));
}

void RegisterNanobindRangeClass(nb::module_& m, const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Range");
  nb::class_<viskores::Range>(m, "Range")
    .def(nb::init<>())
    .def(nb::init<viskores::Float64, viskores::Float64>(), nb::arg("min"), nb::arg("max"))
    .def_rw("Min", &viskores::Range::Min)
    .def_rw("Max", &viskores::Range::Max)
    .def("__repr__",
         [](const viskores::Range& self) {
           std::ostringstream stream;
           stream << "viskores.Range(" << self.Min << ", " << self.Max << ")";
           return stream.str();
         })
    .def("IsNonEmpty", &viskores::Range::IsNonEmpty)
    .def("Contains", [](const viskores::Range& self, double value) { return self.Contains(value); })
    .def("Length", &viskores::Range::Length)
    .def("Center", &viskores::Range::Center)
    .def("Include",
         [](viskores::Range& self, nb::object valueOrRange) {
           if (nb::isinstance<viskores::Range>(valueOrRange))
           {
             self.Include(nb::cast<viskores::Range>(valueOrRange));
           }
           else
           {
             self.Include(nb::cast<double>(valueOrRange));
           }
         },
         nb::arg("value_or_range"))
    .def("Union", &viskores::Range::Union, nb::arg("other"))
    .def("Intersection", &viskores::Range::Intersection, nb::arg("other"))
    .def("__add__", &viskores::Range::operator+)
    .def(nb::self == nb::self)
    .def(nb::self != nb::self);
}

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
void RegisterNanobindImplicitFunctionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Box");
  nb::class_<viskores::Box>(m, "Box")
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::Box* self, nb::object minPointObject, nb::object maxPointObject)
      {
        if (minPointObject.is_none() || maxPointObject.is_none())
        {
          throw std::runtime_error(
            "Box requires both min_point and max_point when either is provided.");
        }
        new (self) viskores::Box(ParseVec3(minPointObject,
                                           viskores::Vec3f(-0.5f, -0.5f, -0.5f)),
                                 ParseVec3(maxPointObject,
                                           viskores::Vec3f(0.5f, 0.5f, 0.5f)));
      },
      nb::arg("min_point"),
      nb::arg("max_point"))
    .def("SetMinPoint",
         [](viskores::Box& self, nb::object pointObject)
         { self.SetMinPoint(ParseVec3(pointObject, self.GetMinPoint())); },
         nb::arg("point"))
    .def("GetMinPoint",
         [](const viskores::Box& self)
         { return nb::make_tuple(self.GetMinPoint()[0], self.GetMinPoint()[1], self.GetMinPoint()[2]); })
    .def("SetMaxPoint",
         [](viskores::Box& self, nb::object pointObject)
         { self.SetMaxPoint(ParseVec3(pointObject, self.GetMaxPoint())); },
         nb::arg("point"))
    .def("GetMaxPoint",
         [](const viskores::Box& self)
         { return nb::make_tuple(self.GetMaxPoint()[0], self.GetMaxPoint()[1], self.GetMaxPoint()[2]); });

  erase_existing_name("Sphere");
  nb::class_<viskores::Sphere>(m, "Sphere")
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::Sphere* self, double radius)
      { new (self) viskores::Sphere(static_cast<viskores::FloatDefault>(radius)); },
      nb::arg("radius"))
    .def(
      "__init__",
      [](viskores::Sphere* self, nb::object centerObject, double radius)
      {
        auto center = centerObject.is_none()
                        ? viskores::Vec3f(0.0f, 0.0f, 0.0f)
                        : ParseVec3(centerObject, viskores::Vec3f(0.0f, 0.0f, 0.0f));
        new (self) viskores::Sphere(center, static_cast<viskores::FloatDefault>(radius));
      },
      nb::arg("center") = nb::none(),
      nb::arg("radius") = 0.5)
    .def("SetCenter",
         [](viskores::Sphere& self, nb::object centerObject)
         { self.SetCenter(ParseVec3(centerObject, self.GetCenter())); },
         nb::arg("center"))
    .def("GetCenter",
         [](const viskores::Sphere& self)
         { return nb::make_tuple(self.GetCenter()[0], self.GetCenter()[1], self.GetCenter()[2]); })
    .def("SetRadius",
         [](viskores::Sphere& self, double radius)
         { self.SetRadius(static_cast<viskores::FloatDefault>(radius)); },
         nb::arg("radius"))
    .def("GetRadius", &viskores::Sphere::GetRadius);

  erase_existing_name("Cylinder");
  nb::class_<viskores::Cylinder>(m, "Cylinder")
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::Cylinder* self, nb::object axisObject, double radius)
      {
        auto axis = axisObject.is_none() ? viskores::Vec3f(0.0f, 1.0f, 0.0f)
                                         : ParseVec3(axisObject, viskores::Vec3f(0.0f, 1.0f, 0.0f));
        new (self) viskores::Cylinder(axis, static_cast<viskores::FloatDefault>(radius));
      },
      nb::arg("axis"),
      nb::arg("radius"))
    .def(
      "__init__",
      [](viskores::Cylinder* self, nb::object centerObject, nb::object axisObject, double radius)
      {
        auto center = centerObject.is_none()
                        ? viskores::Vec3f(0.0f, 0.0f, 0.0f)
                        : ParseVec3(centerObject, viskores::Vec3f(0.0f, 0.0f, 0.0f));
        auto axis = axisObject.is_none() ? viskores::Vec3f(0.0f, 1.0f, 0.0f)
                                         : ParseVec3(axisObject, viskores::Vec3f(0.0f, 1.0f, 0.0f));
        new (self) viskores::Cylinder(center, axis, static_cast<viskores::FloatDefault>(radius));
      },
      nb::arg("center") = nb::none(),
      nb::arg("axis") = nb::none(),
      nb::arg("radius") = 0.5)
    .def("SetCenter",
         [](viskores::Cylinder& self, nb::object centerObject)
         { self.SetCenter(ParseVec3(centerObject, viskores::Vec3f(0.0f, 0.0f, 0.0f))); },
         nb::arg("center"))
    .def("SetAxis",
         [](viskores::Cylinder& self, nb::object axisObject)
         { self.SetAxis(ParseVec3(axisObject, viskores::Vec3f(0.0f, 1.0f, 0.0f))); },
         nb::arg("axis"))
    .def("SetRadius",
         [](viskores::Cylinder& self, double radius)
         { self.SetRadius(static_cast<viskores::FloatDefault>(radius)); },
         nb::arg("radius"));

  erase_existing_name("Plane");
  nb::class_<viskores::Plane>(m, "Plane")
    .def(nb::init<>())
    .def(
      "__init__",
      [](viskores::Plane* self, nb::object normalObject)
      {
        auto normal = normalObject.is_none() ? viskores::Vec3f(0.0f, 0.0f, 1.0f)
                                             : ParseVec3(normalObject, viskores::Vec3f(0.0f, 0.0f, 1.0f));
        new (self) viskores::Plane(normal);
      },
      nb::arg("normal"))
    .def(
      "__init__",
      [](viskores::Plane* self, nb::object originObject, nb::object normalObject)
      {
        auto origin = originObject.is_none() ? viskores::Vec3f(0.0f, 0.0f, 0.0f)
                                             : ParseVec3(originObject, viskores::Vec3f(0.0f, 0.0f, 0.0f));
        auto normal = normalObject.is_none() ? viskores::Vec3f(0.0f, 0.0f, 1.0f)
                                             : ParseVec3(normalObject, viskores::Vec3f(0.0f, 0.0f, 1.0f));
        new (self) viskores::Plane(origin, normal);
      },
      nb::arg("origin") = nb::none(),
      nb::arg("normal") = nb::none())
    .def("SetOrigin",
         [](viskores::Plane& self, nb::object pointObject)
         { self.SetOrigin(ParseVec3(pointObject, self.GetOrigin())); },
         nb::arg("point"))
    .def("GetOrigin",
         [](const viskores::Plane& self)
         { return nb::make_tuple(self.GetOrigin()[0], self.GetOrigin()[1], self.GetOrigin()[2]); })
    .def("SetNormal",
         [](viskores::Plane& self, nb::object pointObject)
         { self.SetNormal(ParseVec3(pointObject, self.GetNormal())); },
         nb::arg("point"))
    .def("GetNormal",
         [](const viskores::Plane& self)
         { return nb::make_tuple(self.GetNormal()[0], self.GetNormal()[1], self.GetNormal()[2]); });
}
#else
void RegisterNanobindImplicitFunctionClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION || VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS || \
  VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
void RegisterNanobindCompatibilityFunctions(nb::module_& m,
                                            const std::function<void(const char*)>& erase_existing_name)
{
#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION
  erase_existing_name("cell_average");
  m.attr("cell_average") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName, const char* outputFieldName) {
      viskores::filter::field_conversion::CellAverage filter;
      filter.SetActiveField(fieldName);
      if (outputFieldName != nullptr)
      {
        filter.SetOutputFieldName(outputFieldName);
      }
      return ExecuteFilterOnPythonDataObject<viskores::filter::field_conversion::CellAverage>(
        filter, datasetObject);
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("output_field_name") = nb::none());

  erase_existing_name("point_average");
  m.attr("point_average") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName, const char* outputFieldName) {
      viskores::filter::field_conversion::PointAverage filter;
      filter.SetActiveField(fieldName);
      if (outputFieldName != nullptr)
      {
        filter.SetOutputFieldName(outputFieldName);
      }
      return ExecuteFilterOnPythonDataObject<viskores::filter::field_conversion::PointAverage>(
        filter, datasetObject);
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("output_field_name") = nb::none());
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS
  erase_existing_name("vector_magnitude");
  m.attr("vector_magnitude") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName, const char* outputFieldName) {
      viskores::filter::vector_analysis::VectorMagnitude filter;
      filter.SetActiveField(fieldName);
      if (outputFieldName != nullptr)
      {
        filter.SetOutputFieldName(outputFieldName);
      }
      return ExecuteFilterOnPythonDataObject<viskores::filter::vector_analysis::VectorMagnitude>(
        filter, datasetObject);
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("output_field_name") = nb::none());

  erase_existing_name("gradient");
  m.attr("gradient") = nb::cpp_function(
    [](nb::object datasetObject,
       const char* fieldName,
       const char* outputFieldName,
       bool computePointGradient,
       bool computeDivergence,
       const char* divergenceName,
       bool computeVorticity,
       const char* vorticityName,
       bool computeQCriterion,
       const char* qCriterionName,
       bool computeGradient,
       bool rowMajorOrdering) {
      viskores::filter::vector_analysis::Gradient filter;
      filter.SetActiveField(fieldName);
      if (outputFieldName != nullptr)
      {
        filter.SetOutputFieldName(outputFieldName);
      }
      filter.SetComputePointGradient(computePointGradient);
      filter.SetComputeDivergence(computeDivergence);
      if (divergenceName != nullptr)
      {
        filter.SetDivergenceName(divergenceName);
      }
      filter.SetComputeVorticity(computeVorticity);
      if (vorticityName != nullptr)
      {
        filter.SetVorticityName(vorticityName);
      }
      filter.SetComputeQCriterion(computeQCriterion);
      if (qCriterionName != nullptr)
      {
        filter.SetQCriterionName(qCriterionName);
      }
      filter.SetComputeGradient(computeGradient);
      if (rowMajorOrdering)
      {
        filter.SetRowMajorOrdering();
      }
      else
      {
        filter.SetColumnMajorOrdering();
      }
      return ExecuteFilterOnPythonDataObject<viskores::filter::vector_analysis::Gradient>(
        filter, datasetObject);
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("output_field_name") = nb::none(),
    nb::arg("compute_point_gradient") = false,
    nb::arg("compute_divergence") = false,
    nb::arg("divergence_name") = nb::none(),
    nb::arg("compute_vorticity") = false,
    nb::arg("vorticity_name") = nb::none(),
    nb::arg("compute_qcriterion") = false,
    nb::arg("qcriterion_name") = nb::none(),
    nb::arg("compute_gradient") = true,
    nb::arg("row_major_ordering") = false);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
  erase_existing_name("contour");
  m.attr("contour") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName, nb::object isoValuesObject, bool generateNormals) {
      viskores::filter::contour::Contour filter;
      filter.SetActiveField(fieldName);
      filter.SetIsoValues(ParseIsoValues(isoValuesObject));
      filter.SetGenerateNormals(generateNormals);
      return ExecuteFilterOnPythonDataObject<viskores::filter::contour::Contour>(filter,
                                                                                  datasetObject);
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("iso_values"),
    nb::arg("generate_normals") = false);
#endif

}
#else
void RegisterNanobindCompatibilityFunctions(nb::module_&, const std::function<void(const char*)>&) {}
#endif

void RegisterNanobindModuleConstants(nb::module_& m)
{
  using Association = viskores::cont::Field::Association;
  m.attr("ASSOCIATION_ANY") = nb::int_(static_cast<int>(Association::Any));
  m.attr("ASSOCIATION_WHOLE_DATASET") = nb::int_(static_cast<int>(Association::WholeDataSet));
  m.attr("ASSOCIATION_POINTS") = nb::int_(static_cast<int>(Association::Points));
  m.attr("ASSOCIATION_CELLS") = nb::int_(static_cast<int>(Association::Cells));
  m.attr("ASSOCIATION_PARTITIONS") = nb::int_(static_cast<int>(Association::Partitions));
  m.attr("ASSOCIATION_GLOBAL") = nb::int_(static_cast<int>(Association::Global));
  m.attr("CELL_SHAPE_VERTEX") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_VERTEX));
  m.attr("CELL_SHAPE_LINE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_LINE));
  m.attr("CELL_SHAPE_POLY_LINE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_POLY_LINE));
  m.attr("CELL_SHAPE_TRIANGLE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_TRIANGLE));
  m.attr("CELL_SHAPE_QUAD") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_QUAD));
  m.attr("CELL_SHAPE_POLYGON") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_POLYGON));
  m.attr("CELL_SHAPE_TETRA") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_TETRA));
  m.attr("CELL_SHAPE_HEXAHEDRON") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_HEXAHEDRON));
  m.attr("CELL_SHAPE_WEDGE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_WEDGE));
  m.attr("CELL_SHAPE_PYRAMID") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_PYRAMID));

#if VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO
  m.attr("INTEGRATION_TYPE_NONE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::IntegrationType::None));
  m.attr("INTEGRATION_TYPE_ARC_LENGTH") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::IntegrationType::ArcLength));
  m.attr("INTEGRATION_TYPE_AREA") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::IntegrationType::Area));
  m.attr("INTEGRATION_TYPE_VOLUME") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::IntegrationType::Volume));
  m.attr("INTEGRATION_TYPE_ALL_MEASURES") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::IntegrationType::AllMeasures));
  m.attr("CELL_METRIC_AREA") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Area));
  m.attr("CELL_METRIC_ASPECT_GAMMA") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::AspectGamma));
  m.attr("CELL_METRIC_ASPECT_RATIO") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::AspectRatio));
  m.attr("CELL_METRIC_CONDITION") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Condition));
  m.attr("CELL_METRIC_DIAGONAL_RATIO") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::DiagonalRatio));
  m.attr("CELL_METRIC_DIMENSION") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Dimension));
  m.attr("CELL_METRIC_JACOBIAN") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Jacobian));
  m.attr("CELL_METRIC_MAX_ANGLE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::MaxAngle));
  m.attr("CELL_METRIC_MAX_DIAGONAL") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::MaxDiagonal));
  m.attr("CELL_METRIC_MIN_ANGLE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::MinAngle));
  m.attr("CELL_METRIC_MIN_DIAGONAL") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::MinDiagonal));
  m.attr("CELL_METRIC_ODDY") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Oddy));
  m.attr("CELL_METRIC_RELATIVE_SIZE_SQUARED") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::RelativeSizeSquared));
  m.attr("CELL_METRIC_SCALED_JACOBIAN") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::ScaledJacobian));
  m.attr("CELL_METRIC_SHAPE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Shape));
  m.attr("CELL_METRIC_SHAPE_AND_SIZE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::ShapeAndSize));
  m.attr("CELL_METRIC_SHEAR") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Shear));
  m.attr("CELL_METRIC_SKEW") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Skew));
  m.attr("CELL_METRIC_STRETCH") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Stretch));
  m.attr("CELL_METRIC_TAPER") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Taper));
  m.attr("CELL_METRIC_VOLUME") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Volume));
  m.attr("CELL_METRIC_WARPAGE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::Warpage));
  m.attr("CELL_METRIC_NONE") =
    nb::int_(static_cast<int>(viskores::filter::mesh_info::CellMetric::None));
#endif

#if VISKORES_PYTHON_ENABLE_IO
  m.attr("FILE_TYPE_ASCII") = nb::int_(static_cast<int>(viskores::io::FileType::ASCII));
  m.attr("FILE_TYPE_BINARY") = nb::int_(static_cast<int>(viskores::io::FileType::BINARY));
  m.attr("PIXEL_DEPTH_8") =
    nb::int_(static_cast<int>(viskores::io::ImageWriterBase::PixelDepth::PIXEL_8));
  m.attr("PIXEL_DEPTH_16") =
    nb::int_(static_cast<int>(viskores::io::ImageWriterBase::PixelDepth::PIXEL_16));
#endif
}

void RegisterNanobindModule(nb::module_& m)
{
  auto erase_existing_name = [&](const char* name) {
    nb::dict module_dict = nb::borrow<nb::dict>(m.attr("__dict__"));
    module_dict.attr("pop")(name, nb::none());
  };

  RegisterNanobindInitializeClasses(m, erase_existing_name);
  RegisterNanobindRangeClass(m, erase_existing_name);
  RegisterNanobindSharedDataClasses(m, erase_existing_name);

#if VISKORES_PYTHON_ENABLE_RENDERING
  RegisterNanobindColorTableClass(m, erase_existing_name);
  RegisterNanobindCameraClass(m, erase_existing_name);
  RegisterNanobindRenderingClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
  RegisterNanobindTestingClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
  RegisterNanobindImplicitFunctionClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_SOURCE
  RegisterNanobindSourceClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION
  RegisterNanobindFieldConversionClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS
  RegisterNanobindVectorAnalysisClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
  RegisterNanobindContourClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_IO
  RegisterNanobindIOClasses(m, erase_existing_name);
#endif
#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM
  RegisterNanobindFieldTransformClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_DENSITY_ESTIMATE
  RegisterNanobindDensityEstimateClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_IMAGE_PROCESSING
  RegisterNanobindImageProcessingClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
  RegisterNanobindEntityExtractionClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID || VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO || \
  VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS || VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK
  RegisterNanobindAdditionalFilterClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_GEOMETRY_REFINEMENT
  RegisterNanobindGeometryRefinementClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_RESAMPLING
  RegisterNanobindResamplingClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
  RegisterNanobindScalarTopologyClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
  RegisterNanobindInteropClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP || VISKORES_PYTHON_ENABLE_TESTING_UTILS
  RegisterNanobindHelperFunctions(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION || VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS || \
  VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
  RegisterNanobindCompatibilityFunctions(m, erase_existing_name);
#endif

  erase_existing_name("create_uniform_dataset");
  m.attr("create_uniform_dataset") = nb::cpp_function(
    [](nb::object dimensions, nb::object origin, nb::object spacing, const char* coordName) {
      const auto parsedDimensions = ParseDimensions(dimensions);
      const auto parsedOrigin = ParseVec3(origin, viskores::Vec3f(0.0f, 0.0f, 0.0f));
      const auto parsedSpacing = ParseVec3(spacing, viskores::Vec3f(1.0f, 1.0f, 1.0f));
      auto dataSet = viskores::cont::DataSetBuilderUniform::Create(
        parsedDimensions, parsedOrigin, parsedSpacing, coordName);
      return WrapDataSet(dataSet);
    },
    nb::arg("dimensions"),
    nb::arg("origin") = nb::none(),
    nb::arg("spacing") = nb::none(),
    nb::arg("coord_name") = "coords");

  erase_existing_name("field_range");
  m.attr("field_range") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName) {
      auto dataset = RequireDataSet(datasetObject);
      if (!dataset)
      {
        throw nb::python_error();
      }
      const auto range = dataset->GetField(fieldName).GetRange().ReadPortal().Get(0);
      return nb::make_tuple(range.Min, range.Max);
    },
    nb::arg("dataset"),
    nb::arg("field_name"));

#if VISKORES_PYTHON_ENABLE_SOURCE
  erase_existing_name("create_tangle_dataset");
  m.attr("create_tangle_dataset") = nb::cpp_function(
    [](nb::object pointDimensions) {
      viskores::source::Tangle tangle;
      if (!pointDimensions.is_none())
      {
        tangle.SetPointDimensions(ParseDimensions(pointDimensions));
      }
      return WrapDataSet(tangle.Execute());
    },
    nb::arg("point_dimensions") = nb::none());
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CORE && VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
  erase_existing_name("partition_uniform_dataset");
  m.attr("partition_uniform_dataset") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName, long long numBlocks) {
      auto dataset = RequireDataSet(datasetObject);
      if (!dataset)
      {
        throw nb::python_error();
      }

      viskores::Id3 globalSize;
      dataset->GetCellSet()
        .CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
          viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);

      const viskores::Id3 blocksPerDim =
        ComputeNumberOfBlocksPerAxis(globalSize, static_cast<viskores::Id>(numBlocks));
      viskores::cont::PartitionedDataSet partitions;
      viskores::cont::ArrayHandle<viskores::Id3> localBlockIndices;
      localBlockIndices.Allocate(static_cast<viskores::Id>(numBlocks));
      auto portal = localBlockIndices.WritePortal();

      for (viskores::Id blockNo = 0; blockNo < static_cast<viskores::Id>(numBlocks); ++blockNo)
      {
        auto [blockIndex, blockOrigin, blockSize] =
          ComputeBlockExtents(globalSize, blocksPerDim, blockNo);
        partitions.AppendPartition(
          CreateSubDataSet(*dataset, blockOrigin, blockSize, fieldName));
        portal.Set(blockNo, blockIndex);
      }

      std::vector<viskores::Id3> blocksPerDimVector{ blocksPerDim };
      return nb::make_tuple(
        WrapPartitionedDataSet(partitions),
        CreateId3ArrayObject(
          viskores::cont::make_ArrayHandle(blocksPerDimVector, viskores::CopyFlag::On)),
        CreateId3ArrayObject(localBlockIndices));
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("num_blocks"));
#endif
}

} // namespace viskores::python::bindings
