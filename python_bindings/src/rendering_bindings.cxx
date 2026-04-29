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
namespace cont = viskores::cont;
namespace rendering = viskores::rendering;

template <typename MapperType, typename ClassType>
ClassType& BindMapperCompositeBackground(ClassType& cls)
{
  cls.def("SetCompositeBackground", &MapperType::SetCompositeBackground, nb::arg("enabled"));
  return cls;
}

template <typename MapperType, typename ClassType, typename Setter>
ClassType& BindMapperFloat32Setter(ClassType& cls,
                                   const char* name,
                                   Setter setter,
                                   const char* argName)
{
  BindCastedSetter<MapperType, viskores::Float32, double>(cls, name, setter, argName);
  return cls;
}

template <typename MapperType, typename ClassType>
ClassType& BindMapperRadiusControls(ClassType& cls)
{
  cls.def("UseVariableRadius", &MapperType::UseVariableRadius, nb::arg("enabled"));
  BindMapperFloat32Setter<MapperType>(cls, "SetRadius", &MapperType::SetRadius, "radius");
  BindMapperFloat32Setter<MapperType>(
    cls, "SetRadiusDelta", &MapperType::SetRadiusDelta, "delta");
  return cls;
}

template <typename MapperType, typename ClassType>
ClassType& BindMapperGlyphSizeControls(ClassType& cls)
{
  BindCastedProperty<MapperType, viskores::Float32, double>(
    cls, "SetBaseSize", "GetBaseSize", &MapperType::SetBaseSize, &MapperType::GetBaseSize, "size");
  BindCastedProperty<MapperType, viskores::Float32, double>(
    cls,
    "SetScaleDelta",
    "GetScaleDelta",
    &MapperType::SetScaleDelta,
    &MapperType::GetScaleDelta,
    "delta");
  return cls;
}

void RegisterNanobindColorTableClass(nb::module_& m,
                                     const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("ColorTable");
  nb::class_<cont::ColorTable>(m, "ColorTable", doc::ClassDoc("ColorTable"))
    .def(nb::init<const std::string&>(), nb::arg("name") = "Default")
    .def("__repr__",
         [](const cont::ColorTable& self)
         {
           std::ostringstream stream;
           stream << "viskores.cont.ColorTable(name=\"" << self.GetName() << "\")";
           return stream.str();
         })
    .def("GetName", &cont::ColorTable::GetName)
    .def("SetName", &cont::ColorTable::SetName)
    .def("GetRange", &cont::ColorTable::GetRange)
    .def("GetNumberOfPoints", &cont::ColorTable::GetNumberOfPoints)
    .def("SetClampingOn", &cont::ColorTable::SetClampingOn)
    .def("SetClampingOff", &cont::ColorTable::SetClampingOff)
    .def("GetClamping", &cont::ColorTable::GetClamping)
    .def(
      "SetBelowRangeColor",
      [](cont::ColorTable& self, nb::object colorObject)
      { self.SetBelowRangeColor(ParseColorTableColor(colorObject)); },
      nb::arg("color"))
    .def(
      "SetAboveRangeColor",
      [](cont::ColorTable& self, nb::object colorObject)
      { self.SetAboveRangeColor(ParseColorTableColor(colorObject)); },
      nb::arg("color"))
    .def(
      "RescaleToRange",
      [](cont::ColorTable& self, nb::object rangeObject)
      { self.RescaleToRange(ParseRange(rangeObject)); },
      nb::arg("range"))
    .def(
      "AddPointAlpha",
      [](cont::ColorTable& self, double x, double alpha)
      { return self.AddPointAlpha(x, static_cast<viskores::Float32>(alpha)); },
      nb::arg("x"),
      nb::arg("alpha"));
}

void RegisterNanobindCameraClass(nb::module_& m,
                                 const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("CameraMode");
  nb::enum_<rendering::Camera::Mode>(m, "CameraMode")
    .value("TwoD", rendering::Camera::Mode::TwoD)
    .value("ThreeD", rendering::Camera::Mode::ThreeD);

  auto camera =
    BindClassWithDefaultConstructor<rendering::Camera>(m, erase_existing_name, "Camera");
  camera
    .def("GetMode", &rendering::Camera::GetMode)
    .def("SetMode", &rendering::Camera::SetMode, nb::arg("mode"))
    .def("SetModeTo2D", &rendering::Camera::SetModeTo2D)
    .def("SetModeTo3D", &rendering::Camera::SetModeTo3D)
    .def("GetLookAt",
         [](const rendering::Camera& self)
         { return Vec3ToTuple(self.GetLookAt()); })
    .def(
      "SetLookAt",
      [](rendering::Camera& self, nb::object valueObject)
      { self.SetLookAt(ParseVec3(valueObject, self.GetLookAt())); },
      nb::arg("value"))
    .def("GetViewUp",
         [](const rendering::Camera& self)
         { return Vec3ToTuple(self.GetViewUp()); })
    .def(
      "SetViewUp",
      [](rendering::Camera& self, nb::object valueObject)
      { self.SetViewUp(ParseVec3(valueObject, self.GetViewUp())); },
      nb::arg("value"))
    .def("GetPosition",
         [](const rendering::Camera& self)
         { return Vec3ToTuple(self.GetPosition()); })
    .def(
      "SetPosition",
      [](rendering::Camera& self, nb::object valueObject)
      { self.SetPosition(ParseVec3(valueObject, self.GetPosition())); },
      nb::arg("value"))
    .def("GetClippingRange", &rendering::Camera::GetClippingRange)
    .def(
      "SetClippingRange",
      [](rendering::Camera& self, double nearPlane, double farPlane)
      { self.SetClippingRange(nearPlane, farPlane); },
      nb::arg("near_plane"),
      nb::arg("far_plane"))
    .def(
      "ResetToBounds",
      [](rendering::Camera& self, nb::object boundsObject)
      { self.ResetToBounds(ParseBounds(boundsObject)); },
      nb::arg("bounds"))
    .def(
      "Azimuth",
      [](rendering::Camera& self, double angle) { self.Azimuth(angle); },
      nb::arg("angle"))
    .def(
      "Elevation",
      [](rendering::Camera& self, double angle) { self.Elevation(angle); },
      nb::arg("angle"))
    .def("GetFieldOfView", &rendering::Camera::GetFieldOfView)
    .def(
      "SetFieldOfView",
      [](rendering::Camera& self, double value) { self.SetFieldOfView(value); },
      nb::arg("value"))
    .def("GetXScale", &rendering::Camera::GetXScale)
    .def(
      "SetXScale",
      [](rendering::Camera& self, double value) { self.SetXScale(value); },
      nb::arg("value"))
    .def("GetViewRange2D",
         [](const rendering::Camera& self)
         {
           viskores::Float32 left = 0.0f;
           viskores::Float32 right = 0.0f;
           viskores::Float32 bottom = 0.0f;
           viskores::Float32 top = 0.0f;
           self.GetViewRange2D(left, right, bottom, top);
           return nb::make_tuple(left, right, bottom, top);
         })
    .def(
      "SetViewRange2D",
      [](rendering::Camera& self, double left, double right, double bottom, double top)
      { self.SetViewRange2D(left, right, bottom, top); },
      nb::arg("left"),
      nb::arg("right"),
      nb::arg("bottom"),
      nb::arg("top"));
}

void RegisterNanobindRenderingClasses(nb::module_& m,
                                      const std::function<void(const char*)>& erase_existing_name)
{
  erase_existing_name("GlyphType");
  nb::enum_<rendering::GlyphType>(m, "GlyphType")
    .value("Arrow", rendering::GlyphType::Arrow)
    .value("Axes", rendering::GlyphType::Axes)
    .value("Cube", rendering::GlyphType::Cube)
    .value("Quad", rendering::GlyphType::Quad)
    .value("Sphere", rendering::GlyphType::Sphere);

  auto make_actor_with_field = [](const auto& dataObject, const std::string& fieldName)
  { return std::make_shared<rendering::Actor>(dataObject, fieldName); };
  auto make_actor_with_field_and_color_table =
    [](const auto& dataObject,
       const std::string& fieldName,
       const std::shared_ptr<cont::ColorTable>& colorTable)
  { return std::make_shared<rendering::Actor>(dataObject, fieldName, *colorTable); };
  auto make_actor_with_coordinate =
    [](const auto& dataObject, const std::string& coordinateName, const std::string& fieldName)
  { return std::make_shared<rendering::Actor>(dataObject, coordinateName, fieldName); };
  auto make_actor_with_coordinate_and_color_table =
    [](const auto& dataObject,
       const std::string& coordinateName,
       const std::string& fieldName,
       const std::shared_ptr<cont::ColorTable>& colorTable)
  {
    return std::make_shared<rendering::Actor>(
      dataObject, coordinateName, fieldName, *colorTable);
  };

  erase_existing_name("Actor");
  nb::class_<rendering::Actor>(m, "Actor", doc::ClassDoc("Actor"))
    .def(
      "__init__",
      [make_actor_with_field](rendering::Actor* self,
                              const std::shared_ptr<cont::DataSet>& dataSet,
                              const std::string& fieldName)
      { new (self) rendering::Actor(*make_actor_with_field(*dataSet, fieldName)); },
      nb::arg("dataset"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_field_and_color_table](
        rendering::Actor* self,
        const std::shared_ptr<cont::DataSet>& dataSet,
        const std::string& fieldName,
        const std::shared_ptr<cont::ColorTable>& colorTable)
      {
        new (self) rendering::Actor(
          *make_actor_with_field_and_color_table(*dataSet, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def(
      "__init__",
      [make_actor_with_coordinate](rendering::Actor* self,
                                   const std::shared_ptr<cont::DataSet>& dataSet,
                                   const std::string& coordinateName,
                                   const std::string& fieldName)
      {
        new (self) rendering::Actor(
          *make_actor_with_coordinate(*dataSet, coordinateName, fieldName));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_coordinate_and_color_table](
        rendering::Actor* self,
        const std::shared_ptr<cont::DataSet>& dataSet,
        const std::string& coordinateName,
        const std::string& fieldName,
        const std::shared_ptr<cont::ColorTable>& colorTable)
      {
        new (self) rendering::Actor(*make_actor_with_coordinate_and_color_table(
          *dataSet, coordinateName, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def(
      "__init__",
      [make_actor_with_field](rendering::Actor* self,
                              const std::shared_ptr<cont::PartitionedDataSet>& dataSet,
                              const std::string& fieldName)
      { new (self) rendering::Actor(*make_actor_with_field(*dataSet, fieldName)); },
      nb::arg("dataset"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_field_and_color_table](
        rendering::Actor* self,
        const std::shared_ptr<cont::PartitionedDataSet>& dataSet,
        const std::string& fieldName,
        const std::shared_ptr<cont::ColorTable>& colorTable)
      {
        new (self) rendering::Actor(
          *make_actor_with_field_and_color_table(*dataSet, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def(
      "__init__",
      [make_actor_with_coordinate](
        rendering::Actor* self,
        const std::shared_ptr<cont::PartitionedDataSet>& dataSet,
        const std::string& coordinateName,
        const std::string& fieldName)
      {
        new (self) rendering::Actor(
          *make_actor_with_coordinate(*dataSet, coordinateName, fieldName));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"))
    .def(
      "__init__",
      [make_actor_with_coordinate_and_color_table](
        rendering::Actor* self,
        const std::shared_ptr<cont::PartitionedDataSet>& dataSet,
        const std::string& coordinateName,
        const std::string& fieldName,
        const std::shared_ptr<cont::ColorTable>& colorTable)
      {
        new (self) rendering::Actor(*make_actor_with_coordinate_and_color_table(
          *dataSet, coordinateName, fieldName, colorTable));
      },
      nb::arg("dataset"),
      nb::arg("coordinate_name"),
      nb::arg("field_name"),
      nb::arg("color_table"))
    .def("__repr__", []() { return "viskores.rendering.Actor()"; })
    .def("GetColorTable",
         [](const rendering::Actor& self)
         { return std::make_shared<cont::ColorTable>(self.GetColorTable()); })
    .def("GetScalarRange",
         [](const rendering::Actor& self)
         {
           const auto range = self.GetScalarRange();
           return nb::make_tuple(range.Min, range.Max);
         })
    .def(
      "SetScalarRange",
      [](rendering::Actor& self, nb::object rangeObject)
      { self.SetScalarRange(ParseRange(rangeObject)); },
      nb::arg("range"))
    .def(
      "SetScalarRange",
      [](rendering::Actor& self, double minValue, double maxValue)
      { self.SetScalarRange(viskores::Range(minValue, maxValue)); },
      nb::arg("min_value"),
      nb::arg("max_value"));

  erase_existing_name("Scene");
  nb::class_<rendering::Scene>(m, "Scene", doc::ClassDoc("Scene"))
    .def(nb::init<>())
    .def(
      "AddActor",
      [](rendering::Scene& self, const std::shared_ptr<rendering::Actor>& actor)
      { self.AddActor(*actor); },
      nb::arg("actor"))
    .def("GetNumberOfActors", &rendering::Scene::GetNumberOfActors)
    .def(
      "GetActor",
      [](const rendering::Scene& self, long index)
      {
        return std::make_shared<rendering::Actor>(
          self.GetActor(static_cast<viskores::IdComponent>(index)));
      },
      nb::arg("index"));

  erase_existing_name("Canvas");
  nb::class_<rendering::Canvas>(m, "Canvas", doc::ClassDoc("Canvas"))
    .def(nb::init<viskores::IdComponent, viskores::IdComponent>(),
         nb::arg("width") = 1024,
         nb::arg("height") = 1024)
    .def("GetWidth", &rendering::Canvas::GetWidth)
    .def("GetHeight", &rendering::Canvas::GetHeight)
    .def("GetColorBuffer",
         [](const rendering::Canvas& self)
         {
           return UnknownArrayToNumPyArray(
             cont::UnknownArrayHandle(self.GetColorBuffer()));
         })
    .def("GetDepthBuffer",
         [](const rendering::Canvas& self)
         {
           return UnknownArrayToNumPyArray(
             cont::UnknownArrayHandle(self.GetDepthBuffer()));
         })
    .def(
      "GetDataSet",
      [](rendering::Canvas& self,
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
      [](rendering::Canvas& self, nb::object colorObject)
      { self.SetBackgroundColor(ParseColor(colorObject, self.GetBackgroundColor())); },
      nb::arg("color"))
    .def("Clear", &rendering::Canvas::Clear)
    .def(
      "AddLine",
      [](rendering::Canvas& self,
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
                     ParseColor(colorObject, rendering::Color(0, 0, 0, 1)));
      },
      nb::arg("x0"),
      nb::arg("y0"),
      nb::arg("x1"),
      nb::arg("y1"),
      nb::arg("line_width"),
      nb::arg("color"))
    .def(
      "AddColorBar",
      [](rendering::Canvas& self,
         nb::object boundsObject,
         const std::shared_ptr<cont::ColorTable>& colorTable,
         bool horizontal)
      {
        self.AddColorBar(ParseBounds(boundsObject), *colorTable, horizontal);
      },
      nb::arg("bounds"),
      nb::arg("color_table"),
      nb::arg("horizontal"))
    .def("BlendBackground", &rendering::Canvas::BlendBackground)
    .def("SaveAs", &rendering::Canvas::SaveAs, nb::arg("file_name"));

  erase_existing_name("CanvasRayTracer");
  nb::class_<rendering::CanvasRayTracer, rendering::Canvas>(
    m, "CanvasRayTracer", doc::ClassDoc("CanvasRayTracer"))
    .def(nb::init<viskores::IdComponent, viskores::IdComponent>(),
         nb::arg("width") = 1024,
         nb::arg("height") = 1024);

  auto mapperVolume = BindClassWithDefaultConstructor<rendering::MapperVolume>(
    m, erase_existing_name, "MapperVolume");
  BindMapperFloat32Setter<rendering::MapperVolume>(
    mapperVolume, "SetSampleDistance", &rendering::MapperVolume::SetSampleDistance, "distance");
  BindMapperCompositeBackground<rendering::MapperVolume>(mapperVolume);

  auto mapperPoint =
    BindClassWithDefaultConstructor<rendering::MapperPoint>(m, erase_existing_name, "MapperPoint");
  mapperPoint
    .def("SetUseCells", &rendering::MapperPoint::SetUseCells)
    .def("SetUsePoints", &rendering::MapperPoint::SetUsePoints);
  BindMapperRadiusControls<rendering::MapperPoint>(mapperPoint);
  BindMapperCompositeBackground<rendering::MapperPoint>(mapperPoint);

  auto mapperQuad =
    BindClassWithDefaultConstructor<rendering::MapperQuad>(m, erase_existing_name, "MapperQuad");
  BindMapperCompositeBackground<rendering::MapperQuad>(mapperQuad);

  auto mapperConnectivity =
    BindClassWithDefaultConstructor<rendering::MapperConnectivity>(
      m, erase_existing_name, "MapperConnectivity");
  BindMapperFloat32Setter<rendering::MapperConnectivity>(mapperConnectivity,
                                                         "SetSampleDistance",
                                                         &rendering::MapperConnectivity::
                                                           SetSampleDistance,
                                                         "distance");

  auto mapperCylinder =
    BindClassWithDefaultConstructor<rendering::MapperCylinder>(
      m, erase_existing_name, "MapperCylinder");
  BindMapperRadiusControls<rendering::MapperCylinder>(mapperCylinder);
  BindMapperCompositeBackground<rendering::MapperCylinder>(mapperCylinder);

  auto mapperGlyphScalar =
    BindClassWithDefaultConstructor<rendering::MapperGlyphScalar>(
      m, erase_existing_name, "MapperGlyphScalar");
  mapperGlyphScalar
    .def("GetGlyphType", &rendering::MapperGlyphScalar::GetGlyphType)
    .def(
      "SetGlyphType", &rendering::MapperGlyphScalar::SetGlyphType, nb::arg("glyph_type"))
    .def("SetUseCells", &rendering::MapperGlyphScalar::SetUseCells)
    .def("SetUsePoints", &rendering::MapperGlyphScalar::SetUsePoints)
    .def("SetScaleByValue",
         &rendering::MapperGlyphScalar::SetScaleByValue,
         nb::arg("enabled"))
    .def("GetScaleByValue", &rendering::MapperGlyphScalar::GetScaleByValue);
  BindMapperGlyphSizeControls<rendering::MapperGlyphScalar>(mapperGlyphScalar);
  BindMapperCompositeBackground<rendering::MapperGlyphScalar>(mapperGlyphScalar);

  auto mapperGlyphVector =
    BindClassWithDefaultConstructor<rendering::MapperGlyphVector>(
      m, erase_existing_name, "MapperGlyphVector");
  mapperGlyphVector
    .def("GetGlyphType", &rendering::MapperGlyphVector::GetGlyphType)
    .def(
      "SetGlyphType", &rendering::MapperGlyphVector::SetGlyphType, nb::arg("glyph_type"))
    .def("SetUseCells", &rendering::MapperGlyphVector::SetUseCells)
    .def("SetUsePoints", &rendering::MapperGlyphVector::SetUsePoints)
    .def("SetScaleByValue",
         &rendering::MapperGlyphVector::SetScaleByValue,
         nb::arg("enabled"))
    .def("GetScaleByValue", &rendering::MapperGlyphVector::GetScaleByValue);
  BindMapperGlyphSizeControls<rendering::MapperGlyphVector>(mapperGlyphVector);
  BindMapperCompositeBackground<rendering::MapperGlyphVector>(mapperGlyphVector);

  auto mapperWireframer =
    BindClassWithDefaultConstructor<rendering::MapperWireframer>(
      m, erase_existing_name, "MapperWireframer");
  mapperWireframer
    .def("GetShowInternalZones", &rendering::MapperWireframer::GetShowInternalZones)
    .def("SetShowInternalZones",
         &rendering::MapperWireframer::SetShowInternalZones,
         nb::arg("enabled"))
    .def("GetIsOverlay", &rendering::MapperWireframer::GetIsOverlay)
    .def("SetIsOverlay", &rendering::MapperWireframer::SetIsOverlay, nb::arg("enabled"));
  BindMapperCompositeBackground<rendering::MapperWireframer>(mapperWireframer);

  auto mapperRayTracer =
    BindClassWithDefaultConstructor<rendering::MapperRayTracer>(
      m, erase_existing_name, "MapperRayTracer");
  BindMapperCompositeBackground<rendering::MapperRayTracer>(mapperRayTracer);
  mapperRayTracer.def(
    "SetShadingOn", &rendering::MapperRayTracer::SetShadingOn, nb::arg("enabled"));

  erase_existing_name("ScalarRendererResult");
  nb::class_<rendering::ScalarRenderer::Result>(
    m, "ScalarRendererResult", doc::ClassDoc("ScalarRendererResult"))
    .def_prop_ro("Width",
                 [](const rendering::ScalarRenderer::Result& self) { return self.Width; })
    .def_prop_ro(
      "Height", [](const rendering::ScalarRenderer::Result& self) { return self.Height; })
    .def_prop_ro(
      "Depths",
      [](const rendering::ScalarRenderer::Result& self)
      { return UnknownArrayToNumPyArray(cont::UnknownArrayHandle(self.Depths)); })
    .def_prop_ro("Scalars",
                 [](const rendering::ScalarRenderer::Result& self)
                 {
                   nb::list output;
                   for (const auto& values : self.Scalars)
                   {
                     output.append(
                       UnknownArrayToNumPyArray(cont::UnknownArrayHandle(values)));
                   }
                   return output;
                 })
    .def_prop_ro("ScalarNames",
                 [](const rendering::ScalarRenderer::Result& self)
                 {
                   nb::list output;
                   for (const auto& name : self.ScalarNames)
                   {
                     output.append(nb::str(name.c_str()));
                   }
                   return output;
                 })
    .def_prop_ro("Ranges",
                 [](const rendering::ScalarRenderer::Result& self)
                 {
                   nb::dict output;
                   for (const auto& [name, range] : self.Ranges)
                   {
                     output[nb::str(name.c_str())] = nb::make_tuple(range.Min, range.Max);
                   }
                   return output;
                 })
    .def("ToDataSet",
         [](rendering::ScalarRenderer::Result& self)
         { return WrapDataSet(self.ToDataSet()); });

  erase_existing_name("ScalarRenderer");
  nb::class_<rendering::ScalarRenderer>(
    m, "ScalarRenderer", doc::ClassDoc("ScalarRenderer"))
    .def(nb::init<>())
    .def(
      "SetInput",
      [](rendering::ScalarRenderer& self,
         const std::shared_ptr<cont::DataSet>& dataSet) { self.SetInput(*dataSet); },
      nb::arg("dataset"))
    .def("SetWidth", &rendering::ScalarRenderer::SetWidth, nb::arg("width"))
    .def("SetHeight", &rendering::ScalarRenderer::SetHeight, nb::arg("height"))
    .def("SetDefaultValue", &rendering::ScalarRenderer::SetDefaultValue, nb::arg("value"))
    .def(
      "Render",
      [](rendering::ScalarRenderer& self,
         const std::shared_ptr<rendering::Camera>& camera)
      { return self.Render(*camera); },
      nb::arg("camera"));

  erase_existing_name("View3D");
  nb::class_<rendering::View3D>(m, "View3D", doc::ClassDoc("View3D"))
    .def(
      "__init__",
      [](rendering::View3D* self,
         const std::shared_ptr<rendering::Scene>& scene,
         nb::object mapperObject,
         const std::shared_ptr<rendering::CanvasRayTracer>& canvas,
         nb::object cameraObject,
         nb::object backgroundObject,
         nb::object foregroundObject)
      {
        const auto background =
          ParseColor(backgroundObject, rendering::Color(0, 0, 0, 1));
        const auto foreground =
          ParseColor(foregroundObject, rendering::Color(1, 1, 1, 1));
        auto mapper = RequireMapper(mapperObject);
        if (!mapper)
        {
          throw std::runtime_error("Expected a viskores.rendering mapper instance.");
        }
        if (cameraObject.is_none())
        {
          new (self) rendering::View3D(*scene, *mapper, *canvas, background, foreground);
        }
        else
        {
          auto camera = RequireCamera(cameraObject);
          if (!camera)
          {
            throw std::runtime_error("Expected a viskores.rendering.Camera instance.");
          }
          new (self)
            rendering::View3D(*scene, *mapper, *canvas, *camera, background, foreground);
        }
      },
      nb::arg("scene"),
      nb::arg("mapper"),
      nb::arg("canvas"),
      nb::arg("camera") = nb::none(),
      nb::arg("background") = nb::none(),
      nb::arg("foreground") = nb::none())
    .def("Paint", &rendering::View3D::Paint)
    .def("SaveAs", &rendering::View3D::SaveAs, nb::arg("file_name"));
}
#else
void RegisterNanobindColorTableClass(nb::module_&, const std::function<void(const char*)>&) {}
void RegisterNanobindCameraClass(nb::module_&, const std::function<void(const char*)>&) {}
void RegisterNanobindRenderingClasses(nb::module_&, const std::function<void(const char*)>&) {}
#endif

} // namespace viskores::python::bindings
