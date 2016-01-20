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
#ifndef vtk_m_cont_DataSetBuilderUniform_h
#define vtk_m_cont_DataSetBuilderUniform_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Assert.h>

namespace vtkm {
namespace cont {

class DataSetBuilderUniform
{
public:
    VTKM_CONT_EXPORT
    DataSetBuilderUniform() {}

    //2D uniform grids.
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const vtkm::Id2 &dimensions,
           const vtkm::Vec<vtkm::FloatDefault,2> &origin = vtkm::Vec<vtkm::FloatDefault,2>(0.0f),
           const vtkm::Vec<vtkm::FloatDefault,2> &spacing = vtkm::Vec<vtkm::FloatDefault,2>(1.0f),
           std::string coordNm="coords", std::string cellNm="cells")
    {
        return CreateDS(2,
                        dimensions[0],dimensions[1],1, origin[0],origin[1],0.0f,
                        spacing[0],spacing[1],1.0f,
                        coordNm, cellNm);
    }

    //3D uniform grids.
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const vtkm::Id3 &dimensions,
           const vtkm::Vec<vtkm::FloatDefault,3> &origin = vtkm::Vec<vtkm::FloatDefault,3>(0.0f),
           const vtkm::Vec<vtkm::FloatDefault,3> &spacing = vtkm::Vec<vtkm::FloatDefault,3>(1.0f),
           std::string coordNm="coords", std::string cellNm="cells")
    {
        return CreateDS(3,
                        dimensions[0],dimensions[1],dimensions[2],
                        origin[0],origin[1],origin[2],
                        spacing[0],spacing[1],spacing[2],
                        coordNm, cellNm);
    }

private:
    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    CreateDS(int dim, vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
             T originX, T originY, T originZ,
             T spacingX, T spacingY, T spacingZ,
             std::string coordNm, std::string cellNm)
    {
        VTKM_ASSERT_CONT(nx>1 && ny>1 && ((dim==2 && nz==1)||(dim==3 && nz>=1)));
        VTKM_ASSERT_CONT(spacingX>0 && spacingY>0 && spacingZ>0);
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


#endif //vtk_m_cont_DataSetBuilderUniform_h
