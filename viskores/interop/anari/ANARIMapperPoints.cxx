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
#include <viskores/Math.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>

namespace viskores
{
namespace interop
{
namespace anari
{

// Helper functions ///////////////////////////////////////////////////////////

VISKORES_CONT static PointsFieldArrays LowerFields(const FieldSet& fields,
                                                   viskores::Id numberOfPoints)
{
  PointsFieldArrays result;

  for (viskores::IdComponent fieldIndex = 0;
       fieldIndex < static_cast<viskores::IdComponent>(fields.size());
       ++fieldIndex)
  {
    const auto index = static_cast<std::size_t>(fieldIndex);
    const auto& field = fields[index];
    const auto numberOfComponents = field.GetData().GetNumberOfComponentsFlat();
    if (field.GetAssociation() != viskores::cont::Field::Association::Points ||
        field.GetNumberOfValues() != numberOfPoints || numberOfComponents < 1 ||
        numberOfComponents > 4)
    {
      continue;
    }

    auto& binding = result.Fields[index];
    binding.Values = viskores::cont::ArrayHandleRuntimeVec<viskores::Float32>(numberOfComponents);
    viskores::cont::ArrayCopyShallowIfPossible(field.GetData(), binding.Values);
    binding.Name = field.GetName();
  }

  return result;
}

VISKORES_CONT static PointsArrays LowerPoints(const viskores::cont::CoordinateSystem& coordinates)
{
  PointsArrays result;
  viskores::cont::ArrayCopyShallowIfPossible(coordinates.GetData(), result.Vertices);
  return result;
}

// ANARIMapperPoints definitions //////////////////////////////////////////////

ANARIMapperPoints::ANARIMapperPoints(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_unique<ANARIMapperPoints::ANARIHandles>();
  this->Handles->Device = device;
  auto& attributes = this->Handles->Parameters.Vertex.Attribute;
  std::fill(attributes.begin(), attributes.end(), nullptr);
  anari_cpp::retain(device, device);
}

ANARIMapperPoints::ANARIMapperPoints(ANARIMapperPoints&&) = default;

ANARIMapperPoints::~ANARIMapperPoints()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

anari_cpp::Geometry ANARIMapperPoints::GetANARIGeometry()
{
  if (this->Handles->Geometry)
  {
    this->ConstructArrays();
    this->UpdateMaterializedObjects();
    return this->Handles->Geometry;
  }

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "sphere");
  this->ConstructArrays();
  this->UpdateGeometry();

  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperPoints::GetANARISurface()
{
  if (this->Handles->Surface)
  {
    this->UpdateMaterializedObjects();
    return this->Handles->Surface;
  }

  auto d = this->GetDevice();

  this->Handles->Surface = anari_cpp::newObject<anari_cpp::Surface>(d);

  if (!this->Handles->Material)
  {
    this->Handles->Material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
    anari_cpp::setParameter(d, this->Handles->Material, "name", this->MakeObjectName("material"));
  }

  auto s = anari_cpp::newObject<anari_cpp::Sampler>(d, "image1D");
  this->Handles->Sampler = s;
  this->UpdateANARISampler(s, "linear");

  this->UpdateMaterial();

  anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
  anari_cpp::setParameter(d, this->Handles->Surface, "geometry", this->GetANARIGeometry());
  anari_cpp::setParameter(d, this->Handles->Surface, "material", this->Handles->Material);
  anari_cpp::commitParameters(d, this->Handles->Surface);
  this->ClearDirty(DirtyCategory::Appearance);
  this->ClearDirty(DirtyCategory::Names);

  return this->Handles->Surface;
}

void ANARIMapperPoints::ConstructArrays()
{
  if (!this->IsDirty(DirtyCategory::Data) && !this->IsDirty(DirtyCategory::Topology))
    return;

  this->Valid = false;

  this->Handles->ReleaseArrays();

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();

  if (coords.GetNumberOfPoints() == 0)
  {
    this->UpdateGeometry();
    this->UpdateMaterial();
    this->Arrays = {};
    this->FieldArrays = {};
    this->ClearDirty(DirtyCategory::Data);
    this->ClearDirty(DirtyCategory::Topology);
    this->ClearDirty(DirtyCategory::Attributes);
    this->RefreshGroup();
    return;
  }

  viskores::Bounds coordBounds = coords.GetBounds();
  // set a default radius
  viskores::Float64 lx = coordBounds.X.Length();
  viskores::Float64 ly = coordBounds.Y.Length();
  viskores::Float64 lz = coordBounds.Z.Length();
  viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
  // same as used in vtk ospray
  constexpr viskores::Float64 heuristic = 500.;
  auto baseRadius = static_cast<viskores::Float32>(mag / heuristic);

  constexpr viskores::Float32 degenerateRadius = 0.01f;
  this->Handles->Parameters.Radius =
    viskores::IsFinite(baseRadius) && baseRadius > 0.f ? baseRadius : degenerateRadius;

  this->PrimaryField = actor.GetPrimaryFieldIndex();

  auto arrays = LowerPoints(coords);
  const auto numPoints = static_cast<viskores::UInt64>(arrays.Vertices.GetNumberOfValues());
  auto fieldArrays = LowerFields(actor.GetFieldSet(),
                                 static_cast<viskores::Id>(arrays.Vertices.GetNumberOfValues()));
  this->Handles->Parameters.NumPrimitives = numPoints;

  auto* positionPointer = static_cast<const viskores::Vec3f_32*>(
    arrays.Vertices.GetBuffers()[0].ReadPointerHost(*arrays.Token));

  auto d = this->GetDevice();
  this->Handles->Parameters.Vertex.Position =
    anari_cpp::newArray1D(d, positionPointer, NoopANARIDeleter, nullptr, numPoints);

  for (viskores::IdComponent fieldIndex = 0;
       fieldIndex < static_cast<viskores::IdComponent>(fieldArrays.Fields.size());
       ++fieldIndex)
  {
    const auto index = static_cast<std::size_t>(fieldIndex);
    const auto& binding = fieldArrays.Fields[index];
    if (binding.Values.GetNumberOfValues() == 0)
    {
      continue;
    }

    const auto& buffers = binding.Values.GetBuffers();
    auto* attributePointer = buffers[0].ReadPointerHost(*fieldArrays.Token);
    if (!attributePointer && buffers.size() > 1)
    {
      attributePointer = buffers[1].ReadPointerHost(*fieldArrays.Token);
    }
    const auto elementType =
      static_cast<ANARIDataType>(ANARI_FLOAT32 + binding.Values.GetNumberOfComponents() - 1);
    this->Handles->Parameters.Vertex.Attribute[index] = attributePointer
      ? anariNewArray1D(d,
                        attributePointer,
                        NoopANARIDeleter,
                        nullptr,
                        elementType,
                        static_cast<viskores::UInt64>(binding.Values.GetNumberOfValues()))
      : nullptr;
    this->Handles->Parameters.Vertex.AttributeName[index] = binding.Name;
  }

  this->UpdateGeometry();
  this->UpdateMaterial();

  this->Arrays = arrays;
  this->FieldArrays = fieldArrays;
  this->Valid = true;
  this->ClearDirty(DirtyCategory::Data);
  this->ClearDirty(DirtyCategory::Topology);
  this->ClearDirty(DirtyCategory::Attributes);

  this->RefreshGroup();
}

void ANARIMapperPoints::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.radius");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "radius");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute0");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute1");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute2");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.attribute3");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute0.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute1.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute2.name");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "usd::attribute3.name");

  anari_cpp::setParameter(d, this->Handles->Geometry, "name", this->MakeObjectName("geometry"));

  if (this->Handles->Parameters.Vertex.Position)
  {
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.position", this->Handles->Parameters.Vertex.Position);
    anari_cpp::setParameter(d, this->Handles->Geometry, "radius", this->Handles->Parameters.Radius);
    if (this->GetMapFieldAsAttribute())
    {
      for (viskores::IdComponent fieldIndex = 0; fieldIndex <
           static_cast<viskores::IdComponent>(this->Handles->Parameters.Vertex.Attribute.size());
           ++fieldIndex)
      {
        const auto index = static_cast<std::size_t>(fieldIndex);
        const auto attribute = this->Handles->Parameters.Vertex.Attribute[index];
        if (!attribute)
        {
          continue;
        }

        const auto attributeName = std::string(AnariMaterialInputString(fieldIndex));
        const auto vertexParameter = std::string("vertex.") + attributeName;
        const auto usdNameParameter = std::string("usd::") + attributeName + ".name";
        anari_cpp::setParameter(d, this->Handles->Geometry, vertexParameter.c_str(), attribute);
        const auto& fieldName = this->Handles->Parameters.Vertex.AttributeName[index];
        if (!fieldName.empty())
        {
          anari_cpp::setParameter(d, this->Handles->Geometry, usdNameParameter.c_str(), fieldName);
        }
      }
    }
  }

  anari_cpp::commitParameters(d, this->Handles->Geometry);
  this->ClearDirty(DirtyCategory::Attributes);
}

void ANARIMapperPoints::UpdateMaterial()
{
  if (!this->Handles->Material)
    return;

  auto d = this->GetDevice();
  auto s = this->Handles->Sampler;
  anari_cpp::setParameter(d, this->Handles->Material, "name", this->MakeObjectName("material"));
  bool havePrimaryField = false;
  if (this->PrimaryField >= 0 &&
      this->PrimaryField <
        static_cast<viskores::IdComponent>(this->Handles->Parameters.Vertex.Attribute.size()))
  {
    havePrimaryField =
      this->Handles->Parameters.Vertex.Attribute[static_cast<std::size_t>(this->PrimaryField)] !=
      nullptr;
  }
  if (s && havePrimaryField && this->GetMapFieldAsAttribute())
  {
    anari_cpp::setParameter(d, s, "inAttribute", AnariMaterialInputString(this->PrimaryField));
    anari_cpp::commitParameters(d, s);
    anari_cpp::setParameter(d, this->Handles->Material, "color", s);
  }
  else
    anari_cpp::setParameter(d, this->Handles->Material, "color", viskores::Vec3f_32(1.f));

  anari_cpp::commitParameters(d, this->Handles->Material);
}

void ANARIMapperPoints::UpdateMaterializedObjects()
{
  const bool namesChanged = this->IsDirty(DirtyCategory::Names);
  this->ANARIMapper::UpdateMaterializedObjects();

  if ((this->IsDirty(DirtyCategory::Data) || this->IsDirty(DirtyCategory::Topology)) &&
      this->Handles->Geometry)
  {
    this->ConstructArrays();
  }
  if (this->IsDirty(DirtyCategory::Attributes) && this->Handles->Geometry)
  {
    this->UpdateGeometry();
    this->UpdateMaterial();
  }
  if (this->IsDirty(DirtyCategory::Appearance) && this->Handles->Sampler)
  {
    this->UpdateANARISampler(this->Handles->Sampler, "linear");
    this->ClearDirty(DirtyCategory::Appearance);
  }
  if (namesChanged)
  {
    this->UpdateGeometry();
    this->UpdateANARISampler(this->Handles->Sampler, "linear");
    this->UpdateMaterial();
    if (this->Handles->Surface)
    {
      auto d = this->GetDevice();
      anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
      anari_cpp::commitParameters(d, this->Handles->Surface);
    }
  }
}

ANARIMapperPoints::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Surface);
  anari_cpp::release(this->Device, this->Material);
  anari_cpp::release(this->Device, this->Sampler);
  anari_cpp::release(this->Device, this->Geometry);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperPoints::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->Parameters.Vertex.Position);
  this->Parameters.Vertex.Position = nullptr;
  this->Parameters.NumPrimitives = 0;
  for (std::size_t fieldIndex = 0; fieldIndex < this->Parameters.Vertex.Attribute.size();
       ++fieldIndex)
  {
    anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[fieldIndex]);
    this->Parameters.Vertex.Attribute[fieldIndex] = nullptr;
    this->Parameters.Vertex.AttributeName[fieldIndex].clear();
  }
}

} // namespace anari
} // namespace interop
} // namespace viskores
