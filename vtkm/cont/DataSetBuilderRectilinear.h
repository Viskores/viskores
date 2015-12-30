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

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Assert.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>

namespace vtkm {
namespace cont {

typedef vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> DFA;

class DataSetBuilderRectilinear
{
public:
    VTKM_CONT_EXPORT
    DataSetBuilderRectilinear() {}

    //2D grids.
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(vtkm::Id nx, vtkm::Id ny,
	   T *xvals, T *yvals,
           std::string coordNm="coords", std::string cellNm="cells")
    {
	T zvals = 0;
	return Create(2, nx,ny, 1, xvals, yvals, &zvals, coordNm, cellNm);
    }
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const std::vector<T> &xvals, const std::vector<T> &yvals,
           std::string coordNm="coords", std::string cellNm="cells")
    {
	std::vector<T> zvals(1,0);
	return Create(2, xvals, yvals, zvals, coordNm, cellNm);
    }
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const vtkm::cont::ArrayHandle<T> &xvals,
	   const vtkm::cont::ArrayHandle<T> &yvals,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        VTKM_ASSERT_CONT(xvals.GetNumberOfValues()>1 && yvals.GetNumberOfValues()>1);

	vtkm::cont::ArrayHandle<T> zvals;
	DFA::Copy(vtkm::cont::make_ArrayHandle(std::vector<T>(1,0)), zvals);
	return BuildDataSet(2, xvals,yvals,zvals, coordNm, cellNm);
    }

    //3D grids.
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
	   T *xvals, T *yvals, T *zvals,
           std::string coordNm="coords", std::string cellNm="cells")
    {
	return Create(3, nx,ny,nz, xvals, yvals, zvals, coordNm, cellNm);
    }
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const std::vector<T> &xvals,
	   const std::vector<T> &yvals, 
	   const std::vector<T> &zvals,
           std::string coordNm="coords", std::string cellNm="cells")
    {
	return Create(3, xvals, yvals, zvals, coordNm, cellNm);
    }
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const vtkm::cont::ArrayHandle<T> &xvals,
	   const vtkm::cont::ArrayHandle<T> &yvals,
	   const vtkm::cont::ArrayHandle<T> &zvals,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        VTKM_ASSERT_CONT(xvals.GetNumberOfValues()>1 &&
			 yvals.GetNumberOfValues()>1 &&
			 zvals.GetNumberOfValues()>1);
	return BuildDataSet(3, xvals,yvals,zvals, coordNm, cellNm);
    }

private:
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(int dim, vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
           T *xvals, T *yvals, T *zvals,
           std::string coordNm, std::string cellNm)
    {
        VTKM_ASSERT_CONT(nx>1 && ny>1 &&
			 ((dim==2 && nz==1)||(dim==3 && nz>=1)));
	
	vtkm::cont::ArrayHandle<T> Xc, Yc, Zc;
	DFA::Copy(vtkm::cont::make_ArrayHandle(xvals,nx), Xc);
	DFA::Copy(vtkm::cont::make_ArrayHandle(yvals,ny), Yc);
	DFA::Copy(vtkm::cont::make_ArrayHandle(zvals,nz), Zc);
	
	return BuildDataSet(dim, Xc,Yc,Zc, coordNm, cellNm);
    }
    
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(int dim,
	   const std::vector<T> &xvals,
	   const std::vector<T> &yvals,
	   const std::vector<T> &zvals,
           std::string coordNm, std::string cellNm)
    {
        VTKM_ASSERT_CONT(xvals.size()>1 && yvals.size()>1 &&
			 ((dim==2 && zvals.size()==1)||(dim==3 && zvals.size()>=1)));

	vtkm::cont::ArrayHandle<T> Xc, Yc, Zc;
	DFA::Copy(vtkm::cont::make_ArrayHandle(xvals), Xc);
	DFA::Copy(vtkm::cont::make_ArrayHandle(yvals), Yc);
	DFA::Copy(vtkm::cont::make_ArrayHandle(zvals), Zc);
	
	return BuildDataSet(dim, Xc,Yc,Zc, coordNm, cellNm);
    }

    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    BuildDataSet(int dim,
		 const vtkm::cont::ArrayHandle<T> &X,
		 const vtkm::cont::ArrayHandle<T> &Y,
		 const vtkm::cont::ArrayHandle<T> &Z,
		 std::string coordNm, std::string cellNm)
    {
        vtkm::cont::DataSet dataSet;

	vtkm::cont::ArrayHandleCartesianProduct<
	    vtkm::cont::ArrayHandle<T>,
	    vtkm::cont::ArrayHandle<T>,
	    vtkm::cont::ArrayHandle<T> > coords;

	vtkm::cont::ArrayHandle<T> Xc, Yc, Zc;
	DFA::Copy(X, Xc);
	DFA::Copy(Y, Yc);
	DFA::Copy(Z, Zc);
	
	coords = vtkm::cont::make_ArrayHandleCartesianProduct(Xc,Yc,Zc);
        vtkm::cont::CoordinateSystem cs(coordNm, 1, coords);
        dataSet.AddCoordinateSystem(cs);
    
        if (dim == 2)
        {
            vtkm::cont::CellSetStructured<2> cellSet(cellNm);
            cellSet.SetPointDimensions(vtkm::make_Vec(Xc.GetNumberOfValues(),
						      Yc.GetNumberOfValues()));
            dataSet.AddCellSet(cellSet);
        }
        else
        {
            vtkm::cont::CellSetStructured<3> cellSet(cellNm);
            cellSet.SetPointDimensions(vtkm::make_Vec(Xc.GetNumberOfValues(),
						      Yc.GetNumberOfValues(),
						      Zc.GetNumberOfValues()));
            dataSet.AddCellSet(cellSet);
        }

        return dataSet;
	
    }
};

} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_DataSetBuilderRectilinear_h
