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

namespace field_transform = viskores::filter::field_transform;

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM
void RegisterNanobindFieldTransformClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  auto generateIds =
    BindClassWithDefaultConstructor<field_transform::GenerateIds>(
      m, erase_existing_name, "GenerateIds");
  generateIds
    .def("SetPointFieldName", &field_transform::GenerateIds::SetPointFieldName)
    .def("GetPointFieldName", &field_transform::GenerateIds::GetPointFieldName)
    .def("SetCellFieldName", &field_transform::GenerateIds::SetCellFieldName)
    .def("GetCellFieldName", &field_transform::GenerateIds::GetCellFieldName)
    .def("SetGeneratePointIds",
         &field_transform::GenerateIds::SetGeneratePointIds)
    .def("GetGeneratePointIds",
         &field_transform::GenerateIds::GetGeneratePointIds)
    .def("SetGenerateCellIds", &field_transform::GenerateIds::SetGenerateCellIds)
    .def("GetGenerateCellIds", &field_transform::GenerateIds::GetGenerateCellIds)
    .def("SetUseFloat", &field_transform::GenerateIds::SetUseFloat)
    .def("GetUseFloat", &field_transform::GenerateIds::GetUseFloat);
  BindFilterExecuteMethod<field_transform::GenerateIds>(generateIds);

  auto compositeVectors =
    BindClassWithDefaultConstructor<field_transform::CompositeVectors>(
      m, erase_existing_name, "CompositeVectors");
  BindFilterOutputFieldMethods<field_transform::CompositeVectors>(
    compositeVectors);
  compositeVectors
    .def(
      "SetFieldNameList",
      [](field_transform::CompositeVectors& self,
         nb::object fieldNamesObject,
         nb::object associationObject)
      {
        if (!nb::isinstance<nb::sequence>(fieldNamesObject) ||
            nb::isinstance<nb::str>(fieldNamesObject))
        {
          throw std::runtime_error("Expected a sequence of field names.");
        }
        nb::sequence sequence = nb::borrow<nb::sequence>(fieldNamesObject);
        std::vector<std::string> fieldNames;
        const size_t size = static_cast<size_t>(nb::len(sequence));
        fieldNames.reserve(static_cast<std::size_t>(size));
        for (size_t index = 0; index < size; ++index)
        {
          nb::handle item = sequence[index];
          if (!nb::isinstance<nb::str>(item))
          {
            throw std::runtime_error("Field names must be strings.");
          }
          fieldNames.emplace_back(nb::cast<std::string>(item));
        }
        self.SetFieldNameList(
          fieldNames, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("field_names"),
      nb::arg("association") = nb::none())
    .def("GetNumberOfFields",
         &field_transform::CompositeVectors::GetNumberOfFields);
  BindFilterExecuteMethod<field_transform::CompositeVectors>(compositeVectors);

  auto logValues =
    BindClassWithDefaultConstructor<field_transform::LogValues>(
      m, erase_existing_name, "LogValues");
  BindFilterActiveFieldNameMethods<field_transform::LogValues>(logValues);
  BindFilterOutputFieldMethods<field_transform::LogValues>(logValues);
  BindCastedProperty<field_transform::LogValues, viskores::FloatDefault, double>(
    logValues,
    "SetMinValue",
    "GetMinValue",
    &field_transform::LogValues::SetMinValue,
    &field_transform::LogValues::GetMinValue);
  logValues
    .def("SetBaseValueToE", &field_transform::LogValues::SetBaseValueToE)
    .def("SetBaseValueTo2", &field_transform::LogValues::SetBaseValueTo2)
    .def("SetBaseValueTo10", &field_transform::LogValues::SetBaseValueTo10);
  BindFilterExecuteMethod<field_transform::LogValues>(logValues);

  auto pointElevation =
    BindClassWithDefaultConstructor<field_transform::PointElevation>(
      m, erase_existing_name, "PointElevation");
  BindFilterOutputFieldMethods<field_transform::PointElevation>(pointElevation);
  BindFilterCoordinateSystemFieldMethods<field_transform::PointElevation>(
    pointElevation);
  pointElevation
    .def(
      "SetLowPoint",
      [](field_transform::PointElevation& self, nb::object pointObject)
      { self.SetLowPoint(ParseVec3(pointObject, viskores::Vec3f_32{ 0.f, 0.f, 0.f })); },
      nb::arg("point"))
    .def(
      "SetHighPoint",
      [](field_transform::PointElevation& self, nb::object pointObject)
      { self.SetHighPoint(ParseVec3(pointObject, viskores::Vec3f_32{ 0.f, 0.f, 1.f })); },
      nb::arg("point"))
    .def(
      "SetRange",
      [](field_transform::PointElevation& self, double low, double high)
      { self.SetRange(low, high); },
      nb::arg("low"),
      nb::arg("high"));
  BindFilterExecuteMethod<field_transform::PointElevation>(pointElevation);

  auto pointTransform =
    BindClassWithDefaultConstructor<field_transform::PointTransform>(
      m, erase_existing_name, "PointTransform");
  BindFilterOutputFieldMethods<field_transform::PointTransform>(pointTransform);
  BindFilterCoordinateSystemFieldMethods<field_transform::PointTransform>(
    pointTransform);
  pointTransform
    .def(
      "SetTranslation",
      [](field_transform::PointTransform& self, nb::object valueObject)
      { self.SetTranslation(ParseVec3(valueObject, viskores::Vec3f_32{ 0.f, 0.f, 0.f })); },
      nb::arg("translation"))
    .def(
      "SetScale",
      [](field_transform::PointTransform& self, nb::object valueObject)
      { self.SetScale(ParseVec3(valueObject, viskores::Vec3f_32{ 1.f, 1.f, 1.f })); },
      nb::arg("scale"))
    .def(
      "SetRotation",
      [](field_transform::PointTransform& self,
         double angle,
         nb::object axisObject)
      {
        self.SetRotation(static_cast<viskores::FloatDefault>(angle),
                         ParseVec3(axisObject, viskores::Vec3f_32{ 0.f, 0.f, 1.f }));
      },
      nb::arg("angle"),
      nb::arg("axis"))
    .def("SetChangeCoordinateSystem",
         &field_transform::PointTransform::SetChangeCoordinateSystem,
         nb::arg("enabled"))
    .def("GetChangeCoordinateSystem", &field_transform::PointTransform::GetChangeCoordinateSystem);
  BindFilterExecuteMethod<field_transform::PointTransform>(pointTransform);

  auto cylindricalCoordinateTransform =
    BindClassWithDefaultConstructor<
      field_transform::CylindricalCoordinateTransform>(
      m, erase_existing_name, "CylindricalCoordinateTransform");
  BindFilterActiveFieldNameMethods<
    field_transform::CylindricalCoordinateTransform>(
    cylindricalCoordinateTransform);
  BindFilterCoordinateSystemFieldMethods<
    field_transform::CylindricalCoordinateTransform>(
    cylindricalCoordinateTransform);
  cylindricalCoordinateTransform
    .def(
      "SetCartesianToCylindrical",
      &field_transform::CylindricalCoordinateTransform::SetCartesianToCylindrical)
    .def(
      "SetCylindricalToCartesian",
      &field_transform::CylindricalCoordinateTransform::
        SetCylindricalToCartesian);
  BindFilterExecuteMethod<field_transform::CylindricalCoordinateTransform>(
    cylindricalCoordinateTransform);

  auto sphericalCoordinateTransform =
    BindClassWithDefaultConstructor<
      field_transform::SphericalCoordinateTransform>(
      m, erase_existing_name, "SphericalCoordinateTransform");
  BindFilterActiveFieldNameMethods<
    field_transform::SphericalCoordinateTransform>(sphericalCoordinateTransform);
  BindFilterCoordinateSystemFieldMethods<
    field_transform::SphericalCoordinateTransform>(sphericalCoordinateTransform);
  sphericalCoordinateTransform
    .def("SetCartesianToSpherical",
         &field_transform::SphericalCoordinateTransform::SetCartesianToSpherical)
    .def("SetSphericalToCartesian",
         &field_transform::SphericalCoordinateTransform::SetSphericalToCartesian);
  BindFilterExecuteMethod<field_transform::SphericalCoordinateTransform>(
    sphericalCoordinateTransform);

  erase_existing_name("FieldToColors");
  auto fieldToColors = nb::class_<field_transform::FieldToColors>(
                         m, "FieldToColors", doc::ClassDoc("FieldToColors"))
    .def(
      "__init__",
      [](field_transform::FieldToColors* self, nb::object colorTableObject)
      {
        if (colorTableObject.is_none())
        {
          new (self) field_transform::FieldToColors();
        }
        else
        {
          auto colorTable = RequireColorTable(colorTableObject);
          if (!colorTable)
          {
            throw nb::python_error();
          }
          new (self) field_transform::FieldToColors(*colorTable);
        }
      },
      nb::arg("color_table") = nb::none());
  BindFilterActiveFieldNameMethods<field_transform::FieldToColors>(
    fieldToColors);
  BindFilterOutputFieldMethods<field_transform::FieldToColors>(fieldToColors);
  fieldToColors
    .def(
      "SetColorTable",
      [](field_transform::FieldToColors& self, nb::object colorTableObject)
      {
        auto colorTable = RequireColorTable(colorTableObject);
        if (!colorTable)
        {
          throw nb::python_error();
        }
        self.SetColorTable(*colorTable);
      },
      nb::arg("color_table"))
    .def("SetOutputToRGB", &field_transform::FieldToColors::SetOutputToRGB)
    .def("SetOutputToRGBA", &field_transform::FieldToColors::SetOutputToRGBA)
    .def("SetMappingToScalar",
         &field_transform::FieldToColors::SetMappingToScalar)
    .def("SetMappingToMagnitude",
         &field_transform::FieldToColors::SetMappingToMagnitude)
    .def("SetMappingToComponent", &field_transform::FieldToColors::SetMappingToComponent);
  BindCastedSetter<field_transform::FieldToColors, viskores::IdComponent, long>(
    fieldToColors,
    "SetMappingComponent",
    &field_transform::FieldToColors::SetMappingComponent,
    "component");
  BindCastedSetter<field_transform::FieldToColors, viskores::Int32, long>(
    fieldToColors,
    "SetNumberOfSamplingPoints",
    &field_transform::FieldToColors::SetNumberOfSamplingPoints,
    "count");
  BindFilterExecuteMethod<field_transform::FieldToColors>(fieldToColors);

  auto warp =
    BindClassWithDefaultConstructor<field_transform::Warp>(
      m, erase_existing_name, "Warp");
  BindFilterActiveFieldNameMethods<field_transform::Warp>(warp);
  BindFilterOutputFieldMethods<field_transform::Warp>(warp);
  BindFilterCoordinateSystemFieldMethods<field_transform::Warp>(warp);
  warp
    .def("SetDirectionField", &field_transform::Warp::SetDirectionField)
    .def("GetDirectionFieldName", &field_transform::Warp::GetDirectionFieldName)
    .def(
      "SetConstantDirection",
      [](field_transform::Warp& self, nb::object directionObject)
      {
        self.SetConstantDirection(ParseVec3(directionObject, viskores::Vec3f_32{ 0.f, 0.f, 1.f }));
      },
      nb::arg("direction"))
    .def("GetConstantDirection",
         [](const field_transform::Warp& self)
         { return Vec3ToTuple(self.GetConstantDirection()); })
    .def("SetUseConstantDirection", &field_transform::Warp::SetUseConstantDirection, nb::arg("enabled"))
    .def("GetUseConstantDirection", &field_transform::Warp::GetUseConstantDirection)
    .def("SetScaleField", &field_transform::Warp::SetScaleField)
    .def("GetScaleFieldName", &field_transform::Warp::GetScaleFieldName)
    .def("SetUseScaleField", &field_transform::Warp::SetUseScaleField, nb::arg("enabled"))
    .def("GetUseScaleField", &field_transform::Warp::GetUseScaleField);
  BindCastedProperty<field_transform::Warp, viskores::FloatDefault, double>(
    warp,
    "SetScaleFactor",
    "GetScaleFactor",
    &field_transform::Warp::SetScaleFactor,
    &field_transform::Warp::GetScaleFactor,
    "scale");
  warp
    .def("SetChangeCoordinateSystem",
         &field_transform::Warp::SetChangeCoordinateSystem,
         nb::arg("enabled"))
    .def("GetChangeCoordinateSystem", &field_transform::Warp::GetChangeCoordinateSystem);
  BindFilterExecuteMethod<field_transform::Warp>(warp);
}
#else
void RegisterNanobindFieldTransformClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
