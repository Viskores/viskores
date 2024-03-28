//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DataSetBuilderExplicit_h
#define vtk_m_cont_DataSetBuilderExplicit_h

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace cont
{

//Coordinates builder??

class VTKM_CONT_EXPORT DataSetBuilderExplicit
{
public:
  VTKM_CONT
  DataSetBuilderExplicit() {}

  /// \brief Create a 1D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] xVals An array providing the x coordinate of each point.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `vtkm::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords")
  {
    std::vector<T> yVals(xVals.size(), 0), zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm);
  }

  /// \brief Create a 2D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] xVals An array providing the x coordinate of each point.
  /// @param[in] yVals An array providing the x coordinate of each point.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `vtkm::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords")
  {
    std::vector<T> zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm);
  }

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] xVals An array providing the x coordinate of each point.
  /// @param[in] yVals An array providing the x coordinate of each point.
  /// @param[in] zVals An array providing the x coordinate of each point.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `vtkm::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<T>& zVals,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `vtkm::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& coords,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity.
  ///
  /// The cell connectivity is specified with arrays defining the shape and point
  /// connections of each cell.
  /// In this form, the cell connectivity and coordinates are specified as `ArrayHandle`
  /// and the memory will be shared with the created data object. That said, the `DataSet`
  /// construction will generate a new array for offsets.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] shapes An array of shapes for each cell. Each entry should be one of the
  ///   `vtkm::CELL_SHAPE_*` values identifying the shape of the corresponding cell.
  /// @param[in] numIndices An array containing for each cell the number of points incident
  ///   on that cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   the @a numIndices array. These variable length arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numIndices);
    return DataSetBuilderExplicit::BuildDataSet(coords, shapes, offsets, connectivity, coordsNm);
  }

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity for a single cell type.
  ///
  /// The cell connectivity is specified with an array defining the point
  /// connections of each cell.
  /// All the cells in the `DataSet` are of the same shape and contain the same number
  /// of incident points.
  /// In this form, the cell connectivity and coordinates are specified as `std::vector`
  /// and the data will be copied to create the data object.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] tag A tag object representing the shape of all the cells in the mesh.
  ///   Cell shape tag objects have a name of the form `vtkm::CellShapeTag*` such as
  ///   `vtkm::CellShapeTagTriangle` or `vtkm::CellShapeTagHexahedron`. To specify a
  ///   cell shape determined at runtime, use `vtkm::CellShapeTagGeneric`.
  /// @param[in] numberOfPointsPerCell The number of points that are incident to each cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   @a numberOfPointsPerCell. These short arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& coords,
                                              CellShapeTag tag,
                                              vtkm::IdComponent numberOfPointsPerCell,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  /// \brief Create a 3D `DataSet` with arbitrary cell connectivity for a single cell type.
  ///
  /// The cell connectivity is specified with an array defining the point
  /// connections of each cell.
  /// All the cells in the `DataSet` are of the same shape and contain the same number
  /// of incident points.
  /// In this form, the cell connectivity and coordinates are specified as `ArrayHandle`
  /// and the memory will be shared with the created data object.
  ///
  /// @param[in] coords An array of point coordinates.
  /// @param[in] tag A tag object representing the shape of all the cells in the mesh.
  ///   Cell shape tag objects have a name of the form `vtkm::CellShapeTag*` such as
  ///   `vtkm::CellShapeTagTriangle` or `vtkm::CellShapeTagHexahedron`. To specify a
  ///   cell shape determined at runtime, use `vtkm::CellShapeTagGeneric`.
  /// @param[in] numberOfPointsPerCell The number of points that are incident to each cell.
  /// @param[in] connectivity An array specifying for each cell the indicies of points
  ///   incident on each cell. Each cell has a short array of indices that reference points
  ///   in the @a coords array. The length of each of these short arrays is specified by
  ///   @a numberOfPointsPerCell. These short arrays are tightly packed together
  ///   in this @a connectivity array.
  /// @param[in] coordsNm (optional) The name to register the coordinates as.
  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    CellShapeTag tag,
    vtkm::IdComponent numberOfPointsPerCell,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    return DataSetBuilderExplicit::BuildDataSet(
      coords, tag, numberOfPointsPerCell, connectivity, coordsNm);
  }

private:
  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::Id>& offsets,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm);

  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    CellShapeTag tag,
    vtkm::IdComponent numberOfPointsPerCell,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm);
};

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<T>& xVals,
  const std::vector<T>& yVals,
  const std::vector<T>& zVals,
  const std::vector<vtkm::UInt8>& shapes,
  const std::vector<vtkm::IdComponent>& numIndices,
  const std::vector<vtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  VTKM_ASSERT(xVals.size() == yVals.size() && yVals.size() == zVals.size() && xVals.size() > 0);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> coordsArray;
  coordsArray.Allocate(static_cast<vtkm::Id>(xVals.size()));
  auto coordsPortal = coordsArray.WritePortal();
  for (std::size_t index = 0; index < xVals.size(); ++index)
  {
    coordsPortal.Set(static_cast<vtkm::Id>(index),
                     vtkm::make_Vec(static_cast<vtkm::FloatDefault>(xVals[index]),
                                    static_cast<vtkm::FloatDefault>(yVals[index]),
                                    static_cast<vtkm::FloatDefault>(zVals[index])));
  }

  auto shapesArray = vtkm::cont::make_ArrayHandle(shapes, vtkm::CopyFlag::On);
  auto connArray = vtkm::cont::make_ArrayHandle(connectivity, vtkm::CopyFlag::On);

  auto offsetsArray = vtkm::cont::ConvertNumComponentsToOffsets(
    vtkm::cont::make_ArrayHandle(numIndices, vtkm::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    coordsArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<vtkm::Vec<T, 3>>& coords,
  const std::vector<vtkm::UInt8>& shapes,
  const std::vector<vtkm::IdComponent>& numIndices,
  const std::vector<vtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  auto coordsArray = vtkm::cont::make_ArrayHandle(coords, vtkm::CopyFlag::On);
  auto shapesArray = vtkm::cont::make_ArrayHandle(shapes, vtkm::CopyFlag::On);
  auto connArray = vtkm::cont::make_ArrayHandle(connectivity, vtkm::CopyFlag::On);

  auto offsetsArray = vtkm::cont::ConvertNumComponentsToOffsets(
    vtkm::cont::make_ArrayHandle(numIndices, vtkm::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    coordsArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
  const vtkm::cont::ArrayHandle<vtkm::Id>& offsets,
  const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  vtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm, coords));
  vtkm::Id nPts = static_cast<vtkm::Id>(coords.GetNumberOfValues());
  vtkm::cont::CellSetExplicit<> cellSet;

  cellSet.Fill(nPts, shapes, connectivity, offsets);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

template <typename T, typename CellShapeTag>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<vtkm::Vec<T, 3>>& coords,
  CellShapeTag tag,
  vtkm::IdComponent numberOfPointsPerCell,
  const std::vector<vtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  auto coordsArray = vtkm::cont::make_ArrayHandle(coords, vtkm::CopyFlag::On);
  auto connArray = vtkm::cont::make_ArrayHandle(connectivity, vtkm::CopyFlag::On);

  return DataSetBuilderExplicit::Create(
    coordsArray, tag, numberOfPointsPerCell, connArray, coordsNm);
}

template <typename T, typename CellShapeTag>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
  CellShapeTag tag,
  vtkm::IdComponent numberOfPointsPerCell,
  const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  (void)tag; //C4100 false positive workaround
  vtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm, coords));
  vtkm::cont::CellSetSingleType<> cellSet;

  cellSet.Fill(coords.GetNumberOfValues(), tag.Id, numberOfPointsPerCell, connectivity);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

/// @brief Helper class to build a `DataSet` by iteratively adding points and cells.
///
/// This class allows you to specify a `DataSet` by adding points and cells one at a time.
class VTKM_CONT_EXPORT DataSetBuilderExplicitIterative
{
public:
  VTKM_CONT DataSetBuilderExplicitIterative();

  /// @brief Begin defining points and cells of a `DataSet`.
  ///
  /// The state of this object is initialized to be ready to use `AddPoint` and
  /// `AddCell` methods.
  ///
  /// @param[in] coordName (optional) The name to register the coordinates as.
  VTKM_CONT void Begin(const std::string& coordName = "coords");

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] pt The coordinates of the point to add.
  /// @returns The index of the newly created point.
  VTKM_CONT vtkm::Id AddPoint(const vtkm::Vec3f& pt);

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] pt The coordinates of the point to add.
  /// @returns The index of the newly created point.
  template <typename T>
  VTKM_CONT vtkm::Id AddPoint(const vtkm::Vec<T, 3>& pt)
  {
    return AddPoint(static_cast<vtkm::Vec3f>(pt));
  }

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] x The x coordinate of the newly created point.
  /// @param[in] y The y coordinate of the newly created point.
  /// @param[in] z The z coordinate of the newly created point.
  /// @returns The index of the newly created point.
  VTKM_CONT vtkm::Id AddPoint(const vtkm::FloatDefault& x,
                              const vtkm::FloatDefault& y,
                              const vtkm::FloatDefault& z = 0);

  /// @brief Add a point to the `DataSet`.
  ///
  /// @param[in] x The x coordinate of the newly created point.
  /// @param[in] y The y coordinate of the newly created point.
  /// @param[in] z The z coordinate of the newly created point.
  /// @returns The index of the newly created point.
  template <typename T>
  VTKM_CONT vtkm::Id AddPoint(const T& x, const T& y, const T& z = 0)
  {
    return AddPoint(static_cast<vtkm::FloatDefault>(x),
                    static_cast<vtkm::FloatDefault>(y),
                    static_cast<vtkm::FloatDefault>(z));
  }

  /// @brief Add a cell to the `DataSet`.
  ///
  /// @param[in] shape Identifies the shape of the cell. Use one of the
  ///   `vtkm::CELL_SHAPE_*` values.
  /// @param[in] conn List of indices to the incident points.
  VTKM_CONT void AddCell(const vtkm::UInt8& shape, const std::vector<vtkm::Id>& conn);

  /// @brief Add a cell to the `DataSet`.
  ///
  /// @param[in] shape Identifies the shape of the cell. Use one of the
  ///   `vtkm::CELL_SHAPE_*` values.
  /// @param[in] conn List of indices to the incident points.
  /// @param[in] n The number of incident points (and the length of the `conn` array).
  VTKM_CONT void AddCell(const vtkm::UInt8& shape,
                         const vtkm::Id* conn,
                         const vtkm::IdComponent& n);

  /// @brief Start adding a cell to the `DataSet`.
  ///
  /// The incident points are later added one at a time using `AddCellPoint`.
  /// The cell is completed the next time `AddCell` or `Create` is called.
  ///
  /// @param[in] shape Identifies the shape of the cell. Use one of the
  VTKM_CONT void AddCell(vtkm::UInt8 shape);

  /// @brief Add an incident point to the current cell.
  ///
  /// @param[in] pointIndex Index to the incident point.
  VTKM_CONT void AddCellPoint(vtkm::Id pointIndex);

  /// @brief Produce the `DataSet`.
  ///
  /// The points and cells previously added are finalized and the resulting `DataSet`
  /// is returned.
  VTKM_CONT vtkm::cont::DataSet Create();

private:
  std::string coordNm;

  std::vector<vtkm::Vec3f> points;
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numIdx;
  std::vector<vtkm::Id> connectivity;
};
}
}

#endif //vtk_m_cont_DataSetBuilderExplicit_h
