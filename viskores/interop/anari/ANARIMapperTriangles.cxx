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
#include <viskores/cont/ArrayCopy.h>
#include <viskores/filter/vector_analysis/SurfaceNormals.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>
#include <viskores/worklet/WorkletMapField.h>
// std
#include <algorithm>
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

template <typename IndexType>
class ExtractTriangleIndices : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_EXEC void operator()(const viskores::Id4& triangle,
                                viskores::Vec<IndexType, 3>& indices) const
  {
    indices = { static_cast<IndexType>(triangle[1]),
                static_cast<IndexType>(triangle[2]),
                static_cast<IndexType>(triangle[3]) };
  }
};

class ExtractTriangleCellField : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, WholeArrayIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename FieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id4& triangle,
                                const FieldPortalType& field,
                                viskores::Float32& value) const
  {
    value = static_cast<viskores::Float32>(field.Get(triangle[0]));
  }
};

// Helper functions ///////////////////////////////////////////////////////////

static TriangleFieldArrays LowerFields(viskores::cont::ArrayHandle<viskores::Id4> triangles,
                                       const FieldSet& fields)
{
  TriangleFieldArrays retval;

  for (viskores::IdComponent fieldIndex = 0;
       fieldIndex < static_cast<viskores::IdComponent>(fields.size());
       ++fieldIndex)
  {
    const auto& field = fields[static_cast<std::size_t>(fieldIndex)];
    const auto association = field.GetAssociation();
    if (field.GetNumberOfValues() == 0 || field.GetData().GetNumberOfComponentsFlat() != 1 ||
        (association != viskores::cont::Field::Association::Points &&
         association != viskores::cont::Field::Association::Cells))
    {
      continue;
    }

    auto& binding = retval.Fields[static_cast<std::size_t>(fieldIndex)];
    binding.Name = field.GetName();
    binding.Association = association;
    if (association == viskores::cont::Field::Association::Points)
    {
      viskores::cont::ArrayCopyShallowIfPossible(field.GetData(), binding.Values);
    }
    else
    {
      viskores::cont::ArrayHandle<viskores::Float32> cellValues;
      viskores::cont::ArrayCopyShallowIfPossible(field.GetData(), cellValues);
      binding.Values.Allocate(triangles.GetNumberOfValues());
      viskores::worklet::DispatcherMapField<ExtractTriangleCellField>().Invoke(
        triangles, cellValues, binding.Values);
    }
  }

  return retval;
}

static TriangleArrays LowerTriangles(viskores::cont::ArrayHandle<viskores::Id4> triangles,
                                     const viskores::cont::CoordinateSystem& coordinates,
                                     const viskores::cont::ArrayHandle<viskores::Vec3f_32>& normals)
{
  TriangleArrays retval;

  viskores::cont::ArrayCopyShallowIfPossible(coordinates.GetData(), retval.Vertices);
  retval.Normals = normals;

  const auto numberOfVertices = static_cast<viskores::UInt64>(retval.Vertices.GetNumberOfValues());
  const auto maximumUInt32 = (std::numeric_limits<viskores::UInt32>::max)();
  if (numberOfVertices == 0 || numberOfVertices - 1 <= maximumUInt32)
  {
    retval.Indices32.Allocate(triangles.GetNumberOfValues());
    viskores::worklet::DispatcherMapField<ExtractTriangleIndices<viskores::UInt32>>().Invoke(
      triangles, retval.Indices32);
    retval.IndexType = ANARI_UINT32_VEC3;
  }
  else
  {
    retval.Indices64.Allocate(triangles.GetNumberOfValues());
    viskores::worklet::DispatcherMapField<ExtractTriangleIndices<viskores::UInt64>>().Invoke(
      triangles, retval.Indices64);
    retval.IndexType = ANARI_UINT64_VEC3;
  }

  return retval;
}

// ANARIMapperTriangles definitions ///////////////////////////////////////////

ANARIMapperTriangles::ANARIMapperTriangles(anari_cpp::Device device,
                                           const ANARIActor& actor,
                                           const std::string& name,
                                           const viskores::cont::ColorTable& colorTable)
  : ANARIMapper(device, actor, name, colorTable)
{
  this->Handles = std::make_unique<ANARIMapperTriangles::ANARIHandles>();
  this->Handles->Device = device;
  auto& vertexAttributes = this->Handles->Parameters.Vertex.Attribute;
  auto& primitiveAttributes = this->Handles->Parameters.Primitive.Attribute;
  std::fill(vertexAttributes.begin(), vertexAttributes.end(), nullptr);
  std::fill(primitiveAttributes.begin(), primitiveAttributes.end(), nullptr);
  anari_cpp::retain(device, device);
}

ANARIMapperTriangles::ANARIMapperTriangles(ANARIMapperTriangles&&) = default;

ANARIMapperTriangles::~ANARIMapperTriangles()
{
  // ensure ANARI handles are released before host memory goes away
  this->Handles.reset();
}

void ANARIMapperTriangles::SetCalculateNormals(bool enabled)
{
  if (this->CalculateNormals == enabled)
  {
    return;
  }

  this->CalculateNormals = enabled;
  this->MarkDirty(DirtyCategory::Data);
  this->UpdateMaterializedObjects();
}

anari_cpp::Geometry ANARIMapperTriangles::GetANARIGeometry()
{
  if (this->Handles->Geometry)
  {
    this->ConstructArrays();
    this->UpdateMaterializedObjects();
    return this->Handles->Geometry;
  }

  auto d = this->GetDevice();
  this->Handles->Geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "triangle");
  this->ConstructArrays();
  this->UpdateGeometry();
  return this->Handles->Geometry;
}

anari_cpp::Surface ANARIMapperTriangles::GetANARISurface()
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

bool ANARIMapperTriangles::NeedToGenerateData() const
{
  const bool haveNormals = this->Handles->Parameters.Vertex.Normal != nullptr;
  return this->IsDirty(DirtyCategory::Data) || this->IsDirty(DirtyCategory::Topology) ||
    this->CalculateNormals != haveNormals;
}

void ANARIMapperTriangles::ConstructArrays()
{
  if (!this->NeedToGenerateData())
  {
    return;
  }

  this->Valid = false;

  this->Handles->ReleaseArrays();

  const auto& actor = this->GetActor();
  const auto& cells = actor.GetCellSet();

  if (cells.GetNumberOfCells() == 0)
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

  viskores::rendering::raytracing::TriangleExtractor triExtractor;
  triExtractor.ExtractCells(cells);

  if (triExtractor.GetNumberOfTriangles() == 0)
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

  viskores::cont::ArrayHandle<viskores::Vec3f_32> inNormals;

  if (this->CalculateNormals)
  {
    viskores::filter::vector_analysis::SurfaceNormals normalsFilter;
    normalsFilter.SetOutputFieldName("Normals");
    auto dataset = normalsFilter.Execute(actor.MakeDataSet());
    auto field = dataset.GetField("Normals");
    auto fieldArray = field.GetData();
    viskores::cont::ArrayCopyShallowIfPossible(fieldArray, inNormals);
  }

  auto tris = triExtractor.GetTriangles();

  auto arrays = LowerTriangles(tris, actor.GetCoordinateSystem(), inNormals);
  auto fieldArrays = LowerFields(tris, actor.GetFieldSet());

  this->PrimaryField = actor.GetPrimaryFieldIndex();

  auto d = this->GetDevice();
  this->Handles->Parameters.NumPrimitives = static_cast<viskores::UInt64>(tris.GetNumberOfValues());

  auto* positionPointer = static_cast<const viskores::Vec3f_32*>(
    arrays.Vertices.GetBuffers()[0].ReadPointerHost(*arrays.Token));
  this->Handles->Parameters.Vertex.Position =
    anari_cpp::newArray1D(d,
                          positionPointer,
                          NoopANARIDeleter,
                          nullptr,
                          static_cast<viskores::UInt64>(arrays.Vertices.GetNumberOfValues()));

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

    auto* attributePointer = static_cast<const viskores::Float32*>(
      binding.Values.GetBuffers()[0].ReadPointerHost(*fieldArrays.Token));
    auto attribute =
      anari_cpp::newArray1D(d,
                            attributePointer,
                            NoopANARIDeleter,
                            nullptr,
                            static_cast<viskores::UInt64>(binding.Values.GetNumberOfValues()));
    if (binding.Association == viskores::cont::Field::Association::Points)
    {
      this->Handles->Parameters.Vertex.Attribute[index] = attribute;
      this->Handles->Parameters.Vertex.AttributeName[index] = binding.Name;
    }
    else
    {
      this->Handles->Parameters.Primitive.Attribute[index] = attribute;
      this->Handles->Parameters.Primitive.AttributeName[index] = binding.Name;
    }
  }

  if (arrays.Normals.GetNumberOfValues() != 0)
  {
    auto* normalPointer = static_cast<const viskores::Vec3f_32*>(
      arrays.Normals.GetBuffers()[0].ReadPointerHost(*arrays.Token));
    this->Handles->Parameters.Vertex.Normal =
      anari_cpp::newArray1D(d,
                            normalPointer,
                            NoopANARIDeleter,
                            nullptr,
                            static_cast<viskores::UInt64>(arrays.Normals.GetNumberOfValues()));
  }

  // The USD device requires explicit indices, so preserve them even though
  // ANARI triangle geometry also permits implicit identity connectivity.
  const void* indexPointer = nullptr;
  if (arrays.IndexType == ANARI_UINT32_VEC3)
  {
    indexPointer = arrays.Indices32.GetBuffers()[0].ReadPointerHost(*arrays.Token);
  }
  else
  {
    indexPointer = arrays.Indices64.GetBuffers()[0].ReadPointerHost(*arrays.Token);
  }
  this->Handles->Parameters.Primitive.Index =
    anari_cpp::newArray1D(d,
                          indexPointer,
                          NoopANARIDeleter,
                          nullptr,
                          arrays.IndexType,
                          this->Handles->Parameters.NumPrimitives);

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

void ANARIMapperTriangles::UpdateGeometry()
{
  if (!this->Handles->Geometry)
    return;

  auto d = this->GetDevice();

  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.position");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "vertex.normal");
  anari_cpp::unsetParameter(d, this->Handles->Geometry, "primitive.index");
  for (viskores::IdComponent fieldIndex = 0; fieldIndex <
       static_cast<viskores::IdComponent>(this->Handles->Parameters.Vertex.Attribute.size());
       ++fieldIndex)
  {
    const auto attributeName = std::string(AnariMaterialInputString(fieldIndex));
    const auto vertexParameter = std::string("vertex.") + attributeName;
    const auto primitiveParameter = std::string("primitive.") + attributeName;
    const auto usdNameParameter = std::string("usd::") + attributeName + ".name";
    anari_cpp::unsetParameter(d, this->Handles->Geometry, vertexParameter.c_str());
    anari_cpp::unsetParameter(d, this->Handles->Geometry, primitiveParameter.c_str());
    anari_cpp::unsetParameter(d, this->Handles->Geometry, usdNameParameter.c_str());
  }

  anari_cpp::setParameter(d, this->Handles->Geometry, "name", this->MakeObjectName("geometry"));

  if (this->Handles->Parameters.Vertex.Position)
  {
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "vertex.position", this->Handles->Parameters.Vertex.Position);
    if (this->GetMapFieldAsAttribute())
    {
      for (viskores::IdComponent fieldIndex = 0; fieldIndex <
           static_cast<viskores::IdComponent>(this->Handles->Parameters.Vertex.Attribute.size());
           ++fieldIndex)
      {
        const auto index = static_cast<std::size_t>(fieldIndex);
        const auto attributeName = std::string(AnariMaterialInputString(fieldIndex));
        const auto vertexParameter = std::string("vertex.") + attributeName;
        const auto primitiveParameter = std::string("primitive.") + attributeName;
        const auto usdNameParameter = std::string("usd::") + attributeName + ".name";
        const auto vertexAttribute = this->Handles->Parameters.Vertex.Attribute[index];
        const auto primitiveAttribute = this->Handles->Parameters.Primitive.Attribute[index];
        if (vertexAttribute)
        {
          anari_cpp::setParameter(
            d, this->Handles->Geometry, vertexParameter.c_str(), vertexAttribute);
        }
        if (primitiveAttribute)
        {
          anari_cpp::setParameter(
            d, this->Handles->Geometry, primitiveParameter.c_str(), primitiveAttribute);
        }

        const auto& vertexName = this->Handles->Parameters.Vertex.AttributeName[index];
        const auto& primitiveName = this->Handles->Parameters.Primitive.AttributeName[index];
        const auto& fieldName = vertexName.empty() ? primitiveName : vertexName;
        if (!fieldName.empty())
        {
          anari_cpp::setParameter(d, this->Handles->Geometry, usdNameParameter.c_str(), fieldName);
        }
      }
    }

    if (this->Handles->Parameters.Vertex.Normal)
    {
      anari_cpp::setParameter(
        d, this->Handles->Geometry, "vertex.normal", this->Handles->Parameters.Vertex.Normal);
    }
    anari_cpp::setParameter(
      d, this->Handles->Geometry, "primitive.index", this->Handles->Parameters.Primitive.Index);
  }

  anari_cpp::commitParameters(d, this->Handles->Geometry);
  this->ClearDirty(DirtyCategory::Attributes);
}

void ANARIMapperTriangles::UpdateMaterial()
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
    const auto index = static_cast<std::size_t>(this->PrimaryField);
    havePrimaryField = this->Handles->Parameters.Vertex.Attribute[index] != nullptr ||
      this->Handles->Parameters.Primitive.Attribute[index] != nullptr;
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

void ANARIMapperTriangles::UpdateMaterializedObjects()
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
  anari_cpp::release(this->Device, this->Parameters.Vertex.Position);
  anari_cpp::release(this->Device, this->Parameters.Vertex.Normal);
  for (std::size_t fieldIndex = 0; fieldIndex < this->Parameters.Vertex.Attribute.size();
       ++fieldIndex)
  {
    anari_cpp::release(this->Device, this->Parameters.Vertex.Attribute[fieldIndex]);
    anari_cpp::release(this->Device, this->Parameters.Primitive.Attribute[fieldIndex]);
    this->Parameters.Vertex.Attribute[fieldIndex] = nullptr;
    this->Parameters.Primitive.Attribute[fieldIndex] = nullptr;
    this->Parameters.Vertex.AttributeName[fieldIndex].clear();
    this->Parameters.Primitive.AttributeName[fieldIndex].clear();
  }
  anari_cpp::release(this->Device, this->Parameters.Primitive.Index);
  this->Parameters.Vertex.Position = nullptr;
  this->Parameters.Vertex.Normal = nullptr;
  this->Parameters.Primitive.Index = nullptr;
  this->Parameters.NumPrimitives = 0;
}

} // namespace anari
} // namespace interop
} // namespace viskores
