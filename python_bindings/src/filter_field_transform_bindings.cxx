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

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_TRANSFORM
void RegisterNanobindFieldTransformClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("GenerateIds");
  nb::class_<viskores::filter::field_transform::GenerateIds>(m, "GenerateIds")
    .def(nb::init<>())
    .def("SetPointFieldName", &viskores::filter::field_transform::GenerateIds::SetPointFieldName)
    .def("GetPointFieldName", &viskores::filter::field_transform::GenerateIds::GetPointFieldName)
    .def("SetCellFieldName", &viskores::filter::field_transform::GenerateIds::SetCellFieldName)
    .def("GetCellFieldName", &viskores::filter::field_transform::GenerateIds::GetCellFieldName)
    .def("SetGeneratePointIds",
         &viskores::filter::field_transform::GenerateIds::SetGeneratePointIds)
    .def("GetGeneratePointIds",
         &viskores::filter::field_transform::GenerateIds::GetGeneratePointIds)
    .def("SetGenerateCellIds",
         &viskores::filter::field_transform::GenerateIds::SetGenerateCellIds)
    .def("GetGenerateCellIds",
         &viskores::filter::field_transform::GenerateIds::GetGenerateCellIds)
    .def("SetUseFloat", &viskores::filter::field_transform::GenerateIds::SetUseFloat)
    .def("GetUseFloat", &viskores::filter::field_transform::GenerateIds::GetUseFloat)
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::GenerateIds>);

  erase_existing_name("CompositeVectors");
  nb::class_<viskores::filter::field_transform::CompositeVectors>(m, "CompositeVectors")
    .def(nb::init<>())
    .def("SetOutputFieldName",
         &viskores::filter::field_transform::CompositeVectors::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::field_transform::CompositeVectors::GetOutputFieldName)
    .def("SetFieldNameList",
         [](viskores::filter::field_transform::CompositeVectors& self,
            nb::object fieldNamesObject,
            nb::object associationObject) {
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
    .def("GetNumberOfFields", &viskores::filter::field_transform::CompositeVectors::GetNumberOfFields)
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::CompositeVectors>);

  erase_existing_name("LogValues");
  nb::class_<viskores::filter::field_transform::LogValues>(m, "LogValues")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::field_transform::LogValues& self, const char* name) {
           self.SetActiveField(name);
         },
         nb::arg("name"))
    .def("GetActiveFieldName", &viskores::filter::field_transform::LogValues::GetActiveFieldName)
    .def("SetOutputFieldName", &viskores::filter::field_transform::LogValues::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::field_transform::LogValues::GetOutputFieldName)
    .def("SetMinValue",
         [](viskores::filter::field_transform::LogValues& self, double value) {
           self.SetMinValue(static_cast<viskores::FloatDefault>(value));
         },
         nb::arg("value"))
    .def("GetMinValue", &viskores::filter::field_transform::LogValues::GetMinValue)
    .def("SetBaseValueToE", &viskores::filter::field_transform::LogValues::SetBaseValueToE)
    .def("SetBaseValueTo2", &viskores::filter::field_transform::LogValues::SetBaseValueTo2)
    .def("SetBaseValueTo10", &viskores::filter::field_transform::LogValues::SetBaseValueTo10)
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::LogValues>);

  erase_existing_name("PointElevation");
  nb::class_<viskores::filter::field_transform::PointElevation>(m, "PointElevation")
    .def(nb::init<>())
    .def("SetOutputFieldName",
         &viskores::filter::field_transform::PointElevation::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::field_transform::PointElevation::GetOutputFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::field_transform::PointElevation& self, bool enabled) {
           self.SetUseCoordinateSystemAsField(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         &viskores::filter::field_transform::PointElevation::GetUseCoordinateSystemAsField)
    .def("SetLowPoint",
         [](viskores::filter::field_transform::PointElevation& self, nb::object pointObject) {
           self.SetLowPoint(ParseVec3(pointObject, viskores::Vec3f_32{ 0.f, 0.f, 0.f }));
         },
         nb::arg("point"))
    .def("SetHighPoint",
         [](viskores::filter::field_transform::PointElevation& self, nb::object pointObject) {
           self.SetHighPoint(ParseVec3(pointObject, viskores::Vec3f_32{ 0.f, 0.f, 1.f }));
         },
         nb::arg("point"))
    .def("SetRange",
         [](viskores::filter::field_transform::PointElevation& self, double low, double high) {
           self.SetRange(low, high);
         },
         nb::arg("low"),
         nb::arg("high"))
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::PointElevation>);

  erase_existing_name("PointTransform");
  nb::class_<viskores::filter::field_transform::PointTransform>(m, "PointTransform")
    .def(nb::init<>())
    .def("SetOutputFieldName",
         &viskores::filter::field_transform::PointTransform::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::field_transform::PointTransform::GetOutputFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::field_transform::PointTransform& self, bool enabled) {
           self.SetUseCoordinateSystemAsField(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         &viskores::filter::field_transform::PointTransform::GetUseCoordinateSystemAsField)
    .def("SetTranslation",
         [](viskores::filter::field_transform::PointTransform& self, nb::object valueObject) {
           self.SetTranslation(ParseVec3(valueObject, viskores::Vec3f_32{ 0.f, 0.f, 0.f }));
         },
         nb::arg("translation"))
    .def("SetScale",
         [](viskores::filter::field_transform::PointTransform& self, nb::object valueObject) {
           self.SetScale(ParseVec3(valueObject, viskores::Vec3f_32{ 1.f, 1.f, 1.f }));
         },
         nb::arg("scale"))
    .def("SetRotation",
         [](viskores::filter::field_transform::PointTransform& self,
            double angle,
            nb::object axisObject) {
           self.SetRotation(static_cast<viskores::FloatDefault>(angle),
                            ParseVec3(axisObject, viskores::Vec3f_32{ 0.f, 0.f, 1.f }));
         },
         nb::arg("angle"),
         nb::arg("axis"))
    .def("SetChangeCoordinateSystem",
         [](viskores::filter::field_transform::PointTransform& self, bool enabled) {
           self.SetChangeCoordinateSystem(enabled);
         },
         nb::arg("enabled"))
    .def("GetChangeCoordinateSystem",
         &viskores::filter::field_transform::PointTransform::GetChangeCoordinateSystem)
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::PointTransform>);

  erase_existing_name("CylindricalCoordinateTransform");
  nb::class_<viskores::filter::field_transform::CylindricalCoordinateTransform>(
    m, "CylindricalCoordinateTransform")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::field_transform::CylindricalCoordinateTransform& self,
            const char* name) { self.SetActiveField(name); },
         nb::arg("name"))
    .def("GetActiveFieldName",
         &viskores::filter::field_transform::CylindricalCoordinateTransform::GetActiveFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::field_transform::CylindricalCoordinateTransform& self, bool enabled) {
           self.SetUseCoordinateSystemAsField(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         &viskores::filter::field_transform::CylindricalCoordinateTransform::GetUseCoordinateSystemAsField)
    .def("SetCartesianToCylindrical",
         &viskores::filter::field_transform::CylindricalCoordinateTransform::SetCartesianToCylindrical)
    .def("SetCylindricalToCartesian",
         &viskores::filter::field_transform::CylindricalCoordinateTransform::SetCylindricalToCartesian)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::field_transform::CylindricalCoordinateTransform>);

  erase_existing_name("SphericalCoordinateTransform");
  nb::class_<viskores::filter::field_transform::SphericalCoordinateTransform>(
    m, "SphericalCoordinateTransform")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::field_transform::SphericalCoordinateTransform& self,
            const char* name) { self.SetActiveField(name); },
         nb::arg("name"))
    .def("GetActiveFieldName",
         &viskores::filter::field_transform::SphericalCoordinateTransform::GetActiveFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::field_transform::SphericalCoordinateTransform& self, bool enabled) {
           self.SetUseCoordinateSystemAsField(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         &viskores::filter::field_transform::SphericalCoordinateTransform::GetUseCoordinateSystemAsField)
    .def("SetCartesianToSpherical",
         &viskores::filter::field_transform::SphericalCoordinateTransform::SetCartesianToSpherical)
    .def("SetSphericalToCartesian",
         &viskores::filter::field_transform::SphericalCoordinateTransform::SetSphericalToCartesian)
    .def("Execute",
         &ExecuteFilterToPython<viskores::filter::field_transform::SphericalCoordinateTransform>);

  erase_existing_name("FieldToColors");
  nb::class_<viskores::filter::field_transform::FieldToColors>(m, "FieldToColors")
    .def("__init__",
         [](viskores::filter::field_transform::FieldToColors* self,
            nb::object colorTableObject) {
           if (colorTableObject.is_none())
           {
             new (self) viskores::filter::field_transform::FieldToColors();
           }
           else
           {
             auto colorTable = RequireColorTable(colorTableObject);
             if (!colorTable)
             {
               throw nb::python_error();
             }
             new (self) viskores::filter::field_transform::FieldToColors(*colorTable);
           }
         },
         nb::arg("color_table") = nb::none())
    .def("SetActiveField",
         [](viskores::filter::field_transform::FieldToColors& self, const char* name) {
           self.SetActiveField(name);
         },
         nb::arg("name"))
    .def("GetActiveFieldName",
         &viskores::filter::field_transform::FieldToColors::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::field_transform::FieldToColors::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::field_transform::FieldToColors::GetOutputFieldName)
    .def("SetColorTable",
         [](viskores::filter::field_transform::FieldToColors& self,
            nb::object colorTableObject) {
           auto colorTable = RequireColorTable(colorTableObject);
           if (!colorTable)
           {
             throw nb::python_error();
           }
           self.SetColorTable(*colorTable);
         },
         nb::arg("color_table"))
    .def("SetOutputToRGB", &viskores::filter::field_transform::FieldToColors::SetOutputToRGB)
    .def("SetOutputToRGBA", &viskores::filter::field_transform::FieldToColors::SetOutputToRGBA)
    .def("SetMappingToScalar",
         &viskores::filter::field_transform::FieldToColors::SetMappingToScalar)
    .def("SetMappingToMagnitude",
         &viskores::filter::field_transform::FieldToColors::SetMappingToMagnitude)
    .def("SetMappingToComponent",
         &viskores::filter::field_transform::FieldToColors::SetMappingToComponent)
    .def("SetMappingComponent",
         [](viskores::filter::field_transform::FieldToColors& self, long component) {
           self.SetMappingComponent(static_cast<viskores::IdComponent>(component));
         },
         nb::arg("component"))
    .def("SetNumberOfSamplingPoints",
         [](viskores::filter::field_transform::FieldToColors& self, long count) {
           self.SetNumberOfSamplingPoints(static_cast<viskores::Int32>(count));
         },
         nb::arg("count"))
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::FieldToColors>);

  erase_existing_name("Warp");
  nb::class_<viskores::filter::field_transform::Warp>(m, "Warp")
    .def(nb::init<>())
    .def("SetActiveField",
         [](viskores::filter::field_transform::Warp& self, const char* name) {
           self.SetActiveField(name);
         },
         nb::arg("name"))
    .def("GetActiveFieldName", &viskores::filter::field_transform::Warp::GetActiveFieldName)
    .def("SetOutputFieldName", &viskores::filter::field_transform::Warp::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::field_transform::Warp::GetOutputFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::field_transform::Warp& self, bool enabled) {
           self.SetUseCoordinateSystemAsField(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         &viskores::filter::field_transform::Warp::GetUseCoordinateSystemAsField)
    .def("SetDirectionField", &viskores::filter::field_transform::Warp::SetDirectionField)
    .def("GetDirectionFieldName", &viskores::filter::field_transform::Warp::GetDirectionFieldName)
    .def("SetConstantDirection",
         [](viskores::filter::field_transform::Warp& self, nb::object directionObject) {
           self.SetConstantDirection(
             ParseVec3(directionObject, viskores::Vec3f_32{ 0.f, 0.f, 1.f }));
         },
         nb::arg("direction"))
    .def("GetConstantDirection",
         [](const viskores::filter::field_transform::Warp& self) {
           const auto direction = self.GetConstantDirection();
           return nb::make_tuple(direction[0], direction[1], direction[2]);
         })
    .def("SetUseConstantDirection",
         [](viskores::filter::field_transform::Warp& self, bool enabled) {
           self.SetUseConstantDirection(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseConstantDirection",
         &viskores::filter::field_transform::Warp::GetUseConstantDirection)
    .def("SetScaleField", &viskores::filter::field_transform::Warp::SetScaleField)
    .def("GetScaleFieldName", &viskores::filter::field_transform::Warp::GetScaleFieldName)
    .def("SetUseScaleField",
         [](viskores::filter::field_transform::Warp& self, bool enabled) {
           self.SetUseScaleField(enabled);
         },
         nb::arg("enabled"))
    .def("GetUseScaleField", &viskores::filter::field_transform::Warp::GetUseScaleField)
    .def("SetScaleFactor",
         [](viskores::filter::field_transform::Warp& self, double scale) {
           self.SetScaleFactor(static_cast<viskores::FloatDefault>(scale));
         },
         nb::arg("scale"))
    .def("GetScaleFactor", &viskores::filter::field_transform::Warp::GetScaleFactor)
    .def("SetChangeCoordinateSystem",
         [](viskores::filter::field_transform::Warp& self, bool enabled) {
           self.SetChangeCoordinateSystem(enabled);
         },
         nb::arg("enabled"))
    .def("GetChangeCoordinateSystem",
         &viskores::filter::field_transform::Warp::GetChangeCoordinateSystem)
    .def("Execute", &ExecuteFilterToPython<viskores::filter::field_transform::Warp>);
}
#else
void RegisterNanobindFieldTransformClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
