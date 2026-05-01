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
  nb::class_<viskores::cont::DeviceAdapterId>(
    m, "DeviceAdapterId", doc::ClassDoc("DeviceAdapterId"))
    .def(
      "__init__",
      [](viskores::cont::DeviceAdapterId* self, int value)
      {
        new (self) viskores::cont::DeviceAdapterId(
          viskores::cont::make_DeviceAdapterId(static_cast<viskores::Int8>(value)));
      },
      nb::arg("value"))
    .def(
      "__init__",
      [](viskores::cont::DeviceAdapterId* self, const std::string& name)
      { new (self) viskores::cont::DeviceAdapterId(viskores::cont::make_DeviceAdapterId(name)); },
      nb::arg("name"))
    .def("__repr__", &DeviceAdapterIdRepr)
    .def("IsValueValid", &viskores::cont::DeviceAdapterId::IsValueValid)
    .def("GetValue", &viskores::cont::DeviceAdapterId::GetValue)
    .def("GetName", &viskores::cont::DeviceAdapterId::GetName)
    .def(nb::self == nb::self)
    .def(nb::self != nb::self)
    .def(nb::self < nb::self);

  erase_existing_name("InitializeResult");
  nb::class_<viskores::cont::InitializeResult>(
    m, "InitializeResult", doc::ClassDoc("InitializeResult"))
    .def(nb::init<>())
    .def_rw("Device", &viskores::cont::InitializeResult::Device)
    .def_rw("Usage", &viskores::cont::InitializeResult::Usage)
    .def("__repr__",
         [](const viskores::cont::InitializeResult& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.InitializeResult(Device=" << DeviceAdapterIdRepr(self.Device)
                  << ", Usage=" << std::quoted(self.Usage) << ")";
           return stream.str();
         });

  erase_existing_name("InitializeOptions");
  nb::enum_<viskores::cont::InitializeOptions>(
    m, "InitializeOptions", nb::is_arithmetic(), nb::is_flag())
    .value("None", viskores::cont::InitializeOptions::None)
    .value("RequireDevice", viskores::cont::InitializeOptions::RequireDevice)
    .value("DefaultAnyDevice", viskores::cont::InitializeOptions::DefaultAnyDevice)
    .value("AddHelp", viskores::cont::InitializeOptions::AddHelp)
    .value("ErrorOnBadOption", viskores::cont::InitializeOptions::ErrorOnBadOption)
    .value("ErrorOnBadArgument", viskores::cont::InitializeOptions::ErrorOnBadArgument)
    .value("Strict", viskores::cont::InitializeOptions::Strict);

  erase_existing_name("Initialize");
  m.def("Initialize", []() { return viskores::cont::Initialize(); }, doc::Initialize);
  m.def("Initialize",
        &InitializeFromArgv,
        nb::arg("argv"),
        nb::arg("opts") = viskores::cont::InitializeOptions::None,
        nb::sig("def Initialize(argv: list[str], opts: InitializeOptions) -> InitializeResult"),
        doc::Initialize);

  erase_existing_name("IsInitialized");
  m.def("IsInitialized", &viskores::cont::IsInitialized, doc::IsInitialized);

  erase_existing_name("make_DeviceAdapterId");
  m.def("make_DeviceAdapterId",
        nb::overload_cast<const viskores::cont::DeviceAdapterNameType&>(
          &viskores::cont::make_DeviceAdapterId),
        nb::arg("name"),
        doc::MakeDeviceAdapterId);
  m.def("make_DeviceAdapterId",
        nb::overload_cast<viskores::Int8>(&viskores::cont::make_DeviceAdapterId),
        nb::arg("value"),
        doc::MakeDeviceAdapterId);
}

void RegisterNanobindRangeClass(nb::module_& m,
                                const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Range");
  nb::class_<viskores::Range>(m, "Range", doc::ClassDoc("Range"))
    .def(nb::init<>())
    .def(nb::init<viskores::Float64, viskores::Float64>(), nb::arg("min"), nb::arg("max"))
    .def_rw("Min", &viskores::Range::Min)
    .def_rw("Max", &viskores::Range::Max)
    .def("__repr__",
         [](const viskores::Range& self)
         {
           std::ostringstream stream;
           stream << "viskores.Range(" << self.Min << ", " << self.Max << ")";
           return stream.str();
         })
    .def("IsNonEmpty", &viskores::Range::IsNonEmpty)
    .def("Contains", [](const viskores::Range& self, double value) { return self.Contains(value); })
    .def("Length", &viskores::Range::Length)
    .def("Center", &viskores::Range::Center)
    .def(
      "Include",
      [](viskores::Range& self, nb::object valueOrRange)
      {
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

void RegisterNanobindBoundsClass(nb::module_& m,
                                 const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Bounds");
  nb::class_<viskores::Bounds>(m, "Bounds", doc::ClassDoc("Bounds"))
    .def(nb::init<>())
    .def(nb::init<const viskores::Range&, const viskores::Range&, const viskores::Range&>(),
         nb::arg("x_range"),
         nb::arg("y_range"),
         nb::arg("z_range"))
    .def(nb::init<viskores::Float64,
                  viskores::Float64,
                  viskores::Float64,
                  viskores::Float64,
                  viskores::Float64,
                  viskores::Float64>(),
         nb::arg("min_x"),
         nb::arg("max_x"),
         nb::arg("min_y"),
         nb::arg("max_y"),
         nb::arg("min_z"),
         nb::arg("max_z"))
    .def_rw("X", &viskores::Bounds::X)
    .def_rw("Y", &viskores::Bounds::Y)
    .def_rw("Z", &viskores::Bounds::Z)
    .def("__repr__",
         [](const viskores::Bounds& self)
         {
           std::ostringstream stream;
           stream << "viskores.Bounds(" << self.X.Min << ", " << self.X.Max << ", "
                  << self.Y.Min << ", " << self.Y.Max << ", " << self.Z.Min << ", "
                  << self.Z.Max << ")";
           return stream.str();
         })
    .def("IsNonEmpty", &viskores::Bounds::IsNonEmpty)
    .def("Contains",
         [](const viskores::Bounds& self, nb::object point)
         { return self.Contains(ParseVec3(point, viskores::Vec3f(0.0f, 0.0f, 0.0f))); },
         nb::arg("point"))
    .def("Volume", &viskores::Bounds::Volume)
    .def("Area", &viskores::Bounds::Area)
    .def("Center", [](const viskores::Bounds& self) { return Vec3ToTuple(self.Center()); })
    .def("MinCorner", [](const viskores::Bounds& self) { return Vec3ToTuple(self.MinCorner()); })
    .def("MaxCorner", [](const viskores::Bounds& self) { return Vec3ToTuple(self.MaxCorner()); })
    .def(
      "Include",
      [](viskores::Bounds& self, nb::object pointOrBounds)
      {
        viskores::Bounds boundsValue;
        if (nb::try_cast(pointOrBounds, boundsValue))
        {
          self.Include(boundsValue);
        }
        else
        {
          self.Include(ParseVec3(pointOrBounds, viskores::Vec3f(0.0f, 0.0f, 0.0f)));
        }
      },
      nb::arg("point_or_bounds"))
    .def("Union", &viskores::Bounds::Union, nb::arg("other"))
    .def("Intersection", &viskores::Bounds::Intersection, nb::arg("other"))
    .def("__add__", &viskores::Bounds::operator+)
    .def(nb::self == nb::self)
    .def(nb::self != nb::self);
}

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_CORE
void RegisterNanobindFilterBaseClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Filter");
  nb::class_<viskores::filter::Filter> filter(m, "Filter", doc::ClassDoc("Filter"));
  BindFilterBaseMethods(filter);
}
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR || VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
void RegisterNanobindImplicitFunctionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  (void)m;
  (void)erase_existing_name;
}
#else
void RegisterNanobindImplicitFunctionClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

void RegisterNanobindCellShapeConstants(nb::module_& m,
                                        const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("CELL_SHAPE_VERTEX");
  erase_existing_name("CELL_SHAPE_LINE");
  erase_existing_name("CELL_SHAPE_POLY_LINE");
  erase_existing_name("CELL_SHAPE_TRIANGLE");
  erase_existing_name("CELL_SHAPE_QUAD");
  erase_existing_name("CELL_SHAPE_POLYGON");
  erase_existing_name("CELL_SHAPE_TETRA");
  erase_existing_name("CELL_SHAPE_HEXAHEDRON");
  erase_existing_name("CELL_SHAPE_WEDGE");
  erase_existing_name("CELL_SHAPE_PYRAMID");

  m.attr("CELL_SHAPE_VERTEX") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_VERTEX));
  m.attr("CELL_SHAPE_LINE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_LINE));
  m.attr("CELL_SHAPE_POLY_LINE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_POLY_LINE));
  m.attr("CELL_SHAPE_TRIANGLE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_TRIANGLE));
  m.attr("CELL_SHAPE_QUAD") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_QUAD));
  m.attr("CELL_SHAPE_POLYGON") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_POLYGON));
  m.attr("CELL_SHAPE_TETRA") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_TETRA));
  m.attr("CELL_SHAPE_HEXAHEDRON") =
    nb::int_(static_cast<int>(viskores::CELL_SHAPE_HEXAHEDRON));
  m.attr("CELL_SHAPE_WEDGE") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_WEDGE));
  m.attr("CELL_SHAPE_PYRAMID") = nb::int_(static_cast<int>(viskores::CELL_SHAPE_PYRAMID));
}

void RegisterNanobindModule(nb::module_& m)
{
  auto erase_existing_name = [&](const char* name)
  {
    nb::dict module_dict = nb::borrow<nb::dict>(m.attr("__dict__"));
    module_dict.attr("pop")(name, nb::none());
  };

  RegisterNanobindInitializeClasses(m, erase_existing_name);
  RegisterNanobindRangeClass(m, erase_existing_name);
  RegisterNanobindBoundsClass(m, erase_existing_name);
  RegisterNanobindCellShapeConstants(m, erase_existing_name);
  RegisterNanobindGeneratedEnums(m);
  RegisterNanobindIOClasses(m, erase_existing_name);
  RegisterNanobindSharedDataClasses(m, erase_existing_name);

#if VISKORES_PYTHON_ENABLE_RENDERING
  RegisterNanobindCameraClass(m, erase_existing_name);
  RegisterNanobindRenderingClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
  RegisterNanobindTestingClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
  RegisterNanobindImplicitFunctionClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CORE
  RegisterNanobindFilterBaseClasses(m, erase_existing_name);
#endif
  RegisterNanobindGeneratedClasses(m, erase_existing_name);

#if VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
  RegisterNanobindScalarTopologyClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
  RegisterNanobindInteropClasses(m, erase_existing_name);
#endif

#if VISKORES_PYTHON_ENABLE_INTEROP
  RegisterNanobindHelperFunctions(m, erase_existing_name);
#endif

  erase_existing_name("_create_uniform_dataset");
  m.attr("_create_uniform_dataset") = nb::cpp_function(
    [](nb::object dimensions, nb::object origin, nb::object spacing, const char* coordName)
    {
      const auto parsedDimensions = ParseDimensions(dimensions);
      const auto parsedOrigin = ParseVec3(origin, viskores::Vec3f(0.0f, 0.0f, 0.0f));
      const auto parsedSpacing = ParseVec3(spacing, viskores::Vec3f(1.0f, 1.0f, 1.0f));
      auto dataSet = viskores::cont::DataSetBuilderUniform::Create(
        parsedDimensions, parsedOrigin, parsedSpacing, coordName);
      return nb::cast(dataSet);
    },
    nb::arg("dimensions"),
    nb::arg("origin") = nb::none(),
    nb::arg("spacing") = nb::none(),
    nb::arg("coord_name") = "coords",
    doc::CreateUniformDataSet);

#if VISKORES_PYTHON_ENABLE_FILTER_CORE && VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY
  erase_existing_name("_partition_uniform_dataset");
  m.attr("_partition_uniform_dataset") = nb::cpp_function(
    [](nb::object datasetObject, const char* fieldName, long long numBlocks)
    {
      auto dataset = RequireDataSet(datasetObject);
      if (!dataset)
      {
        throw nb::python_error();
      }

      viskores::Id3 globalSize;
      dataset->GetCellSet().CastAndCallForTypes<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED>(
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
        partitions.AppendPartition(CreateSubDataSet(*dataset, blockOrigin, blockSize, fieldName));
        portal.Set(blockNo, blockIndex);
      }

      std::vector<viskores::Id3> blocksPerDimVector{ blocksPerDim };
      return nb::make_tuple(nb::cast(partitions),
                            CreateId3ArrayObject(viskores::cont::make_ArrayHandle(
                              blocksPerDimVector, viskores::CopyFlag::On)),
                            CreateId3ArrayObject(localBlockIndices));
    },
    nb::arg("dataset"),
    nb::arg("field_name"),
    nb::arg("num_blocks"));
#endif
}

} // namespace viskores::python::bindings
