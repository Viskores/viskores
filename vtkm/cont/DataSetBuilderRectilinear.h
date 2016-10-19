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
#ifndef vtk_m_cont_DataSetBuilderRectilinear_h
#define vtk_m_cont_DataSetBuilderRectilinear_h

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

namespace vtkm {
namespace cont {

class DataSetBuilderRectilinear
{
  template<typename T, typename U>
  VTKM_CONT
  static
  void CopyInto(const std::vector<T>& input,
                vtkm::cont::ArrayHandle<U>& output )
  {
    DataSetBuilderRectilinear::CopyInto(
      vtkm::cont::make_ArrayHandle(input), output);
  }

  template<typename T, typename U>
  VTKM_CONT
  static
  void CopyInto(const vtkm::cont::ArrayHandle<T>& input,
                vtkm::cont::ArrayHandle<U>& output )
  {
    typedef vtkm::cont::DeviceAdapterAlgorithm<
        vtkm::cont::DeviceAdapterTagSerial> Algorithm;
    Algorithm::Copy(input, output);
  }

  template<typename T, typename U>
  VTKM_CONT
  static
  void CopyInto(const T* input, vtkm::Id len,
                vtkm::cont::ArrayHandle<U>& output )
  {
    DataSetBuilderRectilinear::CopyInto(
      vtkm::cont::make_ArrayHandle(input, len), output);
  }
public:
  VTKM_CONT
  DataSetBuilderRectilinear() {}

  //1D grids.
  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(const std::vector<T> &xvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    std::vector<T> yvals(1,0), zvals(1,0);
    return DataSetBuilderRectilinear::BuildDataSet(
          1, xvals,yvals,zvals, coordNm,cellNm);
  }

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(vtkm::Id nx, T *xvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    T yvals = 0, zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(
          1, nx,1, 1, xvals, &yvals, &zvals, coordNm, cellNm);
  }

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(const vtkm::cont::ArrayHandle<T> &xvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    vtkm::cont::ArrayHandle<T> yvals, zvals;
    yvals.Allocate(1);
    yvals.GetPortalControl().Set(0,0.0);
    zvals.Allocate(1);
    zvals.GetPortalControl().Set(0,0.0);
    return DataSetBuilderRectilinear::BuildDataSet(
          1, xvals,yvals,zvals, coordNm, cellNm);
  }

  //2D grids.
  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(const std::vector<T> &xvals, const std::vector<T> &yvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    std::vector<T> zvals(1,0);
    return DataSetBuilderRectilinear::BuildDataSet(
          2, xvals,yvals,zvals, coordNm,cellNm);
  }

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(vtkm::Id nx, vtkm::Id ny,
         T *xvals, T *yvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    T zvals = 0;
    return DataSetBuilderRectilinear::BuildDataSet(
          2, nx,ny, 1, xvals, yvals, &zvals, coordNm, cellNm);
  }
  
  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(const vtkm::cont::ArrayHandle<T> &xvals,
         const vtkm::cont::ArrayHandle<T> &yvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    vtkm::cont::ArrayHandle<T> zvals;
    zvals.Allocate(1);
    zvals.GetPortalControl().Set(0,0.0);
    return DataSetBuilderRectilinear::BuildDataSet(
          2, xvals,yvals,zvals, coordNm, cellNm);
  }

  //3D grids.
  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
         T *xvals, T *yvals, T *zvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    return DataSetBuilderRectilinear::BuildDataSet(
          3, nx,ny,nz, xvals, yvals, zvals, coordNm, cellNm);
  }

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(const std::vector<T> &xvals,
         const std::vector<T> &yvals,
         const std::vector<T> &zvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    return DataSetBuilderRectilinear::BuildDataSet(
          3, xvals, yvals, zvals, coordNm, cellNm);
  }

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  Create(const vtkm::cont::ArrayHandle<T> &xvals,
         const vtkm::cont::ArrayHandle<T> &yvals,
         const vtkm::cont::ArrayHandle<T> &zvals,
         std::string coordNm="coords", std::string cellNm="cells")
  {
    return DataSetBuilderRectilinear::BuildDataSet(
          3, xvals,yvals,zvals, coordNm, cellNm);
  }

private:
  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  BuildDataSet(int dim,
               const std::vector<T> &xvals,
               const std::vector<T> &yvals,
               const std::vector<T> &zvals,
               std::string coordNm, std::string cellNm)
  {
    VTKM_ASSERT((dim==1 && xvals.size()>1 && yvals.size()==1 && zvals.size()==1) ||
                (dim==2 && xvals.size()>1 && yvals.size()>1 && zvals.size()==1) ||
                (dim==3 && xvals.size()>1 && yvals.size()>1 && zvals.size()>1));

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(
          dim, Xc,Yc,Zc, coordNm, cellNm);
  }

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  BuildDataSet(int dim, vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
               const T *xvals, const T *yvals, const T *zvals,
               std::string coordNm, std::string cellNm)
  {
    VTKM_ASSERT((dim==1 && nx>1 && ny==1 && nz==1) ||
                (dim==2 && nx>1 && ny>1 && nz==1) ||
                (dim==3 && nx>1 && ny>1 && nz>1));

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(xvals, nx, Xc);
    DataSetBuilderRectilinear::CopyInto(yvals, ny, Yc);
    DataSetBuilderRectilinear::CopyInto(zvals, nz, Zc);

    return DataSetBuilderRectilinear::BuildDataSet(
          dim, Xc,Yc,Zc, coordNm, cellNm);
  }    

  template<typename T>
  VTKM_CONT
  static
  vtkm::cont::DataSet
  BuildDataSet(int dim,
               const vtkm::cont::ArrayHandle<T> &X,
               const vtkm::cont::ArrayHandle<T> &Y,
               const vtkm::cont::ArrayHandle<T> &Z,
               std::string coordNm, std::string cellNm)
  {
    vtkm::cont::DataSet dataSet;

    //Convert all coordinates to floatDefault.
    vtkm::cont::ArrayHandleCartesianProduct<
        vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
        vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
        vtkm::cont::ArrayHandle<vtkm::FloatDefault> > coords;

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    DataSetBuilderRectilinear::CopyInto(X, Xc);
    DataSetBuilderRectilinear::CopyInto(Y, Yc);
    DataSetBuilderRectilinear::CopyInto(Z, Zc);

    coords = vtkm::cont::make_ArrayHandleCartesianProduct(Xc,Yc,Zc);
    vtkm::cont::CoordinateSystem cs(coordNm, coords);
    dataSet.AddCoordinateSystem(cs);

    if (dim == 1)
    {
      vtkm::cont::CellSetStructured<1> cellSet(cellNm);
      cellSet.SetPointDimensions(Xc.GetNumberOfValues());
      dataSet.AddCellSet(cellSet);        
    }
    else if (dim == 2)
    {
      vtkm::cont::CellSetStructured<2> cellSet(cellNm);
      cellSet.SetPointDimensions(vtkm::make_Vec(Xc.GetNumberOfValues(),
                                                Yc.GetNumberOfValues()));
      dataSet.AddCellSet(cellSet);
    }
    else if (dim == 3)
    {
      vtkm::cont::CellSetStructured<3> cellSet(cellNm);
      cellSet.SetPointDimensions(vtkm::make_Vec(Xc.GetNumberOfValues(),
                                                Yc.GetNumberOfValues(),
                                                Zc.GetNumberOfValues()));
      dataSet.AddCellSet(cellSet);
    }
    else
        throw vtkm::cont::ErrorControlBadValue("Invalid cell set dimension");

    return dataSet;
  }
};

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_DataSetBuilderRectilinear_h
