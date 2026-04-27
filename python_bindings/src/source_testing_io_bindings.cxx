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

#if VISKORES_PYTHON_ENABLE_TESTING_UTILS && VISKORES_PYTHON_ENABLE_FILTER_SCALAR_TOPOLOGY && \
  __has_include(<viskores/filter/scalar_topology/testing/SuperArcHelper.h>)
#define VISKORES_PYTHON_ENABLE_SCALAR_TOPOLOGY_TESTING_HELPERS 1
#include <viskores/filter/scalar_topology/testing/SuperArcHelper.h>
#include <viskores/filter/scalar_topology/testing/VolumeHelper.h>
#include <viskores/filter/scalar_topology/worklet/branch_decomposition/HierarchicalVolumetricBranchDecomposer.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/BranchCompiler.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_distributed/TreeCompiler.h>
#else
#define VISKORES_PYTHON_ENABLE_SCALAR_TOPOLOGY_TESTING_HELPERS 0
#endif

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_TESTING_UTILS
void RegisterNanobindTestingClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("MakeTestDataSet");
  auto make_test_data_set = nb::class_<viskores::cont::testing::MakeTestDataSet>(
    m, "MakeTestDataSet", doc::ClassDoc("MakeTestDataSet"));

#define VISKORES_NB_MAKE_TEST_DATASET_METHOD(name)                                           \
  make_test_data_set.def(                                                                    \
    #name,                                                                                   \
    [](viskores::cont::testing::MakeTestDataSet& self) { return WrapDataSet(self.name()); }, \
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
    [](viskores::cont::testing::MakeTestDataSet& self, nb::object dimsObject)
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

#if VISKORES_PYTHON_ENABLE_SCALAR_TOPOLOGY_TESTING_HELPERS
  erase_existing_name("CompileDistributedContourTreeSuperarcs");
  m.attr("CompileDistributedContourTreeSuperarcs") = nb::cpp_function(
    [](nb::handle partitionedObject)
    {
      auto partitioned = RequirePartitionedDataSet(partitionedObject);
      if (!partitioned)
      {
        throw nb::python_error();
      }

      viskores::worklet::contourtree_distributed::TreeCompiler treeCompiler;
      for (viskores::Id dsNo = 0; dsNo < partitioned->GetNumberOfPartitions(); ++dsNo)
      {
        treeCompiler.AddHierarchicalTree(partitioned->GetPartition(dsNo));
      }
      treeCompiler.ComputeSuperarcs();

      nb::list edges;
      for (const auto& edge : treeCompiler.superarcs)
      {
        edges.append(nb::make_tuple(std::min(edge.low, edge.high), std::max(edge.low, edge.high)));
      }
      return edges;
    },
    nb::arg("partitioned_dataset"),
    doc::CompileDistributedContourTreeSuperarcs);

  erase_existing_name("CanonicalizeDistributedBranchDecomposition");
  m.attr("CanonicalizeDistributedBranchDecomposition") = nb::cpp_function(
    [](nb::handle partitionedObject)
    {
      auto partitioned = RequirePartitionedDataSet(partitionedObject);
      if (!partitioned)
      {
        throw nb::python_error();
      }

      viskores::filter::testing::contourtree_uniform_distributed::SuperArcHelper helper;
      for (viskores::Id dsNo = 0; dsNo < partitioned->GetNumberOfPartitions(); ++dsNo)
      {
        auto ds = partitioned->GetPartition(dsNo);
        using viskores::filter::scalar_topology::HierarchicalVolumetricBranchDecomposer;
        helper.Parse(HierarchicalVolumetricBranchDecomposer::PrintBranches(ds));
      }

      std::stringstream compact;
      helper.Print(compact);

      std::stringstream in(compact.str());
      viskores::worklet::contourtree_distributed::BranchCompiler compiler;
      compiler.Parse(in);

      std::ostringstream out;
      compiler.Print(out);
      return out.str();
    },
    nb::arg("partitioned_dataset"),
    doc::CanonicalizeDistributedBranchDecomposition);

  erase_existing_name("CanonicalizeDistributedAugmentedTreeVolumes");
  m.attr("CanonicalizeDistributedAugmentedTreeVolumes") = nb::cpp_function(
    [](nb::handle partitionedObject, nb::handle globalSizeObject)
    {
      auto partitioned = RequirePartitionedDataSet(partitionedObject);
      if (!partitioned)
      {
        throw nb::python_error();
      }

      const viskores::Id3 globalSize = ParseDimensions(globalSizeObject);
      viskores::filter::testing::contourtree_uniform_distributed::VolumeHelper volumeHelper;

      for (viskores::Id dsNo = 0; dsNo < partitioned->GetNumberOfPartitions(); ++dsNo)
      {
        auto ds = partitioned->GetPartition(dsNo);
        viskores::worklet::contourtree_augmented::IdArrayType supernodes;
        ds.GetField("Supernodes").GetData().AsArrayHandle(supernodes);
        viskores::worklet::contourtree_augmented::IdArrayType superarcs;
        ds.GetField("Superarcs").GetData().AsArrayHandle(superarcs);
        viskores::worklet::contourtree_augmented::IdArrayType regularNodeGlobalIds;
        ds.GetField("RegularNodeGlobalIds").GetData().AsArrayHandle(regularNodeGlobalIds);
        const viskores::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
        viskores::worklet::contourtree_augmented::IdArrayType intrinsicVolume;
        ds.GetField("IntrinsicVolume").GetData().AsArrayHandle(intrinsicVolume);
        viskores::worklet::contourtree_augmented::IdArrayType dependentVolume;
        ds.GetField("DependentVolume").GetData().AsArrayHandle(dependentVolume);

        std::string dumpVolumesString =
          viskores::worklet::contourtree_distributed::HierarchicalContourTree<
            viskores::Float32>::DumpVolumes(supernodes,
                                            superarcs,
                                            regularNodeGlobalIds,
                                            totalVolume,
                                            intrinsicVolume,
                                            dependentVolume);
        volumeHelper.Parse(dumpVolumesString);
      }

      std::ostringstream out;
      volumeHelper.Print(out);
      return out.str();
    },
    nb::arg("partitioned_dataset"),
    nb::arg("global_size"),
    doc::CanonicalizeDistributedAugmentedTreeVolumes);
#endif
}
#else
void RegisterNanobindTestingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_SOURCE
void RegisterNanobindSourceClasses(nb::module_& m,
                                   const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Tangle");
  nb::class_<viskores::source::Tangle>(m, "Tangle", doc::ClassDoc("Tangle"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::source::Tangle& self)
         {
           const auto dims = self.GetPointDimensions();
           std::ostringstream stream;
           stream << "viskores.source.Tangle(point_dimensions=(" << dims[0] << ", " << dims[1]
                  << ", " << dims[2] << "))";
           return stream.str();
         })
    .def("GetPointDimensions",
         [](const viskores::source::Tangle& self)
         {
           const auto dims = self.GetPointDimensions();
           return nb::make_tuple(dims[0], dims[1], dims[2]);
         })
    .def(
      "SetPointDimensions",
      [](viskores::source::Tangle& self, nb::object dimsObject)
      { self.SetPointDimensions(ParseDimensions(dimsObject)); },
      nb::arg("dims"))
    .def("GetCellDimensions",
         [](const viskores::source::Tangle& self)
         {
           const auto dims = self.GetCellDimensions();
           return nb::make_tuple(dims[0], dims[1], dims[2]);
         })
    .def(
      "SetCellDimensions",
      [](viskores::source::Tangle& self, nb::object dimsObject)
      { self.SetCellDimensions(ParseDimensions(dimsObject)); },
      nb::arg("dims"))
    .def(
      "Execute",
      [](viskores::source::Tangle& self) { return WrapDataSet(self.Execute()); },
      doc::ExecuteSource);

  erase_existing_name("PerlinNoise");
  nb::class_<viskores::source::PerlinNoise>(m, "PerlinNoise", doc::ClassDoc("PerlinNoise"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::source::PerlinNoise& self)
         {
           const auto dims = self.GetPointDimensions();
           std::ostringstream stream;
           stream << "viskores.source.PerlinNoise(point_dimensions=(" << dims[0] << ", " << dims[1]
                  << ", " << dims[2] << "))";
           return stream.str();
         })
    .def("GetPointDimensions",
         [](const viskores::source::PerlinNoise& self)
         {
           const auto dims = self.GetPointDimensions();
           return nb::make_tuple(dims[0], dims[1], dims[2]);
         })
    .def(
      "SetPointDimensions",
      [](viskores::source::PerlinNoise& self, nb::object dimsObject)
      { self.SetPointDimensions(ParseDimensions(dimsObject)); },
      nb::arg("dims"))
    .def("GetCellDimensions",
         [](const viskores::source::PerlinNoise& self)
         {
           const auto dims = self.GetCellDimensions();
           return nb::make_tuple(dims[0], dims[1], dims[2]);
         })
    .def(
      "SetCellDimensions",
      [](viskores::source::PerlinNoise& self, nb::object dimsObject)
      { self.SetCellDimensions(ParseDimensions(dimsObject)); },
      nb::arg("dims"))
    .def(
      "SetOrigin",
      [](viskores::source::PerlinNoise& self, nb::object valueObject)
      { self.SetOrigin(ParseVec3(valueObject, self.GetOrigin())); },
      nb::arg("value"))
    .def("GetOrigin",
         [](const viskores::source::PerlinNoise& self)
         {
           const auto value = self.GetOrigin();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def("SetSeed", &viskores::source::PerlinNoise::SetSeed, nb::arg("seed"))
    .def("GetSeed", &viskores::source::PerlinNoise::GetSeed)
    .def(
      "Execute",
      [](viskores::source::PerlinNoise& self) { return WrapDataSet(self.Execute()); },
      doc::ExecuteSource);

  erase_existing_name("Oscillator");
  nb::class_<viskores::source::Oscillator>(m, "Oscillator", doc::ClassDoc("Oscillator"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::source::Oscillator& self)
         {
           const auto dims = self.GetPointDimensions();
           std::ostringstream stream;
           stream << "viskores.source.Oscillator(point_dimensions=(" << dims[0] << ", " << dims[1]
                  << ", " << dims[2] << "))";
           return stream.str();
         })
    .def("GetPointDimensions",
         [](const viskores::source::Oscillator& self)
         {
           const auto dims = self.GetPointDimensions();
           return nb::make_tuple(dims[0], dims[1], dims[2]);
         })
    .def(
      "SetPointDimensions",
      [](viskores::source::Oscillator& self, nb::object dimsObject)
      { self.SetPointDimensions(ParseDimensions(dimsObject)); },
      nb::arg("dims"))
    .def("GetCellDimensions",
         [](const viskores::source::Oscillator& self)
         {
           const auto dims = self.GetCellDimensions();
           return nb::make_tuple(dims[0], dims[1], dims[2]);
         })
    .def(
      "SetCellDimensions",
      [](viskores::source::Oscillator& self, nb::object dimsObject)
      { self.SetCellDimensions(ParseDimensions(dimsObject)); },
      nb::arg("dims"))
    .def("SetTime", &viskores::source::Oscillator::SetTime, nb::arg("time"))
    .def("AddPeriodic",
         &viskores::source::Oscillator::AddPeriodic,
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"),
         nb::arg("radius"),
         nb::arg("omega"),
         nb::arg("zeta"))
    .def("AddDamped",
         &viskores::source::Oscillator::AddDamped,
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"),
         nb::arg("radius"),
         nb::arg("omega"),
         nb::arg("zeta"))
    .def("AddDecaying",
         &viskores::source::Oscillator::AddDecaying,
         nb::arg("x"),
         nb::arg("y"),
         nb::arg("z"),
         nb::arg("radius"),
         nb::arg("omega"),
         nb::arg("zeta"))
    .def(
      "Execute",
      [](viskores::source::Oscillator& self) { return WrapDataSet(self.Execute()); },
      doc::ExecuteSource);

  erase_existing_name("Wavelet");
  nb::class_<viskores::source::Wavelet>(m, "Wavelet", doc::ClassDoc("Wavelet"))
    .def(nb::init<>())
    .def("__repr__",
         [](const viskores::source::Wavelet& self)
         {
           const auto minExtent = self.GetMinimumExtent();
           const auto maxExtent = self.GetMaximumExtent();
           std::ostringstream stream;
           stream << "viskores.source.Wavelet(min_extent=(" << minExtent[0] << ", " << minExtent[1]
                  << ", " << minExtent[2] << "), max_extent=(" << maxExtent[0] << ", "
                  << maxExtent[1] << ", " << maxExtent[2] << "))";
           return stream.str();
         })
    .def(
      "SetCenter",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetCenter(ParseVec3(valueObject, self.GetCenter())); },
      nb::arg("value"))
    .def("GetCenter",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetCenter();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetOrigin",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetOrigin(ParseVec3(valueObject, self.GetOrigin())); },
      nb::arg("value"))
    .def("GetOrigin",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetOrigin();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetSpacing",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetSpacing(ParseVec3(valueObject, self.GetSpacing())); },
      nb::arg("value"))
    .def("GetSpacing",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetSpacing();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetFrequency",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetFrequency(ParseVec3(valueObject, self.GetFrequency())); },
      nb::arg("value"))
    .def("GetFrequency",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetFrequency();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetMagnitude",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetMagnitude(ParseVec3(valueObject, self.GetMagnitude())); },
      nb::arg("value"))
    .def("GetMagnitude",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetMagnitude();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetMinimumExtent",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetMinimumExtent(ParseDimensions(valueObject)); },
      nb::arg("value"))
    .def("GetMinimumExtent",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetMinimumExtent();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetMaximumExtent",
      [](viskores::source::Wavelet& self, nb::object valueObject)
      { self.SetMaximumExtent(ParseDimensions(valueObject)); },
      nb::arg("value"))
    .def("GetMaximumExtent",
         [](const viskores::source::Wavelet& self)
         {
           const auto value = self.GetMaximumExtent();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetExtent",
      [](viskores::source::Wavelet& self, nb::object minObject, nb::object maxObject)
      { self.SetExtent(ParseDimensions(minObject), ParseDimensions(maxObject)); },
      nb::arg("min_extent"),
      nb::arg("max_extent"))
    .def("SetMaximumValue", &viskores::source::Wavelet::SetMaximumValue, nb::arg("value"))
    .def("GetMaximumValue", &viskores::source::Wavelet::GetMaximumValue)
    .def("SetStandardDeviation", &viskores::source::Wavelet::SetStandardDeviation, nb::arg("value"))
    .def("GetStandardDeviation", &viskores::source::Wavelet::GetStandardDeviation)
    .def(
      "Execute",
      [](viskores::source::Wavelet& self) { return WrapDataSet(self.Execute()); },
      doc::ExecuteSource);
}
#else
void RegisterNanobindSourceClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_IO
void RegisterNanobindIOClasses(nb::module_& m,
                               const std::function<void(const char*)>& erase_existing_name)
{
#if VISKORES_PYTHON_ENABLE_IO_HDF5
  erase_existing_name("ImageReaderHDF5");
  nb::class_<viskores::io::ImageReaderHDF5>(m, "ImageReaderHDF5", doc::ClassDoc("ImageReaderHDF5"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet",
         [](viskores::io::ImageReaderHDF5& self) { return WrapDataSet(self.ReadDataSet()); })
    .def("GetPointFieldName", &viskores::io::ImageReaderHDF5::GetPointFieldName)
    .def("SetPointFieldName", &viskores::io::ImageReaderHDF5::SetPointFieldName, nb::arg("name"))
    .def("GetFileName", &viskores::io::ImageReaderHDF5::GetFileName)
    .def("SetFileName", &viskores::io::ImageReaderHDF5::SetFileName, nb::arg("file_name"));

  erase_existing_name("ImageWriterHDF5");
  nb::class_<viskores::io::ImageWriterHDF5>(m, "ImageWriterHDF5", doc::ClassDoc("ImageWriterHDF5"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def(
      "WriteDataSet",
      [](viskores::io::ImageWriterHDF5& self, nb::object dataObject, nb::object colorFieldObject)
      {
        auto dataSet = RequireDataSet(dataObject);
        if (!dataSet)
        {
          throw nb::python_error();
        }
        if (colorFieldObject.is_none())
        {
          self.WriteDataSet(*dataSet);
        }
        else
        {
          self.WriteDataSet(*dataSet, nb::cast<std::string>(colorFieldObject));
        }
      },
      nb::arg("data"),
      nb::arg("color_field") = nb::none())
    .def("GetPixelDepth",
         [](const viskores::io::ImageWriterHDF5& self)
         { return static_cast<int>(self.GetPixelDepth()); })
    .def(
      "SetPixelDepth",
      [](viskores::io::ImageWriterHDF5& self, int depth)
      { self.SetPixelDepth(static_cast<viskores::io::ImageWriterBase::PixelDepth>(depth)); },
      nb::arg("depth"))
    .def("GetFileName", &viskores::io::ImageWriterHDF5::GetFileName)
    .def("SetFileName", &viskores::io::ImageWriterHDF5::SetFileName, nb::arg("file_name"));
#endif

  erase_existing_name("ImageReaderPNG");
  nb::class_<viskores::io::ImageReaderPNG>(m, "ImageReaderPNG", doc::ClassDoc("ImageReaderPNG"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet",
         [](viskores::io::ImageReaderPNG& self) { return WrapDataSet(self.ReadDataSet()); })
    .def("GetPointFieldName", &viskores::io::ImageReaderPNG::GetPointFieldName)
    .def("SetPointFieldName", &viskores::io::ImageReaderPNG::SetPointFieldName, nb::arg("name"))
    .def("GetFileName", &viskores::io::ImageReaderPNG::GetFileName)
    .def("SetFileName", &viskores::io::ImageReaderPNG::SetFileName, nb::arg("file_name"));

  erase_existing_name("ImageReaderPNM");
  nb::class_<viskores::io::ImageReaderPNM>(m, "ImageReaderPNM", doc::ClassDoc("ImageReaderPNM"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet",
         [](viskores::io::ImageReaderPNM& self) { return WrapDataSet(self.ReadDataSet()); })
    .def("GetPointFieldName", &viskores::io::ImageReaderPNM::GetPointFieldName)
    .def("SetPointFieldName", &viskores::io::ImageReaderPNM::SetPointFieldName, nb::arg("name"))
    .def("GetFileName", &viskores::io::ImageReaderPNM::GetFileName)
    .def("SetFileName", &viskores::io::ImageReaderPNM::SetFileName, nb::arg("file_name"));

  erase_existing_name("ImageWriterPNG");
  nb::class_<viskores::io::ImageWriterPNG>(m, "ImageWriterPNG", doc::ClassDoc("ImageWriterPNG"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def(
      "WriteDataSet",
      [](viskores::io::ImageWriterPNG& self, nb::object dataObject, nb::object colorFieldObject)
      {
        auto dataSet = RequireDataSet(dataObject);
        if (!dataSet)
        {
          throw nb::python_error();
        }
        if (colorFieldObject.is_none())
        {
          self.WriteDataSet(*dataSet);
        }
        else
        {
          self.WriteDataSet(*dataSet, nb::cast<std::string>(colorFieldObject));
        }
      },
      nb::arg("data"),
      nb::arg("color_field") = nb::none())
    .def("GetPixelDepth",
         [](const viskores::io::ImageWriterPNG& self)
         { return static_cast<int>(self.GetPixelDepth()); })
    .def(
      "SetPixelDepth",
      [](viskores::io::ImageWriterPNG& self, int depth)
      { self.SetPixelDepth(static_cast<viskores::io::ImageWriterBase::PixelDepth>(depth)); },
      nb::arg("depth"))
    .def("GetFileName", &viskores::io::ImageWriterPNG::GetFileName)
    .def("SetFileName", &viskores::io::ImageWriterPNG::SetFileName, nb::arg("file_name"));

  erase_existing_name("ImageWriterPNM");
  nb::class_<viskores::io::ImageWriterPNM>(m, "ImageWriterPNM", doc::ClassDoc("ImageWriterPNM"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def(
      "WriteDataSet",
      [](viskores::io::ImageWriterPNM& self, nb::object dataObject, nb::object colorFieldObject)
      {
        auto dataSet = RequireDataSet(dataObject);
        if (!dataSet)
        {
          throw nb::python_error();
        }
        if (colorFieldObject.is_none())
        {
          self.WriteDataSet(*dataSet);
        }
        else
        {
          self.WriteDataSet(*dataSet, nb::cast<std::string>(colorFieldObject));
        }
      },
      nb::arg("data"),
      nb::arg("color_field") = nb::none())
    .def("GetPixelDepth",
         [](const viskores::io::ImageWriterPNM& self)
         { return static_cast<int>(self.GetPixelDepth()); })
    .def(
      "SetPixelDepth",
      [](viskores::io::ImageWriterPNM& self, int depth)
      { self.SetPixelDepth(static_cast<viskores::io::ImageWriterBase::PixelDepth>(depth)); },
      nb::arg("depth"))
    .def("GetFileName", &viskores::io::ImageWriterPNM::GetFileName)
    .def("SetFileName", &viskores::io::ImageWriterPNM::SetFileName, nb::arg("file_name"));

  erase_existing_name("VTKDataSetReader");
  nb::class_<viskores::io::VTKDataSetReader>(
    m, "VTKDataSetReader", doc::ClassDoc("VTKDataSetReader"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet",
         [](viskores::io::VTKDataSetReader& self) { return WrapDataSet(self.ReadDataSet()); });

  erase_existing_name("BOVDataSetReader");
  nb::class_<viskores::io::BOVDataSetReader>(
    m, "BOVDataSetReader", doc::ClassDoc("BOVDataSetReader"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadDataSet",
         [](viskores::io::BOVDataSetReader& self) { return WrapDataSet(self.ReadDataSet()); });

  erase_existing_name("VTKVisItFileReader");
  nb::class_<viskores::io::VTKVisItFileReader>(
    m, "VTKVisItFileReader", doc::ClassDoc("VTKVisItFileReader"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def("ReadPartitionedDataSet",
         [](viskores::io::VTKVisItFileReader& self)
         { return WrapPartitionedDataSet(self.ReadPartitionedDataSet()); });

  erase_existing_name("VTKDataSetWriter");
  nb::class_<viskores::io::VTKDataSetWriter>(
    m, "VTKDataSetWriter", doc::ClassDoc("VTKDataSetWriter"))
    .def(nb::init<const std::string&>(), nb::arg("file_name"))
    .def(
      "WriteDataSet",
      [](viskores::io::VTKDataSetWriter& self, nb::object dataObject)
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
         [](const viskores::io::VTKDataSetWriter& self)
         { return static_cast<int>(self.GetFileType()); })
    .def(
      "SetFileType",
      [](viskores::io::VTKDataSetWriter& self, int fileType)
      { self.SetFileType(static_cast<viskores::io::FileType>(fileType)); },
      nb::arg("file_type"))
    .def("SetFileTypeToAscii", &viskores::io::VTKDataSetWriter::SetFileTypeToAscii)
    .def("SetFileTypeToBinary", &viskores::io::VTKDataSetWriter::SetFileTypeToBinary);
}
#else
void RegisterNanobindIOClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
