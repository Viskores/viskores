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

namespace field_conversion = viskores::filter::field_conversion;
namespace vector_analysis = viskores::filter::vector_analysis;
namespace contour = viskores::filter::contour;

namespace
{

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
template <typename FilterType>
void SetContourImplicitFunction(FilterType& filter, nb::object functionObject)
{
  viskores::ImplicitFunctionGeneral function;
  if (!ParseImplicitFunction(functionObject, function))
  {
    throw std::runtime_error("Implicit function must be a viskores.Box, viskores.Cylinder, "
                             "viskores.Plane, or viskores.Sphere.");
  }
  filter.SetImplicitFunction(function);
}

template <typename FilterType, typename ClassType>
ClassType& BindSetContourImplicitFunction(ClassType& cls)
{
  cls.def(
    "SetImplicitFunction",
    [](FilterType& self, nb::object functionObject)
    { SetContourImplicitFunction(self, functionObject); },
    nb::arg("function"));
  return cls;
}
#endif

} // namespace

#if VISKORES_PYTHON_ENABLE_FILTER_FIELD_CONVERSION
void RegisterNanobindFieldConversionClasses(
  nb::module_& m,
  const std::function<void(const char*)>& erase_existing_name)
{
  (void)m;
  (void)erase_existing_name;
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
  auto gradient = BindClassWithDefaultConstructor<vector_analysis::Gradient>(
    m, erase_existing_name, "Gradient");
  BindFieldFilterMethods<vector_analysis::Gradient>(gradient);
  gradient
    .def("SetComputePointGradient",
         &vector_analysis::Gradient::SetComputePointGradient)
    .def("GetComputePointGradient",
         &vector_analysis::Gradient::GetComputePointGradient)
    .def("SetComputeDivergence", &vector_analysis::Gradient::SetComputeDivergence)
    .def("GetComputeDivergence", &vector_analysis::Gradient::GetComputeDivergence)
    .def("SetDivergenceName", &vector_analysis::Gradient::SetDivergenceName)
    .def("GetDivergenceName", &vector_analysis::Gradient::GetDivergenceName)
    .def("SetComputeVorticity", &vector_analysis::Gradient::SetComputeVorticity)
    .def("GetComputeVorticity", &vector_analysis::Gradient::GetComputeVorticity)
    .def("SetVorticityName", &vector_analysis::Gradient::SetVorticityName)
    .def("GetVorticityName", &vector_analysis::Gradient::GetVorticityName)
    .def("SetComputeQCriterion", &vector_analysis::Gradient::SetComputeQCriterion)
    .def("GetComputeQCriterion", &vector_analysis::Gradient::GetComputeQCriterion)
    .def("SetQCriterionName", &vector_analysis::Gradient::SetQCriterionName)
    .def("GetQCriterionName", &vector_analysis::Gradient::GetQCriterionName)
    .def("SetComputeGradient", &vector_analysis::Gradient::SetComputeGradient)
    .def("GetComputeGradient", &vector_analysis::Gradient::GetComputeGradient)
    .def("SetColumnMajorOrdering",
         &vector_analysis::Gradient::SetColumnMajorOrdering)
    .def("SetRowMajorOrdering", &vector_analysis::Gradient::SetRowMajorOrdering);

}
#else
void RegisterNanobindVectorAnalysisClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

#if VISKORES_PYTHON_ENABLE_FILTER_CONTOUR
void RegisterNanobindContourClasses(nb::module_& m,
                                    const std::function<void(const char*)>& erase_existing_name)
{
  auto contour = BindClassWithDefaultConstructor<contour::Contour>(
    m, erase_existing_name, "Contour");
  contour
    .def("SetIsoValue",
         nb::overload_cast<viskores::Float64>(&contour::Contour::SetIsoValue))
    .def(
      "SetIsoValue",
      [](contour::Contour& self, long long index, double value)
      {
        self.SetIsoValue(static_cast<viskores::Id>(index), static_cast<viskores::Float64>(value));
      },
      nb::arg("index"),
      nb::arg("value"))
    .def(
      "SetIsoValues",
      [](contour::Contour& self, nb::object valuesObject)
      { self.SetIsoValues(ParseIsoValues(valuesObject)); },
      nb::arg("values"))
    .def(
      "SetActiveField",
      [](contour::Contour& self, const char* name, nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("SetGenerateNormals", &contour::Contour::SetGenerateNormals)
    .def("GetGenerateNormals", &contour::Contour::GetGenerateNormals)
    .def("SetComputeFastNormals", &contour::Contour::SetComputeFastNormals)
    .def("GetComputeFastNormals", &contour::Contour::GetComputeFastNormals);
  BindFilterFieldsToPassMethod<contour::Contour>(contour);
  contour
    .def("SetMergeDuplicatePoints", &contour::Contour::SetMergeDuplicatePoints)
    .def("GetMergeDuplicatePoints", &contour::Contour::GetMergeDuplicatePoints);
  BindFilterExecuteMethod<contour::Contour>(contour);

  auto contourMarchingCells =
    BindClassWithDefaultConstructor<contour::ContourMarchingCells>(
      m, erase_existing_name, "ContourMarchingCells");
  contourMarchingCells
    .def("SetIsoValue",
         nb::overload_cast<viskores::Float64>(
           &contour::ContourMarchingCells::SetIsoValue))
    .def(
      "SetActiveField",
      [](contour::ContourMarchingCells& self,
         const char* name,
         nb::object associationObject)
      {
        self.SetActiveField(
          name, ParseAssociation(associationObject, viskores::cont::Field::Association::Any));
      },
      nb::arg("name"),
      nb::arg("association") = nb::none())
    .def("SetGenerateNormals", &contour::ContourMarchingCells::SetGenerateNormals)
    .def("SetMergeDuplicatePoints",
         &contour::ContourMarchingCells::SetMergeDuplicatePoints);
  BindFilterExecuteMethod<contour::ContourMarchingCells>(contourMarchingCells);

  auto clipWithImplicitFunction =
    BindClassWithDefaultConstructor<contour::ClipWithImplicitFunction>(
      m, erase_existing_name, "ClipWithImplicitFunction");
  clipWithImplicitFunction
    .def("SetOffset",
         &contour::ClipWithImplicitFunction::SetOffset,
         nb::arg("offset"))
    .def("GetOffset", &contour::ClipWithImplicitFunction::GetOffset)
    .def("SetInvertClip",
         &contour::ClipWithImplicitFunction::SetInvertClip,
         nb::arg("invert"));
  BindSetContourImplicitFunction<contour::ClipWithImplicitFunction>(
    clipWithImplicitFunction);
  BindFilterFieldsToPassMethod<contour::ClipWithImplicitFunction>(
    clipWithImplicitFunction);
  BindFilterExecuteMethod<contour::ClipWithImplicitFunction>(
    clipWithImplicitFunction);

  auto slice = BindClassWithDefaultConstructor<contour::Slice,
                                               contour::Contour>(
    m, erase_existing_name, "Slice");
  BindSetContourImplicitFunction<contour::Slice>(slice);
  BindFilterExecuteMethod<contour::Slice>(slice);

  auto sliceMultiple =
    BindClassWithDefaultConstructor<contour::SliceMultiple,
                                    contour::Contour>(
      m, erase_existing_name, "SliceMultiple");
  sliceMultiple
    .def(
      "AddImplicitFunction",
      [](contour::SliceMultiple& self, nb::object functionObject)
      {
        viskores::ImplicitFunctionGeneral function;
        if (!ParseImplicitFunction(functionObject, function))
        {
          throw std::runtime_error("Implicit function must be a viskores.Box, viskores.Cylinder, "
                                   "viskores.Plane, or viskores.Sphere.");
        }
        self.AddImplicitFunction(function);
      },
      nb::arg("function"));
  BindFilterExecuteMethod<contour::SliceMultiple>(sliceMultiple);

  auto clipWithField = BindClassWithDefaultConstructor<contour::ClipWithField>(
    m, erase_existing_name, "ClipWithField");
  BindFilterActiveFieldMethods<contour::ClipWithField>(clipWithField);
  clipWithField
    .def("SetClipValue", &contour::ClipWithField::SetClipValue, nb::arg("value"))
    .def("GetClipValue", &contour::ClipWithField::GetClipValue)
    .def(
      "SetInvertClip", &contour::ClipWithField::SetInvertClip, nb::arg("invert"))
    .def("GetInvertClip", &contour::ClipWithField::GetInvertClip);
  BindFilterFieldsToPassMethod<contour::ClipWithField>(clipWithField);
  BindFilterExecuteMethod<contour::ClipWithField>(clipWithField);

  auto mirFilter =
    BindClassWithDefaultConstructor<contour::MIRFilter>(
      m, erase_existing_name, "MIRFilter");
  mirFilter
    .def("SetPositionCellSetName",
         &contour::MIRFilter::SetPositionCellSetName,
         nb::arg("name"))
    .def("SetLengthCellSetName",
         &contour::MIRFilter::SetLengthCellSetName,
         nb::arg("name"))
    .def("SetIDWholeSetName",
         &contour::MIRFilter::SetIDWholeSetName,
         nb::arg("name"))
    .def("SetVFWholeSetName",
         &contour::MIRFilter::SetVFWholeSetName,
         nb::arg("name"))
    .def("SetMaxPercentError",
         &contour::MIRFilter::SetMaxPercentError,
         nb::arg("value"))
    .def(
      "SetMaxIterations",
      [](contour::MIRFilter& self, long long value)
      { self.SetMaxIterations(static_cast<viskores::IdComponent>(value)); },
      nb::arg("value"))
    .def(
      "SetErrorScaling", &contour::MIRFilter::SetErrorScaling, nb::arg("value"))
    .def(
      "SetScalingDecay", &contour::MIRFilter::SetScalingDecay, nb::arg("value"));
  BindFilterOutputFieldMethods<contour::MIRFilter>(mirFilter);
  BindFilterExecuteMethod<contour::MIRFilter>(mirFilter);
}
#else
void RegisterNanobindContourClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
