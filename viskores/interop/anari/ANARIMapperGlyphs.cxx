//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// viskores
#include "viskores/rendering/raytracing/SphereExtractor.h"
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Invoker.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
#include <viskores/worklet/WorkletMapField.h>

#include <cstring>
#include <utility>

namespace viskores
{
namespace interop
{
namespace anari
{

// Worklets ///////////////////////////////////////////////////////////////////

class GeneratePointGlyphs : public viskores::worklet::WorkletMapField
{
public:
  viskores::Float32 SizeFactor{ 0.f };
  bool Offset{ false };

  VISKORES_CONT
  GeneratePointGlyphs(float size = 1.f, bool offset = false)
    : SizeFactor(size)
    , Offset(offset)
  {
  }

  using ControlSignature = void(FieldIn, WholeArrayIn, WholeArrayOut, WholeArrayOut);
  using ExecutionSignature = void(InputIndex, _1, _2, _3, _4);

  template <typename InGradientType,
            typename InPointPortalType,
            typename OutVertexPortalType,
            typename OutRadiusPortalType>
  VISKORES_EXEC void operator()(const viskores::Id idx,
                                const InGradientType gradient,
                                const InPointPortalType& points,
                                OutVertexPortalType& vertices,
                                OutRadiusPortalType& radii) const
  {
    const auto direction = static_cast<viskores::Vec3f_32>(gradient);
    const auto magnitude = viskores::Magnitude(direction);
    const auto pt = points.Get(idx);
    if (!viskores::IsFinite(magnitude) || magnitude <= 0.f)
    {
      for (viskores::IdComponent vertex = 0; vertex < 4; ++vertex)
      {
        vertices.Set(4 * idx + vertex, pt);
        radii.Set(4 * idx + vertex, 0.f);
      }
      return;
    }

    const auto ng = direction / magnitude;
    auto v0 = pt + ng * this->SizeFactor;
    auto v1 = pt + ng * -this->SizeFactor;
    if (this->Offset)
    {
      vertices.Set(4 * idx + 0, pt);
      vertices.Set(4 * idx + 1, v1);
      vertices.Set(4 * idx + 2, v1);
      vertices.Set(4 * idx + 3, v1 - (this->SizeFactor * ng));
    }
    else
    {
      vertices.Set(4 * idx + 0, v0);
      vertices.Set(4 * idx + 1, pt);
      vertices.Set(4 * idx + 2, pt);
      vertices.Set(4 * idx + 3, v1);
    }
    radii.Set(4 * idx + 0, this->SizeFactor / 8);
    radii.Set(4 * idx + 1, this->SizeFactor / 8);
    radii.Set(4 * idx + 2, this->SizeFactor / 4);
    radii.Set(4 * idx + 3, 0.f);
  }
};

// Helper functions ///////////////////////////////////////////////////////////

static bool StringListContains(const char* const* values, const char* expected)
{
  if (!values)
  {
    return false;
  }
  for (const char* const* value = values; *value != nullptr; ++value)
  {
    if (std::strcmp(*value, expected) == 0)
    {
      return true;
    }
  }
  return false;
}

static void RequireConeSupport(anari_cpp::Device device)
{
  constexpr const char* extension = "ANARI_KHR_GEOMETRY_CONE";
  const char** extensions = nullptr;
  const auto extensionsAvailable = anariGetProperty(
    device, device, "extension", ANARI_STRING_LIST, &extensions, sizeof(extensions), ANARI_WAIT);
  if (extensionsAvailable == 0 || !StringListContains(extensions, extension))
  {
    throw viskores::cont::ErrorBadValue(std::string("ANARI device does not support required ") +
                                        extension + " extension.");
  }

  const char** subtypes = anariGetObjectSubtypes(device, ANARI_GEOMETRY);
  if (!StringListContains(subtypes, "cone"))
  {
    throw viskores::cont::ErrorBadValue(
      "ANARI device does not support required 'cone' geometry subtype.");
  }
}

static void ValidateGlyphField(const viskores::cont::Field& field,
                               const viskores::cont::UnknownCellSet& cells,
                               const viskores::cont::CoordinateSystem& coords)
{
  const auto association = field.GetAssociation();
  const bool isPointField = association == viskores::cont::Field::Association::Points;
  const bool isCellField = association == viskores::cont::Field::Association::Cells;

  if (!isPointField && !isCellField)
  {
    throw viskores::cont::ErrorBadValue(
      "ANARI glyphs require a point- or cell-associated vector field.");
  }
  if (field.GetData().GetNumberOfComponentsFlat() != 3)
  {
    throw viskores::cont::ErrorBadValue("ANARI glyphs require a three-component vector field.");
  }
  if (isPointField && field.GetNumberOfValues() != coords.GetNumberOfPoints())
  {
    throw viskores::cont::ErrorBadValue("ANARI point glyphs require one vector per point.");
  }
  if (isCellField && field.GetNumberOfValues() != cells.GetNumberOfCells())
  {
    throw viskores::cont::ErrorBadValue("ANARI cell glyphs require one vector per cell.");
  }
  if (isCellField && cells.GetNumberOfPoints() != coords.GetNumberOfPoints())
  {
    throw viskores::cont::ErrorBadValue(
      "ANARI cell glyphs require one coordinate per cell-set point.");
  }
}

static void ValidateGlyphActor(const ANARIActor& actor)
{
  const auto& field = actor.GetField();
  if (field.GetNumberOfValues() > 0)
  {
    ValidateGlyphField(field, actor.GetCellSet(), actor.GetCoordinateSystem());
  }
}

static GlyphArrays MakeGlyphs(viskores::cont::Field gradients,
                              viskores::cont::UnknownCellSet cells,
                              viskores::cont::CoordinateSystem coords,
                              float glyphSize,
                              bool offset)
{
  const auto numGlyphs = gradients.GetNumberOfValues();

  GlyphArrays retval;

  retval.Vertices.Allocate(numGlyphs * 4);
  retval.Radii.Allocate(numGlyphs * 4);

  GeneratePointGlyphs worklet(glyphSize, offset);
  viskores::cont::Invoker invoke;

  if (gradients.IsPointField())
  {
    auto resolveGradient = [&](const auto& concreteGradients)
    {
      invoke(
        worklet, concreteGradients, coords.GetDataAsMultiplexer(), retval.Vertices, retval.Radii);
    };
    gradients.GetData()
      .CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldVec3,
                                            VISKORES_DEFAULT_STORAGE_LIST>(resolveGradient);
  }
  else
  {
    viskores::cont::DataSet centersInput;
    centersInput.AddCoordinateSystem(coords);
    centersInput.SetCellSet(cells);

    viskores::filter::field_conversion::CellAverage filter;
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetOutputFieldName("Centers");
    auto centersOutput = filter.Execute(centersInput);

    auto resolveGradient = [&](const auto& concreteGradient)
    {
      auto resolveField = [&](const auto& concreteField)
      { invoke(worklet, concreteGradient, concreteField, retval.Vertices, retval.Radii); };
      centersOutput.GetField("Centers")
        .GetData()
        .CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldVec3,
                                              viskores::List<viskores::cont::StorageTagBasic>>(
          resolveField);
    };
    gradients.GetData()
      .CastAndCallForTypesWithFloatFallback<viskores::TypeListFieldVec3,
                                            VISKORES_DEFAULT_STORAGE_LIST>(resolveGradient);
  }

  return retval;
}

// ANARIMapperGlyphs definitions //////////////////////////////////////////////

ANARIMapperGlyphs::ANARIMapperGlyphs(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const char* name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_unique<ANARIMapperGlyphs::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperGlyphs::ANARIMapperGlyphs(ANARIMapperGlyphs&&) = default;

ANARIMapperGlyphs::~ANARIMapperGlyphs()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperGlyphs::SetOffsetGlyphs(bool enabled)
{
  if (this->Offset == enabled)
  {
    return;
  }

  this->Offset = enabled;
  this->MarkDirty(DirtyCategory::Data);
  this->UpdateMaterializedObjects();
}

anari_cpp::Geometry ANARIMapperGlyphs::GetANARIGeometry()
{
  if (this->Handles->Geometry)
  {
    this->ConstructArrays();
    this->UpdateMaterializedObjects();
    return this->Handles->Geometry;
  }

  auto d = this->GetDevice();
  RequireConeSupport(d);
  ValidateGlyphActor(this->GetActor());
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "cone");
  this->ConstructArrays();
  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperGlyphs::GetANARISurface()
{
  if (this->Handles->Surface)
  {
    this->UpdateMaterializedObjects();
    return this->Handles->Surface;
  }

  auto d = this->GetDevice();
  RequireConeSupport(d);
  ValidateGlyphActor(this->GetActor());

  if (!this->Handles->Material)
  {
    this->Handles->Material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
    anari_cpp::setParameter(d, this->Handles->Material, "name", this->MakeObjectName("material"));
  }

  anari_cpp::commitParameters(d, this->Handles->Material);

  this->Handles->Surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
  anari_cpp::setParameter(d, this->Handles->Surface, "geometry", this->GetANARIGeometry());
  anari_cpp::setParameter(d, this->Handles->Surface, "material", this->Handles->Material);
  anari_cpp::commitParameters(d, this->Handles->Surface);
  this->ClearDirty(DirtyCategory::Names);
  return this->Handles->Surface;
}

void ANARIMapperGlyphs::ConstructArrays()
{
  if (!this->IsDirty(DirtyCategory::Data) && !this->IsDirty(DirtyCategory::Topology))
  {
    return;
  }

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();
  const auto& cells = actor.GetCellSet();
  const auto& field = actor.GetField();

  auto numGlyphs = field.GetNumberOfValues();

  if (numGlyphs == 0)
  {
    this->Valid = false;
    this->Handles->ReleaseArrays();
    this->UpdateGeometry();
    this->Handles->Arrays = {};
    this->ClearDirty(DirtyCategory::Data);
    this->ClearDirty(DirtyCategory::Topology);
    this->ClearDirty(DirtyCategory::Attributes);
    this->RefreshGroup();
    return;
  }

  ValidateGlyphField(field, cells, coords);

  this->Valid = false;
  this->Handles->ReleaseArrays();

  viskores::Bounds coordBounds = coords.GetBounds();
  viskores::Float64 lx = coordBounds.X.Length();
  viskores::Float64 ly = coordBounds.Y.Length();
  viskores::Float64 lz = coordBounds.Z.Length();
  viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
  constexpr viskores::Float64 heuristic = 300.;
  constexpr viskores::Float32 fallbackSize = 0.01f;
  auto glyphSize = static_cast<viskores::Float32>(mag / heuristic);
  if (!viskores::IsFinite(glyphSize) || glyphSize <= 0.f)
  {
    glyphSize = fallbackSize;
  }

  auto arrays = MakeGlyphs(field, cells, coords, glyphSize, Offset);

  auto* v = (viskores::Vec3f_32*)arrays.Vertices.GetBuffers()[0].ReadPointerHost(*arrays.Token);
  auto* r = (float*)arrays.Radii.GetBuffers()[0].ReadPointerHost(*arrays.Token);

  auto d = this->GetDevice();
  this->Handles->Parameters.Vertex.Position =
    anari_cpp::newArray1D(d, v, NoopANARIDeleter, nullptr, arrays.Vertices.GetNumberOfValues());
  this->Handles->Parameters.Vertex.Radius =
    anari_cpp::newArray1D(d, r, NoopANARIDeleter, nullptr, arrays.Radii.GetNumberOfValues());
  this->Handles->Parameters.NumPrimitives = numGlyphs;

  this->UpdateGeometry();

  this->Handles->Arrays = std::move(arrays);
  this->Valid = true;
  this->ClearDirty(DirtyCategory::Data);
  this->ClearDirty(DirtyCategory::Topology);
  this->ClearDirty(DirtyCategory::Attributes);

  this->RefreshGroup();
}

void ANARIMapperGlyphs::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.radius");

  anari_cpp::setParameter(d, this->Handles->Geometry, "name", this->MakeObjectName("geometry"));

  if (this->Handles->Parameters.Vertex.Position)
  {
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.position", this->Handles->Parameters.Vertex.Position);
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.radius", this->Handles->Parameters.Vertex.Radius);
    anari_cpp::setParameter(d, this->Handles->Geometry, "caps", "both");
  }

  anari_cpp::commitParameters(d, this->Handles->Geometry);
}

void ANARIMapperGlyphs::UpdateMaterializedObjects()
{
  const bool namesChanged = this->IsDirty(DirtyCategory::Names);
  this->ANARIMapper::UpdateMaterializedObjects();

  if ((this->IsDirty(DirtyCategory::Data) || this->IsDirty(DirtyCategory::Topology)) &&
      this->Handles->Geometry)
  {
    this->ConstructArrays();
  }
  if (namesChanged)
  {
    this->UpdateGeometry();
    auto d = this->GetDevice();
    if (this->Handles->Material)
    {
      anari_cpp::setParameter(d, this->Handles->Material, "name", this->MakeObjectName("material"));
      anari_cpp::commitParameters(d, this->Handles->Material);
    }
    if (this->Handles->Surface)
    {
      anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
      anari_cpp::commitParameters(d, this->Handles->Surface);
    }
  }
}

ANARIMapperGlyphs::ANARIHandles::~ANARIHandles()
{
  anari_cpp::release(this->Device, this->Surface);
  anari_cpp::release(this->Device, this->Material);
  anari_cpp::release(this->Device, this->Geometry);
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperGlyphs::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->Parameters.Vertex.Position);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Radius);
  this->Parameters.Vertex.Position = nullptr;
  this->Parameters.Vertex.Radius = nullptr;
  this->Parameters.NumPrimitives = 0;
}

} // namespace anari
} // namespace interop
} // namespace viskores
