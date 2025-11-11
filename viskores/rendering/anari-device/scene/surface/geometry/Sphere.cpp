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
  }

  auto pointMapper = std::make_shared<viskores::rendering::MapperPoint>();
  pointMapper->SetUsePoints();

  if (this->m_vertexRadius)
  {
    pointMapper->UseVariableRadius(true);
    this->m_dataSet.AddPointField("data", this->m_vertexRadius->dataAsViskoresArray());
  }
  else
  {
    pointMapper->UseVariableRadius(false);
    pointMapper->SetRadius(this->m_globalRadius);
  }

  this->m_mapper = pointMapper;

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

  auto permuteArray = viskores::cont::make_ArrayHandlePermutation(indexArray, vertices);
  this->m_dataSet.AddCoordinateSystem({ "coords", permuteArray });

  // Now handle the radius.
  if (this->m_vertexRadius)
  {
    viskores::cont::ArrayHandle<viskores::FloatDefault> radiusArray;
    auto tmp = this->m_vertexRadius->dataAsViskoresArray();
    if (!tmp.IsValueType<viskores::FloatDefault>())
      viskores::cont::ArrayCopy(tmp, radiusArray);
    else
      radiusArray = tmp.AsArrayHandle<viskores::cont::ArrayHandle<viskores::FloatDefault>>();

    auto permuteArray = viskores::cont::make_ArrayHandlePermutation(indexArray, radiusArray);

    this->m_dataSet.AddPointField("data", permuteArray);
  }
}

} // namespace viskores_device
