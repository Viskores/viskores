//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Cone.h"
// Viskores
#include <viskores/CellShape.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/TriangleExtractor.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>
// std
#include <cmath>
#include <vector>

namespace viskores_device
{
namespace
{

constexpr viskores::IdComponent ConeSegments = 24;
constexpr viskores::Float32 TwoPi = 6.2831853071795864769f;
constexpr viskores::Float32 Epsilon = 1.e-6f;

void AddTriangle(std::vector<viskores::Vec3f_32>& points,
                 std::vector<viskores::Id>& connectivity,
                 std::vector<viskores::Id>& primitiveIds,
                 std::vector<viskores::Id>& vertexIds,
                 viskores::Id primitiveId,
                 const viskores::Vec3f_32& a,
                 viskores::Id aVertexId,
                 const viskores::Vec3f_32& b,
                 viskores::Id bVertexId,
                 const viskores::Vec3f_32& c,
                 viskores::Id cVertexId)
{
  const viskores::Id firstPoint = static_cast<viskores::Id>(points.size());
  points.push_back(a);
  points.push_back(b);
  points.push_back(c);
  connectivity.push_back(firstPoint);
  connectivity.push_back(firstPoint + 1);
  connectivity.push_back(firstPoint + 2);
  primitiveIds.push_back(primitiveId);
  vertexIds.push_back(aVertexId);
  vertexIds.push_back(bVertexId);
  vertexIds.push_back(cVertexId);
}

struct AddExpandedField
{
  viskores::cont::DataSet* DataSet;
  std::string FieldName;
  viskores::cont::Field::Association Association;
  viskores::cont::ArrayHandle<viskores::Id> SourceIds;

  template <typename FieldArrayType>
  void operator()(const FieldArrayType& fieldArray) const
  {
    viskores::cont::ArrayHandle<typename FieldArrayType::ValueType> expandedField;
    viskores::cont::ArrayCopy(
      viskores::cont::make_ArrayHandlePermutation(this->SourceIds, fieldArray), expandedField);
    this->DataSet->AddField(this->FieldName, this->Association, expandedField);
  }
};

const char* AttributeName(viskores::IdComponent attribute)
{
  switch (attribute)
  {
    case 0:
      return "color";
    case 1:
      return "attribute0";
    case 2:
      return "attribute1";
    case 3:
      return "attribute2";
    case 4:
      return "attribute3";
    default:
      return "";
  }
}

template <typename RadiusPortalType>
viskores::Float32 GetRadius(const RadiusPortalType& radii,
                            viskores::Id numberOfRadii,
                            viskores::Id numberOfCones,
                            viskores::Id vertexId,
                            viskores::Id coneId,
                            viskores::Id endpointId,
                            viskores::Float32 globalRadius)
{
  if (vertexId >= 0 && vertexId < numberOfRadii)
    return radii.Get(vertexId);
  if (2 * coneId + endpointId < numberOfRadii)
    return radii.Get(2 * coneId + endpointId);
  if (numberOfRadii >= numberOfCones && coneId < numberOfRadii)
    return radii.Get(coneId);

  return globalRadius;
}

template <typename CapPortalType>
viskores::UInt8 GetCapMask(const CapPortalType& caps,
                           viskores::Id numberOfCaps,
                           viskores::Id numberOfVertices,
                           viskores::Id numberOfCones,
                           viskores::Id firstVertexId,
                           viskores::Id secondVertexId,
                           viskores::Id coneId,
                           viskores::UInt8 globalCapMask)
{
  if (numberOfCaps >= numberOfVertices)
  {
    viskores::UInt8 capMask = 0;
    if (firstVertexId >= 0 && firstVertexId < numberOfCaps && caps.Get(firstVertexId) != 0)
      capMask |= 1;
    if (secondVertexId >= 0 && secondVertexId < numberOfCaps && caps.Get(secondVertexId) != 0)
      capMask |= 2;
    return capMask;
  }
  if (numberOfCaps >= numberOfCones * 2)
  {
    viskores::UInt8 capMask = 0;
    if (caps.Get(2 * coneId) != 0)
      capMask |= 1;
    if (caps.Get(2 * coneId + 1) != 0)
      capMask |= 2;
    return capMask;
  }
  if (numberOfCaps >= numberOfCones && coneId < numberOfCaps)
    return caps.Get(coneId) != 0 ? viskores::UInt8(3) : viskores::UInt8(0);

  return globalCapMask;
}

} // anonymous namespace

Cone::Cone(ViskoresDeviceGlobalState* s)
  : Geometry(s)
  , m_index(this)
  , m_vertexRadius(this)
  , m_vertexCaps(this)
{
  this->m_vertexAttributes.setAttributes(
    this, { "position", "color", "attribute0", "attribute1", "attribute2", "attribute3" });
  this->m_vertexAttributes.setAnariAssociation("vertex");
  this->m_vertexAttributes.setViskoresAssociation(viskores::cont::Field::Association::Points);
}

void Cone::commitParameters()
{
  this->Geometry::commitParameters();

  this->m_index = getParamObject<Array1D>("primitive.index");
  this->m_vertexRadius = getParamObject<Array1D>("vertex.radius");
  this->m_vertexCaps = getParamObject<Array1D>("vertex.cap");
  this->m_globalRadius = getParam<float>("radius", 1.f);
  this->m_globalCapMask = this->ParseCaps(this->getParamString("caps", "none"));
  this->m_vertexAttributes.commitParameters();
}

void Cone::finalize()
{
  this->m_dataSet = viskores::cont::DataSet{};

  helium::ChangeObserverPtr<Array1D>& positionArray = this->m_vertexAttributes.getParam("position");
  if (!positionArray)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "'cone' geometry missing 'vertex.position' parameter");
    return;
  }

  viskores::cont::ArrayHandle<viskores::Vec3f_32> positions;
  viskores::cont::ArrayCopyShallowIfPossible(positionArray->dataAsViskoresArray(), positions);

  viskores::cont::ArrayHandle<viskores::Float32> radii;
  if (this->m_vertexRadius)
  {
    viskores::cont::ArrayCopyShallowIfPossible(this->m_vertexRadius->dataAsViskoresArray(), radii);
  }
  else
  {
    radii.AllocateAndFill(positions.GetNumberOfValues(), this->m_globalRadius);
  }

  viskores::cont::ArrayHandle<viskores::UInt8> caps;
  if (this->m_vertexCaps)
    viskores::cont::ArrayCopyShallowIfPossible(this->m_vertexCaps->dataAsViskoresArray(), caps);

  viskores::cont::ArrayHandleRuntimeVec<viskores::Id> indexArray(2);
  viskores::cont::ArrayHandle<viskores::Id> indexComponents;
  if (this->m_index)
  {
    viskores::cont::ArrayCopyShallowIfPossible(this->m_index->dataAsViskoresArray(), indexArray);
    indexComponents = indexArray.GetComponentsArray();
  }

  const viskores::Id numberOfVertices = positions.GetNumberOfValues();
  const viskores::Id numberOfCones =
    this->m_index ? indexComponents.GetNumberOfValues() / 2 : numberOfVertices / 2;

  auto positionPortal = positions.ReadPortal();
  auto radiusPortal = radii.ReadPortal();
  auto capPortal = caps.ReadPortal();
  auto indexPortal = indexComponents.ReadPortal();

  std::vector<viskores::Vec3f_32> meshPoints;
  std::vector<viskores::Id> connectivity;
  std::vector<viskores::Id> primitiveIds;
  std::vector<viskores::Id> vertexIds;
  meshPoints.reserve(static_cast<std::size_t>(numberOfCones * ConeSegments * 12));
  connectivity.reserve(static_cast<std::size_t>(numberOfCones * ConeSegments * 12));
  primitiveIds.reserve(static_cast<std::size_t>(numberOfCones * ConeSegments * 4));
  vertexIds.reserve(static_cast<std::size_t>(numberOfCones * ConeSegments * 12));

  for (viskores::Id coneId = 0; coneId < numberOfCones; ++coneId)
  {
    const viskores::Id firstVertexId = this->m_index ? indexPortal.Get(2 * coneId) : 2 * coneId;
    const viskores::Id secondVertexId =
      this->m_index ? indexPortal.Get(2 * coneId + 1) : 2 * coneId + 1;

    if (firstVertexId < 0 || firstVertexId >= numberOfVertices || secondVertexId < 0 ||
        secondVertexId >= numberOfVertices)
    {
      continue;
    }

    const viskores::Vec3f_32 firstPoint = positionPortal.Get(firstVertexId);
    const viskores::Vec3f_32 secondPoint = positionPortal.Get(secondVertexId);
    const viskores::Vec3f_32 axis = secondPoint - firstPoint;
    const viskores::Float32 axisLength2 = viskores::dot(axis, axis);
    if (axisLength2 <= Epsilon * Epsilon)
      continue;

    const viskores::Float32 firstRadius = viskores::Max(viskores::Float32(0),
                                                        GetRadius(radiusPortal,
                                                                  radii.GetNumberOfValues(),
                                                                  numberOfCones,
                                                                  firstVertexId,
                                                                  coneId,
                                                                  0,
                                                                  this->m_globalRadius));
    const viskores::Float32 secondRadius = viskores::Max(viskores::Float32(0),
                                                         GetRadius(radiusPortal,
                                                                   radii.GetNumberOfValues(),
                                                                   numberOfCones,
                                                                   secondVertexId,
                                                                   coneId,
                                                                   1,
                                                                   this->m_globalRadius));
    if (firstRadius <= Epsilon && secondRadius <= Epsilon)
      continue;

    const viskores::UInt8 capMask = GetCapMask(capPortal,
                                               caps.GetNumberOfValues(),
                                               numberOfVertices,
                                               numberOfCones,
                                               firstVertexId,
                                               secondVertexId,
                                               coneId,
                                               this->m_globalCapMask);

    const viskores::Vec3f_32 axisDirection = axis / viskores::Sqrt(axisLength2);
    const viskores::Vec3f_32 helper = viskores::Abs(axisDirection[0]) < 0.9f
      ? viskores::Vec3f_32(1.f, 0.f, 0.f)
      : viskores::Vec3f_32(0.f, 1.f, 0.f);
    viskores::Vec3f_32 tangent = viskores::Cross(axisDirection, helper);
    viskores::Normalize(tangent);
    const viskores::Vec3f_32 bitangent = viskores::Cross(axisDirection, tangent);

    std::vector<viskores::Vec3f_32> firstRing(static_cast<std::size_t>(ConeSegments));
    std::vector<viskores::Vec3f_32> secondRing(static_cast<std::size_t>(ConeSegments));
    for (viskores::IdComponent segment = 0; segment < ConeSegments; ++segment)
    {
      const viskores::Float32 angle = TwoPi * static_cast<viskores::Float32>(segment) /
        static_cast<viskores::Float32>(ConeSegments);
      const viskores::Vec3f_32 radialDirection =
        tangent * std::cos(angle) + bitangent * std::sin(angle);
      firstRing[static_cast<std::size_t>(segment)] = firstPoint + radialDirection * firstRadius;
      secondRing[static_cast<std::size_t>(segment)] = secondPoint + radialDirection * secondRadius;
    }

    for (viskores::IdComponent segment = 0; segment < ConeSegments; ++segment)
    {
      const viskores::IdComponent nextSegment = (segment + 1) % ConeSegments;
      const auto segmentIndex = static_cast<std::size_t>(segment);
      const auto nextIndex = static_cast<std::size_t>(nextSegment);

      if (firstRadius > Epsilon && secondRadius > Epsilon)
      {
        AddTriangle(meshPoints,
                    connectivity,
                    primitiveIds,
                    vertexIds,
                    coneId,
                    firstRing[segmentIndex],
                    firstVertexId,
                    secondRing[segmentIndex],
                    secondVertexId,
                    secondRing[nextIndex],
                    secondVertexId);
        AddTriangle(meshPoints,
                    connectivity,
                    primitiveIds,
                    vertexIds,
                    coneId,
                    firstRing[segmentIndex],
                    firstVertexId,
                    secondRing[nextIndex],
                    secondVertexId,
                    firstRing[nextIndex],
                    firstVertexId);
      }
      else if (firstRadius > Epsilon)
      {
        AddTriangle(meshPoints,
                    connectivity,
                    primitiveIds,
                    vertexIds,
                    coneId,
                    firstRing[segmentIndex],
                    firstVertexId,
                    secondPoint,
                    secondVertexId,
                    firstRing[nextIndex],
                    firstVertexId);
      }
      else
      {
        AddTriangle(meshPoints,
                    connectivity,
                    primitiveIds,
                    vertexIds,
                    coneId,
                    firstPoint,
                    firstVertexId,
                    secondRing[nextIndex],
                    secondVertexId,
                    secondRing[segmentIndex],
                    secondVertexId);
      }

      if ((capMask & 1) != 0 && firstRadius > Epsilon)
      {
        AddTriangle(meshPoints,
                    connectivity,
                    primitiveIds,
                    vertexIds,
                    coneId,
                    firstPoint,
                    firstVertexId,
                    firstRing[nextIndex],
                    firstVertexId,
                    firstRing[segmentIndex],
                    firstVertexId);
      }
      if ((capMask & 2) != 0 && secondRadius > Epsilon)
      {
        AddTriangle(meshPoints,
                    connectivity,
                    primitiveIds,
                    vertexIds,
                    coneId,
                    secondPoint,
                    secondVertexId,
                    secondRing[segmentIndex],
                    secondVertexId,
                    secondRing[nextIndex],
                    secondVertexId);
      }
    }
  }

  auto pointsArray = viskores::cont::make_ArrayHandle(meshPoints, viskores::CopyFlag::On);
  auto connectivityArray = viskores::cont::make_ArrayHandle(connectivity, viskores::CopyFlag::On);
  auto primitiveIdArray = viskores::cont::make_ArrayHandle(primitiveIds, viskores::CopyFlag::On);
  auto vertexIdArray = viskores::cont::make_ArrayHandle(vertexIds, viskores::CopyFlag::On);

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(static_cast<viskores::Id>(meshPoints.size()),
               viskores::CELL_SHAPE_TRIANGLE,
               3,
               connectivityArray);
  this->m_dataSet.SetCellSet(cellSet);
  this->m_dataSet.AddCoordinateSystem("coords", pointsArray);

  for (viskores::IdComponent attribute = 0; attribute < 5; ++attribute)
    this->AddExpandedPrimitiveField(AttributeName(attribute), primitiveIdArray, numberOfCones);
  for (viskores::IdComponent attribute = 0; attribute < 5; ++attribute)
    this->AddExpandedVertexField(AttributeName(attribute), vertexIdArray, numberOfVertices);
}

void Cone::render(viskores::rendering::Canvas& canvas,
                  const viskores::rendering::Camera& camera,
                  const viskores::cont::Field& field,
                  const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const
{
  const viskores::cont::DataSet& data = this->getDataSet();
  if (data.GetNumberOfPoints() == 0)
    return;

  viskores::rendering::raytracing::RayTracer tracer;
  viskores::rendering::raytracing::TriangleExtractor triExtractor;

  viskores::cont::CoordinateSystem coords = data.GetCoordinateSystem();

  viskores::Bounds shapeBounds;
  viskores::Range scalarRange = field.GetRange().ReadPortal().Get(0);

  triExtractor.ExtractCells(data.GetCellSet());

  if (triExtractor.GetNumberOfTriangles() > 0)
  {
    auto triIntersector = std::make_shared<viskores::rendering::raytracing::TriangleIntersector>();
    triIntersector->SetData(coords, triExtractor.GetTriangles());
    tracer.AddShapeIntersector(triIntersector);
    shapeBounds.Include(triIntersector->GetShapeBounds());
  }

  viskores::Int32 width = (viskores::Int32)canvas.GetWidth();
  viskores::Int32 height = (viskores::Int32)canvas.GetHeight();

  viskores::rendering::raytracing::Camera rayCamera = camera.CreateRaytracingCamera(width, height);

  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  viskores::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<viskores::rendering::CanvasRayTracer*>(&canvas);
  VISKORES_ASSERT(canvasRT != nullptr);

  rayCamera.CreateRays(rays, shapeBounds);
  tracer.GetCamera() = rayCamera;
  rays.Buffers.at(0).InitConst(0.f);
  viskores::rendering::raytracing::RayOperations::MapCanvasToRays(
    rays, camera.CreateRaytracingCamera(width, height), canvasRT->GetDepthBuffer());

  tracer.SetField(field, scalarRange);
  tracer.SetColorMap(colorMap);
  tracer.SetShadingOn(true);
  tracer.Render(rays);

  canvasRT->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
}

bool Cone::isValid() const
{
  return this->m_vertexAttributes.getParam("position");
}

void Cone::AddExpandedPrimitiveField(const std::string& fieldName,
                                     const viskores::cont::ArrayHandle<viskores::Id>& primitiveIds,
                                     viskores::Id numberOfCones)
{
  const auto& fieldArray = this->m_primitiveAttributes.getParam(fieldName);
  if (!fieldArray)
    return;

  const viskores::cont::UnknownArrayHandle fieldData = fieldArray->dataAsViskoresArray();
  if (fieldData.GetNumberOfValues() < numberOfCones)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "'cone' primitive attribute array is too short");
    return;
  }

  fieldData.CastAndCallForTypesWithFloatFallback<VISKORES_DEFAULT_TYPE_LIST,
                                                 VISKORES_DEFAULT_STORAGE_LIST>(AddExpandedField{
    &this->m_dataSet, fieldName, viskores::cont::Field::Association::Cells, primitiveIds });
}

void Cone::AddExpandedVertexField(const std::string& fieldName,
                                  const viskores::cont::ArrayHandle<viskores::Id>& vertexIds,
                                  viskores::Id numberOfVertices)
{
  const auto& fieldArray = this->m_vertexAttributes.getParam(fieldName);
  if (!fieldArray)
    return;

  const viskores::cont::UnknownArrayHandle fieldData = fieldArray->dataAsViskoresArray();
  if (fieldData.GetNumberOfValues() < numberOfVertices)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "'cone' vertex attribute array is too short");
    return;
  }

  fieldData.CastAndCallForTypesWithFloatFallback<VISKORES_DEFAULT_TYPE_LIST,
                                                 VISKORES_DEFAULT_STORAGE_LIST>(AddExpandedField{
    &this->m_dataSet, fieldName, viskores::cont::Field::Association::Points, vertexIds });
}

viskores::UInt8 Cone::ParseCaps(const std::string& caps)
{
  if (caps == "none")
    return 0;
  if (caps == "first")
    return 1;
  if (caps == "second")
    return 2;
  if (caps == "both")
    return 3;

  reportMessage(ANARI_SEVERITY_WARNING, "invalid 'caps' value on 'cone' geometry");
  return 0;
}

} // namespace viskores_device
