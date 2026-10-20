//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Logging.h>
#include <viskores/filter/vector_analysis/SurfaceNormals.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>
#include <viskores/worklet/WorkletMapField.h>
// std
#include <cstdint>
#include <limits>
// anari
#include <anari/anari_cpp/ext/linalg.h>

namespace viskores
{
namespace interop
{
namespace anari
{

// Worklets ///////////////////////////////////////////////////////////////////

namespace
{

/// Copy the point IDs of each triangle, components 1-3 of the `Id4` that
/// `TriangleExtractor` produces, into a `primitive.index` vector.
class ExtractTriangleIndices : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn triangles, FieldOut indices);
  using ExecutionSignature = void(_1, _2);

  template <typename IndexVecType>
  VISKORES_EXEC void operator()(const viskores::Id4& triangle, IndexVecType& index) const
  {
    using ComponentType = typename IndexVecType::ComponentType;
    index = IndexVecType(static_cast<ComponentType>(triangle[1]),
                         static_cast<ComponentType>(triangle[2]),
                         static_cast<ComponentType>(triangle[3]));
  }
};

/// Give each triangle the value of its cell, which `TriangleExtractor` stores
/// in component 0 of the triangle's `Id4`.
class GatherTriangleCellValues : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn triangles, WholeArrayIn cellValues, FieldOut values);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename CellValuesPortalType>
  VISKORES_EXEC void operator()(const viskores::Id4& triangle,
                                const CellValuesPortalType& cellValues,
                                viskores::Float32& value) const
  {
    value = cellValues.Get(triangle[0]);
  }
};

// Helper functions ///////////////////////////////////////////////////////////

/// Make the `primitive.index` array, with 32-bit indices unless the points
/// need more.
anari_cpp::Array1D MakeIndexArray(anari_cpp::Device d,
                                  const viskores::cont::ArrayHandle<viskores::Id4>& triangles,
                                  viskores::Id numPoints)
{
  viskores::cont::Invoker invoke;
  if (static_cast<viskores::UInt64>(numPoints) <= std::numeric_limits<viskores::UInt32>::max())
  {
    viskores::cont::ArrayHandle<viskores::Vec3ui_32> indices;
    invoke(ExtractTriangleIndices{}, triangles, indices);
    return MakeANARIArray1D(d, indices);
  }

  VISKORES_LOG_S(viskores::cont::LogLevel::Warn,
                 "ANARIMapperTriangles: " << numPoints
                                          << " points do not fit 32-bit indices; sending "
                                             "ANARI_UINT64_VEC3 primitive.index, which some "
                                             "devices do not support.");
  viskores::cont::ArrayHandle<viskores::Vec<viskores::UInt64, 3>> indices;
  invoke(ExtractTriangleIndices{}, triangles, indices);
  return MakeANARIArray1D(
    d, indices, ANARI_UINT64_VEC3, static_cast<std::uint64_t>(indices.GetNumberOfValues()));
}

constexpr const char* VertexAttributeParameters[4] = { "vertex.attribute0",
                                                       "vertex.attribute1",
                                                       "vertex.attribute2",
                                                       "vertex.attribute3" };
constexpr const char* PrimitiveAttributeParameters[4] = { "primitive.attribute0",
                                                          "primitive.attribute1",
                                                          "primitive.attribute2",
                                                          "primitive.attribute3" };
constexpr const char* USDAttributeNameParameters[4] = { "usd::attribute0.name",
                                                        "usd::attribute1.name",
                                                        "usd::attribute2.name",
                                                        "usd::attribute3.name" };

} // namespace

// ANARIMapperTriangles definitions ///////////////////////////////////////////

ANARIMapperTriangles::ANARIMapperTriangles(anari_cpp::Device device,
                                           const ANARIActor& actor,
                                           const std::string& name,
                                           const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_shared<ANARIMapperTriangles::ANARIHandles>();
  this->Handles->Device = device;
  anari_cpp::retain(device, device);
}

ANARIMapperTriangles::~ANARIMapperTriangles() = default;

void ANARIMapperTriangles::SetActor(const ANARIActor& actor)
{
  this->ANARIMapper::SetActor(actor);
  this->ConstructArrays(true);
  this->UpdateMaterial();
}

void ANARIMapperTriangles::SetMapFieldAsAttribute(bool enabled)
{
  this->ANARIMapper::SetMapFieldAsAttribute(enabled);
  this->UpdateGeometry();
  this->UpdateMaterial();
}

void ANARIMapperTriangles::SetANARIColorMap(anari_cpp::Array1D color,
                                            anari_cpp::Array1D opacity,
                                            bool releaseArrays)
{
  this->GetANARISurface();
  auto d = this->GetDevice();
  auto s = this->Handles->Sampler;
  anari_cpp::setParameter(d, s, "image", color);
  anari_cpp::commitParameters(d, s);
  this->ANARIMapper::SetANARIColorMap(color, opacity, releaseArrays);
}

void ANARIMapperTriangles::SetANARIColorMapValueRange(const viskores::Vec2f_32& valueRange)
{
  this->GetANARISurface();
  auto s = this->Handles->Sampler;
  auto d = this->GetDevice();
  auto scale = anari_cpp::scaling_matrix(anari_cpp::float3(1.f / (valueRange[1] - valueRange[0])));
  auto translation = anari_cpp::translation_matrix(anari_cpp::float3(-valueRange[0], 0, 0));
  anari_cpp::setParameter(d, s, "inTransform", anari_cpp::mul(scale, translation));
  anari_cpp::setParameter(d, s, "outTransform", anari_cpp::math::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, s, "inOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::setParameter(d, s, "outOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::commitParameters(d, s);
}

void ANARIMapperTriangles::SetCalculateNormals(bool enabled)
{
  if (enabled == this->CalculateNormals)
    return;

  this->CalculateNormals = enabled;
  this->Current = false;
  if (this->Handles->Geometry)
    this->ConstructArrays(true);
}

anari_cpp::Geometry ANARIMapperTriangles::GetANARIGeometry()
{
  if (this->Handles->Geometry)
    return this->Handles->Geometry;

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "triangle");
  this->ConstructArrays();
  this->UpdateGeometry();
  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperTriangles::GetANARISurface()
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
  anari_cpp::setParameter(d, s, "filter", "nearest");
  anari_cpp::setParameter(d, s, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, s, "name", this->MakeObjectName("colormap"));
  anari_cpp::setParameter(d, s, "inTransform", anari_cpp::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, s, "outTransform", anari_cpp::math::mat4(anari_cpp::identity));
  anari_cpp::setParameter(d, s, "inOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::setParameter(d, s, "outOffset", viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f));
  anari_cpp::commitParameters(d, s);

  this->SetANARIColorMapValueRange(viskores::Vec2f_32(0.f, 10.f));

  this->UpdateMaterial();

  anari_cpp::setParameter(d, this->Handles->Surface, "name", this->MakeObjectName("surface"));
  anari_cpp::setParameter(d, this->Handles->Surface, "geometry", this->GetANARIGeometry());
  anari_cpp::setParameter(d, this->Handles->Surface, "material", this->Handles->Material);
  anari_cpp::commitParameters(d, this->Handles->Surface);

  return this->Handles->Surface;
}

const TrianglesParameters& ANARIMapperTriangles::GetANARITrianglesParameters() const
{
  return this->Handles->Parameters;
}

void ANARIMapperTriangles::ConstructArrays(bool regenerate)
{
  if (regenerate)
    this->Current = false;

  if (this->Current)
    return;

  this->Current = true;
  this->Valid = false;

  this->Handles->ReleaseArrays();

  const auto& actor = this->GetActor();
  const auto& cells = actor.GetCellSet();

  if (cells.GetNumberOfCells() == 0)
  {
    this->RefreshGroup();
    return;
  }

  viskores::rendering::raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cells);

  if (triExtractor.GetNumberOfTriangles() == 0)
  {
    this->RefreshGroup();
    return;
  }

  auto tris = triExtractor.GetTriangles();
  const auto numTris = tris.GetNumberOfValues();
  const auto numPoints = actor.GetCoordinateSystem().GetNumberOfValues();
  const auto numCells = cells.GetNumberOfCells();

  auto d = this->GetDevice();
  auto& params = this->Handles->Parameters;
  params.NumPrimitives = static_cast<unsigned int>(numTris);

  viskores::cont::ArrayHandle<viskores::Vec3f_32> positions;
  viskores::cont::ArrayCopyShallowIfPossible(actor.GetCoordinateSystem().GetData(), positions);
  params.Vertex.Position = MakeANARIArray1D(d, positions);

  // NOTE: the USD device requires indices, even though they are optional in ANARI.
  params.Primitive.Index = MakeIndexArray(d, tris, numPoints);

  if (this->CalculateNormals)
  {
    viskores::filter::vector_analysis::SurfaceNormals normalsFilter;
    normalsFilter.SetOutputFieldName("Normals");
    auto dataset = normalsFilter.Execute(actor.MakeDataSet());
    viskores::cont::ArrayHandle<viskores::Vec3f_32> normals;
    viskores::cont::ArrayCopyShallowIfPossible(dataset.GetField("Normals").GetData(), normals);
    if (normals.GetNumberOfValues() == numPoints)
    {
      params.Vertex.Normal = MakeANARIArray1D(d, normals);
    }
  }

  // Point fields are shared as they are. Cell fields are gathered into one
  // value per triangle. Fields with more than one component, another
  // association, or the wrong number of values are skipped.
  const auto fields = actor.GetFieldSet();
  for (std::size_t slot = 0; slot < fields.size(); ++slot)
  {
    const auto& field = fields[slot];
    const auto numValues = field.GetNumberOfValues();
    if (numValues == 0 || field.GetData().GetNumberOfComponentsFlat() != 1)
    {
      continue;
    }

    const bool isPointField = field.IsPointField() && numValues == numPoints;
    const bool isCellField = field.IsCellField() && numValues == numCells;
    if (!isPointField && !isCellField)
    {
      continue;
    }

    viskores::cont::ArrayHandle<viskores::Float32> values;
    viskores::cont::ArrayCopyShallowIfPossible(field.GetData(), values);

    if (isPointField)
    {
      params.Vertex.Attribute[slot] = MakeANARIArray1D(d, values);
    }
    else
    {
      viskores::cont::ArrayHandle<viskores::Float32> triangleValues;
      viskores::cont::Invoker invoke;
      invoke(GatherTriangleCellValues{}, tris, values, triangleValues);
      params.Primitive.Attribute[slot] = MakeANARIArray1D(d, triangleValues);
    }
    params.AttributeName[slot] = field.GetName();
  }

  this->PrimaryField = actor.GetPrimaryFieldIndex();

  this->UpdateGeometry();
  this->UpdateMaterial();

  this->Valid = true;

  this->RefreshGroup();
}

void ANARIMapperTriangles::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();
  auto geometry = this->Handles->Geometry;
  const auto& params = this->Handles->Parameters;

  anari_cpp::unsetParameter(d, geometry, "vertex.position");
  anari_cpp::unsetParameter(d, geometry, "vertex.normal");
  anari_cpp::unsetParameter(d, geometry, "primitive.index");
  for (std::size_t slot = 0; slot < params.AttributeName.size(); ++slot)
  {
    anari_cpp::unsetParameter(d, geometry, VertexAttributeParameters[slot]);
    anari_cpp::unsetParameter(d, geometry, PrimitiveAttributeParameters[slot]);
    anari_cpp::unsetParameter(d, geometry, USDAttributeNameParameters[slot]);
  }

  anari_cpp::setParameter(d, geometry, "name", this->MakeObjectName("geometry"));

  if (params.Vertex.Position)
  {
    anari_cpp::setParameter(d, geometry, "vertex.position", params.Vertex.Position);
    anari_cpp::setParameter(d, geometry, "primitive.index", params.Primitive.Index);

    if (this->CalculateNormals && params.Vertex.Normal)
    {
      anari_cpp::setParameter(d, geometry, "vertex.normal", params.Vertex.Normal);
    }

    if (this->GetMapFieldAsAttribute())
    {
      for (std::size_t slot = 0; slot < params.AttributeName.size(); ++slot)
      {
        if (params.Vertex.Attribute[slot])
        {
          anari_cpp::setParameter(
            d, geometry, VertexAttributeParameters[slot], params.Vertex.Attribute[slot]);
        }
        if (params.Primitive.Attribute[slot])
        {
          anari_cpp::setParameter(
            d, geometry, PrimitiveAttributeParameters[slot], params.Primitive.Attribute[slot]);
        }
        if (!params.AttributeName[slot].empty())
        {
          anari_cpp::setParameter(
            d, geometry, USDAttributeNameParameters[slot], params.AttributeName[slot]);
        }
      }
    }
  }

  anari_cpp::commitParameters(d, geometry);
}

void ANARIMapperTriangles::UpdateMaterial()
{
  if (!this->Handles->Material)
    return;

  auto d = this->GetDevice();
  auto s = this->Handles->Sampler;
  const auto& params = this->Handles->Parameters;
  const bool haveAttribute =
    params.Vertex.Attribute[PrimaryField] || params.Primitive.Attribute[PrimaryField];
  if (s && haveAttribute && this->GetMapFieldAsAttribute())
  {
    anari_cpp::setParameter(d, s, "inAttribute", AnariMaterialInputString(PrimaryField));
    anari_cpp::commitParameters(d, s);
    anari_cpp::setParameter(d, this->Handles->Material, "color", s);
  }
  else
    anari_cpp::setParameter(d, this->Handles->Material, "color", viskores::Vec3f_32(1.f));

  anari_cpp::commitParameters(d, this->Handles->Material);
}

ANARIMapperTriangles::ANARIHandles::~ANARIHandles()
{
  this->ReleaseArrays();
  anari_cpp::release(this->Device, this->Surface);
  anari_cpp::release(this->Device, this->Material);
  anari_cpp::release(this->Device, this->Sampler);
  anari_cpp::release(this->Device, this->Geometry);
  anari_cpp::release(this->Device, this->Device);
}

void ANARIMapperTriangles::ANARIHandles::ReleaseArrays()
{
  auto& params = this->Parameters;
  anari_cpp::release(this->Device, params.Vertex.Position);
  anari_cpp::release(this->Device, params.Vertex.Normal);
  anari_cpp::release(this->Device, params.Primitive.Index);
  params.Vertex.Position = nullptr;
  params.Vertex.Normal = nullptr;
  params.Primitive.Index = nullptr;
  for (std::size_t slot = 0; slot < params.AttributeName.size(); ++slot)
  {
    anari_cpp::release(this->Device, params.Vertex.Attribute[slot]);
    anari_cpp::release(this->Device, params.Primitive.Attribute[slot]);
    params.Vertex.Attribute[slot] = nullptr;
    params.Primitive.Attribute[slot] = nullptr;
    params.AttributeName[slot].clear();
  }
  params.NumPrimitives = 0;
}

} // namespace anari
} // namespace interop
} // namespace viskores
