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

class DataSetIterativeBuilderExplicit
{
public:
    VTKM_CONT_EXPORT
    DataSetIterativeBuilderExplicit() {}

    VTKM_CONT_EXPORT
    void Begin(const std::string &_coordNm="coords",
	       const std::string &_cellNm="cells")
    {
	coordNm = _coordNm;
	cellNm = _cellNm;
        points.resize(0);
        shapes.resize(0);
        numIdx.resize(0);
        connectivity.resize(0);
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
    void AddCell(const vtkm::UInt8 &shape, const std::vector<vtkm::Id> &conn)
    {
	shapes.push_back(shape);
	numIdx.push_back(static_cast<vtkm::IdComponent>(conn.size()));
	connectivity.insert(connectivity.end(), conn.begin(), conn.end());
    }

    VTKM_CONT_EXPORT
    void AddCell(const vtkm::UInt8 &shape, const vtkm::Id *conn, const vtkm::IdComponent &n)
    {
        shapes.push_back(shape);
        numIdx.push_back(n);
        for (int i = 0; i < n; i++)
            connectivity.push_back(conn[i]);
    }
    
private:
    std::string coordNm, cellNm;

    std::vector<vtkm::Vec<vtkm::Float32,3> > points;
    std::vector<vtkm::UInt8> shapes;
    std::vector<vtkm::IdComponent> numIdx;
    std::vector<vtkm::Id> connectivity;
};

vtkm::cont::DataSet
DataSetIterativeBuilderExplicit::Create()
{
    DataSetBuilderExplicit dsb;
    return dsb.Create(points, shapes, numIdx, connectivity, coordNm, cellNm);
}


}
}

#endif //vtk_m_cont_DataSetBuilderExplicit_h
