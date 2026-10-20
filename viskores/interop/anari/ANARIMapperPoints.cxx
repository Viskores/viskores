//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/Logging.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>
// anari
#include <anari/anari_cpp/ext/linalg.h>

namespace viskores
{
namespace interop
{
namespace anari
{

// Helper functions ///////////////////////////////////////////////////////////

namespace
{

/// Radius of every sphere: a fraction of the diagonal of the point bounds, as
/// in VTK's OSPRay pass. Bounds of zero size (a single point, or all points in
/// one place) fall back to ANARI's default sphere radius of 1.
viskores::Float32 DefaultRadius(const viskores::Bounds& bounds)
{
  const viskores::Float64 lx = bounds.X.Length();
  const viskores::Float64 ly = bounds.Y.Length();
  const viskores::Float64 lz = bounds.Z.Length();
  const viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
  constexpr viskores::Float64 heuristic = 500.;
  const auto radius = static_cast<viskores::Float32>(mag / heuristic);
  return (radius > 0.f && viskores::IsFinite(radius)) ? radius : 1.f;
}

/// An ANARI array of `field` as 1-4 component Float32 values, or null when the
/// field is empty, has another number of components, or does not have one
/// value per point.
anari_cpp::Array1D MakeAttributeArray(anari_cpp::Device d,
                                      const viskores::cont::Field& field,
                                      viskores::Id numPoints)
{
  const auto numValues = field.GetNumberOfValues();
  if (numValues == 0)
  {
    return nullptr;
  }

  if (numValues != numPoints)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "ANARIMapperPoints: skipping field '" << field.GetName() << "', which has "
                                                         << numValues << " values for " << numPoints
                                                         << " points");
    return nullptr;
  }

  const auto fieldData = field.GetData();
  const auto numComps = fieldData.GetNumberOfComponentsFlat();
  if (numComps < 1 || numComps > 4)
  {
    VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                   "ANARIMapperPoints: skipping field '" << field.GetName() << "', which has "
                                                         << numComps
                                                         << " components (1 to 4 are supported)");
    return nullptr;
  }

  viskores::cont::ArrayHandleRuntimeVec<viskores::Float32> values(numComps);
  viskores::cont::ArrayCopyShallowIfPossible(fieldData, values);

  const anari_cpp::DataType type = ANARI_FLOAT32 + numComps - 1;
  return MakeANARIArray1D(
    d, values.GetComponentsArray(), type, static_cast<std::uint64_t>(numValues));
}

} // namespace

// ANARIMapperPoints definitions //////////////////////////////////////////////

ANARIMapperPoints::ANARIMapperPoints(anari_cpp::Device device,
                                     const ANARIActor& actor,
                                     const std::string& name,
                                     const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperPoints::ANARIHandles>();
  this->Handles->Device = device;
  auto& attributes = this->Handles->Parameters.Vertex.Attribute;
  std::fill(attributes.begin(), attributes.end(), nullptr);
  anari_cpp::retain(device, device);
}

ANARIMapperPoints::~ANARIMapperPoints() = default;

void ANARIMapperPoints::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
  this->UpdateMaterial();
}

void ANARIMapperPoints::SetMapFieldAsAttribute(bool enabled)
{
  this->ANARIMapper::SetMapFieldAsAttribute(enabled);
  this->UpdateGeometry();
  this->UpdateMaterial();
}

void ANARIMapperPoints::SetANARIColorMap(anari_cpp::Array1D color,
                                         anari_cpp::Array1D opacity,
                                         bool releaseArrays)
{
  this->GetANARISurface();
  auto s = this->Handles->Sampler;
  if (s)
  {
    auto d = this->GetDevice();
    anari_cpp::setParameter(d, s, "image", color);
    anari_cpp::commitParameters(d, s);
  }
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperPoints::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  this->GetANARISurface();
  auto s = this->Handles->Sampler;
  if (s)
  {
    auto d = this->GetDevice();
    auto scale =
      anari_cpp::scaling_matrix(anari_cpp::float3(1.f / (valueRange[1] - valueRange[0])));
    auto translation = anari_cpp::translation_matrix(anari_cpp::float3(-valueRange[0], 0, 0));
    anari_cpp::setParameter(d, s, "inTransform", anari_cpp::mul(scale, translation));
    anari_cpp::commitParameters(d, s);
  }
}

anari_cpp::Geometry ANARIMapperPoints::GetANARIGeometry()
{
  if (this->Handles->Geometry)
    return this->Handles->Geometry;

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "sphere");
  this->ConstructArrays();
  this->UpdateGeometry();

  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperPoints::GetANARISurface()
{
  if (this->Handles->Surface)
    return this->Handles->Surface;

  auto d = this->GetDevice();

  this->Handles->Surface = anari_cpp::newObject<anari_cpp::Surface>(d);

  if (!this->Handles->Material)
  {
    this->Handles->Material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
    anari_cpp::setParameter(d, this->Handles->Material, "name", this->MakeObjectName("material"));
  }

  auto s = anari_cpp::newObject<anari_cpp::Sampler>(d, "image1D");
  this->Handles->Sampler = s;
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 3);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(1.f, 0.f, 0.f, 0.f);
  colors[1] = viskores::Vec4f_32(0.f, 1.f, 0.f, 0.5f);
  colors[2] = viskores::Vec4f_32(0.f, 0.f, 1.f, 1.f);
  anari_cpp::unmap(d, colorArray);
  anari_cpp::setAndReleaseParameter(d, s, "image", colorArray);
  anari_cpp::setParameter(d, s, "filter", "linear");
  anari_cpp::setParameter(d, s, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, s, "name", this->MakeObjectName("colormap"));
  anari_cpp::commitParameters(d, s);

  this->SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));

  this->UpdateMaterial();

  anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
  anari_cpp::setParameter(d, this->Handles->Surface, "geometry", this->GetANARIGeometry());
  anari_cpp::setParameter(d, this->Handles->Surface, "material", this->Handles->Material);
  anari_cpp::commitParameters(d, this->Handles->Surface);

  return this->Handles->Surface;
}

const PointsParameters& ANARIMapperPoints::GetANARIPointsParameters() const
{
  return this->Handles->Parameters;
}

void ANARIMapperPoints::ConstructArrays(bool regenerate)
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
  const auto numPoints = coords.GetNumberOfPoints();

  if (numPoints == 0)
  {
    this->RefreshGroup();
    return;
  }

  auto d = this->GetDevice();
  auto& params = this->Handles->Parameters;
  params.NumPrimitives = static_cast<unsigned int>(numPoints);
  params.Radius = DefaultRadius(coords.GetBounds());

  viskores::cont::ArrayHandle<viskores::Vec3f_32> positions;
  viskores::cont::ArrayCopyShallowIfPossible(coords.GetData(), positions);
  params.Vertex.Position = MakeANARIArray1D(d, positions);

  const auto fields = actor.GetFieldSet();
  for (std::size_t slot = 0; slot < fields.size(); ++slot)
  {
    params.Vertex.Attribute[slot] = MakeAttributeArray(d, fields[slot], numPoints);
    params.Vertex.AttributeName[slot] =
      params.Vertex.Attribute[slot] ? fields[slot].GetName() : std::string{};
  }

  this->PrimaryField = actor.GetPrimaryFieldIndex();

  this->UpdateGeometry();
  this->UpdateMaterial();

  this->Valid = true;

  this->RefreshGroup();
}

void ANARIMapperPoints::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.position");
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
      // Attributes //

      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute0",
                              this->Handles->Parameters.Vertex.Attribute[0]);
      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute1",
                              this->Handles->Parameters.Vertex.Attribute[1]);
      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute2",
                              this->Handles->Parameters.Vertex.Attribute[2]);
      anari_cpp::setParameter(d,
                              this->Handles->Geometry,
                              "vertex.attribute3",
                              this->Handles->Parameters.Vertex.Attribute[3]);

      // Attribute names for USD //

      if (!this->Handles->Parameters.Vertex.AttributeName[0].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute0.name",
                                this->Handles->Parameters.Vertex.AttributeName[0]);
      }
      if (!this->Handles->Parameters.Vertex.AttributeName[1].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute1.name",
                                this->Handles->Parameters.Vertex.AttributeName[1]);
      }
      if (!this->Handles->Parameters.Vertex.AttributeName[2].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute2.name",
                                this->Handles->Parameters.Vertex.AttributeName[2]);
      }
      if (!this->Handles->Parameters.Vertex.AttributeName[3].empty())
      {
        anari_cpp::setParameter(d,
                                this->Handles->Geometry,
                                "usd::attribute3.name",
                                this->Handles->Parameters.Vertex.AttributeName[3]);
      }
    }
  }

  anari_cpp::commitParameters(d, this->Handles->Geometry);
}

void ANARIMapperPoints::UpdateMaterial()
{
  if (!this->Handles->Material)
    return;

  auto d = this->GetDevice();
  auto s = this->Handles->Sampler;
  auto a = this->Handles->Parameters.Vertex.Attribute[PrimaryField];
  if (s && a && this->GetMapFieldAsAttribute())
  {
    anari_cpp::setParameter(d, s, "inAttribute", AnariMaterialInputString(PrimaryField));
    anari_cpp::commitParameters(d, s);
    anari_cpp::setParameter(d, this->Handles->Material, "color", s);
  }
  else
    anari_cpp::setParameter(d, this->Handles->Material, "color", viskores::Vec3f_32(1.f));

  anari_cpp::commitParameters(d, this->Handles->Material);
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
  for (auto& attribute : this->Parameters.Vertex.Attribute)
  {
    anari_cpp::release(this->Device, attribute);
  }
  this->Parameters = PointsParameters{};
}

} // namespace anari
} // namespace interop
} // namespace viskores
