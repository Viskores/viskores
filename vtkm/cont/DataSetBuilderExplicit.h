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

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
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

  //Single cell explicits.
  //TODO

  //Zoo explicit cell
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

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<T>& zVals,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<T>& xVals,
    const vtkm::cont::ArrayHandle<T>& yVals,
    const vtkm::cont::ArrayHandle<T>& zVals,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numIndices);

    return DataSetBuilderExplicit::BuildDataSet(
      xVals, yVals, zVals, shapes, offsets, connectivity, coordsNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& coords,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numIndices);
    return DataSetBuilderExplicit::BuildDataSet(coords, shapes, offsets, connectivity, coordsNm);
  }

  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& coords,
                                              CellShapeTag tag,
                                              vtkm::IdComponent numberOfPointsPerCell,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

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
  static vtkm::cont::DataSet BuildDataSet(const vtkm::cont::ArrayHandle<T>& X,
                                          const vtkm::cont::ArrayHandle<T>& Y,
                                          const vtkm::cont::ArrayHandle<T>& Z,
                                          const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                                          const vtkm::cont::ArrayHandle<vtkm::Id>& offsets,
                                          const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                                          const std::string& coordsNm);

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

  auto xArray = vtkm::cont::make_ArrayHandle(xVals, vtkm::CopyFlag::On);
  auto yArray = vtkm::cont::make_ArrayHandle(yVals, vtkm::CopyFlag::On);
  auto zArray = vtkm::cont::make_ArrayHandle(zVals, vtkm::CopyFlag::On);

  auto shapesArray = vtkm::cont::make_ArrayHandle(shapes, vtkm::CopyFlag::On);
  auto connArray = vtkm::cont::make_ArrayHandle(connectivity, vtkm::CopyFlag::On);

  auto offsetsArray = vtkm::cont::ConvertNumIndicesToOffsets(
    vtkm::cont::make_ArrayHandle(numIndices, vtkm::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    xArray, yArray, zArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const vtkm::cont::ArrayHandle<T>& X,
  const vtkm::cont::ArrayHandle<T>& Y,
  const vtkm::cont::ArrayHandle<T>& Z,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
  const vtkm::cont::ArrayHandle<vtkm::Id>& offsets,
  const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  VTKM_ASSERT(X.GetNumberOfValues() == Y.GetNumberOfValues() &&
              Y.GetNumberOfValues() == Z.GetNumberOfValues() && X.GetNumberOfValues() > 0 &&
              shapes.GetNumberOfValues() + 1 == offsets.GetNumberOfValues());

  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem(coordsNm, make_ArrayHandleCompositeVector(X, Y, Z)));
  vtkm::Id nPts = X.GetNumberOfValues();
  vtkm::cont::CellSetExplicit<> cellSet;

  cellSet.Fill(nPts, shapes, connectivity, offsets);
  dataSet.SetCellSet(cellSet);

  return dataSet;
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

  auto offsetsArray = vtkm::cont::ConvertNumIndicesToOffsets(
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

class VTKM_CONT_EXPORT DataSetBuilderExplicitIterative
{
public:
  VTKM_CONT
  DataSetBuilderExplicitIterative();

  VTKM_CONT
  void Begin(const std::string& coordName = "coords");

  //Define points.
  VTKM_CONT
  vtkm::cont::DataSet Create();

  VTKM_CONT
  vtkm::Id AddPoint(const vtkm::Vec3f& pt);

  VTKM_CONT
  vtkm::Id AddPoint(const vtkm::FloatDefault& x,
                    const vtkm::FloatDefault& y,
                    const vtkm::FloatDefault& z = 0);

  template <typename T>
  VTKM_CONT vtkm::Id AddPoint(const T& x, const T& y, const T& z = 0)
  {
    return AddPoint(static_cast<vtkm::FloatDefault>(x),
                    static_cast<vtkm::FloatDefault>(y),
                    static_cast<vtkm::FloatDefault>(z));
  }

  template <typename T>
  VTKM_CONT vtkm::Id AddPoint(const vtkm::Vec<T, 3>& pt)
  {
    return AddPoint(static_cast<vtkm::Vec3f>(pt));
  }

  //Define cells.
  VTKM_CONT
  void AddCell(vtkm::UInt8 shape);

  VTKM_CONT
  void AddCell(const vtkm::UInt8& shape, const std::vector<vtkm::Id>& conn);

  VTKM_CONT
  void AddCell(const vtkm::UInt8& shape, const vtkm::Id* conn, const vtkm::IdComponent& n);

  VTKM_CONT
  void AddCellPoint(vtkm::Id pointIndex);

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
