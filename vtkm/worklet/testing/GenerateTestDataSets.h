//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/GhostCellClassify.h>

namespace vtkm
{
namespace worklet
{
namespace testing
{

enum class ExplicitDataSetOption
{
  SINGLE = 0,
  CURVILINEAR,
  EXPLICIT
};

inline vtkm::cont::DataSet CreateUniformDataSet(const vtkm::Bounds& bounds,
                                                const vtkm::Id3& dims,
                                                bool addGhost = false)
{
  vtkm::Vec3f origin(static_cast<vtkm::FloatDefault>(bounds.X.Min),
                     static_cast<vtkm::FloatDefault>(bounds.Y.Min),
                     static_cast<vtkm::FloatDefault>(bounds.Z.Min));
  vtkm::Vec3f spacing(static_cast<vtkm::FloatDefault>(bounds.X.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[0] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Y.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[1] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Z.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[2] - 1)));

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);

  if (addGhost)
  {
    vtkm::filter::GhostCellClassify addGhostFilter;
    return addGhostFilter.Execute(ds);
  }
  return ds;
}

inline vtkm::cont::DataSet CreateRectilinearDataSet(const vtkm::Bounds& bounds,
                                                    const vtkm::Id3& dims,
                                                    bool addGhost = false)
{
  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;
  std::vector<vtkm::FloatDefault> xvals, yvals, zvals;

  vtkm::Vec3f spacing(static_cast<vtkm::FloatDefault>(bounds.X.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[0] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Y.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[1] - 1)),
                      static_cast<vtkm::FloatDefault>(bounds.Z.Length()) /
                        static_cast<vtkm::FloatDefault>((dims[2] - 1)));
  xvals.resize((size_t)dims[0]);
  xvals[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min);
  for (size_t i = 1; i < (size_t)dims[0]; i++)
    xvals[i] = xvals[i - 1] + spacing[0];

  yvals.resize((size_t)dims[1]);
  yvals[0] = static_cast<vtkm::FloatDefault>(bounds.Y.Min);
  for (size_t i = 1; i < (size_t)dims[1]; i++)
    yvals[i] = yvals[i - 1] + spacing[1];

  zvals.resize((size_t)dims[2]);
  zvals[0] = static_cast<vtkm::FloatDefault>(bounds.Z.Min);
  for (size_t i = 1; i < (size_t)dims[2]; i++)
    zvals[i] = zvals[i - 1] + spacing[2];

  vtkm::cont::DataSet ds = dataSetBuilder.Create(xvals, yvals, zvals);

  if (addGhost)
  {
    vtkm::filter::GhostCellClassify addGhostFilter;
    return addGhostFilter.Execute(ds);
  }
  return ds;
}

template <class CellSetType, vtkm::IdComponent NDIM>
inline void MakeExplicitCells(const CellSetType& cellSet,
                              vtkm::Vec<vtkm::Id, NDIM>& cellDims,
                              vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                              vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                              vtkm::cont::ArrayHandle<vtkm::Id>& conn)
{
  using Connectivity = vtkm::internal::ConnectivityStructuredInternals<NDIM>;

  vtkm::Id nCells = cellSet.GetNumberOfCells();
  vtkm::IdComponent nVerts = (NDIM == 2 ? 4 : 8);
  vtkm::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(cellDims + vtkm::Vec<vtkm::Id, NDIM>(1));

  auto connPortal = conn.WritePortal();
  auto shapesPortal = shapes.WritePortal();
  auto numIndicesPortal = numIndices.WritePortal();
  vtkm::Id connectionIndex = 0;
  for (vtkm::Id cellIndex = 0; cellIndex < nCells; cellIndex++)
  {
    auto ptIds = structured.GetPointsOfCell(cellIndex);
    for (vtkm::IdComponent vertexIndex = 0; vertexIndex < nVerts; vertexIndex++, connectionIndex++)
      connPortal.Set(connectionIndex, ptIds[vertexIndex]);

    shapesPortal.Set(cellIndex, (NDIM == 2 ? vtkm::CELL_SHAPE_QUAD : vtkm::CELL_SHAPE_HEXAHEDRON));
    numIndicesPortal.Set(cellIndex, nVerts);
  }
}

inline vtkm::cont::DataSet CreateExplicitFromStructuredDataSet(const vtkm::Bounds& bounds,
                                                               const vtkm::Id3& dims,
                                                               ExplicitDataSetOption option,
                                                               bool addGhost = false)
{
  using CoordType = vtkm::Vec3f;
  auto input = CreateUniformDataSet(bounds, dims, addGhost);

  auto inputCoords = input.GetCoordinateSystem(0).GetData();
  vtkm::cont::ArrayHandle<CoordType> explCoords;
  vtkm::cont::ArrayCopy(inputCoords, explCoords);

  vtkm::cont::DynamicCellSet cellSet = input.GetCellSet();
  vtkm::cont::ArrayHandle<vtkm::Id> conn;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::DataSet output;
  vtkm::cont::DataSetBuilderExplicit dsb;

  using Structured2DType = vtkm::cont::CellSetStructured<2>;
  using Structured3DType = vtkm::cont::CellSetStructured<3>;

  switch (option)
  {
    case ExplicitDataSetOption::SINGLE:
      if (cellSet.IsType<Structured2DType>())
      {
        Structured2DType cells2D = cellSet.Cast<Structured2DType>();
        vtkm::Id2 cellDims = cells2D.GetCellDimensions();
        MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, vtkm::CellShapeTagQuad(), 4, conn, "coordinates");
      }
      else
      {
        Structured3DType cells3D = cellSet.Cast<Structured3DType>();
        vtkm::Id3 cellDims = cells3D.GetCellDimensions();
        MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, vtkm::CellShapeTagHexahedron(), 8, conn, "coordinates");
      }
      break;

    case ExplicitDataSetOption::CURVILINEAR:
      // In this case the cell set/connectivity is the same as the input
      // Only the coords are no longer Uniform / Rectilinear
      output.SetCellSet(cellSet);
      output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", explCoords));
      break;

    case ExplicitDataSetOption::EXPLICIT:
      if (cellSet.IsType<Structured2DType>())
      {
        Structured2DType cells2D = cellSet.Cast<Structured2DType>();
        vtkm::Id2 cellDims = cells2D.GetCellDimensions();
        MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates");
      }
      else
      {
        Structured3DType cells3D = cellSet.Cast<Structured3DType>();
        vtkm::Id3 cellDims = cells3D.GetCellDimensions();
        MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
        output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates");
      }
      break;
  }

  if (addGhost)
    output.AddField(input.GetField("vtkmGhostCells"));
  return output;
}

inline std::vector<vtkm::cont::DataSet> CreateAllDataSets(const vtkm::Bounds& bounds,
                                                          const vtkm::Id3& dims,
                                                          bool addGhost)
{
  std::vector<vtkm::cont::DataSet> dataSets;

  dataSets.push_back(vtkm::worklet::testing::CreateUniformDataSet(bounds, dims, addGhost));
  dataSets.push_back(vtkm::worklet::testing::CreateRectilinearDataSet(bounds, dims, addGhost));
  dataSets.push_back(vtkm::worklet::testing::CreateExplicitFromStructuredDataSet(
    bounds, dims, ExplicitDataSetOption::SINGLE, addGhost));
  dataSets.push_back(vtkm::worklet::testing::CreateExplicitFromStructuredDataSet(
    bounds, dims, ExplicitDataSetOption::CURVILINEAR, addGhost));
  dataSets.push_back(vtkm::worklet::testing::CreateExplicitFromStructuredDataSet(
    bounds, dims, ExplicitDataSetOption::EXPLICIT, addGhost));

  return dataSets;
}

inline std::vector<vtkm::cont::PartitionedDataSet> CreateAllDataSets(
  const std::vector<vtkm::Bounds>& bounds,
  const std::vector<vtkm::Id3>& dims,
  bool addGhost)
{
  std::vector<vtkm::cont::PartitionedDataSet> pds;
  std::vector<std::vector<vtkm::cont::DataSet>> dataSets;

  VTKM_ASSERT(bounds.size() == dims.size());
  std::size_t n = bounds.size();
  for (std::size_t i = 0; i < n; i++)
  {
    auto dsVec = CreateAllDataSets(bounds[i], dims[i], addGhost);
    std::size_t n2 = dsVec.size();
    if (i == 0)
      dataSets.resize(n2);
    for (std::size_t j = 0; j < n2; j++)
      dataSets[j].push_back(dsVec[j]);
  }

  for (auto& dsVec : dataSets)
    pds.push_back(vtkm::cont::PartitionedDataSet(dsVec));

  return pds;
}


inline std::vector<vtkm::cont::PartitionedDataSet>
CreateAllDataSets(const std::vector<vtkm::Bounds>& bounds, const vtkm::Id3& dim, bool addGhost)
{
  std::vector<vtkm::Id3> dims(bounds.size(), dim);
  return CreateAllDataSets(bounds, dims, addGhost);
}


} //namespace testing
} //namespace worklet
} //namespace vtkm
