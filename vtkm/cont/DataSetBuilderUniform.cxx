//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderUniform.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT
DataSetBuilderUniform::DataSetBuilderUniform()
{
}

VTKM_CONT
vtkm::cont::DataSet DataSetBuilderUniform::Create(const vtkm::Id& dimension,
                                                  std::string coordNm,
                                                  std::string cellNm)
{
  return CreateDataSet(vtkm::Id3(dimension, 1, 1), VecType(0), VecType(1), coordNm, cellNm);
}

VTKM_CONT
vtkm::cont::DataSet DataSetBuilderUniform::Create(const vtkm::Id2& dimensions,
                                                  std::string coordNm,
                                                  std::string cellNm)
{
  return CreateDataSet(
    vtkm::Id3(dimensions[0], dimensions[1], 1), VecType(0), VecType(1), coordNm, cellNm);
}

VTKM_CONT
vtkm::cont::DataSet DataSetBuilderUniform::Create(const vtkm::Id3& dimensions,
                                                  std::string coordNm,
                                                  std::string cellNm)
{
  return CreateDataSet(vtkm::Id3(dimensions[0], dimensions[1], dimensions[2]),
                       VecType(0),
                       VecType(1),
                       coordNm,
                       cellNm);
}

VTKM_CONT
vtkm::cont::DataSet DataSetBuilderUniform::CreateDataSet(
  const vtkm::Id3& dimensions,
  const vtkm::Vec<vtkm::FloatDefault, 3>& origin,
  const vtkm::Vec<vtkm::FloatDefault, 3>& spacing,
  std::string coordNm,
  std::string cellNm)
{
  vtkm::Id dims[3] = { 1, 1, 1 };
  int ndims = 0;
  for (int i = 0; i < 3; ++i)
  {
    if (dimensions[i] > 1)
    {
      if (spacing[i] <= 0.0f)
      {
        throw vtkm::cont::ErrorBadValue("spacing must be > 0.0");
      }
      dims[ndims++] = dimensions[i];
    }
  }

  vtkm::cont::DataSet dataSet;
  vtkm::cont::ArrayHandleUniformPointCoordinates coords(dimensions, origin, spacing);
  vtkm::cont::CoordinateSystem cs(coordNm, coords);
  dataSet.AddCoordinateSystem(cs);

  if (ndims == 1)
  {
    vtkm::cont::CellSetStructured<1> cellSet(cellNm);
    cellSet.SetPointDimensions(dims[0]);
    dataSet.AddCellSet(cellSet);
  }
  else if (ndims == 2)
  {
    vtkm::cont::CellSetStructured<2> cellSet(cellNm);
    cellSet.SetPointDimensions(vtkm::Id2(dims[0], dims[1]));
    dataSet.AddCellSet(cellSet);
  }
  else if (ndims == 3)
  {
    vtkm::cont::CellSetStructured<3> cellSet(cellNm);
    cellSet.SetPointDimensions(vtkm::Id3(dims[0], dims[1], dims[2]));
    dataSet.AddCellSet(cellSet);
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("Invalid cell set dimension");
  }

  return dataSet;
}
}
} // end namespace vtkm::cont
