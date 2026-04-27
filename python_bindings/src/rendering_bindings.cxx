//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "pyviskores_common.h"
#include "pyviskores_bindings.h"

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

namespace viskores::python::bindings
{

#if VISKORES_PYTHON_ENABLE_RENDERING
void RegisterNanobindColorTableClass(nb::module_& m,
                                     const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("ColorTable");
  nb::class_<viskores::cont::ColorTable>(m, "ColorTable", doc::ClassDoc("ColorTable"))
    .def(nb::init<const std::string&>(), nb::arg("name") = "Default")
    .def("__repr__",
         [](const viskores::cont::ColorTable& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.ColorTable(name=\"" << self.GetName() << "\")";
           return stream.str();
         })
    .def("GetName", &viskores::cont::ColorTable::GetName)
    .def("SetName", &viskores::cont::ColorTable::SetName)
    .def("GetRange", &viskores::cont::ColorTable::GetRange)
    .def("GetNumberOfPoints", &viskores::cont::ColorTable::GetNumberOfPoints)
    .def("SetClampingOn", &viskores::cont::ColorTable::SetClampingOn)
    .def("SetClampingOff", &viskores::cont::ColorTable::SetClampingOff)
    .def("GetClamping", &viskores::cont::ColorTable::GetClamping)
    .def(
      "SetBelowRangeColor",
      [](viskores::cont::ColorTable& self, nb::object colorObject)
      { self.SetBelowRangeColor(ParseColorTableColor(colorObject)); },
      nb::arg("color"))
    .def(
      "SetAboveRangeColor",
      [](viskores::cont::ColorTable& self, nb::object colorObject)
      { self.SetAboveRangeColor(ParseColorTableColor(colorObject)); },
      nb::arg("color"))
    .def(
      "RescaleToRange",
      [](viskores::cont::ColorTable& self, nb::object rangeObject)
      { self.RescaleToRange(ParseRange(rangeObject)); },
      nb::arg("range"))
    .def(
      "AddPointAlpha",
      [](viskores::cont::ColorTable& self, double x, double alpha)
      { return self.AddPointAlpha(x, static_cast<viskores::Float32>(alpha)); },
      nb::arg("x"),
      nb::arg("alpha"));
}

void RegisterNanobindCameraClass(nb::module_& m,
                                 const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("Camera");
  nb::class_<viskores::rendering::Camera>(m, "Camera", doc::ClassDoc("Camera"))
    .def(nb::init<>())
    .def("GetLookAt",
         [](const viskores::rendering::Camera& self)
         {
           const auto value = self.GetLookAt();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetLookAt",
      [](viskores::rendering::Camera& self, nb::object valueObject)
      { self.SetLookAt(ParseVec3(valueObject, self.GetLookAt())); },
      nb::arg("value"))
    .def("GetViewUp",
         [](const viskores::rendering::Camera& self)
         {
           const auto value = self.GetViewUp();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetViewUp",
      [](viskores::rendering::Camera& self, nb::object valueObject)
      { self.SetViewUp(ParseVec3(valueObject, self.GetViewUp())); },
      nb::arg("value"))
    .def("GetPosition",
         [](const viskores::rendering::Camera& self)
         {
           const auto value = self.GetPosition();
           return nb::make_tuple(value[0], value[1], value[2]);
         })
    .def(
      "SetPosition",
      [](viskores::rendering::Camera& self, nb::object valueObject)
      { self.SetPosition(ParseVec3(valueObject, self.GetPosition())); },
      nb::arg("value"))
    .def("GetClippingRange", &viskores::rendering::Camera::GetClippingRange)
    .def(
      "SetClippingRange",
      [](viskores::rendering::Camera& self, double nearPlane, double farPlane)
      { self.SetClippingRange(nearPlane, farPlane); },
      nb::arg("near_plane"),
      nb::arg("far_plane"))
    .def(
      "ResetToBounds",
      [](viskores::rendering::Camera& self, nb::object boundsObject)
      {
        if (!nb::isinstance<nb::sequence>(boundsObject) || nb::isinstance<nb::str>(boundsObject))
        {
          throw std::runtime_error("Expected a sequence of 6 floats.");
        }
        nb::sequence sequence = nb::borrow<nb::sequence>(boundsObject);
        if (nb::len(sequence) != 6)
        {
          throw std::runtime_error("Expected a sequence of 6 floats.");
        }
        double values[6];
        for (size_t index = 0; index < 6; ++index)
        {
          values[index] = nb::cast<double>(sequence[index]);
        }
        self.ResetToBounds(
          viskores::Bounds(values[0], values[1], values[2], values[3], values[4], values[5]));
      },
      nb::arg("bounds"))
    .def(
      "Azimuth",
      [](viskores::rendering::Camera& self, double angle) { self.Azimuth(angle); },
      nb::arg("angle"))
    .def(
      "Elevation",
      [](viskores::rendering::Camera& self, double angle) { self.Elevation(angle); },
      nb::arg("angle"))
    .def("GetFieldOfView", &viskores::rendering::Camera::GetFieldOfView)
    .def(
      "SetFieldOfView",
      [](viskores::rendering::Camera& self, double value) { self.SetFieldOfView(value); },
      nb::arg("value"));
}

void RegisterNanobindRenderingClasses(nb::module_& m,
                                      const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("GlyphType");
  nb::enum_<viskores::rendering::GlyphType>(m, "GlyphType")
    .value("Arrow", viskores::rendering::GlyphType::Arrow)
    .value("Axes", viskores::rendering::GlyphType::Axes)
    .value("Cube", viskores::rendering::GlyphType::Cube)
    .value("Quad", viskores::rendering::GlyphType::Quad)
    .value("Sphere", viskores::rendering::GlyphType::Sphere);

  auto make_actor_with_field = [](const auto& dataObject, const std::string& fieldName)
  { return std::make_shared<viskores::rendering::Actor>(dataObject, fieldName); };
  auto make_actor_with_field_and_color_table =
    [](const auto& dataObject,
       const std::string& fieldName,
       const std::shared_ptr<viskores::cont::ColorTable>& colorTable)
  { return std::make_shared<viskores::rendering::Actor>(dataObject, fieldName, *colorTable); };
  auto make_actor_with_coordinate =
    [](const auto& dataObject, const std::string& coordinateName, const std::string& fieldName)
  { return std::make_shared<viskores::rendering::Actor>(dataObject, coordinateName, fieldName); };
  auto make_actor_with_coordinate_and_color_table =
    [](const auto& dataObject,
       const std::string& coordinateName,
       const std::string& fieldName,
       const std::shared_ptr<viskores::cont::ColorTable>& colorTable)
  {
    return std::make_shared<viskores::rendering::Actor>(
      dataObject, coordinateName, fieldName, *colorTable);
  };

  erase_existing_name("Actor");
  nb::class_<viskores::rendering::Actor>(m, "Actor", doc::ClassDoc("Actor"))
    .def(
      "__init__",
      [make_actor_with_field](viskores::rendering::Actor* self,
                              const std::shared_ptr<viskores::cont::DataSet>& dataSet,
                              const std::string& fieldName)
      { new (self) viskores::rendering::Actor(*make_actor_with_field(*dataSet, fieldName)); },
      nb::arg("dataset"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_field_and_color_table](
        viskores::rendering::Actor* self,
        const std::shared_ptr<viskores::cont::DataSet>& dataSet,
        const std::string& fieldName,
        const std::shared_ptr<viskores::cont::ColorTable>& colorTable)
      {
        new (self) viskores::rendering::Actor(
          *make_actor_with_field_and_color_table(*dataSet, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def(
      "__init__",
      [make_actor_with_coordinate](viskores::rendering::Actor* self,
                                   const std::shared_ptr<viskores::cont::DataSet>& dataSet,
                                   const std::string& coordinateName,
                                   const std::string& fieldName)
      {
        new (self) viskores::rendering::Actor(
          *make_actor_with_coordinate(*dataSet, coordinateName, fieldName));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_coordinate_and_color_table](
        viskores::rendering::Actor* self,
        const std::shared_ptr<viskores::cont::DataSet>& dataSet,
        const std::string& coordinateName,
        const std::string& fieldName,
        const std::shared_ptr<viskores::cont::ColorTable>& colorTable)
      {
        new (self) viskores::rendering::Actor(*make_actor_with_coordinate_and_color_table(
          *dataSet, coordinateName, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def(
      "__init__",
      [make_actor_with_field](viskores::rendering::Actor* self,
                              const std::shared_ptr<viskores::cont::PartitionedDataSet>& dataSet,
                              const std::string& fieldName)
      { new (self) viskores::rendering::Actor(*make_actor_with_field(*dataSet, fieldName)); },
      nb::arg("dataset"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_field_and_color_table](
        viskores::rendering::Actor* self,
        const std::shared_ptr<viskores::cont::PartitionedDataSet>& dataSet,
        const std::string& fieldName,
        const std::shared_ptr<viskores::cont::ColorTable>& colorTable)
      {
        new (self) viskores::rendering::Actor(
          *make_actor_with_field_and_color_table(*dataSet, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def(
      "__init__",
      [make_actor_with_coordinate](
        viskores::rendering::Actor* self,
        const std::shared_ptr<viskores::cont::PartitionedDataSet>& dataSet,
        const std::string& coordinateName,
        const std::string& fieldName)
      {
        new (self) viskores::rendering::Actor(
          *make_actor_with_coordinate(*dataSet, coordinateName, fieldName));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_coordinate_and_color_table](
        viskores::rendering::Actor* self,
        const std::shared_ptr<viskores::cont::PartitionedDataSet>& dataSet,
        const std::string& coordinateName,
        const std::string& fieldName,
        const std::shared_ptr<viskores::cont::ColorTable>& colorTable)
      {
        new (self) viskores::rendering::Actor(*make_actor_with_coordinate_and_color_table(
          *dataSet, coordinateName, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def("__repr__", []() { return "viskores.rendering.Actor()"; })
    .def("GetColorTable",
         [](const viskores::rendering::Actor& self)
         { return std::make_shared<viskores::cont::ColorTable>(self.GetColorTable()); })
    .def("GetScalarRange",
         [](const viskores::rendering::Actor& self)
         {
           const auto range = self.GetScalarRange();
           return nb::make_tuple(range.Min, range.Max);
         })
    .def(
      "SetScalarRange",
      [](viskores::rendering::Actor& self, nb::object rangeObject)
      { self.SetScalarRange(ParseRange(rangeObject)); },
      nb::arg("range"))
    .def(
      "SetScalarRange",
      [](viskores::rendering::Actor& self, double minValue, double maxValue)
      { self.SetScalarRange(viskores::Range(minValue, maxValue)); },
      nb::arg("min_value"),
      nb::arg("max_value"));

  erase_existing_name("Scene");
  nb::class_<viskores::rendering::Scene>(m, "Scene", doc::ClassDoc("Scene"))
    .def(nb::init<>())
    .def(
      "AddActor",
      [](viskores::rendering::Scene& self, const std::shared_ptr<viskores::rendering::Actor>& actor)
      { self.AddActor(*actor); },
      nb::arg("actor"))
    .def("GetNumberOfActors", &viskores::rendering::Scene::GetNumberOfActors)
    .def(
      "GetActor",
      [](const viskores::rendering::Scene& self, long index)
      {
        return std::make_shared<viskores::rendering::Actor>(
          self.GetActor(static_cast<viskores::IdComponent>(index)));
      },
      nb::arg("index"));

  erase_existing_name("Canvas");
  nb::class_<viskores::rendering::Canvas>(m, "Canvas", doc::ClassDoc("Canvas"))
    .def(nb::init<viskores::IdComponent, viskores::IdComponent>(),
         nb::arg("width") = 1024,
         nb::arg("height") = 1024)
    .def("GetWidth", &viskores::rendering::Canvas::GetWidth)
    .def("GetHeight", &viskores::rendering::Canvas::GetHeight)
    .def("GetColorBuffer",
         [](const viskores::rendering::Canvas& self)
         {
           return UnknownArrayToNumPyArray(
             viskores::cont::UnknownArrayHandle(self.GetColorBuffer()));
         })
    .def("GetDepthBuffer",
         [](const viskores::rendering::Canvas& self)
         {
           return UnknownArrayToNumPyArray(
             viskores::cont::UnknownArrayHandle(self.GetDepthBuffer()));
         })
    .def(
      "GetDataSet",
      [](viskores::rendering::Canvas& self,
         const std::string& colorFieldName,
         nb::object depthFieldName)
      {
        if (!depthFieldName.is_valid() || depthFieldName.is_none())
        {
          return self.GetDataSet(colorFieldName, std::string("depth"));
        }
        return self.GetDataSet(colorFieldName, nb::cast<std::string>(depthFieldName));
      },
      nb::arg("color_field_name") = std::string("color"),
      nb::arg("depth_field_name") = std::string("depth"))
    .def(
      "SetBackgroundColor",
      [](viskores::rendering::Canvas& self, nb::object colorObject)
      { self.SetBackgroundColor(ParseColor(colorObject, self.GetBackgroundColor())); },
      nb::arg("color"))
    .def("Clear", &viskores::rendering::Canvas::Clear)
    .def(
      "AddLine",
      [](viskores::rendering::Canvas& self,
         double x0,
         double y0,
         double x1,
         double y1,
         double lineWidth,
         nb::object colorObject)
      {
        self.AddLine(x0,
                     y0,
                     x1,
                     y1,
                     static_cast<viskores::Float32>(lineWidth),
                     ParseColor(colorObject, viskores::rendering::Color(0, 0, 0, 1)));
      },
      nb::arg("x0"),
      nb::arg("y0"),
      nb::arg("x1"),
      nb::arg("y1"),
      nb::arg("line_width"),
      nb::arg("color"))
    .def(
      "AddColorBar",
      [](viskores::rendering::Canvas& self,
         nb::object boundsObject,
         const std::shared_ptr<viskores::cont::ColorTable>& colorTable,
         bool horizontal)
      {
        if (!nb::isinstance<nb::sequence>(boundsObject) || nb::isinstance<nb::str>(boundsObject))
        {
          throw std::runtime_error("Expected a sequence of 6 floats.");
        }
        nb::sequence sequence = nb::borrow<nb::sequence>(boundsObject);
        if (nb::len(sequence) != 6)
        {
          throw std::runtime_error("Expected a sequence of 6 floats.");
        }
        double values[6];
        for (size_t index = 0; index < 6; ++index)
        {
          values[index] = nb::cast<double>(sequence[index]);
        }
        self.AddColorBar(
          viskores::Bounds(values[0], values[1], values[2], values[3], values[4], values[5]),
          *colorTable,
          horizontal);
      },
      nb::arg("bounds"),
      nb::arg("color_table"),
      nb::arg("horizontal"))
    .def("BlendBackground", &viskores::rendering::Canvas::BlendBackground)
    .def("SaveAs", &viskores::rendering::Canvas::SaveAs, nb::arg("file_name"));

  erase_existing_name("CanvasRayTracer");
  nb::class_<viskores::rendering::CanvasRayTracer, viskores::rendering::Canvas>(
    m, "CanvasRayTracer", doc::ClassDoc("CanvasRayTracer"))
    .def(nb::init<viskores::IdComponent, viskores::IdComponent>(),
         nb::arg("width") = 1024,
         nb::arg("height") = 1024);

  erase_existing_name("MapperVolume");
  nb::class_<viskores::rendering::MapperVolume>(m, "MapperVolume", doc::ClassDoc("MapperVolume"))
    .def(nb::init<>())
    .def(
      "SetSampleDistance",
      [](viskores::rendering::MapperVolume& self, double distance)
      { self.SetSampleDistance(static_cast<viskores::Float32>(distance)); },
      nb::arg("distance"))
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperVolume& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperPoint");
  nb::class_<viskores::rendering::MapperPoint>(m, "MapperPoint", doc::ClassDoc("MapperPoint"))
    .def(nb::init<>())
    .def("SetUseCells", &viskores::rendering::MapperPoint::SetUseCells)
    .def("SetUsePoints", &viskores::rendering::MapperPoint::SetUsePoints)
    .def(
      "UseVariableRadius",
      [](viskores::rendering::MapperPoint& self, bool enabled) { self.UseVariableRadius(enabled); },
      nb::arg("enabled"))
    .def(
      "SetRadius",
      [](viskores::rendering::MapperPoint& self, double radius)
      { self.SetRadius(static_cast<viskores::Float32>(radius)); },
      nb::arg("radius"))
    .def(
      "SetRadiusDelta",
      [](viskores::rendering::MapperPoint& self, double delta)
      { self.SetRadiusDelta(static_cast<viskores::Float32>(delta)); },
      nb::arg("delta"))
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperPoint& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperQuad");
  nb::class_<viskores::rendering::MapperQuad>(m, "MapperQuad", doc::ClassDoc("MapperQuad"))
    .def(nb::init<>())
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperQuad& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperConnectivity");
  nb::class_<viskores::rendering::MapperConnectivity>(
    m, "MapperConnectivity", doc::ClassDoc("MapperConnectivity"))
    .def(nb::init<>())
    .def(
      "SetSampleDistance",
      [](viskores::rendering::MapperConnectivity& self, double distance)
      { self.SetSampleDistance(static_cast<viskores::Float32>(distance)); },
      nb::arg("distance"));

  erase_existing_name("MapperCylinder");
  nb::class_<viskores::rendering::MapperCylinder>(
    m, "MapperCylinder", doc::ClassDoc("MapperCylinder"))
    .def(nb::init<>())
    .def(
      "UseVariableRadius",
      [](viskores::rendering::MapperCylinder& self, bool enabled)
      { self.UseVariableRadius(enabled); },
      nb::arg("enabled"))
    .def(
      "SetRadius",
      [](viskores::rendering::MapperCylinder& self, double radius)
      { self.SetRadius(static_cast<viskores::Float32>(radius)); },
      nb::arg("radius"))
    .def(
      "SetRadiusDelta",
      [](viskores::rendering::MapperCylinder& self, double delta)
      { self.SetRadiusDelta(static_cast<viskores::Float32>(delta)); },
      nb::arg("delta"))
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperCylinder& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperGlyphScalar");
  nb::class_<viskores::rendering::MapperGlyphScalar>(
    m, "MapperGlyphScalar", doc::ClassDoc("MapperGlyphScalar"))
    .def(nb::init<>())
    .def("GetGlyphType", &viskores::rendering::MapperGlyphScalar::GetGlyphType)
    .def(
      "SetGlyphType", &viskores::rendering::MapperGlyphScalar::SetGlyphType, nb::arg("glyph_type"))
    .def("SetUseCells", &viskores::rendering::MapperGlyphScalar::SetUseCells)
    .def("SetUsePoints", &viskores::rendering::MapperGlyphScalar::SetUsePoints)
    .def("SetScaleByValue",
         &viskores::rendering::MapperGlyphScalar::SetScaleByValue,
         nb::arg("enabled"))
    .def("GetScaleByValue", &viskores::rendering::MapperGlyphScalar::GetScaleByValue)
    .def(
      "SetBaseSize",
      [](viskores::rendering::MapperGlyphScalar& self, double size)
      { self.SetBaseSize(static_cast<viskores::Float32>(size)); },
      nb::arg("size"))
    .def("GetBaseSize", &viskores::rendering::MapperGlyphScalar::GetBaseSize)
    .def(
      "SetScaleDelta",
      [](viskores::rendering::MapperGlyphScalar& self, double delta)
      { self.SetScaleDelta(static_cast<viskores::Float32>(delta)); },
      nb::arg("delta"))
    .def("GetScaleDelta", &viskores::rendering::MapperGlyphScalar::GetScaleDelta)
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperGlyphScalar& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperGlyphVector");
  nb::class_<viskores::rendering::MapperGlyphVector>(
    m, "MapperGlyphVector", doc::ClassDoc("MapperGlyphVector"))
    .def(nb::init<>())
    .def("GetGlyphType", &viskores::rendering::MapperGlyphVector::GetGlyphType)
    .def(
      "SetGlyphType", &viskores::rendering::MapperGlyphVector::SetGlyphType, nb::arg("glyph_type"))
    .def("SetUseCells", &viskores::rendering::MapperGlyphVector::SetUseCells)
    .def("SetUsePoints", &viskores::rendering::MapperGlyphVector::SetUsePoints)
    .def("SetScaleByValue",
         &viskores::rendering::MapperGlyphVector::SetScaleByValue,
         nb::arg("enabled"))
    .def("GetScaleByValue", &viskores::rendering::MapperGlyphVector::GetScaleByValue)
    .def(
      "SetBaseSize",
      [](viskores::rendering::MapperGlyphVector& self, double size)
      { self.SetBaseSize(static_cast<viskores::Float32>(size)); },
      nb::arg("size"))
    .def("GetBaseSize", &viskores::rendering::MapperGlyphVector::GetBaseSize)
    .def(
      "SetScaleDelta",
      [](viskores::rendering::MapperGlyphVector& self, double delta)
      { self.SetScaleDelta(static_cast<viskores::Float32>(delta)); },
      nb::arg("delta"))
    .def("GetScaleDelta", &viskores::rendering::MapperGlyphVector::GetScaleDelta)
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperGlyphVector& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperWireframer");
  nb::class_<viskores::rendering::MapperWireframer>(
    m, "MapperWireframer", doc::ClassDoc("MapperWireframer"))
    .def(nb::init<>())
    .def("GetShowInternalZones", &viskores::rendering::MapperWireframer::GetShowInternalZones)
    .def(
      "SetShowInternalZones",
      [](viskores::rendering::MapperWireframer& self, bool enabled)
      { self.SetShowInternalZones(enabled); },
      nb::arg("enabled"))
    .def("GetIsOverlay", &viskores::rendering::MapperWireframer::GetIsOverlay)
    .def(
      "SetIsOverlay",
      [](viskores::rendering::MapperWireframer& self, bool enabled) { self.SetIsOverlay(enabled); },
      nb::arg("enabled"))
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperWireframer& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"));

  erase_existing_name("MapperRayTracer");
  nb::class_<viskores::rendering::MapperRayTracer>(
    m, "MapperRayTracer", doc::ClassDoc("MapperRayTracer"))
    .def(nb::init<>())
    .def(
      "SetCompositeBackground",
      [](viskores::rendering::MapperRayTracer& self, bool enabled)
      { self.SetCompositeBackground(enabled); },
      nb::arg("enabled"))
    .def(
      "SetShadingOn",
      [](viskores::rendering::MapperRayTracer& self, bool enabled) { self.SetShadingOn(enabled); },
      nb::arg("enabled"));

  erase_existing_name("ScalarRendererResult");
  nb::class_<viskores::rendering::ScalarRenderer::Result>(
    m, "ScalarRendererResult", doc::ClassDoc("ScalarRendererResult"))
    .def_prop_ro("Width",
                 [](const viskores::rendering::ScalarRenderer::Result& self) { return self.Width; })
    .def_prop_ro(
      "Height", [](const viskores::rendering::ScalarRenderer::Result& self) { return self.Height; })
    .def_prop_ro(
      "Depths",
      [](const viskores::rendering::ScalarRenderer::Result& self)
      { return UnknownArrayToNumPyArray(viskores::cont::UnknownArrayHandle(self.Depths)); })
    .def_prop_ro("Scalars",
                 [](const viskores::rendering::ScalarRenderer::Result& self)
                 {
                   nb::list output;
                   for (const auto& values : self.Scalars)
                   {
                     output.append(
                       UnknownArrayToNumPyArray(viskores::cont::UnknownArrayHandle(values)));
                   }
                   return output;
                 })
    .def_prop_ro("ScalarNames",
                 [](const viskores::rendering::ScalarRenderer::Result& self)
                 {
                   nb::list output;
                   for (const auto& name : self.ScalarNames)
                   {
                     output.append(nb::str(name.c_str()));
                   }
                   return output;
                 })
    .def_prop_ro("Ranges",
                 [](const viskores::rendering::ScalarRenderer::Result& self)
                 {
                   nb::dict output;
                   for (const auto& [name, range] : self.Ranges)
                   {
                     output[nb::str(name.c_str())] = nb::make_tuple(range.Min, range.Max);
                   }
                   return output;
                 })
    .def("ToDataSet",
         [](viskores::rendering::ScalarRenderer::Result& self)
         { return WrapDataSet(self.ToDataSet()); });

  erase_existing_name("ScalarRenderer");
  nb::class_<viskores::rendering::ScalarRenderer>(
    m, "ScalarRenderer", doc::ClassDoc("ScalarRenderer"))
    .def(nb::init<>())
    .def(
      "SetInput",
      [](viskores::rendering::ScalarRenderer& self,
         const std::shared_ptr<viskores::cont::DataSet>& dataSet) { self.SetInput(*dataSet); },
      nb::arg("dataset"))
    .def("SetWidth", &viskores::rendering::ScalarRenderer::SetWidth, nb::arg("width"))
    .def("SetHeight", &viskores::rendering::ScalarRenderer::SetHeight, nb::arg("height"))
    .def("SetDefaultValue", &viskores::rendering::ScalarRenderer::SetDefaultValue, nb::arg("value"))
    .def(
      "Render",
      [](viskores::rendering::ScalarRenderer& self,
         const std::shared_ptr<viskores::rendering::Camera>& camera)
      { return self.Render(*camera); },
      nb::arg("camera"));

  erase_existing_name("View3D");
  nb::class_<viskores::rendering::View3D>(m, "View3D", doc::ClassDoc("View3D"))
    .def(
      "__init__",
      [](viskores::rendering::View3D* self,
         const std::shared_ptr<viskores::rendering::Scene>& scene,
         nb::object mapperObject,
         const std::shared_ptr<viskores::rendering::CanvasRayTracer>& canvas,
         nb::object cameraObject,
         nb::object backgroundObject,
         nb::object foregroundObject)
      {
        const auto background =
          ParseColor(backgroundObject, viskores::rendering::Color(0, 0, 0, 1));
        const auto foreground =
          ParseColor(foregroundObject, viskores::rendering::Color(1, 1, 1, 1));
        auto mapper = RequireMapper(mapperObject);
        if (!mapper)
        {
          throw std::runtime_error("Expected a viskores.rendering mapper instance.");
        }
        if (cameraObject.is_none())
        {
          new (self) viskores::rendering::View3D(*scene, *mapper, *canvas, background, foreground);
        }
        else
        {
          auto camera = RequireCamera(cameraObject);
          if (!camera)
          {
            throw std::runtime_error("Expected a viskores.rendering.Camera instance.");
          }
          new (self)
            viskores::rendering::View3D(*scene, *mapper, *canvas, *camera, background, foreground);
        }
      },
      nb::arg("scene"),
      nb::arg("mapper"),
      nb::arg("canvas"),
      nb::arg("camera") = nb::none(),
      nb::arg("background") = nb::none(),
      nb::arg("foreground") = nb::none())
    .def("Paint", &viskores::rendering::View3D::Paint)
    .def("SaveAs", &viskores::rendering::View3D::SaveAs, nb::arg("file_name"));
}
#else
void RegisterNanobindColorTableClass(nb::module_&, const std::function<void(const char*)>&) {}
void RegisterNanobindCameraClass(nb::module_&, const std::function<void(const char*)>&) {}
void RegisterNanobindRenderingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
