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
#include <viskores/cont/UncertainArrayHandle.h>
#include <viskores/cont/UncertainCellSet.h>

namespace viskores::python::bindings
{

namespace cont_testing = viskores::cont::testing;

namespace
{

struct AcceptResolvedType
{
  template <typename ResolvedType>
  void operator()(const ResolvedType&) const
  {
  }
};

bool CanNarrowToCoordinateSystemData(const viskores::cont::UnknownArrayHandle& array)
{
  try
  {
    viskores::cont::UncertainArrayHandle<viskores::TypeListFieldVec3,
                                         VISKORES_DEFAULT_STORAGE_LIST>
      narrowed(array);
    narrowed.CastAndCall(AcceptResolvedType{});
    return true;
  }
  catch (const std::exception&)
  {
    return false;
  }
}

bool CanNarrowToStructuredCellSet(const viskores::cont::UnknownCellSet& cellSet)
{
  try
  {
    viskores::cont::UncertainCellSet<VISKORES_DEFAULT_CELL_SET_LIST_STRUCTURED> narrowed(cellSet);
    narrowed.CastAndCall(AcceptResolvedType{});
    return true;
  }
  catch (const std::exception&)
  {
    return false;
  }
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
    [](cont_testing::MakeTestDataSet& self) { return nb::cast(self.name()); }, \
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
      return nb::cast(self.Make3DUniformDataSet3(dims));
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

  erase_existing_name("can_narrow_to_coordinate_system_data");
  m.attr("can_narrow_to_coordinate_system_data") = nb::cpp_function(
    &CanNarrowToCoordinateSystemData,
    nb::arg("array"),
    "Return whether an UnknownArrayHandle can narrow to the coordinate-system array type list.");

  erase_existing_name("can_narrow_to_structured_cell_set");
  m.attr("can_narrow_to_structured_cell_set") = nb::cpp_function(
    &CanNarrowToStructuredCellSet,
    nb::arg("cell_set"),
    "Return whether an UnknownCellSet can narrow to the structured-cell-set type list.");
}
#else
void RegisterNanobindTestingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_IO
void RegisterNanobindIOClasses(nb::module_& m,
                               const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("ImageWriterBase");
  auto image_writer_base = nb::class_<viskores::io::ImageWriterBase>(
    m, "ImageWriterBase", doc::ClassDoc("ImageWriterBase"));

  nb::enum_<viskores::io::ImageWriterBase::PixelDepth>(image_writer_base, "PixelDepth")
    .value("PIXEL_8", viskores::io::ImageWriterBase::PixelDepth::PIXEL_8)
    .value("PIXEL_16", viskores::io::ImageWriterBase::PixelDepth::PIXEL_16);
}
#else
void RegisterNanobindIOClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
