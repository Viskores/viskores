//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/interop/anari/ANARIMapperVolume.h>

namespace vtkm
{
namespace interop
{
namespace anari
{

ANARIMapperVolume::ANARIMapperVolume(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const vtkm::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperVolume::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperVolume::~ANARIMapperVolume()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperVolume::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
}

void ANARIMapperVolume::SetANARIColorMap(anari_cpp::Array1D color,
                                         anari_cpp::Array1D opacity,
                                         bool releaseArrays)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "color", color);
  anari_cpp::setParameter(d, v, "opacity", opacity);
  anari_cpp::commitParameters(d, v);
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperVolume::SetANARIColorMapValueRange(const vtkm::Vec2f_32& valueRange)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "valueRange", ANARI_FLOAT32_BOX1, &valueRange);
  anari_cpp::commitParameters(d, v);
}

void ANARIMapperVolume::SetANARIColorMapOpacityScale(vtkm::Float32 opacityScale)
{
  auto d = this->GetDevice();
  auto v = this->GetANARIVolume();
  anari_cpp::setParameter(d, v, "densityScale", opacityScale);
  anari_cpp::commitParameters(d, v);
}

anari_cpp::SpatialField ANARIMapperVolume::GetANARISpatialField()
{
  if (this->Handles->SpatialField)
    return this->Handles->SpatialField;

  this->Handles->SpatialField =
    anari_cpp::newObject<anari_cpp::SpatialField>(this->GetDevice(), "structuredRegular");
  this->ConstructArrays();
  this->UpdateSpatialField();
  return this->Handles->SpatialField;
}

anari_cpp::Volume ANARIMapperVolume::GetANARIVolume()
{
  if (this->Handles->Volume)
    return this->Handles->Volume;

  auto d = this->GetDevice();

  this->Handles->Volume = anari_cpp::newObject<anari_cpp::Volume>(d, "transferFunction1D");

  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
  auto* colors = anari_cpp::map<vtkm::Vec3f_32>(d, colorArray);
  colors[0] = vtkm::Vec3f_32(1.f, 0.f, 0.f);
  colors[1] = vtkm::Vec3f_32(0.f, 1.f, 0.f);
  colors[2] = vtkm::Vec3f_32(0.f, 0.f, 1.f);
  anari_cpp::unmap(d, colorArray);

  auto opacityArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* opacities = anari_cpp::map<float>(d, opacityArray);
  opacities[0] = 0.f;
  opacities[1] = 1.f;
  anari_cpp::unmap(d, opacityArray);

  anari_cpp::setAndReleaseParameter(d, this->Handles->Volume, "color", colorArray);
  anari_cpp::setAndReleaseParameter(d, this->Handles->Volume, "opacity", opacityArray);
  anari_cpp::setParameter(d, this->Handles->Volume, "field", this->GetANARISpatialField());
  anari_cpp::setParameter(d, this->Handles->Volume, "name", this->MakeObjectName("volume"));
  anari_cpp::commitParameters(d, this->Handles->Volume);

  SetANARIColorMapValueRange(vtkm::Vec2f_32(0.f, 10.f));

  return this->Handles->Volume;
}

void ANARIMapperVolume::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  this->Current = true;
  this->Valid = false;

  const auto& actor = this->GetActor();
  const auto& coords = actor.GetCoordinateSystem();
  const auto& cells = actor.GetCellSet();
  const auto& fieldArray = actor.GetField().GetData();

  const bool isStructured = cells.CanConvert<vtkm::cont::CellSetStructured<3>>();
  const bool isFloat = fieldArray.CanConvert<vtkm::cont::ArrayHandle<vtkm::Float32>>();

  this->Handles->ReleaseArrays();

  if (isStructured && isFloat)
  {
    auto structuredCells = cells.AsCellSet<vtkm::cont::CellSetStructured<3>>();
    auto pdims = structuredCells.GetPointDimensions();

    VolumeArrays arrays;

    auto d = this->GetDevice();

    arrays.Data = fieldArray.AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Float32>>();

    auto* ptr = (float*)arrays.Data.GetBuffers()[0].ReadPointerHost(*arrays.Token);

    auto bounds = coords.GetBounds();
    vtkm::Vec3f_32 bLower(bounds.X.Min, bounds.Y.Min, bounds.Z.Min);
    vtkm::Vec3f_32 bUpper(bounds.X.Max, bounds.Y.Max, bounds.Z.Max);
    vtkm::Vec3f_32 size = bUpper - bLower;

    vtkm::Vec3ui_32 dims(pdims[0], pdims[1], pdims[2]);
    auto spacing = size / (vtkm::Vec3f_32(dims) - 1.f);

    std::memcpy(this->Handles->Parameters.Dims, &dims, sizeof(dims));
    std::memcpy(this->Handles->Parameters.Origin, &bLower, sizeof(bLower));
    std::memcpy(this->Handles->Parameters.Spacing, &spacing, sizeof(spacing));
    this->Handles->Parameters.Data =
      anari_cpp::newArray3D(d, ptr, NoopANARIDeleter, nullptr, dims[0], dims[1], dims[2]);

    this->Arrays = arrays;
    this->Valid = true;
  }

  this->UpdateSpatialField();
  this->RefreshGroup();
}

void ANARIMapperVolume::UpdateSpatialField()
{
  if (!this->Handles->SpatialField)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "origin");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "spacing");
  anari_cpp::unsetParameter(d, this->Handles->SpatialField, "data");

  anari_cpp::setParameter(
    d, this->Handles->SpatialField, "name", this->MakeObjectName("spatialField"));

  if (this->Handles->Parameters.Data)
  {
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "origin", this->Handles->Parameters.Origin);
    anari_cpp::setParameter(
      d, this->Handles->SpatialField, "spacing", this->Handles->Parameters.Spacing);
    anari_cpp::setParameter(d, this->Handles->SpatialField, "data", this->Handles->Parameters.Data);
  }

  anari_cpp::commitParameters(d, this->Handles->SpatialField);
}

ANARIMapperVolume::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Volume);
  anari_cpp::release(this->Device, this->SpatialField);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperVolume::ANARIHandles::ReleaseArrays()
{
  anari_cpp::release(this->Device, this->Parameters.Data);
  this->Parameters.Data = nullptr;
}

} // namespace anari
} // namespace interop
} // namespace vtkm
