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

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION
void RegisterNanobindFieldConversionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("CellAverage");
  nb::class_<viskores::filter::field_conversion::CellAverage>(
    m, "CellAverage", doc::ClassDoc("CellAverage"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::field_conversion::CellAverage& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::field_conversion::CellAverage::GetActiveFieldName)
    .def("SetOutputFieldName", &viskores::filter::field_conversion::CellAverage::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::field_conversion::CellAverage::GetOutputFieldName)
    .def(
      "SetUseCoordinateSystemAsField",
      [](viskores::filter::field_conversion::CellAverage& self, bool enable)
      { self.SetUseCoordinateSystemAsField(enable); },
      nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         [](const viskores::filter::field_conversion::CellAverage& self)
         { return self.GetUseCoordinateSystemAsField(); })
    .def(
      "Execute",
      [](viskores::filter::field_conversion::CellAverage& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter)
    .def("__repr__",
         [](const viskores::filter::field_conversion::CellAverage& self)
         {
           std::ostringstream stream;
           stream << "viskores.filter.field_conversion.CellAverage(active_field=\""
                  << self.GetActiveFieldName() << "\")";
           return stream.str();
         });

  erase_existing_name("PointAverage");
  nb::class_<viskores::filter::field_conversion::PointAverage>(
    m, "PointAverage", doc::ClassDoc("PointAverage"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::field_conversion::PointAverage& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::field_conversion::PointAverage::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::field_conversion::PointAverage::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::field_conversion::PointAverage::GetOutputFieldName)
    .def(
      "SetUseCoordinateSystemAsField",
      [](viskores::filter::field_conversion::PointAverage& self, bool enable)
      { self.SetUseCoordinateSystemAsField(enable); },
      nb::arg("enabled"))
    .def("GetUseCoordinateSystemAsField",
         [](const viskores::filter::field_conversion::PointAverage& self)
         { return self.GetUseCoordinateSystemAsField(); })
    .def(
      "Execute",
      [](viskores::filter::field_conversion::PointAverage& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter)
    .def("__repr__",
         [](const viskores::filter::field_conversion::PointAverage& self)
         {
           std::ostringstream stream;
           stream << "viskores.filter.field_conversion.PointAverage(active_field=\""
                  << self.GetActiveFieldName() << "\")";
           return stream.str();
         });
}
#else
void RegisterNanobindFieldConversionClasses(nb::module_&, const std::function<void(const char*)>&)
{
}
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_VECTOR_ANALYSIS
void RegisterNanobindVectorAnalysisClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("VectorMagnitude");
  nb::class_<viskores::filter::vector_analysis::VectorMagnitude>(
    m, "VectorMagnitude", doc::ClassDoc("VectorMagnitude"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::vector_analysis::VectorMagnitude& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName",
         &viskores::filter::vector_analysis::VectorMagnitude::GetActiveFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::vector_analysis::VectorMagnitude::SetOutputFieldName)
    .def("GetOutputFieldName",
         &viskores::filter::vector_analysis::VectorMagnitude::GetOutputFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::vector_analysis::VectorMagnitude& self, bool enable)
         { self.SetUseCoordinateSystemAsField(enable); })
    .def("GetUseCoordinateSystemAsField",
         [](const viskores::filter::vector_analysis::VectorMagnitude& self)
         { return self.GetUseCoordinateSystemAsField(); })
    .def(
      "Execute",
      [](viskores::filter::vector_analysis::VectorMagnitude& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Gradient");
  nb::class_<viskores::filter::vector_analysis::Gradient>(m, "Gradient", doc::ClassDoc("Gradient"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::vector_analysis::Gradient& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::vector_analysis::Gradient::GetActiveFieldName)
    .def("SetOutputFieldName", &viskores::filter::vector_analysis::Gradient::SetOutputFieldName)
    .def("GetOutputFieldName", &viskores::filter::vector_analysis::Gradient::GetOutputFieldName)
    .def("SetUseCoordinateSystemAsField",
         [](viskores::filter::vector_analysis::Gradient& self, bool enable)
         { self.SetUseCoordinateSystemAsField(enable); })
    .def("GetUseCoordinateSystemAsField",
         [](const viskores::filter::vector_analysis::Gradient& self)
         { return self.GetUseCoordinateSystemAsField(); })
    .def("SetComputePointGradient",
         &viskores::filter::vector_analysis::Gradient::SetComputePointGradient)
    .def("GetComputePointGradient",
         &viskores::filter::vector_analysis::Gradient::GetComputePointGradient)
    .def("SetComputeDivergence", &viskores::filter::vector_analysis::Gradient::SetComputeDivergence)
    .def("GetComputeDivergence", &viskores::filter::vector_analysis::Gradient::GetComputeDivergence)
    .def("SetDivergenceName", &viskores::filter::vector_analysis::Gradient::SetDivergenceName)
    .def("GetDivergenceName", &viskores::filter::vector_analysis::Gradient::GetDivergenceName)
    .def("SetComputeVorticity", &viskores::filter::vector_analysis::Gradient::SetComputeVorticity)
    .def("GetComputeVorticity", &viskores::filter::vector_analysis::Gradient::GetComputeVorticity)
    .def("SetVorticityName", &viskores::filter::vector_analysis::Gradient::SetVorticityName)
    .def("GetVorticityName", &viskores::filter::vector_analysis::Gradient::GetVorticityName)
    .def("SetComputeQCriterion", &viskores::filter::vector_analysis::Gradient::SetComputeQCriterion)
    .def("GetComputeQCriterion", &viskores::filter::vector_analysis::Gradient::GetComputeQCriterion)
    .def("SetQCriterionName", &viskores::filter::vector_analysis::Gradient::SetQCriterionName)
    .def("GetQCriterionName", &viskores::filter::vector_analysis::Gradient::GetQCriterionName)
    .def("SetComputeGradient", &viskores::filter::vector_analysis::Gradient::SetComputeGradient)
    .def("GetComputeGradient", &viskores::filter::vector_analysis::Gradient::GetComputeGradient)
    .def("SetColumnMajorOrdering",
         &viskores::filter::vector_analysis::Gradient::SetColumnMajorOrdering)
    .def("SetRowMajorOrdering", &viskores::filter::vector_analysis::Gradient::SetRowMajorOrdering)
    .def(
      "Execute",
      [](viskores::filter::vector_analysis::Gradient& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("CrossProduct");
  nb::class_<viskores::filter::vector_analysis::CrossProduct>(
    m, "CrossProduct", doc::ClassDoc("CrossProduct"))
    .def(nb::init<>())
    .def(
      "SetPrimaryField",
      [](viskores::filter::vector_analysis::CrossProduct& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetPrimaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetPrimaryFieldName",
         &viskores::filter::vector_analysis::CrossProduct::GetPrimaryFieldName)
    .def("GetPrimaryFieldAssociation",
         &viskores::filter::vector_analysis::CrossProduct::GetPrimaryFieldAssociation)
    .def("SetUseCoordinateSystemAsPrimaryField",
         &viskores::filter::vector_analysis::CrossProduct::SetUseCoordinateSystemAsPrimaryField)
    .def("GetUseCoordinateSystemAsPrimaryField",
         &viskores::filter::vector_analysis::CrossProduct::GetUseCoordinateSystemAsPrimaryField)
    .def("SetPrimaryCoordinateSystem",
         &viskores::filter::vector_analysis::CrossProduct::SetPrimaryCoordinateSystem)
    .def("GetPrimaryCoordinateSystemIndex",
         &viskores::filter::vector_analysis::CrossProduct::GetPrimaryCoordinateSystemIndex)
    .def(
      "SetSecondaryField",
      [](viskores::filter::vector_analysis::CrossProduct& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetSecondaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetSecondaryFieldName",
         &viskores::filter::vector_analysis::CrossProduct::GetSecondaryFieldName)
    .def("GetSecondaryFieldAssociation",
         &viskores::filter::vector_analysis::CrossProduct::GetSecondaryFieldAssociation)
    .def("SetUseCoordinateSystemAsSecondaryField",
         &viskores::filter::vector_analysis::CrossProduct::SetUseCoordinateSystemAsSecondaryField)
    .def("GetUseCoordinateSystemAsSecondaryField",
         &viskores::filter::vector_analysis::CrossProduct::GetUseCoordinateSystemAsSecondaryField)
    .def("SetSecondaryCoordinateSystem",
         &viskores::filter::vector_analysis::CrossProduct::SetSecondaryCoordinateSystem)
    .def("GetSecondaryCoordinateSystemIndex",
         &viskores::filter::vector_analysis::CrossProduct::GetSecondaryCoordinateSystemIndex)
    .def(
      "Execute",
      [](viskores::filter::vector_analysis::CrossProduct& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("DotProduct");
  nb::class_<viskores::filter::vector_analysis::DotProduct>(
    m, "DotProduct", doc::ClassDoc("DotProduct"))
    .def(nb::init<>())
    .def(
      "SetPrimaryField",
      [](viskores::filter::vector_analysis::DotProduct& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetPrimaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetPrimaryFieldName", &viskores::filter::vector_analysis::DotProduct::GetPrimaryFieldName)
    .def("GetPrimaryFieldAssociation",
         &viskores::filter::vector_analysis::DotProduct::GetPrimaryFieldAssociation)
    .def("SetUseCoordinateSystemAsPrimaryField",
         &viskores::filter::vector_analysis::DotProduct::SetUseCoordinateSystemAsPrimaryField)
    .def("GetUseCoordinateSystemAsPrimaryField",
         &viskores::filter::vector_analysis::DotProduct::GetUseCoordinateSystemAsPrimaryField)
    .def("SetPrimaryCoordinateSystem",
         &viskores::filter::vector_analysis::DotProduct::SetPrimaryCoordinateSystem)
    .def("GetPrimaryCoordinateSystemIndex",
         &viskores::filter::vector_analysis::DotProduct::GetPrimaryCoordinateSystemIndex)
    .def(
      "SetSecondaryField",
      [](viskores::filter::vector_analysis::DotProduct& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetSecondaryField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetSecondaryFieldName",
         &viskores::filter::vector_analysis::DotProduct::GetSecondaryFieldName)
    .def("GetSecondaryFieldAssociation",
         &viskores::filter::vector_analysis::DotProduct::GetSecondaryFieldAssociation)
    .def("SetUseCoordinateSystemAsSecondaryField",
         &viskores::filter::vector_analysis::DotProduct::SetUseCoordinateSystemAsSecondaryField)
    .def("GetUseCoordinateSystemAsSecondaryField",
         &viskores::filter::vector_analysis::DotProduct::GetUseCoordinateSystemAsSecondaryField)
    .def("SetSecondaryCoordinateSystem",
         &viskores::filter::vector_analysis::DotProduct::SetSecondaryCoordinateSystem)
    .def("GetSecondaryCoordinateSystemIndex",
         &viskores::filter::vector_analysis::DotProduct::GetSecondaryCoordinateSystemIndex)
    .def(
      "Execute",
      [](viskores::filter::vector_analysis::DotProduct& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("SurfaceNormals");
  nb::class_<viskores::filter::vector_analysis::SurfaceNormals>(
    m, "SurfaceNormals", doc::ClassDoc("SurfaceNormals"))
    .def(nb::init<>())
    .def("SetGenerateCellNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::SetGenerateCellNormals)
    .def("GetGenerateCellNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::GetGenerateCellNormals)
    .def("SetNormalizeCellNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::SetNormalizeCellNormals)
    .def("GetNormalizeCellNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::GetNormalizeCellNormals)
    .def("SetGeneratePointNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::SetGeneratePointNormals)
    .def("GetGeneratePointNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::GetGeneratePointNormals)
    .def("SetCellNormalsName",
         &viskores::filter::vector_analysis::SurfaceNormals::SetCellNormalsName)
    .def("GetCellNormalsName",
         &viskores::filter::vector_analysis::SurfaceNormals::GetCellNormalsName)
    .def("SetPointNormalsName",
         &viskores::filter::vector_analysis::SurfaceNormals::SetPointNormalsName)
    .def("GetPointNormalsName",
         &viskores::filter::vector_analysis::SurfaceNormals::GetPointNormalsName)
    .def("SetAutoOrientNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::SetAutoOrientNormals)
    .def("GetAutoOrientNormals",
         &viskores::filter::vector_analysis::SurfaceNormals::GetAutoOrientNormals)
    .def("SetFlipNormals", &viskores::filter::vector_analysis::SurfaceNormals::SetFlipNormals)
    .def("GetFlipNormals", &viskores::filter::vector_analysis::SurfaceNormals::GetFlipNormals)
    .def("SetConsistency", &viskores::filter::vector_analysis::SurfaceNormals::SetConsistency)
    .def("GetConsistency", &viskores::filter::vector_analysis::SurfaceNormals::GetConsistency)
    .def(
      "Execute",
      [](viskores::filter::vector_analysis::SurfaceNormals& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);
}
#else
void RegisterNanobindVectorAnalysisClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
void RegisterNanobindContourClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Contour");
  nb::class_<viskores::filter::contour::Contour>(m, "Contour", doc::ClassDoc("Contour"))
    .def(nb::init<>())
    .def("SetIsoValue",
         nb::overload_cast<viskores::Float64>(&viskores::filter::contour::Contour::SetIsoValue))
    .def(
      "SetIsoValue",
      [](viskores::filter::contour::Contour& self, long long index, double value)
      {
        self.SetIsoValue(static_cast<viskores::Id>(index), static_cast<viskores::Float64>(value));
      },
      nb::arg("index"),
      nb::arg("value"))
    .def(
      "SetIsoValues",
      [](viskores::filter::contour::Contour& self, nb::object valuesObject)
      { self.SetIsoValues(ParseIsoValues(valuesObject)); },
      nb::arg("values"))
    .def(
      "SetActiveField",
      [](viskores::filter::contour::Contour& self, const char* name, nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("SetGenerateNormals", &viskores::filter::contour::Contour::SetGenerateNormals)
    .def("GetGenerateNormals", &viskores::filter::contour::Contour::GetGenerateNormals)
    .def("SetComputeFastNormals", &viskores::filter::contour::Contour::SetComputeFastNormals)
    .def("GetComputeFastNormals", &viskores::filter::contour::Contour::GetComputeFastNormals)
    .def(
      "SetFieldsToPass",
      [](viskores::filter::contour::Contour& self, nb::object fieldsObject)
      {
        auto& selection = self.GetFieldsToPass();
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
        for (size_t index = 0; index < static_cast<size_t>(nb::len(sequence)); ++index)
        {
          nb::handle item = sequence[index];
          if (!nb::isinstance<nb::str>(item))
          {
            throw std::runtime_error("fields must contain only strings.");
          }
          selection.AddField(nb::cast<std::string>(item));
        }
      },
      nb::arg("fields"))
    .def("SetMergeDuplicatePoints", &viskores::filter::contour::Contour::SetMergeDuplicatePoints)
    .def("GetMergeDuplicatePoints", &viskores::filter::contour::Contour::GetMergeDuplicatePoints)
    .def(
      "Execute",
      [](viskores::filter::contour::Contour& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ContourMarchingCells");
  nb::class_<viskores::filter::contour::ContourMarchingCells>(
    m, "ContourMarchingCells", doc::ClassDoc("ContourMarchingCells"))
    .def(nb::init<>())
    .def("SetIsoValue",
         nb::overload_cast<viskores::Float64>(
           &viskores::filter::contour::ContourMarchingCells::SetIsoValue))
    .def(
      "SetActiveField",
      [](viskores::filter::contour::ContourMarchingCells& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("SetGenerateNormals", &viskores::filter::contour::ContourMarchingCells::SetGenerateNormals)
    .def("SetMergeDuplicatePoints",
         &viskores::filter::contour::ContourMarchingCells::SetMergeDuplicatePoints)
    .def(
      "Execute",
      [](viskores::filter::contour::ContourMarchingCells& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ClipWithImplicitFunction");
  nb::class_<viskores::filter::contour::ClipWithImplicitFunction>(
    m, "ClipWithImplicitFunction", doc::ClassDoc("ClipWithImplicitFunction"))
    .def(nb::init<>())
    .def(
      "SetImplicitFunction",
      [](viskores::filter::contour::ClipWithImplicitFunction& self, nb::object functionObject)
      {
        viskores::ImplicitFunctionGeneral function;
        if (!ParseImplicitFunction(functionObject, function))
        {
          throw std::runtime_error("Implicit function must be a viskores.Box, viskores.Cylinder, "
                                   "viskores.Plane, or viskores.Sphere.");
        }
        self.SetImplicitFunction(function);
      },
      nb::arg("function"))
    .def("SetOffset",
         &viskores::filter::contour::ClipWithImplicitFunction::SetOffset,
         nb::arg("offset"))
    .def("GetOffset", &viskores::filter::contour::ClipWithImplicitFunction::GetOffset)
    .def("SetInvertClip",
         &viskores::filter::contour::ClipWithImplicitFunction::SetInvertClip,
         nb::arg("invert"))
    .def(
      "SetFieldsToPass",
      [](viskores::filter::contour::ClipWithImplicitFunction& self, nb::object fieldsObject)
      {
        auto& selection = self.GetFieldsToPass();
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
        for (size_t index = 0; index < static_cast<size_t>(nb::len(sequence)); ++index)
        {
          nb::handle item = sequence[index];
          if (!nb::isinstance<nb::str>(item))
          {
            throw std::runtime_error("fields must contain only strings.");
          }
          selection.AddField(nb::cast<std::string>(item));
        }
      },
      nb::arg("fields"))
    .def(
      "Execute",
      [](viskores::filter::contour::ClipWithImplicitFunction& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("Slice");
  nb::class_<viskores::filter::contour::Slice, viskores::filter::contour::Contour>(
    m, "Slice", doc::ClassDoc("Slice"))
    .def(nb::init<>())
    .def(
      "SetImplicitFunction",
      [](viskores::filter::contour::Slice& self, nb::object functionObject)
      {
        viskores::ImplicitFunctionGeneral function;
        if (!ParseImplicitFunction(functionObject, function))
        {
          throw std::runtime_error("Implicit function must be a viskores.Box, viskores.Cylinder, "
                                   "viskores.Plane, or viskores.Sphere.");
        }
        self.SetImplicitFunction(function);
      },
      nb::arg("function"))
    .def(
      "Execute",
      [](viskores::filter::contour::Slice& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("SliceMultiple");
  nb::class_<viskores::filter::contour::SliceMultiple, viskores::filter::contour::Contour>(
    m, "SliceMultiple", doc::ClassDoc("SliceMultiple"))
    .def(nb::init<>())
    .def(
      "AddImplicitFunction",
      [](viskores::filter::contour::SliceMultiple& self, nb::object functionObject)
      {
        viskores::ImplicitFunctionGeneral function;
        if (!ParseImplicitFunction(functionObject, function))
        {
          throw std::runtime_error("Implicit function must be a viskores.Box, viskores.Cylinder, "
                                   "viskores.Plane, or viskores.Sphere.");
        }
        self.AddImplicitFunction(function);
      },
      nb::arg("function"))
    .def(
      "Execute",
      [](viskores::filter::contour::SliceMultiple& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("ClipWithField");
  nb::class_<viskores::filter::contour::ClipWithField>(
    m, "ClipWithField", doc::ClassDoc("ClipWithField"))
    .def(nb::init<>())
    .def(
      "SetActiveField",
      [](viskores::filter::contour::ClipWithField& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("GetActiveFieldName", &viskores::filter::contour::ClipWithField::GetActiveFieldName)
    .def("SetClipValue", &viskores::filter::contour::ClipWithField::SetClipValue, nb::arg("value"))
    .def("GetClipValue", &viskores::filter::contour::ClipWithField::GetClipValue)
    .def(
      "SetInvertClip", &viskores::filter::contour::ClipWithField::SetInvertClip, nb::arg("invert"))
    .def("GetInvertClip", &viskores::filter::contour::ClipWithField::GetInvertClip)
    .def(
      "SetFieldsToPass",
      [](viskores::filter::contour::ClipWithField& self, nb::object fieldsObject)
      {
        auto& selection = self.GetFieldsToPass();
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
        for (size_t index = 0; index < static_cast<size_t>(nb::len(sequence)); ++index)
        {
          nb::handle item = sequence[index];
          if (!nb::isinstance<nb::str>(item))
          {
            throw std::runtime_error("fields must contain only strings.");
          }
          selection.AddField(nb::cast<std::string>(item));
        }
      },
      nb::arg("fields"))
    .def(
      "Execute",
      [](viskores::filter::contour::ClipWithField& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);

  erase_existing_name("MIRFilter");
  nb::class_<viskores::filter::contour::MIRFilter>(m, "MIRFilter", doc::ClassDoc("MIRFilter"))
    .def(nb::init<>())
    .def("SetPositionCellSetName",
         &viskores::filter::contour::MIRFilter::SetPositionCellSetName,
         nb::arg("name"))
    .def("SetLengthCellSetName",
         &viskores::filter::contour::MIRFilter::SetLengthCellSetName,
         nb::arg("name"))
    .def("SetIDWholeSetName",
         &viskores::filter::contour::MIRFilter::SetIDWholeSetName,
         nb::arg("name"))
    .def("SetVFWholeSetName",
         &viskores::filter::contour::MIRFilter::SetVFWholeSetName,
         nb::arg("name"))
    .def("SetMaxPercentError",
         &viskores::filter::contour::MIRFilter::SetMaxPercentError,
         nb::arg("value"))
    .def(
      "SetMaxIterations",
      [](viskores::filter::contour::MIRFilter& self, long long value)
      { self.SetMaxIterations(static_cast<viskores::IdComponent>(value)); },
      nb::arg("value"))
    .def(
      "SetErrorScaling", &viskores::filter::contour::MIRFilter::SetErrorScaling, nb::arg("value"))
    .def(
      "SetScalingDecay", &viskores::filter::contour::MIRFilter::SetScalingDecay, nb::arg("value"))
    .def("GetOutputFieldName", &viskores::filter::contour::MIRFilter::GetOutputFieldName)
    .def("SetOutputFieldName",
         &viskores::filter::contour::MIRFilter::SetOutputFieldName,
         nb::arg("name"))
    .def(
      "Execute",
      [](viskores::filter::contour::MIRFilter& self, nb::object dataObject)
      { return ExecuteFilterToPython(self, dataObject); },
      nb::arg("data"),
      doc::ExecuteFilter);
}
#else
void RegisterNanobindContourClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
