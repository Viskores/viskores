//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// vtk-m
#include "vtkm/rendering/raytracing/SphereExtractor.h"
#include <vtkm/VectorAnalysis.h>
#include <vtkm/filter/field_conversion/CellAverage.h>
#include <vtkm/interop/anari/ANARIMapperGlyphs.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace interop
{
namespace anari
{

// Worklets ///////////////////////////////////////////////////////////////////

class GeneratePointGlyphs : public vtkm::worklet::WorkletMapField
{
public:
  vtkm::Float32 SizeFactor{ 0.f };
  bool Offset{ false };

  VTKM_CONT
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
  VTKM_EXEC void operator()(const vtkm::Id idx,
                            const InGradientType gradient,
                            const InPointPortalType& points,
                            OutVertexPortalType& vertices,
                            OutRadiusPortalType& radii) const
  {
    auto ng = vtkm::Normal(static_cast<vtkm::Vec3f_32>(gradient));
    auto pt = points.Get(idx);
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

static GlyphArrays MakeGlyphs(vtkm::cont::Field gradients,
                              vtkm::cont::UnknownCellSet cells,
                              vtkm::cont::CoordinateSystem coords,
                              float glyphSize,
                              bool offset)
{
  const auto numGlyphs = gradients.GetNumberOfValues();

  GlyphArrays retval;

  retval.Vertices.Allocate(numGlyphs * 4);
  retval.Radii.Allocate(numGlyphs * 4);

  GeneratePointGlyphs worklet(glyphSize, offset);
  vtkm::worklet::DispatcherMapField<GeneratePointGlyphs> dispatch(worklet);

  if (gradients.IsPointField())
    dispatch.Invoke(gradients, coords, retval.Vertices, retval.Radii);
  else
  {
    vtkm::cont::DataSet centersInput;
    centersInput.AddCoordinateSystem(coords);
    centersInput.SetCellSet(cells);

    vtkm::filter::field_conversion::CellAverage filter;
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetOutputFieldName("Centers");
    auto centersOutput = filter.Execute(centersInput);

    auto resolveField = [&](const auto& concreteField) {
      dispatch.Invoke(gradients, concreteField, retval.Vertices, retval.Radii);
    };
    centersOutput.GetField("Centers")
      .GetData()
      .CastAndCallForTypesWithFloatFallback<vtkm::TypeListFieldVec3,
                                            vtkm::List<vtkm::cont::StorageTagBasic>>(resolveField);
  }

  return retval;
}

// ANARIMapperGlyphs definitions //////////////////////////////////////////////

ANARIMapperGlyphs::ANARIMapperGlyphs(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const char* name,
                                     const vtkm::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperGlyphs::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperGlyphs::~ANARIMapperGlyphs()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperGlyphs::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
}

void ANARIMapperGlyphs::SetOffsetGlyphs(bool enabled)
{
  this->Offset = enabled;
}

anari_cpp::Geometry ANARIMapperGlyphs::GetANARIGeometry()
{
  if (this->Handles->Geometry)
    return this->Handles->Geometry;

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "cone");
  this->ConstructArrays();
  this->UpdateGeometry();
  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperGlyphs::GetANARISurface()
{
  if (this->Handles->Surface)
    return this->Handles->Surface;

  auto d = this->GetDevice();

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
  return this->Handles->Surface;
}

void ANARIMapperGlyphs::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  this->Current = true;
  this->Valid = false;

  this->Handles->ReleaseArrays();

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();
  const auto& cells = actor.GetCellSet();
  const auto& field = actor.GetField();

  auto numGlyphs = field.GetNumberOfValues();

  if (numGlyphs == 0)
  {
    this->RefreshGroup();
    return;
  }

  vtkm::Bounds coordBounds = coords.GetBounds();
  vtkm::Float64 lx = coordBounds.X.Length();
  vtkm::Float64 ly = coordBounds.Y.Length();
  vtkm::Float64 lz = coordBounds.Z.Length();
  vtkm::Float64 mag = vtkm::Sqrt(lx * lx + ly * ly + lz * lz);
  constexpr vtkm::Float64 heuristic = 300.;
  auto glyphSize = static_cast<vtkm::Float32>(mag / heuristic);

  auto arrays = MakeGlyphs(field, cells, coords, glyphSize, Offset);

  auto* v = (vtkm::Vec3f_32*)arrays.Vertices.GetBuffers()[0].ReadPointerHost(*arrays.Token);
  auto* r = (float*)arrays.Radii.GetBuffers()[0].ReadPointerHost(*arrays.Token);

  auto d = this->GetDevice();
  this->Handles->Parameters.Vertex.Position =
    anari_cpp::newArray1D(d, v, NoopANARIDeleter, nullptr, arrays.Vertices.GetNumberOfValues());
  this->Handles->Parameters.Vertex.Radius =
    anari_cpp::newArray1D(d, r, NoopANARIDeleter, nullptr, arrays.Radii.GetNumberOfValues());
  this->Handles->Parameters.NumPrimitives = numGlyphs;

  this->UpdateGeometry();

  this->Arrays = arrays;
  this->Valid = true;

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

ANARIMapperGlyphs::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Surface);
  anari_cpp::release(this->Device, this->Material);
  anari_cpp::release(this->Device, this->Geometry);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperGlyphs::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->Parameters.Vertex.Position);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Radius);
  this->Parameters.Vertex.Position = nullptr;
  this->Parameters.Vertex.Radius = nullptr;
}

} // namespace anari
} // namespace interop
} // namespace vtkm
