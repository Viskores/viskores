//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Sphere.h"

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/ArrayRangeCompute.h>
#include <viskores/rendering/raytracing/Camera.h>
#include <viskores/rendering/raytracing/Ray.h>
#include <viskores/rendering/raytracing/RayOperations.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/SphereExtractor.h>
#include <viskores/rendering/raytracing/SphereIntersector.h>


#include <numeric>

namespace viskores_device
{

Sphere::Sphere(ViskoresDeviceGlobalState* s)
  : Geometry(s)
  , m_index(this)
  , m_vertexPosition(this)
  , m_vertexRadius(this)
{
}

void Sphere::commitParameters()
{
  this->Geometry::commitParameters();

  m_index = getParamObject<Array1D>("primitive.index");
  m_vertexPosition = getParamObject<Array1D>("vertex.position");
  m_vertexRadius = getParamObject<Array1D>("vertex.radius");
}

void Sphere::finalize()
{
  this->Geometry::finalize();

  if (!m_vertexPosition)
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "missing required parameter 'vertex.position' on sphere geometry");
    return;
  }

  m_globalRadius = getParam<float>("radius", 0.01f);
  const auto numSpheres = m_index ? m_index->size() : m_vertexPosition->size();

  this->m_dataSet = viskores::cont::DataSet{};

  if (m_index)
  {
    this->SetupIndexBased();
  }
  else
  {
    this->m_dataSet.AddCoordinateSystem(
      { "coords", this->m_vertexPosition->dataAsViskoresArray() });
    this->m_dataSet.AddPointField("radius", this->m_vertexRadius->dataAsViskoresArray());
  }

  if (this->m_vertexRadius)
  {
    this->m_radiusRange = this->m_dataSet.GetField("radius").GetRange().ReadPortal().Get(0);
  }

  auto connIdx = viskores::cont::make_ArrayHandleIndex(numSpheres);
  viskores::cont::ArrayHandle<viskores::Id> conn;
  viskores::cont::ArrayCopy(connIdx, conn);

  viskores::cont::CellSetSingleType<> cellSet;
  cellSet.Fill(static_cast<viskores::Id>(numSpheres), viskores::CELL_SHAPE_VERTEX, 1, conn);
  this->m_dataSet.SetCellSet(cellSet);
}

void Sphere::SetupIndexBased()
{
  viskores::cont::ArrayHandle<viskores::Id> indexArray;
  viskores::cont::ArrayHandle<viskores::Vec3f> vertices;

  auto viskoresArray = this->m_index->dataAsViskoresArray();
  if (!viskoresArray.IsValueType<viskores::Id>())
    viskores::cont::ArrayCopy(viskoresArray, indexArray);
  else
  {
    indexArray = viskoresArray.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>();
  }

  auto positionArray = m_vertexPosition->dataAsViskoresArray();
  if (!positionArray.IsValueType<viskores::Vec3f>())
  {
    viskores::cont::ArrayCopy(positionArray, vertices);
  }
  else
  {
    vertices = positionArray.AsArrayHandle<viskores::cont::ArrayHandle<viskores::Vec3f>>();
  }

  // KEN: Instead of permuting arrays (does that even work?), why not build a set of vertex
  // cells and use that to permute the values? The underlying raycaster already supports that.
  auto vertexPermute = viskores::cont::make_ArrayHandlePermutation(indexArray, vertices);
  this->m_dataSet.AddCoordinateSystem({ "coords", vertexPermute });

  // Now handle the radius.
  if (this->m_vertexRadius)
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> radiusArray;
    this->m_vertexRadius->dataAsViskoresArray().CopyShallowIfPossible(radiusArray);

    auto radiusPermute = viskores::cont::make_ArrayHandlePermutation(indexArray, radiusArray);

    this->m_dataSet.AddPointField("radius", radiusPermute);
  }
}

void Sphere::render(viskores::rendering::Canvas& canvas,
                    const viskores::rendering::Camera& camera,
                    const viskores::cont::Field& field,
                    const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const
{
  viskores::rendering::raytracing::RayTracer tracer;
  viskores::rendering::raytracing::SphereExtractor sphereExtractor;

  const viskores::cont::DataSet& data = this->getDataSet();
  viskores::cont::CoordinateSystem coords = data.GetCoordinateSystem();

  viskores::Bounds shapeBounds;
  viskores::Range scalarRange = field.GetRange().ReadPortal().Get(0);

  if (this->m_vertexRadius)
  {
    // This builds the radius array using an adjustment of the radius based on the desired
    // min and max radius. However, we want to take the radius at face value, so insert values
    // that compute back to the original value. This is silly, so we should implement a simple
    // version that just takes the array.
    sphereExtractor.ExtractCoordinates(coords,
                                       this->m_dataSet.GetField("radius"),
                                       static_cast<viskores::Float32>(this->m_radiusRange.Min),
                                       static_cast<viskores::Float32>(this->m_radiusRange.Max));
  }
  else
  {
    sphereExtractor.ExtractCoordinates(coords, this->m_globalRadius);
  }

  if (sphereExtractor.GetNumberOfSpheres() > 0)
  {
    auto sphereIntersector = std::make_shared<viskores::rendering::raytracing::SphereIntersector>();
    sphereIntersector->SetData(coords, sphereExtractor.GetPointIds(), sphereExtractor.GetRadii());
    tracer.AddShapeIntersector(sphereIntersector);
    shapeBounds.Include(sphereIntersector->GetShapeBounds());
  }

  //
  // Create rays
  //
  viskores::Int32 width = (viskores::Int32)canvas.GetWidth();
  viskores::Int32 height = (viskores::Int32)canvas.GetHeight();

  viskores::rendering::raytracing::Camera rayCamera;
  rayCamera.SetParameters(camera, width, height);

  viskores::rendering::raytracing::Ray<viskores::Float32> rays;
  viskores::rendering::CanvasRayTracer* canvasRT =
    dynamic_cast<viskores::rendering::CanvasRayTracer*>(&canvas);
  VISKORES_ASSERT(canvasRT != nullptr);

  rayCamera.CreateRays(rays, shapeBounds);
  rays.Buffers.at(0).InitConst(0.f);
  viskores::rendering::raytracing::RayOperations::MapCanvasToRays(rays, camera, *canvasRT);

  tracer.SetField(field, scalarRange);
  tracer.GetCamera() = rayCamera;
  tracer.SetColorMap(colorMap);
  tracer.Render(rays);

  canvasRT->WriteToCanvas(rays, rays.Buffers.at(0).Buffer, camera);
}

} // namespace viskores_device
