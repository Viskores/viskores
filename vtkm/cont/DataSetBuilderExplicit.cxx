//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DataSetBuilderExplicit.h>

namespace vtkm
{
namespace cont
{

VTKM_CONT
DataSetBuilderExplicitIterative::DataSetBuilderExplicitIterative()
{
}


VTKM_CONT
void DataSetBuilderExplicitIterative::Begin(const std::string& coordName,
                                            const std::string& cellName)
{
  this->coordNm = coordName;
  this->cellNm = cellName;
  this->points.resize(0);
  this->shapes.resize(0);
  this->numIdx.resize(0);
  this->connectivity.resize(0);
}

//Define points.
VTKM_CONT
vtkm::cont::DataSet DataSetBuilderExplicitIterative::Create()
{
  DataSetBuilderExplicit dsb;
  return dsb.Create(points, shapes, numIdx, connectivity, coordNm, cellNm);
}

VTKM_CONT
vtkm::Id DataSetBuilderExplicitIterative::AddPoint(const vtkm::Vec<vtkm::Float32, 3>& pt)
{
  points.push_back(pt);
  vtkm::Id id = static_cast<vtkm::Id>(points.size());
  return id;
}

VTKM_CONT
vtkm::Id DataSetBuilderExplicitIterative::AddPoint(const vtkm::Float32& x,
                                                   const vtkm::Float32& y,
                                                   const vtkm::Float32& z)
{
  points.push_back(vtkm::make_Vec(x, y, z));
  vtkm::Id id = static_cast<vtkm::Id>(points.size());
  return id;
}

//Define cells.
VTKM_CONT
void DataSetBuilderExplicitIterative::AddCell(vtkm::UInt8 shape)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(0);
}

VTKM_CONT
void DataSetBuilderExplicitIterative::AddCell(const vtkm::UInt8& shape,
                                              const std::vector<vtkm::Id>& conn)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(static_cast<vtkm::IdComponent>(conn.size()));
  connectivity.insert(connectivity.end(), conn.begin(), conn.end());
}

VTKM_CONT
void DataSetBuilderExplicitIterative::AddCell(const vtkm::UInt8& shape,
                                              const vtkm::Id* conn,
                                              const vtkm::IdComponent& n)
{
  this->shapes.push_back(shape);
  this->numIdx.push_back(n);
  for (int i = 0; i < n; i++)
  {
    connectivity.push_back(conn[i]);
  }
}

VTKM_CONT
void DataSetBuilderExplicitIterative::AddCellPoint(vtkm::Id pointIndex)
{
  VTKM_ASSERT(this->numIdx.size() > 0);
  this->connectivity.push_back(pointIndex);
  this->numIdx.back() += 1;
}
}
} // end namespace vtkm::cont
