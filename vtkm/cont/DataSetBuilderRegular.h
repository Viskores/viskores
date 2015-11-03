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
#ifndef vtk_m_cont_DataSetBuilderRegular_h
#define vtk_m_cont_DataSetBuilderRegular_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Assert.h>

namespace vtkm {
namespace cont {

class DataSetBuilderRegular
{
public:
    VTKM_CONT_EXPORT
    DataSetBuilderRegular() {}

    //2D regular grids.
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(vtkm::Id nx, vtkm::Id ny,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        return Create(2, nx, ny, 1, 0,0,0, 1,1,1, coordNm, cellNm);
    }

    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(vtkm::Id nx, vtkm::Id ny,
           T originX, T originY, T spacingX, T spacingY,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        Create(2, nx,ny,1, originX,originY,0,
               spacingX,spacingY,1,
               coordNm, cellNm);
    }

    //3D regular grids.
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        return Create(3, nx, ny, nz, 0,0,0, 1,1,1, coordNm, cellNm);
    }

    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
           T originX, T originY, T originZ, T spacingX, T spacingY, T spacingZ,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        return Create(3, nx,ny,nz, originX,originY,originZ,
                      spacingX,spacingY,spacingZ,
                      coordNm, cellNm);
    }

private:
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(int dim, vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
           T originX, T originY, T originZ, T spacingX, T spacingY, T spacingZ,
           std::string coordNm, std::string cellNm)
    {
        VTKM_ASSERT_CONT(nx>1 && ny>1 && ((dim==2 && nz==1)||(dim==3 && nz>1)));
        vtkm::cont::DataSet dataSet;

        vtkm::cont::ArrayHandleUniformPointCoordinates
            coords(vtkm::Id3(nx, ny, nz),
                   vtkm::Vec<T,3>(originX, originY,originZ),
                   vtkm::Vec<T,3>(spacingX, spacingY,spacingZ));
    
        vtkm::cont::CoordinateSystem cs(coordNm, 1, coords);
        dataSet.AddCoordinateSystem(cs);
    
        if (dim == 2)
        {
            vtkm::cont::CellSetStructured<2> cellSet(cellNm);
            cellSet.SetPointDimensions(vtkm::make_Vec(nx,ny));
            dataSet.AddCellSet(cellSet);
        }
        else
        {
            vtkm::cont::CellSetStructured<3> cellSet(cellNm);
            cellSet.SetPointDimensions(vtkm::make_Vec(nx,ny,nz));
            dataSet.AddCellSet(cellSet);
        }

        return dataSet;
    }

};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_DataSetBuilderRegular_h
