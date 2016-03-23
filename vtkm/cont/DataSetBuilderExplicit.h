//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DataSetBuilderExplicit_h
#define vtk_m_cont_DataSetBuilderExplicit_h

#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm {
namespace cont {

//Coordinates builder??
//Need a singlecellset handler.

class DataSetBuilderExplicit
{
  template<typename T>
  VTKM_CONT_EXPORT
  static
  void CopyInto(const std::vector<T>& input,
                vtkm::cont::ArrayHandle<T>& output )
  {
    output.Allocate( static_cast<vtkm::Id>(input.size()) );
    std::copy( input.begin(), input.end(),
               ArrayPortalToIteratorBegin(output.GetPortalControl()) );
  }
public:
  VTKM_CONT_EXPORT
  DataSetBuilderExplicit() {}

  //Single cell explicits.
  //TODO

  //Zoo explicit cell
  template<typename T>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const std::vector<T> &xVals,
         const std::vector<T> &yVals,
         const std::vector<vtkm::UInt8> &shapes,
         const std::vector<vtkm::IdComponent> &numIndices,
         const std::vector<vtkm::Id> &connectivity,
         int dimensionality=2,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells")
  {
    std::vector<T> zVals(xVals.size(),0);
    return DataSetBuilderExplicit::Create(xVals,yVals,zVals,
                                          shapes,numIndices,connectivity,
                                          dimensionality, coordsNm,cellNm);
  }

  template<typename T>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const std::vector<T> &xVals,
         const std::vector<T> &yVals,
         const std::vector<T> &zVals,
         const std::vector<vtkm::UInt8> &shapes,
         const std::vector<vtkm::IdComponent> &numIndices,
         const std::vector<vtkm::Id> &connectivity,
         int dimensionality=3,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells");

  template<typename T>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const vtkm::cont::ArrayHandle<T> &xVals,
         const vtkm::cont::ArrayHandle<T> &yVals,
         const vtkm::cont::ArrayHandle<T> &zVals,
         const vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes,
         const vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices,
         const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
         int dimensionality=3,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells")
  {
    return DataSetBuilderExplicit::BuildDataSet(
          xVals,yVals,zVals,
          shapes,numIndices,connectivity,
          dimensionality, coordsNm,cellNm);
  }


  template<typename T>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const std::vector<vtkm::Vec<T,3> > &coords,
         const std::vector<vtkm::UInt8> &shapes,
         const std::vector<vtkm::IdComponent> &numIndices,
         const std::vector<vtkm::Id> &connectivity,
         int dimensionality=3,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells");

  template<typename T>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &coords,
         const vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes,
         const vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices,
         const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
         int dimensionality=3,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells")
  {
    return DataSetBuilderExplicit::BuildDataSet(coords,
                                                shapes,
                                                numIndices,
                                                connectivity,
                                                dimensionality,
                                                coordsNm,
                                                cellNm);
  }

  template<typename T, typename CellShapeTag>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const std::vector<vtkm::Vec<T,3> > &coords,
         CellShapeTag tag,
         const std::vector<vtkm::Id> &connectivity,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells");

  template<typename T, typename CellShapeTag>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  Create(const vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &coords,
         CellShapeTag tag,
         const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
         const std::string &coordsNm="coords",
         const std::string &cellNm="cells")
  {
    return DataSetBuilderExplicit::BuildDataSet(coords,
                                                tag,
                                                connectivity,
                                                coordsNm,
                                                cellNm);
  }

private:
  template<typename T>
  static
  vtkm::cont::DataSet
  BuildDataSet(const vtkm::cont::ArrayHandle<T> &X,
               const vtkm::cont::ArrayHandle<T> &Y,
               const vtkm::cont::ArrayHandle<T> &Z,
               const vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes,
               const vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices,
               const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
               int dimensionality,
               const std::string &coordsNm,
               const std::string &cellNm);

  template<typename T>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  BuildDataSet(const vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &coords,
              const vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes,
              const vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices,
              const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
              int dimensionality,
              const std::string &coordsNm,
              const std::string &cellNm);

  template<typename T, typename CellShapeTag>
  VTKM_CONT_EXPORT
  static
  vtkm::cont::DataSet
  BuildDataSet(const vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &coords,
               CellShapeTag tag,
               const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
               const std::string &coordsNm,
               const std::string &cellNm);
};

template<typename T>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<T> &xVals,
                               const std::vector<T> &yVals,
                               const std::vector<T> &zVals,
                               const std::vector<vtkm::UInt8> &shapes,
                               const std::vector<vtkm::IdComponent> &numIndices,
                               const std::vector<vtkm::Id> &connectivity,
                               int dimensionality,
                               const std::string &coordsNm,
                               const std::string &cellNm)
{
  VTKM_ASSERT_CONT(xVals.size() == yVals.size() &&
                   yVals.size() == zVals.size() &&
                   xVals.size() > 0);

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

  return DataSetBuilderExplicit::BuildDataSet(
        Xc,Yc,Zc, Sc,Nc,Cc, dimensionality, coordsNm, cellNm);
}

template<typename T>
vtkm::cont::DataSet
DataSetBuilderExplicit::BuildDataSet(
    const vtkm::cont::ArrayHandle<T> &X,
    const vtkm::cont::ArrayHandle<T> &Y,
    const vtkm::cont::ArrayHandle<T> &Z,
    const vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes,
    const vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices,
    const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
    int dimensionality,
    const std::string &coordsNm,
    const std::string &cellNm)
{
  VTKM_ASSERT_CONT(X.GetNumberOfValues() == Y.GetNumberOfValues() &&
                  Y.GetNumberOfValues() == Z.GetNumberOfValues() &&
                  X.GetNumberOfValues() > 0 &&
                  shapes.GetNumberOfValues() == numIndices.GetNumberOfValues());

  vtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem(coordsNm,
      make_ArrayHandleCompositeVector(X,0, Y,0, Z,0)));
  vtkm::Id nPts = X.GetNumberOfValues();
  vtkm::cont::CellSetExplicit<> cellSet(nPts, cellNm, dimensionality);

  cellSet.Fill(shapes, numIndices, connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

template<typename T>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<vtkm::Vec<T,3> > &coords,
                               const std::vector<vtkm::UInt8> &shapes,
                               const std::vector<vtkm::IdComponent> &numIndices,
                               const std::vector<vtkm::Id> &connectivity,
                               int dimensionality,
                               const std::string &coordsNm,
                               const std::string &cellNm)
{
  vtkm::cont::ArrayHandle<Vec<T,3> > coordsArray;
  DataSetBuilderExplicit::CopyInto(coords, coordsArray);

  vtkm::cont::ArrayHandle<vtkm::UInt8> Sc;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> Nc;
  vtkm::cont::ArrayHandle<vtkm::Id> Cc;
  DataSetBuilderExplicit::CopyInto(shapes, Sc);
  DataSetBuilderExplicit::CopyInto(numIndices, Nc);
  DataSetBuilderExplicit::CopyInto(connectivity, Cc);

  return DataSetBuilderExplicit::Create(
        coordsArray, Sc, Nc, Cc, dimensionality, coordsNm, cellNm);
}

template<typename T>
VTKM_CONT_EXPORT
vtkm::cont::DataSet
DataSetBuilderExplicit::BuildDataSet(const vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &coords,
                     const vtkm::cont::ArrayHandle<vtkm::UInt8> &shapes,
                     const vtkm::cont::ArrayHandle<vtkm::IdComponent> &numIndices,
                     const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
                     int dimensionality,
                     const std::string &coordsNm,
                     const std::string &cellNm)
{
  vtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm,
                               coords));
  vtkm::Id nPts = static_cast<vtkm::Id>(coords.GetNumberOfValues());
  vtkm::cont::CellSetExplicit<> cellSet(nPts, cellNm, dimensionality);

  cellSet.Fill(shapes, numIndices, connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

template<typename T, typename CellShapeTag>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<vtkm::Vec<T,3> > &coords,
                               CellShapeTag tag,
                               const std::vector<vtkm::Id> &connectivity,
                               const std::string &coordsNm,
                               const std::string &cellNm)
{
  vtkm::cont::ArrayHandle<Vec<T,3> > coordsArray;
  DataSetBuilderExplicit::CopyInto(coords, coordsArray);

  vtkm::cont::ArrayHandle<vtkm::Id> Cc;
  DataSetBuilderExplicit::CopyInto(connectivity, Cc);

  return DataSetBuilderExplicit::Create(coordsArray, tag, Cc, coordsNm, cellNm);
}

template<typename T, typename CellShapeTag>
VTKM_CONT_EXPORT
vtkm::cont::DataSet
DataSetBuilderExplicit::BuildDataSet(const vtkm::cont::ArrayHandle<vtkm::Vec<T,3> > &coords,
                                     CellShapeTag tag,
                                     const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity,
                                     const std::string &coordsNm,
                                     const std::string &cellNm)
{
  vtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordsNm, coords));
  vtkm::cont::CellSetSingleType<> cellSet(tag, cellNm);

  cellSet.Fill(connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}


class DataSetBuilderExplicitIterative
{
public:
  VTKM_CONT_EXPORT
  DataSetBuilderExplicitIterative() {}

  VTKM_CONT_EXPORT
  void Begin(int dim,
             const std::string &coordName="coords",
             const std::string &cellName="cells")
  {
    this->dimensionality = dim;
    this->coordNm = coordName;
    this->cellNm = cellName;
    this->points.resize(0);
    this->shapes.resize(0);
    this->numIdx.resize(0);
    this->connectivity.resize(0);
  }

  //Define points.
  VTKM_CONT_EXPORT
  vtkm::cont::DataSet Create();

  VTKM_CONT_EXPORT
  vtkm::Id AddPoint(const vtkm::Vec<vtkm::Float32, 3> &pt)
  {
    points.push_back(pt);
    vtkm::Id id = static_cast<vtkm::Id>(points.size());
    return id;
  }

  VTKM_CONT_EXPORT
  vtkm::Id AddPoint(const vtkm::Float32 &x,
        const vtkm::Float32 &y,
        const vtkm::Float32 &z=0)
  {
    points.push_back(vtkm::make_Vec(x,y,z));
    vtkm::Id id = static_cast<vtkm::Id>(points.size());
    return id;
  }

  template<typename T>
  VTKM_CONT_EXPORT
  vtkm::Id AddPoint(const T &x, const T &y, const T &z=0)
  {
    return AddPoint(static_cast<vtkm::Float32>(x),
      static_cast<vtkm::Float32>(y),
      static_cast<vtkm::Float32>(z));
  }

  template<typename T>
  VTKM_CONT_EXPORT
  vtkm::Id AddPoint(const vtkm::Vec<T,3> &pt)
  {
    return AddPoint(static_cast<vtkm::Vec<vtkm::Float32,3> >(pt));
  }

  //Define cells.
  VTKM_CONT_EXPORT
  void AddCell(vtkm::UInt8 shape)
  {
    this->shapes.push_back(shape);
    this->numIdx.push_back(0);
  }

  VTKM_CONT_EXPORT
  void AddCell(const vtkm::UInt8 &shape, const std::vector<vtkm::Id> &conn)
  {
    this->shapes.push_back(shape);
    this->numIdx.push_back(static_cast<vtkm::IdComponent>(conn.size()));
    connectivity.insert(connectivity.end(), conn.begin(), conn.end());
  }

  VTKM_CONT_EXPORT
  void AddCell(const vtkm::UInt8 &shape, const vtkm::Id *conn, const vtkm::IdComponent &n)
  {
    this->shapes.push_back(shape);
    this->numIdx.push_back(n);
    for (int i = 0; i < n; i++)
      {
        connectivity.push_back(conn[i]);
      }
  }

  VTKM_CONT_EXPORT
  void AddCellPoint(vtkm::Id pointIndex)
  {
    VTKM_ASSERT_CONT(this->numIdx.size() > 0);
    this->connectivity.push_back(pointIndex);
    this->numIdx.back() += 1;
  }

private:
  std::string coordNm, cellNm;
  int dimensionality;

  std::vector<vtkm::Vec<vtkm::Float32,3> > points;
  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numIdx;
  std::vector<vtkm::Id> connectivity;
};

vtkm::cont::DataSet
DataSetBuilderExplicitIterative::Create()
{
  DataSetBuilderExplicit dsb;
  return dsb.Create(points, shapes, numIdx, connectivity, dimensionality, coordNm, cellNm);
}



#if 0
template<typename T, typename CellType>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<T> &xVals,
             const std::vector<T> &yVals,
             const std::vector<vtkm::Id> &connectivity,
             const std::string &coordsNm,
             const std::string &cellNm)
{
  VTKM_CONT_ASSERT(xVals.size() == yVals.size() && xVals.size() > 0);
  vtkm::cont::DataSet dataSet;

  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(xVals.size());

  for (size_t i=0; i < coords.size(); i++)
  {
    coords[i][0] = xVals[i];
    coords[i][1] = yVals[i];
    coords[i][2] = 0;
  }
  dataSet.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem(coordsNm, coords));

  vtkm::cont::CellSetSingleType< > cellSet(CellType(), cellNm);
  cellSet.FillViaCopy(connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

template<typename T, typename CellType>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<T> &xVals,
             const std::vector<T> &yVals,
             const std::vector<T> &zVals,
             const std::vector<vtkm::Id> &connectivity,
             const std::string &coordsNm,
             const std::string &cellNm)
{
  VTKM_CONT_ASSERT(xVals.size() == yVals.size() &&
                   yVals.size() == zVals.size() &&
                   xVals.size() > 0);
  vtkm::cont::DataSet dataSet;

  typedef vtkm::Vec<vtkm::Float32,3> CoordType;
  std::vector<CoordType> coords(xVals.size());

  vtkm::Id nPts = static_cast<vtkm::Id>(coords.size());
  for (vtkm::Id i=0; i < nPts; i++)
  {
    coords[i][0] = xVals[i];
    coords[i][1] = yVals[i];
    coords[i][2] = zVals[i];
  }
  dataSet.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem(coordsNm, coords));

  vtkm::cont::CellSetSingleType< > cellSet(CellType(), cellNm);
  cellSet.FillViaCopy(connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}

template<typename T, typename CellType>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<vtkm::Vec<T,3> > &coords,
                               const std::vector<vtkm::Id> &connectivity,
                               const std::string &coordsNm,
                               const std::string &cellNm)
{
  vtkm::cont::DataSet dataSet;

  vtkm::cont::ArrayHandle<Vec<T,3> > coordsArray;
  CopyInto(coords, coordsArray);

  dataSet.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem(coordsNm, coordsArray));

  vtkm::cont::CellSetSingleType< > cellSet(CellType(), cellNm);
  cellSet.FillViaCopy(connectivity);
  dataSet.AddCellSet(cellSet);

  return dataSet;
}
#endif

}
}

#endif //vtk_m_cont_DataSetBuilderExplicit_h
