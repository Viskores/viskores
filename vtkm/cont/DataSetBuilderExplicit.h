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

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Assert.h>

namespace vtkm {
namespace cont {

//Coordinates builder??
//Need a singlecellset handler.

class DataSetBuilderExplicit
{
public:
    VTKM_CONT_EXPORT
    DataSetBuilderExplicit() {}

    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const std::vector<T> &xVals,
           const std::vector<T> &yVals,
           const std::vector<vtkm::UInt8> &shapes,
           const std::vector<vtkm::IdComponent> &numIndices,
           const std::vector<vtkm::Id> &connectivity,
           const std::string &coordsNm="coords",
           const std::string &cellNm="cells");

    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const std::vector<T> &xVals,
           const std::vector<T> &yVals,
           const std::vector<T> &zVals,
           const std::vector<vtkm::UInt8> &shapes,
           const std::vector<vtkm::IdComponent> &numIndices,
           const std::vector<vtkm::Id> &connectivity,
           const std::string &coordsNm="coords",
           const std::string &cellNm="cells");

    template<typename T>
    VTKM_CONT_EXPORT
    vtkm::cont::DataSet
    Create(const std::vector<vtkm::Vec<T,3> > &coords,
           const std::vector<vtkm::UInt8> &shapes,
           const std::vector<vtkm::IdComponent> &numIndices,
           const std::vector<vtkm::Id> &connectivity,
           const std::string &coordsNm="coords",
           const std::string &cellNm="cells");

private:
};

template<typename T>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<T> &xVals,
                               const std::vector<T> &yVals,
                               const std::vector<vtkm::UInt8> &shapes,
                               const std::vector<vtkm::IdComponent> &numIndices,
                               const std::vector<vtkm::Id> &connectivity,
                               const std::string &coordsNm,
                               const std::string &cellNm)
{
    VTKM_CONT_ASSERT(xVals.size() == yVals.size() && xVals.size() > 0);

    vtkm::cont::DataSet dataSet;

    typedef vtkm::Vec<vtkm::Float32,3> CoordType;
    std::vector<CoordType> coords(xVals.size());
    
    size_t nPts = xVals.size();
    for (size_t i=0; i < nPts; i++)
    {
        coords[i][0] = xVals[i];
        coords[i][1] = yVals[i];
        coords[i][2] = 0;
    }
    dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem(coordsNm, 1, coords));

    vtkm::cont::CellSetExplicit<> cellSet((vtkm::Id)nPts, cellNm, 2);
    cellSet.FillViaCopy(shapes, numIndices, connectivity);
    dataSet.AddCellSet(cellSet);

    return dataSet;
}

template<typename T>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<T> &xVals,
                               const std::vector<T> &yVals,
                               const std::vector<T> &zVals,
                               const std::vector<vtkm::UInt8> &shapes,
                               const std::vector<vtkm::IdComponent> &numIndices,
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
    
    size_t nPts = xVals.size();
    for (size_t i=0; i < nPts; i++)
    {
        coords[i][0] = xVals[i];
        coords[i][1] = yVals[i];
        coords[i][2] = zVals[i];
    }
    dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem(coordsNm, 1, coords));

    vtkm::cont::CellSetExplicit<> cellSet((vtkm::Id)nPts, cellNm, 3);
    cellSet.FillViaCopy(shapes, numIndices, connectivity);
    dataSet.AddCellSet(cellSet);

    return dataSet;
}

template<typename T>
vtkm::cont::DataSet
DataSetBuilderExplicit::Create(const std::vector<vtkm::Vec<T,3> > &coords,
                               const std::vector<vtkm::UInt8> &shapes,
                               const std::vector<vtkm::IdComponent> &numIndices,
                               const std::vector<vtkm::Id> &connectivity,
                               const std::string &coordsNm,
                               const std::string &cellNm)
{
    vtkm::cont::DataSet dataSet;

    size_t nPts = coords.size();
    dataSet.AddCoordinateSystem(
        vtkm::cont::CoordinateSystem(coordsNm, 1, coords));

    vtkm::cont::CellSetExplicit<> cellSet((vtkm::Id)nPts, cellNm, 3);
    cellSet.FillViaCopy(shapes, numIndices, connectivity);
    dataSet.AddCellSet(cellSet);
    
    return dataSet;
}


}
}

#endif //vtk_m_cont_DataSetBuilderExplicit_h
