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

namespace entity = viskores::filter::entity_extraction;
namespace clean_grid = viskores::filter::clean_grid;
namespace mesh_info = viskores::filter::mesh_info;
namespace connected = viskores::filter::connected_components;
namespace multi_block = viskores::filter::multi_block;

namespace
{

template <typename FilterType>
void SetImplicitFunction(FilterType& filter, nb::object functionObject)
{
  viskores::ImplicitFunctionGeneral function;
  if (!ParseImplicitFunction(functionObject, function))
  {
    throw std::runtime_error("Implicit function must be a viskores.Box, viskores.Cylinder, "
                             "viskores.Plane, or viskores.Sphere.");
  }
  filter.SetImplicitFunction(function);
}

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_ENTITY_EXTRACTION
void RegisterNanobindEntityExtractionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  auto threshold =
    BindClassWithDefaultConstructor<entity::Threshold>(
      m, erase_existing_name, "Threshold");
  BindFilterActiveFieldNameMethods<entity::Threshold>(threshold);
  BindFilterOutputFieldMethods<entity::Threshold>(threshold);
  BindFilterFieldsToPassMethod<entity::Threshold>(threshold);
  threshold
    .def(
      "SetLowerThreshold",
      [](entity::Threshold& self, double value)
      { self.SetLowerThreshold(value); },
      nb::arg("value"))
    .def("GetLowerThreshold", &entity::Threshold::GetLowerThreshold)
    .def(
      "SetUpperThreshold",
      [](entity::Threshold& self, double value)
      { self.SetUpperThreshold(value); },
      nb::arg("value"))
    .def("GetUpperThreshold", &entity::Threshold::GetUpperThreshold)
    .def(
      "SetThresholdBelow",
      [](entity::Threshold& self, double value)
      { self.SetThresholdBelow(value); },
      nb::arg("value"))
    .def(
      "SetThresholdAbove",
      [](entity::Threshold& self, double value)
      { self.SetThresholdAbove(value); },
      nb::arg("value"))
    .def(
      "SetThresholdBetween",
      [](entity::Threshold& self, double lower, double upper)
      { self.SetThresholdBetween(lower, upper); },
      nb::arg("lower"),
      nb::arg("upper"))
    .def("SetComponentToTestToAny",
         &entity::Threshold::SetComponentToTestToAny)
    .def("SetComponentToTestToAll",
         &entity::Threshold::SetComponentToTestToAll);
  BindCastedSetter<entity::Threshold,
                   viskores::IdComponent,
                   long>(threshold,
                         "SetComponentToTest",
                         &entity::Threshold::SetComponentToTest,
                         "component");
  threshold
    .def("SetAllInRange", &entity::Threshold::SetAllInRange, nb::arg("enabled"))
    .def("GetAllInRange", &entity::Threshold::GetAllInRange)
    .def("SetInvert", &entity::Threshold::SetInvert, nb::arg("enabled"))
    .def("GetInvert", &entity::Threshold::GetInvert);
  BindFilterExecuteMethod<entity::Threshold>(threshold);

  auto mask =
    BindClassWithDefaultConstructor<entity::Mask>(
      m, erase_existing_name, "Mask");
  BindFilterFieldsToPassMethod<entity::Mask>(mask);
  mask
    .def("SetCompactPoints", &entity::Mask::SetCompactPoints, nb::arg("enabled"))
    .def("GetCompactPoints", &entity::Mask::GetCompactPoints);
  BindCastedProperty<entity::Mask, viskores::Id, long long>(
    mask,
    "SetStride",
    "GetStride",
    &entity::Mask::SetStride,
    &entity::Mask::GetStride,
    "stride");
  BindFilterExecuteMethod<entity::Mask>(mask);

  auto maskPoints =
    BindClassWithDefaultConstructor<entity::MaskPoints>(
      m, erase_existing_name, "MaskPoints");
  BindFilterFieldsToPassMethod<entity::MaskPoints>(maskPoints);
  maskPoints
    .def("SetCompactPoints", &entity::MaskPoints::SetCompactPoints, nb::arg("enabled"))
    .def("GetCompactPoints", &entity::MaskPoints::GetCompactPoints);
  BindCastedProperty<entity::MaskPoints, viskores::Id, long long>(
    maskPoints,
    "SetStride",
    "GetStride",
    &entity::MaskPoints::SetStride,
    &entity::MaskPoints::GetStride,
    "stride");
  BindFilterExecuteMethod<entity::MaskPoints>(maskPoints);

  auto thresholdPoints =
    BindClassWithDefaultConstructor<entity::ThresholdPoints>(
      m, erase_existing_name, "ThresholdPoints");
  BindFilterActiveFieldNameMethods<entity::ThresholdPoints>(
    thresholdPoints);
  BindFilterOutputFieldMethods<entity::ThresholdPoints>(
    thresholdPoints);
  BindFilterFieldsToPassMethod<entity::ThresholdPoints>(
    thresholdPoints);
  thresholdPoints
    .def("SetCompactPoints", &entity::ThresholdPoints::SetCompactPoints, nb::arg("enabled"))
    .def("GetCompactPoints", &entity::ThresholdPoints::GetCompactPoints);
  thresholdPoints
    .def(
      "SetLowerThreshold",
      [](entity::ThresholdPoints& self, double value)
      { self.SetLowerThreshold(value); },
      nb::arg("value"))
    .def("GetLowerThreshold",
         &entity::ThresholdPoints::GetLowerThreshold)
    .def(
      "SetUpperThreshold",
      [](entity::ThresholdPoints& self, double value)
      { self.SetUpperThreshold(value); },
      nb::arg("value"))
    .def("GetUpperThreshold",
         &entity::ThresholdPoints::GetUpperThreshold)
    .def(
      "SetThresholdBelow",
      [](entity::ThresholdPoints& self, double value)
      { self.SetThresholdBelow(value); },
      nb::arg("value"))
    .def(
      "SetThresholdAbove",
      [](entity::ThresholdPoints& self, double value)
      { self.SetThresholdAbove(value); },
      nb::arg("value"))
    .def(
      "SetThresholdBetween",
      [](entity::ThresholdPoints& self, double lower, double upper)
      { self.SetThresholdBetween(lower, upper); },
      nb::arg("lower"),
      nb::arg("upper"));
  BindFilterExecuteMethod<entity::ThresholdPoints>(thresholdPoints);

  auto externalFaces =
    BindClassWithDefaultConstructor<entity::ExternalFaces>(
      m, erase_existing_name, "ExternalFaces");
  BindFilterFieldsToPassMethod<entity::ExternalFaces>(
    externalFaces);
  externalFaces
    .def("SetCompactPoints", &entity::ExternalFaces::SetCompactPoints, nb::arg("enabled"))
    .def("GetCompactPoints", &entity::ExternalFaces::GetCompactPoints)
    .def("SetPassPolyData", &entity::ExternalFaces::SetPassPolyData, nb::arg("enabled"))
    .def("GetPassPolyData", &entity::ExternalFaces::GetPassPolyData);
  BindFilterExecuteMethod<entity::ExternalFaces>(externalFaces);

  auto extractStructured =
    BindClassWithDefaultConstructor<entity::ExtractStructured>(
      m, erase_existing_name, "ExtractStructured");
  BindFilterFieldsToPassMethod<entity::ExtractStructured>(
    extractStructured);
  extractStructured
    .def("SetVOI",
         [](entity::ExtractStructured& self, nb::args args)
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
         [](entity::ExtractStructured& self, nb::args args)
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
    .def("SetIncludeBoundary", &entity::ExtractStructured::SetIncludeBoundary, nb::arg("enabled"))
    .def("GetIncludeBoundary", &entity::ExtractStructured::GetIncludeBoundary);
  BindFilterExecuteMethod<entity::ExtractStructured>(
    extractStructured);

  auto extractPoints =
    BindClassWithDefaultConstructor<entity::ExtractPoints>(
      m, erase_existing_name, "ExtractPoints");
  extractPoints
    .def(
      "SetImplicitFunction",
      [](entity::ExtractPoints& self, nb::object functionObject)
      { SetImplicitFunction(self, functionObject); },
      nb::arg("function"))
    .def("SetExtractInside", &entity::ExtractPoints::SetExtractInside, nb::arg("enabled"))
    .def("GetExtractInside", &entity::ExtractPoints::GetExtractInside)
    .def("SetCompactPoints", &entity::ExtractPoints::SetCompactPoints, nb::arg("enabled"))
    .def("GetCompactPoints", &entity::ExtractPoints::GetCompactPoints);
  BindFilterExecuteMethod<entity::ExtractPoints>(extractPoints);

  auto extractGeometry =
    BindClassWithDefaultConstructor<entity::ExtractGeometry>(
      m, erase_existing_name, "ExtractGeometry");
  extractGeometry
    .def(
      "SetImplicitFunction",
      [](entity::ExtractGeometry& self, nb::object functionObject)
      { SetImplicitFunction(self, functionObject); },
      nb::arg("function"))
    .def("SetExtractInside", &entity::ExtractGeometry::SetExtractInside, nb::arg("enabled"))
    .def("GetExtractInside", &entity::ExtractGeometry::GetExtractInside)
    .def("SetExtractBoundaryCells",
         &entity::ExtractGeometry::SetExtractBoundaryCells,
         nb::arg("enabled"))
    .def("GetExtractBoundaryCells", &entity::ExtractGeometry::GetExtractBoundaryCells)
    .def("SetExtractOnlyBoundaryCells",
         &entity::ExtractGeometry::SetExtractOnlyBoundaryCells,
         nb::arg("enabled"))
    .def("GetExtractOnlyBoundaryCells", &entity::ExtractGeometry::GetExtractOnlyBoundaryCells);
  BindFilterExecuteMethod<entity::ExtractGeometry>(extractGeometry);

  auto ghostCellRemove =
    BindClassWithDefaultConstructor<entity::GhostCellRemove>(
      m, erase_existing_name, "GhostCellRemove");
  BindFilterActiveFieldNameMethods<entity::GhostCellRemove>(
    ghostCellRemove);
  ghostCellRemove
    .def("SetRemoveGhostField", &entity::GhostCellRemove::SetRemoveGhostField, nb::arg("enabled"))
    .def("GetRemoveGhostField", &entity::GhostCellRemove::GetRemoveGhostField);
  BindCastedProperty<entity::GhostCellRemove,
                     viskores::UInt8,
                     unsigned long>(ghostCellRemove,
                                    "SetTypesToRemove",
                                    "GetTypesToRemove",
                                    &entity::GhostCellRemove::
                                      SetTypesToRemove,
                                    &entity::GhostCellRemove::
                                      GetTypesToRemove);
  ghostCellRemove
    .def("SetTypesToRemoveToAll",
         &entity::GhostCellRemove::SetTypesToRemoveToAll)
    .def("AreAllTypesRemoved",
         &entity::GhostCellRemove::AreAllTypesRemoved);
  ghostCellRemove
    .def("SetUseGhostCellsAsField",
         &entity::GhostCellRemove::SetUseGhostCellsAsField,
         nb::arg("enabled"))
    .def("GetUseGhostCellsAsField", &entity::GhostCellRemove::GetUseGhostCellsAsField);
  BindFilterExecuteMethod<entity::GhostCellRemove>(ghostCellRemove);
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
  auto cleanGrid = BindClassWithDefaultConstructor<clean_grid::CleanGrid>(
    m, erase_existing_name, "CleanGrid");
  BindFilterFieldsToPassMethod<clean_grid::CleanGrid>(cleanGrid);
  cleanGrid
    .def("SetCompactPointFields",
         &clean_grid::CleanGrid::SetCompactPointFields,
         nb::arg("enabled"))
    .def("GetCompactPointFields", &clean_grid::CleanGrid::GetCompactPointFields)
    .def("SetMergePoints", &clean_grid::CleanGrid::SetMergePoints, nb::arg("enabled"))
    .def("GetMergePoints", &clean_grid::CleanGrid::GetMergePoints);
  cleanGrid
    .def(
      "SetTolerance",
      [](clean_grid::CleanGrid& self, double value) { self.SetTolerance(value); },
      nb::arg("value"))
    .def("GetTolerance", &clean_grid::CleanGrid::GetTolerance);
  cleanGrid
    .def("SetToleranceIsAbsolute",
         &clean_grid::CleanGrid::SetToleranceIsAbsolute,
         nb::arg("enabled"))
    .def("GetToleranceIsAbsolute", &clean_grid::CleanGrid::GetToleranceIsAbsolute)
    .def("SetRemoveDegenerateCells",
         &clean_grid::CleanGrid::SetRemoveDegenerateCells,
         nb::arg("enabled"))
    .def("GetRemoveDegenerateCells", &clean_grid::CleanGrid::GetRemoveDegenerateCells)
    .def("SetFastMerge", &clean_grid::CleanGrid::SetFastMerge, nb::arg("enabled"))
    .def("GetFastMerge", &clean_grid::CleanGrid::GetFastMerge);
  BindFilterExecuteMethod<clean_grid::CleanGrid>(cleanGrid);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_MESH_INFO
  auto ghostCellClassify =
    BindClassWithDefaultConstructor<mesh_info::GhostCellClassify>(
      m, erase_existing_name, "GhostCellClassify");
  ghostCellClassify
    .def("SetGhostCellName",
         &mesh_info::GhostCellClassify::SetGhostCellName,
         nb::arg("field_name"))
    .def("GetGhostCellName", &mesh_info::GhostCellClassify::GetGhostCellName);
  BindFilterExecuteMethod<mesh_info::GhostCellClassify>(ghostCellClassify);

  auto cellMeasures =
    BindClassWithDefaultConstructor<mesh_info::CellMeasures>(
      m, erase_existing_name, "CellMeasures");
  BindCastedProperty<mesh_info::CellMeasures,
                     mesh_info::IntegrationType,
                     long>(cellMeasures,
                           "SetMeasure",
                           "GetMeasure",
                           &mesh_info::CellMeasures::SetMeasure,
                           &mesh_info::CellMeasures::GetMeasure,
                           "measure");
  cellMeasures
    .def("SetMeasureToArcLength", &mesh_info::CellMeasures::SetMeasureToArcLength)
    .def("SetMeasureToArea", &mesh_info::CellMeasures::SetMeasureToArea)
    .def("SetMeasureToVolume", &mesh_info::CellMeasures::SetMeasureToVolume)
    .def("SetMeasureToAll", &mesh_info::CellMeasures::SetMeasureToAll)
    .def("SetCellMeasureName",
         &mesh_info::CellMeasures::SetCellMeasureName,
         nb::arg("name"))
    .def("GetCellMeasureName", &mesh_info::CellMeasures::GetCellMeasureName);
  BindFilterExecuteMethod<mesh_info::CellMeasures>(cellMeasures);

  auto meshQuality =
    BindClassWithDefaultConstructor<mesh_info::MeshQuality>(
      m, erase_existing_name, "MeshQuality");
  BindCastedProperty<mesh_info::MeshQuality,
                     mesh_info::CellMetric,
                     long>(meshQuality,
                           "SetMetric",
                           "GetMetric",
                           &mesh_info::MeshQuality::SetMetric,
                           &mesh_info::MeshQuality::GetMetric,
                           "metric");
  meshQuality
    .def("GetMetricName", &mesh_info::MeshQuality::GetMetricName);
  BindFilterExecuteMethod<mesh_info::MeshQuality>(meshQuality);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONNECTED_COMPONENTS
  auto cellSetConnectivity =
    BindClassWithDefaultConstructor<
      connected::CellSetConnectivity>(
      m, erase_existing_name, "CellSetConnectivity");
  BindFilterOutputFieldMethods<connected::CellSetConnectivity>(
    cellSetConnectivity);
  BindFilterExecuteMethod<connected::CellSetConnectivity>(
    cellSetConnectivity);

  auto imageConnectivity =
    BindClassWithDefaultConstructor<connected::ImageConnectivity>(
      m, erase_existing_name, "ImageConnectivity");
  BindFilterActiveFieldNameMethods<connected::ImageConnectivity>(
    imageConnectivity);
  BindFilterOutputFieldMethods<connected::ImageConnectivity>(
    imageConnectivity);
  BindFilterExecuteMethod<connected::ImageConnectivity>(
    imageConnectivity);
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_MULTI_BLOCK
  auto mergeDataSets =
    BindClassWithDefaultConstructor<multi_block::MergeDataSets>(
      m, erase_existing_name, "MergeDataSets");
  mergeDataSets
    .def("SetInvalidValue",
         &multi_block::MergeDataSets::SetInvalidValue,
         nb::arg("invalid_value"))
    .def("GetInvalidValue", &multi_block::MergeDataSets::GetInvalidValue);
  BindFilterExecuteMethod<multi_block::MergeDataSets>(mergeDataSets);
#endif
}
#else
void RegisterNanobindAdditionalFilterClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

} // namespace viskores::python::bindings
