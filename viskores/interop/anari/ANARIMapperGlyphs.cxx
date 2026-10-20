//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores
#include "viskores/rendering/raytracing/SphereExtractor.h"
#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Logging.h>
#include <viskores/filter/field_conversion/CellAverage.h>
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
#include <viskores/worklet/WorkletMapField.h>
// std
#include <limits>

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
    const auto direction = static_cast<viskores::Vec3f_64>(gradient);
    const viskores::Vec3f_32 point = points.Get(idx);

    // A zero or non-finite vector gets a zero-size glyph at its point instead
    // of NaN vertices. Scaling by the largest component first keeps the
    // squared length from overflowing or underflowing.
    const viskores::Float64 largest =
      viskores::Max(viskores::Abs(direction[0]),
                    viskores::Max(viskores::Abs(direction[1]), viskores::Abs(direction[2])));
    if (!(largest > 0.) || !viskores::IsFinite(largest))
    {
      WriteEmptyGlyph(idx, point, vertices, radii);
      return;
    }
    const auto scaled = direction * (1. / largest);
    const viskores::Float64 scaledLengthSquared = viskores::Dot(scaled, scaled);
    if (!viskores::IsFinite(scaledLengthSquared))
    {
      WriteEmptyGlyph(idx, point, vertices, radii);
      return;
    }

    const auto unitDirection =
      static_cast<viskores::Vec3f_32>(scaled * (1. / viskores::Sqrt(scaledLengthSquared)));
    const auto tip = point + unitDirection * this->SizeFactor;
    const auto tail = point + unitDirection * -this->SizeFactor;
    if (this->Offset)
    {
      vertices.Set(4 * idx + 0, point);
      vertices.Set(4 * idx + 1, tail);
      vertices.Set(4 * idx + 2, tail);
      vertices.Set(4 * idx + 3, tail - (this->SizeFactor * unitDirection));
    }
    else
    {
      vertices.Set(4 * idx + 0, tip);
      vertices.Set(4 * idx + 1, point);
      vertices.Set(4 * idx + 2, point);
      vertices.Set(4 * idx + 3, tail);
    }
    radii.Set(4 * idx + 0, this->SizeFactor / 8);
    radii.Set(4 * idx + 1, this->SizeFactor / 8);
    radii.Set(4 * idx + 2, this->SizeFactor / 4);
    radii.Set(4 * idx + 3, 0.f);
  }

private:
  template <typename OutVertexPortalType, typename OutRadiusPortalType>
  VISKORES_EXEC static void WriteEmptyGlyph(const viskores::Id idx,
                                            const viskores::Vec3f_32& point,
                                            OutVertexPortalType& vertices,
                                            OutRadiusPortalType& radii)
  {
    for (viskores::Id vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
    {
      vertices.Set(4 * idx + vertexIndex, point);
      radii.Set(4 * idx + vertexIndex, 0.f);
    }
  }
};

// Helper functions ///////////////////////////////////////////////////////////

namespace
{

struct GlyphArrays
{
  viskores::cont::ArrayHandle<viskores::Vec3f_32> Vertices;
  viskores::cont::ArrayHandle<viskores::Float32> Radii;
};

} // namespace

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
  this->Handles = std::make_shared<ANARIMapperGlyphs::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperGlyphs::~ANARIMapperGlyphs() = default;

void ANARIMapperGlyphs::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
}

void ANARIMapperGlyphs::SetOffsetGlyphs(bool enabled)
{
  if (enabled == this->Offset)
  {
    return;
  }

  this->Offset = enabled;
  this->Current = false;
  if (this->Handles->Geometry)
  {
    this->ConstructArrays(true);
  }
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

const GlyphsParameters& ANARIMapperGlyphs::GetANARIGlyphsParameters() const
{
  return this->Handles->Parameters;
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

  using NumPrimitivesType = decltype(this->Handles->Parameters.NumPrimitives);
  const char* unsupportedReason = nullptr;
  if (numGlyphs > 0)
  {
    if (static_cast<viskores::UInt64>(numGlyphs) > std::numeric_limits<NumPrimitivesType>::max())
    {
      unsupportedReason = "its field has too many values to count as glyphs";
    }
    else if (field.GetData().GetNumberOfComponentsFlat() != 3)
    {
      unsupportedReason = "its field does not have 3 components";
    }
    else if (!field.IsPointField() && !field.IsCellField())
    {
      unsupportedReason = "its field is not a point or cell field";
    }
    else if (field.IsPointField() && numGlyphs != coords.GetNumberOfValues())
    {
      unsupportedReason = "its point field does not have one value per point";
    }
    else if (field.IsCellField() && numGlyphs != cells.GetNumberOfCells())
    {
      unsupportedReason = "its cell field does not have one value per cell";
    }
  }

  if (numGlyphs == 0 || unsupportedReason)
  {
    if (unsupportedReason)
    {
      VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                     "ANARIMapperGlyphs '" << this->GetName()
                                           << "' cannot map its actor: " << unsupportedReason);
    }
    this->UpdateGeometry();
    this->RefreshGroup();
    return;
  }

  viskores::Bounds coordBounds = coords.GetBounds();
  viskores::Float64 lx = coordBounds.X.Length();
  viskores::Float64 ly = coordBounds.Y.Length();
  viskores::Float64 lz = coordBounds.Z.Length();
  viskores::Float64 mag = viskores::Sqrt(lx * lx + ly * ly + lz * lz);
  constexpr viskores::Float64 heuristic = 300.;
  auto glyphSize = static_cast<viskores::Float32>(mag / heuristic);
  // Zero-size bounds, such as a single point, would give invisible glyphs.
  if (!(glyphSize > 0.f) || !viskores::IsFinite(glyphSize))
  {
    glyphSize = 1.f;
  }

  auto arrays = MakeGlyphs(field, cells, coords, glyphSize, Offset);

  auto d = this->GetDevice();
  this->Handles->Parameters.Vertex.Position = MakeANARIArray1D(d, arrays.Vertices);
  this->Handles->Parameters.Vertex.Radius = MakeANARIArray1D(d, arrays.Radii);
  this->Handles->Parameters.NumPrimitives = static_cast<NumPrimitivesType>(numGlyphs);

  this->UpdateGeometry();

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
  this->Parameters.NumPrimitives = 0;
}

} // namespace anari
} // namespace interop
} // namespace viskores
