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

namespace cont_testing = viskores::cont::testing;
namespace source = viskores::source;
namespace io = viskores::io;

namespace
{

template <typename SourceType, typename ClassType>
ClassType& BindSourcePointDimensionsRepr(ClassType& cls, const char* qualifiedName)
{
  cls.def("__repr__",
          [qualifiedName](const SourceType& self)
          {
            const auto dims = self.GetPointDimensions();
            std::ostringstream stream;
            stream << qualifiedName << "(point_dimensions=(" << dims[0] << ", " << dims[1]
                   << ", " << dims[2] << "))";
            return stream.str();
          });
  return cls;
}

template <typename SourceType, typename ClassType>
ClassType& BindSourcePointCellDimensions(ClassType& cls)
{
  BindId3Property<SourceType>(
    cls, "SetPointDimensions", "GetPointDimensions", &SourceType::SetPointDimensions,
    &SourceType::GetPointDimensions, "dims");
  BindId3Property<SourceType>(
    cls, "SetCellDimensions", "GetCellDimensions", &SourceType::SetCellDimensions,
    &SourceType::GetCellDimensions, "dims");
  return cls;
}

template <typename SourceType, typename ClassType>
ClassType& BindSourceExecute(ClassType& cls)
{
  cls.def(
    "Execute", [](SourceType& self) { return WrapDataSet(self.Execute()); }, doc::ExecuteSource);
  return cls;
}

template <typename ReaderType>
void RegisterImageReader(nb::module_& m,
                         const std::function<void(const char*)>& erase_existing_name,
                         const char* name)
{
  erase_existing_name(name);
  nb::class_<ReaderType>(m, name, doc::ClassDoc(name))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet", [](ReaderType& self) { return WrapDataSet(self.ReadDataSet()); })
    .def("GetPointFieldName", &ReaderType::GetPointFieldName)
    .def("SetPointFieldName", &ReaderType::SetPointFieldName, nb::arg("name"))
    .def("GetFileName", &ReaderType::GetFileName)
    .def("SetFileName", &ReaderType::SetFileName, nb::arg("file_name"));
}

template <typename WriterType>
void WriteImageDataSet(WriterType& writer, nb::object dataObject, nb::object colorFieldObject)
{
  auto dataSet = RequireDataSet(dataObject);
  if (!dataSet)
  {
    throw nb::python_error();
  }
  if (colorFieldObject.is_none())
  {
    writer.WriteDataSet(*dataSet);
  }
  else
  {
    writer.WriteDataSet(*dataSet, nb::cast<std::string>(colorFieldObject));
  }
}

template <typename WriterType>
void RegisterImageWriter(nb::module_& m,
                         const std::function<void(const char*)>& erase_existing_name,
                         const char* name)
{
  erase_existing_name(name);
  nb::class_<WriterType>(m, name, doc::ClassDoc(name))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("WriteDataSet",
         &WriteImageDataSet<WriterType>,
         nb::arg("data"),
         nb::arg("color_field") = nb::none())
    .def("GetPixelDepth", [](const WriterType& self) { return static_cast<int>(self.GetPixelDepth()); })
    .def("SetPixelDepth",
         [](WriterType& self, int depth)
         { self.SetPixelDepth(static_cast<io::ImageWriterBase::PixelDepth>(depth)); },
         nb::arg("depth"))
    .def("GetFileName", &WriterType::GetFileName)
    .def("SetFileName", &WriterType::SetFileName, nb::arg("file_name"));
}

template <typename ReaderType>
void RegisterDataSetReader(nb::module_& m,
                           const std::function<void(const char*)>& erase_existing_name,
                           const char* name)
{
  erase_existing_name(name);
  nb::class_<ReaderType>(m, name, doc::ClassDoc(name))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet", [](ReaderType& self) { return WrapDataSet(self.ReadDataSet()); });
}

} // namespace

#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
void RegisterNanobindTestingClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("MakeTestDataSet");
  auto make_test_data_set = nb::class_<cont_testing::MakeTestDataSet>(
    m, "MakeTestDataSet", doc::ClassDoc("MakeTestDataSet"));

#define VISKORES_NB_MAKE_TEST_DATASET_METHOD(name)                                           \
  make_test_data_set.def(                                                                    \
    #name,                                                                                   \
    [](cont_testing::MakeTestDataSet& self) { return WrapDataSet(self.name()); }, \
    "Create the Viskores test data set named " #name ".")

  make_test_data_set.def(nb::init<>());
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make1DUniformDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make1DUniformDataSet1);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make1DUniformDataSet2);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make1DExplicitDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make2DUniformDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make2DUniformDataSet1);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make2DUniformDataSet2);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make2DUniformDataSet3);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DUniformDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DUniformDataSet1);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DUniformDataSet2);
  make_test_data_set.def(
    "Make3DUniformDataSet3",
    [](cont_testing::MakeTestDataSet& self, nb::object dimsObject)
    {
      const viskores::Id3 dims =
        dimsObject.is_none() ? viskores::Id3(10, 10, 10) : ParseDimensions(dimsObject);
      return WrapDataSet(self.Make3DUniformDataSet3(dims));
    },
    nb::arg("dims") = nb::none());
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DUniformDataSet4);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DRegularDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DRegularDataSet1);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make2DRectilinearDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DRectilinearDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make2DExplicitDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet0);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet1);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet2);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet3);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet4);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet5);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet6);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet7);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSet8);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSetZoo);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSetPolygonal);
  VISKORES_NB_MAKE_TEST_DATASET_METHOD(Make3DExplicitDataSetCowNose);

#undef VISKORES_NB_MAKE_TEST_DATASET_METHOD
}
#else
void RegisterNanobindTestingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_SOURCE
void RegisterNanobindSourceClasses(nb::module_& m,
                                   const std::function<void(const char*)>& erase_existing_name)
{
  auto tangle = BindClassWithDefaultConstructor<source::Tangle>(
    m, erase_existing_name, "Tangle");
  BindSourcePointDimensionsRepr<source::Tangle>(tangle, "viskores.source.Tangle");
  BindSourcePointCellDimensions<source::Tangle>(tangle);
  BindSourceExecute<source::Tangle>(tangle);

  auto perlinNoise = BindClassWithDefaultConstructor<source::PerlinNoise>(
    m, erase_existing_name, "PerlinNoise");
  BindSourcePointDimensionsRepr<source::PerlinNoise>(perlinNoise, "viskores.source.PerlinNoise");
  BindSourcePointCellDimensions<source::PerlinNoise>(perlinNoise);
  BindVec3Property<source::PerlinNoise>(
    perlinNoise, "SetOrigin", "GetOrigin", &source::PerlinNoise::SetOrigin,
    &source::PerlinNoise::GetOrigin);
  perlinNoise
    .def("SetSeed", &source::PerlinNoise::SetSeed, nb::arg("seed"))
    .def("GetSeed", &source::PerlinNoise::GetSeed);
  BindSourceExecute<source::PerlinNoise>(perlinNoise);

  auto oscillator = BindClassWithDefaultConstructor<source::Oscillator>(
    m, erase_existing_name, "Oscillator");
  BindSourcePointDimensionsRepr<source::Oscillator>(oscillator, "viskores.source.Oscillator");
  BindSourcePointCellDimensions<source::Oscillator>(oscillator);
  oscillator
    .def("SetTime", &source::Oscillator::SetTime, nb::arg("time"))
    .def("AddPeriodic",
         &source::Oscillator::AddPeriodic,
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"),
         nb::arg("radius"),
         nb::arg("omega"),
         nb::arg("zeta"))
    .def("AddDamped",
         &source::Oscillator::AddDamped,
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"),
         nb::arg("radius"),
         nb::arg("omega"),
         nb::arg("zeta"))
    .def("AddDecaying",
         &source::Oscillator::AddDecaying,
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"),
         nb::arg("radius"),
         nb::arg("omega"),
         nb::arg("zeta"));
  BindSourceExecute<source::Oscillator>(oscillator);

  auto wavelet = BindClassWithDefaultConstructor<source::Wavelet>(
    m, erase_existing_name, "Wavelet");
  wavelet
    .def("__repr__",
         [](const source::Wavelet& self)
         {
           const auto minExtent = self.GetMinimumExtent();
           const auto maxExtent = self.GetMaximumExtent();
           std::ostringstream stream;
           stream << "viskores.source.Wavelet(min_extent=(" << minExtent[0] << ", "
                  << minExtent[1] << ", " << minExtent[2] << "), max_extent=(" << maxExtent[0]
                  << ", " << maxExtent[1] << ", " << maxExtent[2] << "))";
           return stream.str();
         })
    .def("SetExtent",
         [](source::Wavelet& self, nb::object minObject, nb::object maxObject)
         { self.SetExtent(ParseDimensions(minObject), ParseDimensions(maxObject)); },
         nb::arg("min_extent"),
         nb::arg("max_extent"))
    .def("SetMaximumValue", &source::Wavelet::SetMaximumValue, nb::arg("value"))
    .def("GetMaximumValue", &source::Wavelet::GetMaximumValue)
    .def("SetStandardDeviation", &source::Wavelet::SetStandardDeviation, nb::arg("value"))
    .def("GetStandardDeviation", &source::Wavelet::GetStandardDeviation);
  BindVec3Property<source::Wavelet>(
    wavelet, "SetCenter", "GetCenter", &source::Wavelet::SetCenter,
    &source::Wavelet::GetCenter);
  BindVec3Property<source::Wavelet>(
    wavelet, "SetOrigin", "GetOrigin", &source::Wavelet::SetOrigin,
    &source::Wavelet::GetOrigin);
  BindVec3Property<source::Wavelet>(
    wavelet, "SetSpacing", "GetSpacing", &source::Wavelet::SetSpacing,
    &source::Wavelet::GetSpacing);
  BindVec3Property<source::Wavelet>(
    wavelet, "SetFrequency", "GetFrequency", &source::Wavelet::SetFrequency,
    &source::Wavelet::GetFrequency);
  BindVec3Property<source::Wavelet>(
    wavelet, "SetMagnitude", "GetMagnitude", &source::Wavelet::SetMagnitude,
    &source::Wavelet::GetMagnitude);
  BindId3Property<source::Wavelet>(
    wavelet, "SetMinimumExtent", "GetMinimumExtent", &source::Wavelet::SetMinimumExtent,
    &source::Wavelet::GetMinimumExtent);
  BindId3Property<source::Wavelet>(
    wavelet, "SetMaximumExtent", "GetMaximumExtent", &source::Wavelet::SetMaximumExtent,
    &source::Wavelet::GetMaximumExtent);
  BindSourceExecute<source::Wavelet>(wavelet);
}
#else
void RegisterNanobindSourceClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_IO
void RegisterNanobindIOClasses(nb::module_& m,
                               const std::function<void(const char*)>& erase_existing_name)
{
#if VISKORES_PYTHON_ENABLE_IO_HDF5
  RegisterImageReader<io::ImageReaderHDF5>(m, erase_existing_name, "ImageReaderHDF5");
  RegisterImageWriter<io::ImageWriterHDF5>(m, erase_existing_name, "ImageWriterHDF5");
#endif

  RegisterImageReader<io::ImageReaderPNG>(m, erase_existing_name, "ImageReaderPNG");
  RegisterImageReader<io::ImageReaderPNM>(m, erase_existing_name, "ImageReaderPNM");
  RegisterImageWriter<io::ImageWriterPNG>(m, erase_existing_name, "ImageWriterPNG");
  RegisterImageWriter<io::ImageWriterPNM>(m, erase_existing_name, "ImageWriterPNM");
  RegisterDataSetReader<io::VTKDataSetReader>(m, erase_existing_name, "VTKDataSetReader");
  RegisterDataSetReader<io::BOVDataSetReader>(m, erase_existing_name, "BOVDataSetReader");

  erase_existing_name("VTKVisItFileReader");
  nb::class_<io::VTKVisItFileReader>(
    m, "VTKVisItFileReader", doc::ClassDoc("VTKVisItFileReader"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadPartitionedDataSet",
         [](io::VTKVisItFileReader& self)
         { return WrapPartitionedDataSet(self.ReadPartitionedDataSet()); });

  erase_existing_name("VTKDataSetWriter");
  nb::class_<io::VTKDataSetWriter>(
    m, "VTKDataSetWriter", doc::ClassDoc("VTKDataSetWriter"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def(
      "WriteDataSet",
      [](io::VTKDataSetWriter& self, nb::object dataObject)
      {
        auto dataSet = RequireDataSet(dataObject);
        if (!dataSet)
        {
          throw nb::python_error();
        }
        self.WriteDataSet(*dataSet);
      },
      nb::arg("data"))
    .def("GetFileType",
         [](const io::VTKDataSetWriter& self)
         { return static_cast<int>(self.GetFileType()); })
    .def(
      "SetFileType",
      [](io::VTKDataSetWriter& self, int fileType)
      { self.SetFileType(static_cast<io::FileType>(fileType)); },
      nb::arg("file_type"))
    .def("SetFileTypeToAscii", &io::VTKDataSetWriter::SetFileTypeToAscii)
    .def("SetFileTypeToBinary", &io::VTKDataSetWriter::SetFileTypeToBinary);
}
#else
void RegisterNanobindIOClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
