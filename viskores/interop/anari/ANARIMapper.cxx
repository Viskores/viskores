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

#include <viskores/interop/anari/ANARIMapper.h>

#include <viskores/Math.h>

#include <anari/anari_cpp/ext/linalg.h>

namespace viskores
{
namespace interop
{
namespace anari
{

namespace
{

constexpr viskores::Int32 NumberOfColorSamples = 256;

viskores::Vec2f_32 NormalizeValueRange(viskores::Float64 minimum, viskores::Float64 maximum)
{
  if (viskores::IsFinite(minimum) && viskores::IsFinite(maximum) && maximum > minimum)
  {
    const auto minimum32 = static_cast<viskores::Float32>(minimum);
    const auto maximum32 = static_cast<viskores::Float32>(maximum);
    if (viskores::IsFinite(minimum32) && viskores::IsFinite(maximum32) && maximum32 > minimum32)
    {
      return { minimum32, maximum32 };
    }
  }

  if (viskores::IsFinite(minimum) && minimum == maximum)
  {
    const auto center = static_cast<viskores::Float32>(minimum);
    if (viskores::IsFinite(center))
    {
      const auto radius = viskores::Max(0.5f, viskores::Abs(center) * 0.5f);
      if (viskores::IsFinite(center - radius) && viskores::IsFinite(center + radius))
      {
        return { center - radius, center + radius };
      }
    }
  }

  return { 0.f, 1.f };
}

} // anonymous namespace

struct ANARIMapper::AppearanceState
{
  enum class Source
  {
    ColorTable,
    RawArrays
  };

  anari_cpp::Device Device{ nullptr };
  Source ColorSource{ Source::ColorTable };
  anari_cpp::Array1D RawColor{ nullptr };
  anari_cpp::Array1D RawOpacity{ nullptr };
  anari_cpp::Array1D SurfaceColor{ nullptr };
  anari_cpp::Array1D VolumeColor{ nullptr };
  anari_cpp::Array1D VolumeOpacity{ nullptr };
  viskores::Vec2f_32 ValueRange{ 0.f, 1.f };
  viskores::Float32 OpacityScale{ 1.f };
  bool ValueRangeIsExplicit{ false };

  ~AppearanceState() { this->ReleaseArrays(); }

  void ReleaseSampledArrays()
  {
    anari_cpp::release(this->Device, this->SurfaceColor);
    anari_cpp::release(this->Device, this->VolumeColor);
    anari_cpp::release(this->Device, this->VolumeOpacity);
    this->SurfaceColor = nullptr;
    this->VolumeColor = nullptr;
    this->VolumeOpacity = nullptr;
  }

  void ReleaseArrays()
  {
    anari_cpp::release(this->Device, this->RawColor);
    anari_cpp::release(this->Device, this->RawOpacity);
    this->RawColor = nullptr;
    this->RawOpacity = nullptr;
    this->ReleaseSampledArrays();
  }
};

ANARIMapper::ANARIMapper(anari_cpp::Device device,
                         const ANARIActor& actor,
                         const std::string& name,
                         const viskores::cont::ColorTable& colorTable)
  : Actor(actor)
  , ColorTable(colorTable)
  , Name(name)
{
  this->Handles = std::make_unique<ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
  this->Appearance = std::make_unique<AppearanceState>();
  this->Appearance->Device = device;
  this->UpdateDefaultValueRange();
}

ANARIMapper::~ANARIMapper() = default;

ANARIMapper::ANARIMapper(ANARIMapper&&) = default;

anari_cpp::Device ANARIMapper::GetDevice() const
{
  return this->Handles->Device;
}

const ANARIActor& ANARIMapper::GetActor() const
{
  return this->Actor;
}

const char* ANARIMapper::GetName() const
{
  return this->Name.c_str();
}

void ANARIMapper::SetActor(const ANARIActor& actor)
{
  const auto previousActor = this->Actor;
  const auto previousDirtyCategories = this->DirtyCategories;
  const auto previousValueRange = this->Appearance->ValueRange;
  this->Actor = actor;
  this->MarkDirty(DirtyCategory::Data);
  this->MarkDirty(DirtyCategory::Topology);
  this->MarkDirty(DirtyCategory::Attributes);
  if (!this->Appearance->ValueRangeIsExplicit)
  {
    this->UpdateDefaultValueRange();
    this->MarkDirty(DirtyCategory::Appearance);
  }
  try
  {
    this->UpdateMaterializedObjects();
  }
  catch (...)
  {
    this->Actor = previousActor;
    this->DirtyCategories = previousDirtyCategories;
    this->Appearance->ValueRange = previousValueRange;
    throw;
  }
}

void ANARIMapper::SetMapFieldAsAttribute(bool enabled)
{
  if (this->MapFieldAsAttribute == enabled)
  {
    return;
  }
  this->MapFieldAsAttribute = enabled;
  this->MarkDirty(DirtyCategory::Attributes);
  this->UpdateMaterializedObjects();
}

bool ANARIMapper::GetMapFieldAsAttribute() const
{
  return this->MapFieldAsAttribute;
}

const viskores::cont::ColorTable& ANARIMapper::GetColorTable() const
{
  return this->ColorTable;
}

void ANARIMapper::SetANARIColorMap(anari_cpp::Array1D color,
                                   anari_cpp::Array1D opacity,
                                   bool releaseArrays)
{
  if (!releaseArrays)
  {
    anari_cpp::retain(this->GetDevice(), color);
    anari_cpp::retain(this->GetDevice(), opacity);
  }

  this->Appearance->ReleaseArrays();
  this->Appearance->ColorSource = AppearanceState::Source::RawArrays;
  this->Appearance->RawColor = color;
  this->Appearance->RawOpacity = opacity;
  this->MarkDirty(DirtyCategory::Appearance);
  this->UpdateMaterializedObjects();
}

void ANARIMapper::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  this->Appearance->ValueRange = NormalizeValueRange(valueRange[0], valueRange[1]);
  this->Appearance->ValueRangeIsExplicit = true;
  this->MarkDirty(DirtyCategory::Appearance);
  this->UpdateMaterializedObjects();
}

void ANARIMapper::SetANARIColorMapOpacityScale(viskores::Float32 opacityScale)
{
  this->Appearance->OpacityScale =
    viskores::IsFinite(opacityScale) && opacityScale >= 0.f ? opacityScale : 1.f;
  this->MarkDirty(DirtyCategory::Appearance);
  this->UpdateMaterializedObjects();
}

void ANARIMapper::SetName(const char* name)
{
  const std::string nextName = name ? name : "";
  if (this->Name == nextName)
  {
    return;
  }
  this->Name = nextName;
  this->MarkDirty(DirtyCategory::Names);
  this->UpdateMaterializedObjects();
}

void ANARIMapper::SetColorTable(const viskores::cont::ColorTable& colorTable)
{
  this->ColorTable = colorTable;
  this->Appearance->ReleaseArrays();
  this->Appearance->ColorSource = AppearanceState::Source::ColorTable;
  if (!this->Appearance->ValueRangeIsExplicit)
  {
    this->UpdateDefaultValueRange();
  }
  this->MarkDirty(DirtyCategory::Appearance);
  this->UpdateMaterializedObjects();
}

anari_cpp::Geometry ANARIMapper::GetANARIGeometry()
{
  return nullptr;
}

anari_cpp::SpatialField ANARIMapper::GetANARISpatialField()
{
  return nullptr;
}

anari_cpp::Surface ANARIMapper::GetANARISurface()
{
  return nullptr;
}

anari_cpp::Volume ANARIMapper::GetANARIVolume()
{
  return nullptr;
}

anari_cpp::Group ANARIMapper::GetANARIGroup()
{
  if (!this->Handles->Group)
  {
    auto d = this->GetDevice();
    this->Handles->Group = anari_cpp::newObject<anari_cpp::Group>(d);
    this->RefreshGroup();
  }

  return this->Handles->Group;
}

anari_cpp::Instance ANARIMapper::GetANARIInstance()
{
  if (!this->Handles->Instance)
  {
    auto d = this->GetDevice();
    this->Handles->Instance = anari_cpp::newObject<anari_cpp::Instance>(d, "transform");
    auto group = this->GetANARIGroup();
    anari_cpp::setParameter(d, this->Handles->Instance, "group", group);
    anari_cpp::setParameter(d, this->Handles->Instance, "name", MakeObjectName("instance"));
    anari_cpp::commitParameters(d, this->Handles->Instance);
  }

  return this->Handles->Instance;
}

bool ANARIMapper::GroupIsEmpty() const
{
  return !this->Valid;
}

std::string ANARIMapper::MakeObjectName(const char* suffix) const
{
  std::string name = this->GetName();
  name += '.';
  name += suffix;
  return name;
}

bool ANARIMapper::IsDirty(DirtyCategory category) const
{
  return (this->DirtyCategories & static_cast<viskores::UInt8>(category)) != 0;
}

void ANARIMapper::MarkDirty(DirtyCategory category)
{
  this->DirtyCategories |= static_cast<viskores::UInt8>(category);
}

void ANARIMapper::ClearDirty(DirtyCategory category)
{
  this->DirtyCategories &= static_cast<viskores::UInt8>(~static_cast<viskores::UInt8>(category));
}

void ANARIMapper::UpdateMaterializedObjects()
{
  if (!this->IsDirty(DirtyCategory::Names))
  {
    return;
  }

  auto d = this->GetDevice();
  if (this->Handles->Group)
  {
    anari_cpp::setParameter(d, this->Handles->Group, "name", this->MakeObjectName("group"));
    anari_cpp::commitParameters(d, this->Handles->Group);
  }
  if (this->Handles->Instance)
  {
    anari_cpp::setParameter(d, this->Handles->Instance, "name", this->MakeObjectName("instance"));
    anari_cpp::commitParameters(d, this->Handles->Instance);
  }
  this->ClearDirty(DirtyCategory::Names);
}

void ANARIMapper::UpdateANARISampler(anari_cpp::Sampler sampler, const char* filter)
{
  if (!sampler)
  {
    return;
  }

  auto d = this->GetDevice();
  this->EnsureSampledColorArrays();

  const auto color = this->Appearance->ColorSource == AppearanceState::Source::RawArrays
    ? this->Appearance->RawColor
    : this->Appearance->SurfaceColor;
  if (color)
  {
    anari_cpp::setParameter(d, sampler, "image", color);
  }
  else
  {
    anari_cpp::unsetParameter(d, sampler, "image");
  }

  const auto minimum = this->Appearance->ValueRange[0];
  const auto maximum = this->Appearance->ValueRange[1];
  const auto scale = anari_cpp::scaling_matrix(anari_cpp::float3(1.f / (maximum - minimum)));
  const auto translation = anari_cpp::translation_matrix(anari_cpp::float3(-minimum, 0, 0));
  anari_cpp::setParameter(d, sampler, "inTransform", anari_cpp::mul(scale, translation));
  anari_cpp::setParameter(d, sampler, "outTransform", anari_cpp::math::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, sampler, "inOffset", viskores::Vec4f_32(0.f));
  anari_cpp::setParameter(d, sampler, "outOffset", viskores::Vec4f_32(0.f));
  anari_cpp::setParameter(d, sampler, "filter", filter);
  anari_cpp::setParameter(d, sampler, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, sampler, "name", this->MakeObjectName("colormap"));
  anari_cpp::commitParameters(d, sampler);
}

void ANARIMapper::UpdateANARIVolumeAppearance(anari_cpp::Volume volume)
{
  if (!volume)
  {
    return;
  }

  this->EnsureSampledColorArrays();

  auto d = this->GetDevice();
  const auto color = this->Appearance->ColorSource == AppearanceState::Source::RawArrays
    ? this->Appearance->RawColor
    : this->Appearance->VolumeColor;
  const auto opacity = this->Appearance->ColorSource == AppearanceState::Source::RawArrays
    ? this->Appearance->RawOpacity
    : this->Appearance->VolumeOpacity;
  if (color)
  {
    anari_cpp::setParameter(d, volume, "color", color);
  }
  else
  {
    anari_cpp::unsetParameter(d, volume, "color");
  }
  if (opacity)
  {
    anari_cpp::setParameter(d, volume, "opacity", opacity);
  }
  else
  {
    anari_cpp::unsetParameter(d, volume, "opacity");
  }
  anari_cpp::setParameter(
    d, volume, "valueRange", ANARI_FLOAT32_BOX1, &this->Appearance->ValueRange);
  anari_cpp::setParameter(d, volume, "densityScale", this->Appearance->OpacityScale);
  anari_cpp::setParameter(d, volume, "name", this->MakeObjectName("volume"));
  anari_cpp::commitParameters(d, volume);
}

void ANARIMapper::EnsureSampledColorArrays()
{
  if (this->Appearance->ColorSource != AppearanceState::Source::ColorTable ||
      this->Appearance->SurfaceColor)
  {
    return;
  }

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> samples;
  this->ColorTable.Sample(NumberOfColorSamples, samples);
  auto portal = samples.ReadPortal();

  auto d = this->GetDevice();
  this->Appearance->SurfaceColor =
    anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, NumberOfColorSamples);
  this->Appearance->VolumeColor =
    anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, NumberOfColorSamples);
  this->Appearance->VolumeOpacity = anari_cpp::newArray1D(d, ANARI_FLOAT32, NumberOfColorSamples);
  auto* surfaceColors = anari_cpp::map<viskores::Vec4f_32>(d, this->Appearance->SurfaceColor);
  auto* volumeColors = anari_cpp::map<viskores::Vec3f_32>(d, this->Appearance->VolumeColor);
  auto* opacities = anari_cpp::map<viskores::Float32>(d, this->Appearance->VolumeOpacity);
  constexpr viskores::Float32 normalize = 1.f / 255.f;
  for (viskores::Id sampleIndex = 0; sampleIndex < NumberOfColorSamples; ++sampleIndex)
  {
    const auto sample = portal.Get(sampleIndex);
    const viskores::Vec4f_32 rgba(
      sample[0] * normalize, sample[1] * normalize, sample[2] * normalize, sample[3] * normalize);
    surfaceColors[sampleIndex] = rgba;
    volumeColors[sampleIndex] = viskores::Vec3f_32(rgba[0], rgba[1], rgba[2]);
    opacities[sampleIndex] = rgba[3];
  }
  anari_cpp::unmap(d, this->Appearance->SurfaceColor);
  anari_cpp::unmap(d, this->Appearance->VolumeColor);
  anari_cpp::unmap(d, this->Appearance->VolumeOpacity);
}

void ANARIMapper::UpdateDefaultValueRange()
{
  const auto fields = this->Actor.GetFieldSet();
  const auto primaryField = this->Actor.GetPrimaryFieldIndex();
  if (primaryField >= 0 && primaryField < static_cast<viskores::IdComponent>(fields.size()))
  {
    const auto& field = fields[static_cast<std::size_t>(primaryField)];
    if (field.GetNumberOfValues() > 0 && field.GetData().GetNumberOfComponentsFlat() > 0)
    {
      const auto ranges = field.GetRange();
      if (ranges.GetNumberOfValues() > 0)
      {
        const auto range = ranges.ReadPortal().Get(0);
        this->Appearance->ValueRange = NormalizeValueRange(range.Min, range.Max);
        return;
      }
    }
  }

  const auto range = this->ColorTable.GetRange();
  this->Appearance->ValueRange = NormalizeValueRange(range.Min, range.Max);
}

void ANARIMapper::RefreshGroup()
{
  if (!this->Handles->Group)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Group, "surface");
  anari_cpp::unsetParameter(d, this->Handles->Group, "volume");

  auto surface = this->GetANARISurface();
  auto volume = this->GetANARIVolume();

  if (!this->GroupIsEmpty())
  {
    if (surface)
      anari_cpp::setParameterArray1D(d, this->Handles->Group, "surface", &surface, 1);

    if (volume)
      anari_cpp::setParameterArray1D(d, this->Handles->Group, "volume", &volume, 1);

    anari_cpp::setParameter(d, this->Handles->Group, "name", MakeObjectName("group"));
  }

  anari_cpp::commitParameters(d, this->Handles->Group);
}

ANARIMapper::ANARIHandles::~ANARIHandles()
{
  anari_cpp::release(this->Device, this->Group);
  anari_cpp::release(this->Device, this->Instance);
  anari_cpp::release(this->Device, this->Device);
}

} // namespace anari
} // namespace interop
} // namespace viskores
