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

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm {
namespace cont {

class DataSetBuilderUniform
{
public:
    VTKM_CONT_EXPORT
    DataSetBuilderUniform() {}

    //1D uniform grid
    template <typename T>
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    Create(const vtkm::Id &dimension,
           const T &origin, const T &spacing,
           std::string coordNm="coords", std::string cellNm="cells")
    {
      vtkm::Vec<T,3> origin3d(origin, 0, 0);
      vtkm::Vec<T,3> spacing3d(spacing, 1, 1);
      return DataSetBuilderUniform::CreateDS(1,
                                             dimension,1,1,
                                             origin3d,
                                             spacing3d,
                                             coordNm, cellNm);
    }
    
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    Create(const vtkm::Id &dimension,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        vtkm::FloatDefault origin(0), spacing(1);
        return Create(dimension, origin, spacing, coordNm, cellNm);
    }

    //2D uniform grids.
    template <typename T>
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    Create(const vtkm::Id2 &dimensions,
           const vtkm::Vec<T,2> &origin, const vtkm::Vec<T,2> &spacing,
           std::string coordNm="coords", std::string cellNm="cells")
    {
      vtkm::Vec<T,3> origin3d(origin[0], origin[1], 0);
      vtkm::Vec<T,3> spacing3d(spacing[0], spacing[1], 1);
      return DataSetBuilderUniform::CreateDS(2,
                                             dimensions[0],dimensions[1],1,
                                             origin3d,
                                             spacing3d,
                                             coordNm, cellNm);
    }
    
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    Create(const vtkm::Id2 &dimensions,
           std::string coordNm="coords", std::string cellNm="cells")
    {
        vtkm::Vec<vtkm::FloatDefault,2> origin(0), spacing(1);
        return Create(dimensions, origin, spacing, coordNm, cellNm);
    }

    //3D uniform grids.
    template <typename T>    
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    Create(const vtkm::Id3 &dimensions,
           const vtkm::Vec<T,3> &origin, const vtkm::Vec<T,3> &spacing,
           std::string coordNm="coords", std::string cellNm="cells")
    {
      return DataSetBuilderUniform::CreateDS(
            3,
            dimensions[0],dimensions[1],dimensions[2],
            origin,
            spacing,
            coordNm, cellNm);
    }
    
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    Create(const vtkm::Id3 &dimensions,
           std::string coordNm="coords", std::string cellNm="cells")
    {
      vtkm::Vec<vtkm::FloatDefault,3> origin(0), spacing(1);
      return Create(dimensions, origin, spacing, coordNm, cellNm);
    }    

private:
    VTKM_CONT_EXPORT
    static
    vtkm::cont::DataSet
    CreateDS(int dim, vtkm::Id nx, vtkm::Id ny, vtkm::Id nz,
             const vtkm::Vec<vtkm::FloatDefault,3> &origin,
             const vtkm::Vec<vtkm::FloatDefault,3> &spacing,
             std::string coordNm, std::string cellNm)
    {
        VTKM_ASSERT((dim==1 && nx>1 && ny==1 && nz==1) ||
                    (dim==2 && nx>1 && ny>1 && nz==1) ||
                    (dim==3 && nx>1 && ny>1 && nz>1));
        VTKM_ASSERT(spacing[0]>0 && spacing[1]>0 && spacing[2]>0);
        
        vtkm::cont::DataSet dataSet;
        vtkm::cont::ArrayHandleUniformPointCoordinates
            coords(vtkm::Id3(nx, ny, nz),
                   origin, spacing);
        vtkm::cont::CoordinateSystem cs(coordNm, coords);
        dataSet.AddCoordinateSystem(cs);

        if (dim == 1)
        {
            vtkm::cont::CellSetStructured<1> cellSet(cellNm);
            cellSet.SetPointDimensions(nx);
            dataSet.AddCellSet(cellSet);
        }
        else if (dim == 2)
        {
            vtkm::cont::CellSetStructured<2> cellSet(cellNm);
            cellSet.SetPointDimensions(vtkm::make_Vec(nx,ny));
            dataSet.AddCellSet(cellSet);
        }
        else if (dim == 3)
        {
            vtkm::cont::CellSetStructured<3> cellSet(cellNm);
            cellSet.SetPointDimensions(vtkm::make_Vec(nx,ny,nz));
            dataSet.AddCellSet(cellSet);
        }
        else
            throw vtkm::cont::ErrorControlBadValue("Invalid cell set dimension");

        return dataSet;
    }

};

} // namespace cont
} // namespace vtkm


#endif //vtk_m_cont_DataSetBuilderUniform_h
