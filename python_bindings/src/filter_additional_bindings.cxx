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

namespace
{

template <typename FilterType>
void SetFieldsToPass(FilterType& filter, nb::object fieldsObject)
{
  auto& selection = filter.GetFieldsToPass();
  selection.ClearFields();
  selection.SetMode(viskores::filter::FieldSelection::Mode::Select);

  if (nb::isinstance<nb::str>(fieldsObject))
  {
    selection.AddField(nb::cast<std::string>(fieldsObject));
    return;
  }

  if (!nb::isinstance<nb::sequence>(fieldsObject) || nb::isinstance<nb::str>(fieldsObject))
  {
    throw std::runtime_error("fields must be a string or sequence");
  }

  nb::sequence sequence = nb::borrow<nb::sequence>(fieldsObject);
  const size_t size = static_cast<size_t>(nb::len(sequence));
  for (size_t index = 0; index < size; ++index)
  {
    nb::handle item = sequence[index];
    if (!nb::isinstance<nb::str>(item))
    {
      throw std::runtime_error("fields must contain only strings.");
    }
    selection.AddField(nb::cast<std::string>(item));
  }
}

template <typename FilterType>
void SetImplicitFunction(FilterType& filter, nb::object functionObject)
{
  viskores::ImplicitFunctionGeneral function;
  if (!ParseImplicitFunction(functionObject, function))
  {
    throw std::runtime_error("Implicit function must be a viskores.Box or viskores.Sphere.");
  }
  filter.SetImplicitFunction(function);
}

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
void RegisterNanobindEntityExtractionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Threshold");
  nb::class_<viskores::filter::entity_extraction::Threshold>(
    m, "Threshold", doc::ClassDoc("Threshold"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::entity_extraction::Threshold& self, const char* name)
      { self.SetActiveField(name); },
      nb::arg("name"))
    .def("GetActiveFieldName", &viskores::filter::entity_extraction::Threshold::GetActiveFieldName)
    .def("SetOutputFieldName", &viskores::filter::entity_extraction::Threshold::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::entity_extraction::Threshold::GetOutputFieldName)
    .def(
      "SetFieldsToPass",
      [](viskores::filter::entity_extraction::Threshold& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "SetLowerThreshold",
      [](viskores::filter::entity_extraction::Threshold& self, double value)
      { self.SetLowerThreshold(value); },
      nb::arg("value"))
    .def("GetLowerThreshold", &viskores::filter::entity_extraction::Threshold::GetLowerThreshold)
    .def(
      "SetUpperThreshold",
      [](viskores::filter::entity_extraction::Threshold& self, double value)
      { self.SetUpperThreshold(value); },
      nb::arg("value"))
    .def("GetUpperThreshold", &viskores::filter::entity_extraction::Threshold::GetUpperThreshold)
    .def(
      "SetThresholdBelow",
      [](viskores::filter::entity_extraction::Threshold& self, double value)
      { self.SetThresholdBelow(value); },
      nb::arg("value"))
    .def(
      "SetThresholdAbove",
      [](viskores::filter::entity_extraction::Threshold& self, double value)
      { self.SetThresholdAbove(value); },
      nb::arg("value"))
    .def(
      "SetThresholdBetween",
      [](viskores::filter::entity_extraction::Threshold& self, double lower, double upper)
      { self.SetThresholdBetween(lower, upper); },
      nb::arg("lower"),
      nb::arg("upper"))
    .def(
      "SetComponentToTest",
      [](viskores::filter::entity_extraction::Threshold& self, long value)
      { self.SetComponentToTest(static_cast<viskores::IdComponent>(value)); },
      nb::arg("component"))
    .def("SetComponentToTestToAny",
         &viskores::filter::entity_extraction::Threshold::SetComponentToTestToAny)
    .def("SetComponentToTestToAll",
         &viskores::filter::entity_extraction::Threshold::SetComponentToTestToAll)
    .def(
      "SetAllInRange",
      [](viskores::filter::entity_extraction::Threshold& self, bool enabled)
      { self.SetAllInRange(enabled); },
      nb::arg("enabled"))
    .def("GetAllInRange", &viskores::filter::entity_extraction::Threshold::GetAllInRange)
    .def(
      "SetInvert",
      [](viskores::filter::entity_extraction::Threshold& self, bool enabled)
      { self.SetInvert(enabled); },
      nb::arg("enabled"))
    .def("GetInvert", &viskores::filter::entity_extraction::Threshold::GetInvert)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::Threshold>,
         doc::ExecuteFilter);

  erase_existing_name("Mask");
  nb::class_<viskores::filter::entity_extraction::Mask>(m, "Mask", doc::ClassDoc("Mask"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::entity_extraction::Mask& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "SetCompactPoints",
      [](viskores::filter::entity_extraction::Mask& self, bool enabled)
      { self.SetCompactPoints(enabled); },
      nb::arg("enabled"))
    .def("GetCompactPoints", &viskores::filter::entity_extraction::Mask::GetCompactPoints)
    .def(
      "SetStride",
      [](viskores::filter::entity_extraction::Mask& self, long long stride)
      {
        viskores::Id strideValue = static_cast<viskores::Id>(stride);
        self.SetStride(strideValue);
      },
      nb::arg("stride"))
    .def("GetStride", &viskores::filter::entity_extraction::Mask::GetStride)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::Mask>,
         doc::ExecuteFilter);

  erase_existing_name("MaskPoints");
  nb::class_<viskores::filter::entity_extraction::MaskPoints>(
    m, "MaskPoints", doc::ClassDoc("MaskPoints"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::entity_extraction::MaskPoints& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "SetCompactPoints",
      [](viskores::filter::entity_extraction::MaskPoints& self, bool enabled)
      { self.SetCompactPoints(enabled); },
      nb::arg("enabled"))
    .def("GetCompactPoints", &viskores::filter::entity_extraction::MaskPoints::GetCompactPoints)
    .def(
      "SetStride",
      [](viskores::filter::entity_extraction::MaskPoints& self, long long stride)
      { self.SetStride(static_cast<viskores::Id>(stride)); },
      nb::arg("stride"))
    .def("GetStride", &viskores::filter::entity_extraction::MaskPoints::GetStride)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::MaskPoints>,
         doc::ExecuteFilter);

  erase_existing_name("ThresholdPoints");
  nb::class_<viskores::filter::entity_extraction::ThresholdPoints>(
    m, "ThresholdPoints", doc::ClassDoc("ThresholdPoints"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, const char* name)
      { self.SetActiveField(name); },
      nb::arg("name"))
    .def("GetActiveFieldName",
         &viskores::filter::entity_extraction::ThresholdPoints::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::entity_extraction::ThresholdPoints::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::entity_extraction::ThresholdPoints::GetOutputFieldName)
    .def(
      "SetFieldsToPass",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "SetCompactPoints",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, bool enabled)
      { self.SetCompactPoints(enabled); },
      nb::arg("enabled"))
    .def("GetCompactPoints",
         &viskores::filter::entity_extraction::ThresholdPoints::GetCompactPoints)
    .def(
      "SetLowerThreshold",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, double value)
      { self.SetLowerThreshold(value); },
      nb::arg("value"))
    .def("GetLowerThreshold",
         &viskores::filter::entity_extraction::ThresholdPoints::GetLowerThreshold)
    .def(
      "SetUpperThreshold",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, double value)
      { self.SetUpperThreshold(value); },
      nb::arg("value"))
    .def("GetUpperThreshold",
         &viskores::filter::entity_extraction::ThresholdPoints::GetUpperThreshold)
    .def(
      "SetThresholdBelow",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, double value)
      { self.SetThresholdBelow(value); },
      nb::arg("value"))
    .def(
      "SetThresholdAbove",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, double value)
      { self.SetThresholdAbove(value); },
      nb::arg("value"))
    .def(
      "SetThresholdBetween",
      [](viskores::filter::entity_extraction::ThresholdPoints& self, double lower, double upper)
      { self.SetThresholdBetween(lower, upper); },
      nb::arg("lower"),
      nb::arg("upper"))
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::ThresholdPoints>,
         doc::ExecuteFilter);

  erase_existing_name("ExternalFaces");
  nb::class_<viskores::filter::entity_extraction::ExternalFaces>(
    m, "ExternalFaces", doc::ClassDoc("ExternalFaces"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::entity_extraction::ExternalFaces& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "SetCompactPoints",
      [](viskores::filter::entity_extraction::ExternalFaces& self, bool enabled)
      { self.SetCompactPoints(enabled); },
      nb::arg("enabled"))
    .def("GetCompactPoints", &viskores::filter::entity_extraction::ExternalFaces::GetCompactPoints)
    .def(
      "SetPassPolyData",
      [](viskores::filter::entity_extraction::ExternalFaces& self, bool enabled)
      { self.SetPassPolyData(enabled); },
      nb::arg("enabled"))
    .def("GetPassPolyData", &viskores::filter::entity_extraction::ExternalFaces::GetPassPolyData)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::ExternalFaces>,
         doc::ExecuteFilter);

  erase_existing_name("ExtractStructured");
  nb::class_<viskores::filter::entity_extraction::ExtractStructured>(
    m, "ExtractStructured", doc::ClassDoc("ExtractStructured"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::entity_extraction::ExtractStructured& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def("SetVOI",
         [](viskores::filter::entity_extraction::ExtractStructured& self, nb::args args)
         {
           if (args.size() == 1)
           {
             const auto range = ParseRangeId3(args[0]);
             self.SetVOI(range);
             return;
           }
           if (args.size() != 6)
           {
             throw std::runtime_error("SetVOI expects a range object or 6 integer values.");
           }
           long long values[6];
           for (size_t index = 0; index < 6; ++index)
           {
             values[index] = nb::cast<long long>(args[index]);
           }
           self.SetVOI(values[0], values[1], values[2], values[3], values[4], values[5]);
         })
    .def("SetSampleRate",
         [](viskores::filter::entity_extraction::ExtractStructured& self, nb::args args)
         {
           if (args.size() == 1)
           {
             self.SetSampleRate(ParseDimensions(args[0]));
             return;
           }
           if (args.size() != 3)
           {
             throw std::runtime_error(
               "SetSampleRate expects a dimensions object or 3 integer values.");
           }
           long long values[3];
           for (size_t index = 0; index < 3; ++index)
           {
             values[index] = nb::cast<long long>(args[index]);
           }
           self.SetSampleRate(values[0], values[1], values[2]);
         })
    .def(
      "SetIncludeBoundary",
      [](viskores::filter::entity_extraction::ExtractStructured& self, bool enabled)
      { self.SetIncludeBoundary(enabled); },
      nb::arg("enabled"))
    .def("GetIncludeBoundary",
         &viskores::filter::entity_extraction::ExtractStructured::GetIncludeBoundary)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::ExtractStructured>,
         doc::ExecuteFilter);

  erase_existing_name("ExtractPoints");
  nb::class_<viskores::filter::entity_extraction::ExtractPoints>(
    m, "ExtractPoints", doc::ClassDoc("ExtractPoints"))
    .def(nb::init<>())
    .def(
      "SetImplicitFunction",
      [](viskores::filter::entity_extraction::ExtractPoints& self, nb::object functionObject)
      { SetImplicitFunction(self, functionObject); },
      nb::arg("function"))
    .def(
      "SetExtractInside",
      [](viskores::filter::entity_extraction::ExtractPoints& self, bool enabled)
      { self.SetExtractInside(enabled); },
      nb::arg("enabled"))
    .def("GetExtractInside", &viskores::filter::entity_extraction::ExtractPoints::GetExtractInside)
    .def(
      "SetCompactPoints",
      [](viskores::filter::entity_extraction::ExtractPoints& self, bool enabled)
      { self.SetCompactPoints(enabled); },
      nb::arg("enabled"))
    .def("GetCompactPoints", &viskores::filter::entity_extraction::ExtractPoints::GetCompactPoints)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::ExtractPoints>,
         doc::ExecuteFilter);

  erase_existing_name("ExtractGeometry");
  nb::class_<viskores::filter::entity_extraction::ExtractGeometry>(
    m, "ExtractGeometry", doc::ClassDoc("ExtractGeometry"))
    .def(nb::init<>())
    .def(
      "SetImplicitFunction",
      [](viskores::filter::entity_extraction::ExtractGeometry& self, nb::object functionObject)
      { SetImplicitFunction(self, functionObject); },
      nb::arg("function"))
    .def(
      "SetExtractInside",
      [](viskores::filter::entity_extraction::ExtractGeometry& self, bool enabled)
      { self.SetExtractInside(enabled); },
      nb::arg("enabled"))
    .def("GetExtractInside",
         &viskores::filter::entity_extraction::ExtractGeometry::GetExtractInside)
    .def(
      "SetExtractBoundaryCells",
      [](viskores::filter::entity_extraction::ExtractGeometry& self, bool enabled)
      { self.SetExtractBoundaryCells(enabled); },
      nb::arg("enabled"))
    .def("GetExtractBoundaryCells",
         &viskores::filter::entity_extraction::ExtractGeometry::GetExtractBoundaryCells)
    .def(
      "SetExtractOnlyBoundaryCells",
      [](viskores::filter::entity_extraction::ExtractGeometry& self, bool enabled)
      { self.SetExtractOnlyBoundaryCells(enabled); },
      nb::arg("enabled"))
    .def("GetExtractOnlyBoundaryCells",
         &viskores::filter::entity_extraction::ExtractGeometry::GetExtractOnlyBoundaryCells)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::ExtractGeometry>,
         doc::ExecuteFilter);

  erase_existing_name("GhostCellRemove");
  nb::class_<viskores::filter::entity_extraction::GhostCellRemove>(
    m, "GhostCellRemove", doc::ClassDoc("GhostCellRemove"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::entity_extraction::GhostCellRemove& self, const char* name)
      { self.SetActiveField(name); },
      nb::arg("name"))
    .def("GetActiveFieldName",
         &viskores::filter::entity_extraction::GhostCellRemove::GetActiveFieldName)
    .def(
      "SetRemoveGhostField",
      [](viskores::filter::entity_extraction::GhostCellRemove& self, bool enabled)
      { self.SetRemoveGhostField(enabled); },
      nb::arg("enabled"))
    .def("GetRemoveGhostField",
         &viskores::filter::entity_extraction::GhostCellRemove::GetRemoveGhostField)
    .def(
      "SetTypesToRemove",
      [](viskores::filter::entity_extraction::GhostCellRemove& self, unsigned long value)
      { self.SetTypesToRemove(static_cast<viskores::UInt8>(value)); },
      nb::arg("value"))
    .def("GetTypesToRemove",
         &viskores::filter::entity_extraction::GhostCellRemove::GetTypesToRemove)
    .def("SetTypesToRemoveToAll",
         &viskores::filter::entity_extraction::GhostCellRemove::SetTypesToRemoveToAll)
    .def("AreAllTypesRemoved",
         &viskores::filter::entity_extraction::GhostCellRemove::AreAllTypesRemoved)
    .def(
      "SetUseGhostCellsAsField",
      [](viskores::filter::entity_extraction::GhostCellRemove& self, bool enabled)
      { self.SetUseGhostCellsAsField(enabled); },
      nb::arg("enabled"))
    .def("GetUseGhostCellsAsField",
         &viskores::filter::entity_extraction::GhostCellRemove::GetUseGhostCellsAsField)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::entity_extraction::GhostCellRemove>,
         doc::ExecuteFilter);
}
#else
void RegisterNanobindEntityExtractionClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID || VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO || \
  VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS || VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK
void RegisterNanobindAdditionalFilterClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
#if VISKORES_PYTHON_ENABLE_FILTER_CLEAN_GRID
  erase_existing_name("CleanGrid");
  nb::class_<viskores::filter::clean_grid::CleanGrid>(m, "CleanGrid", doc::ClassDoc("CleanGrid"))
    .def(nb::init<>())
    .def(
      "SetFieldsToPass",
      [](viskores::filter::clean_grid::CleanGrid& self, nb::object fieldsObject)
      { SetFieldsToPass(self, fieldsObject); },
      nb::arg("fields"))
    .def(
      "SetCompactPointFields",
      [](viskores::filter::clean_grid::CleanGrid& self, bool enabled)
      { self.SetCompactPointFields(enabled); },
      nb::arg("enabled"))
    .def("GetCompactPointFields", &viskores::filter::clean_grid::CleanGrid::GetCompactPointFields)
    .def(
      "SetMergePoints",
      [](viskores::filter::clean_grid::CleanGrid& self, bool enabled)
      { self.SetMergePoints(enabled); },
      nb::arg("enabled"))
    .def("GetMergePoints", &viskores::filter::clean_grid::CleanGrid::GetMergePoints)
    .def(
      "SetTolerance",
      [](viskores::filter::clean_grid::CleanGrid& self, double value) { self.SetTolerance(value); },
      nb::arg("value"))
    .def("GetTolerance", &viskores::filter::clean_grid::CleanGrid::GetTolerance)
    .def(
      "SetToleranceIsAbsolute",
      [](viskores::filter::clean_grid::CleanGrid& self, bool enabled)
      { self.SetToleranceIsAbsolute(enabled); },
      nb::arg("enabled"))
    .def("GetToleranceIsAbsolute", &viskores::filter::clean_grid::CleanGrid::GetToleranceIsAbsolute)
    .def(
      "SetRemoveDegenerateCells",
      [](viskores::filter::clean_grid::CleanGrid& self, bool enabled)
      { self.SetRemoveDegenerateCells(enabled); },
      nb::arg("enabled"))
    .def("GetRemoveDegenerateCells",
         &viskores::filter::clean_grid::CleanGrid::GetRemoveDegenerateCells)
    .def(
      "SetFastMerge",
      [](viskores::filter::clean_grid::CleanGrid& self, bool enabled)
      { self.SetFastMerge(enabled); },
      nb::arg("enabled"))
    .def("GetFastMerge", &viskores::filter::clean_grid::CleanGrid::GetFastMerge)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::clean_grid::CleanGrid>,
         doc::ExecuteFilter);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO
  erase_existing_name("GhostCellClassify");
  nb::class_<viskores::filter::mesh_info::GhostCellClassify>(
    m, "GhostCellClassify", doc::ClassDoc("GhostCellClassify"))
    .def(nb::init<>())
    .def("SetGhostCellName",
         &viskores::filter::mesh_info::GhostCellClassify::SetGhostCellName,
         nb::arg("field_name"))
    .def("GetGhostCellName", &viskores::filter::mesh_info::GhostCellClassify::GetGhostCellName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::mesh_info::GhostCellClassify>,
         doc::ExecuteFilter);

  erase_existing_name("CellMeasures");
  nb::class_<viskores::filter::mesh_info::CellMeasures>(
    m, "CellMeasures", doc::ClassDoc("CellMeasures"))
    .def(nb::init<>())
    .def(
      "SetMeasure",
      [](viskores::filter::mesh_info::CellMeasures& self, long value)
      { self.SetMeasure(static_cast<viskores::filter::mesh_info::IntegrationType>(value)); },
      nb::arg("measure"))
    .def("GetMeasure", &viskores::filter::mesh_info::CellMeasures::GetMeasure)
    .def("SetMeasureToArcLength", &viskores::filter::mesh_info::CellMeasures::SetMeasureToArcLength)
    .def("SetMeasureToArea", &viskores::filter::mesh_info::CellMeasures::SetMeasureToArea)
    .def("SetMeasureToVolume", &viskores::filter::mesh_info::CellMeasures::SetMeasureToVolume)
    .def("SetMeasureToAll", &viskores::filter::mesh_info::CellMeasures::SetMeasureToAll)
    .def("SetCellMeasureName",
         &viskores::filter::mesh_info::CellMeasures::SetCellMeasureName,
         nb::arg("name"))
    .def("GetCellMeasureName", &viskores::filter::mesh_info::CellMeasures::GetCellMeasureName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::mesh_info::CellMeasures>,
         doc::ExecuteFilter);

  erase_existing_name("MeshQuality");
  nb::class_<viskores::filter::mesh_info::MeshQuality>(
    m, "MeshQuality", doc::ClassDoc("MeshQuality"))
    .def(nb::init<>())
    .def(
      "SetMetric",
      [](viskores::filter::mesh_info::MeshQuality& self, long value)
      { self.SetMetric(static_cast<viskores::filter::mesh_info::CellMetric>(value)); },
      nb::arg("metric"))
    .def("GetMetric", &viskores::filter::mesh_info::MeshQuality::GetMetric)
    .def("GetMetricName", &viskores::filter::mesh_info::MeshQuality::GetMetricName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::mesh_info::MeshQuality>,
         doc::ExecuteFilter);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS
  erase_existing_name("CellSetConnectivity");
  nb::class_<viskores::filter::connected_components::CellSetConnectivity>(
    m, "CellSetConnectivity", doc::ClassDoc("CellSetConnectivity"))
    .def(nb::init<>())
    .def("SetOutputFieldName",
         &viskores::filter::connected_components::CellSetConnectivity::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::connected_components::CellSetConnectivity::GetOutputFieldName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::connected_components::CellSetConnectivity>,
         doc::ExecuteFilter);

  erase_existing_name("ImageConnectivity");
  nb::class_<viskores::filter::connected_components::ImageConnectivity>(
    m, "ImageConnectivity", doc::ClassDoc("ImageConnectivity"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::connected_components::ImageConnectivity& self, const char* name)
      { self.SetActiveField(name); },
      nb::arg("name"))
    .def("GetActiveFieldName",
         &viskores::filter::connected_components::ImageConnectivity::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::connected_components::ImageConnectivity::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::connected_components::ImageConnectivity::GetOutputFieldName)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::connected_components::ImageConnectivity>,
         doc::ExecuteFilter);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK
  erase_existing_name("MergeDataSets");
  nb::class_<viskores::filter::multi_block::MergeDataSets>(
    m, "MergeDataSets", doc::ClassDoc("MergeDataSets"))
    .def(nb::init<>())
    .def("SetInvalidValue",
         &viskores::filter::multi_block::MergeDataSets::SetInvalidValue,
         nb::arg("invalid_value"))
    .def("GetInvalidValue", &viskores::filter::multi_block::MergeDataSets::GetInvalidValue)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::multi_block::MergeDataSets>,
         doc::ExecuteFilter);
#endif
}
#else
void RegisterNanobindAdditionalFilterClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
