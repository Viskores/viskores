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

#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace cont
{

//Coordinates builder??
//Need a singlecellset handler.

class VTKM_CONT_EXPORT DataSetBuilderExplicit
{
  template <typename T>
  VTKM_CONT static void CopyInto(const std::vector<T>& input, vtkm::cont::ArrayHandle<T>& output)
  {
    output.Allocate(static_cast<vtkm::Id>(input.size()));
    std::copy(input.begin(), input.end(), ArrayPortalToIteratorBegin(output.GetPortalControl()));
  }

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
                                              const std::string& coordsNm = "coords",
                                              const std::string& cellNm = "cells")
  {
    std::vector<T> yVals(xVals.size(), 0), zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm, cellNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords",
                                              const std::string& cellNm = "cells")
  {
    std::vector<T> zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm, cellNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<T>& zVals,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords",
                                              const std::string& cellNm = "cells");

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<T>& xVals,
    const vtkm::cont::ArrayHandle<T>& yVals,
    const vtkm::cont::ArrayHandle<T>& zVals,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords",
    const std::string& cellNm = "cells")
  {
    return DataSetBuilderExplicit::BuildDataSet(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm, cellNm);
  }

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& coords,
                                              const std::vector<vtkm::UInt8>& shapes,
                                              const std::vector<vtkm::IdComponent>& numIndices,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords",
                                              const std::string& cellNm = "cells");

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords",
    const std::string& cellNm = "cells")
  {
    return DataSetBuilderExplicit::BuildDataSet(
      coords, shapes, numIndices, connectivity, coordsNm, cellNm);
  }

  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet Create(const std::vector<vtkm::Vec<T, 3>>& coords,
                                              CellShapeTag tag,
                                              vtkm::IdComponent numberOfPointsPerCell,
                                              const std::vector<vtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords",
                                              const std::string& cellNm = "cells");

  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet Create(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    CellShapeTag tag,
    vtkm::IdComponent numberOfPointsPerCell,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm = "coords",
    const std::string& cellNm = "cells")
  {
    return DataSetBuilderExplicit::BuildDataSet(
      coords, tag, numberOfPointsPerCell, connectivity, coordsNm, cellNm);
  }

private:
  template <typename T>
  static vtkm::cont::DataSet BuildDataSet(
    const vtkm::cont::ArrayHandle<T>& X,
    const vtkm::cont::ArrayHandle<T>& Y,
    const vtkm::cont::ArrayHandle<T>& Z,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm,
    const std::string& cellNm);

  template <typename T>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm,
    const std::string& cellNm);

  template <typename T, typename CellShapeTag>
  VTKM_CONT static vtkm::cont::DataSet BuildDataSet(
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
    CellShapeTag tag,
    vtkm::IdComponent numberOfPointsPerCell,
    const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
    const std::string& coordsNm,
    const std::string& cellNm);
};

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<T>& xVals,
  const std::vector<T>& yVals,
  const std::vector<T>& zVals,
  const std::vector<vtkm::UInt8>& shapes,
  const std::vector<vtkm::IdComponent>& numIndices,
  const std::vector<vtkm::Id>& connectivity,
  const std::string& coordsNm,
  const std::string& cellNm)
{
  VTKM_ASSERT(xVals.size() == yVals.size() && yVals.size() == zVals.size() && xVals.size() > 0);

  vtkm::cont::ArrayHandle<T> Xc, Yc, Zc;
  DataSetBuilderExplicit::CopyInto(xVals, Xc);
  DataSetBuilderExplicit::CopyInto(yVals, Yc);
  DataSetBuilderExplicit::CopyInto(zVals, Zc);

  vtkm::cont::ArrayHandle<vtkm::UInt8> Sc;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> Nc;
  vtkm::cont::ArrayHandle<vtkm::Id> Cc;
  DataSetBuilderExplicit::CopyInto(shapes, Sc);
  DataSetBuilderExplicit::CopyInto(numIndices, Nc);
  DataSetBuilderExplicit::CopyInto(connectivity, Cc);

  return DataSetBuilderExplicit::BuildDataSet(Xc, Yc, Zc, Sc, Nc, Cc, coordsNm, cellNm);
}

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const vtkm::cont::ArrayHandle<T>& X,
  const vtkm::cont::ArrayHandle<T>& Y,
  const vtkm::cont::ArrayHandle<T>& Z,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
  const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
  const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
  const std::string& coordsNm,
  const std::string& cellNm)
{
  VTKM_ASSERT(X.GetNumberOfValues() == Y.GetNumberOfValues() &&
              Y.GetNumberOfValues() == Z.GetNumberOfValues() && X.GetNumberOfValues() > 0 &&
              shapes.GetNumberOfValues() == numIndices.GetNumberOfValues());

  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem(coordsNm, make_ArrayHandleCompositeVector(X, Y, Z)));
  vtkm::Id nPts = X.GetNumberOfValues();
  vtkm::cont::CellSetExplicit<> cellSet(cellNm);

  cellSet.Fill(nPts, shapes, numIndices, connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<vtkm::Vec<T, 3>>& coords,
  const std::vector<vtkm::UInt8>& shapes,
  const std::vector<vtkm::IdComponent>& numIndices,
  const std::vector<vtkm::Id>& connectivity,
  const std::string& coordsNm,
  const std::string& cellNm)
{
  vtkm::cont::ArrayHandle<Vec<T, 3>> coordsArray;
  DataSetBuilderExplicit::CopyInto(coords, coordsArray);

  vtkm::cont::ArrayHandle<vtkm::UInt8> Sc;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> Nc;
  vtkm::cont::ArrayHandle<vtkm::Id> Cc;
  DataSetBuilderExplicit::CopyInto(shapes, Sc);
  DataSetBuilderExplicit::CopyInto(numIndices, Nc);
  DataSetBuilderExplicit::CopyInto(connectivity, Cc);

  return DataSetBuilderExplicit::Create(coordsArray, Sc, Nc, Cc, coordsNm, cellNm);
}

template <typename T>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
  const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
  const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
  const std::string& coordsNm,
  const std::string& cellNm)
{
  vtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm, coords));
  vtkm::Id nPts = static_cast<vtkm::Id>(coords.GetNumberOfValues());
  vtkm::cont::CellSetExplicit<> cellSet(cellNm);

  cellSet.Fill(nPts, shapes, numIndices, connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

template <typename T, typename CellShapeTag>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<vtkm::Vec<T, 3>>& coords,
  CellShapeTag tag,
  vtkm::IdComponent numberOfPointsPerCell,
  const std::vector<vtkm::Id>& connectivity,
  const std::string& coordsNm,
  const std::string& cellNm)
{
  vtkm::cont::ArrayHandle<Vec<T, 3>> coordsArray;
  DataSetBuilderExplicit::CopyInto(coords, coordsArray);

  vtkm::cont::ArrayHandle<vtkm::Id> Cc;
  DataSetBuilderExplicit::CopyInto(connectivity, Cc);

  return DataSetBuilderExplicit::Create(
    coordsArray, tag, numberOfPointsPerCell, Cc, coordsNm, cellNm);
}

template <typename T, typename CellShapeTag>
inline VTKM_CONT vtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>>& coords,
  CellShapeTag tag,
  vtkm::IdComponent numberOfPointsPerCell,
  const vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
  const std::string& coordsNm,
  const std::string& cellNm)
{
  (void)tag; //C4100 false positive workaround
  vtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm, coords));
  vtkm::cont::CellSetSingleType<> cellSet(cellNm);

  cellSet.Fill(coords.GetNumberOfValues(), tag.Id, numberOfPointsPerCell, connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

class VTKM_CONT_EXPORT DataSetBuilderExplicitIterative
{
public:
  VTKM_CONT
  DataSetBuilderExplicitIterative();

  VTKM_CONT
  void Begin(const std::string& coordName = "coords", const std::string& cellName = "cells");

  //Define points.
  VTKM_CONT
  vtkm::cont::DataSet Create();

  VTKM_CONT
  vtkm::Id AddPoint(const vtkm::Vec<vtkm::Float32, 3>& pt);

  VTKM_CONT
  vtkm::Id AddPoint(const vtkm::Float32& x, const vtkm::Float32& y, const vtkm::Float32& z = 0);

  template <typename T>
  VTKM_CONT vtkm::Id AddPoint(const T& x, const T& y, const T& z = 0)
  {
    return AddPoint(
      static_cast<vtkm::Float32>(x), static_cast<vtkm::Float32>(y), static_cast<vtkm::Float32>(z));
  }

  template <typename T>
  VTKM_CONT vtkm::Id AddPoint(const vtkm::Vec<T, 3>& pt)
  {
    return AddPoint(static_cast<vtkm::Vec<vtkm::Float32, 3>>(pt));
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
  std::string coordNm, cellNm;

  std::vector<vtkm::Vec<vtkm::Float32, 3>> points;
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numIdx;
  std::vector<vtkm::Id> connectivity;
};
}
}

#endif //vtk_m_cont_DataSetBuilderExplicit_h
