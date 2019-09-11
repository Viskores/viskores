//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
void DataSetBuilderExplicitIterative::Begin(const std::string& coordName)
{
  this->coordNm = coordName;
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
  return dsb.Create(points, shapes, numIdx, connectivity, coordNm);
}

VTKM_CONT
vtkm::Id DataSetBuilderExplicitIterative::AddPoint(const vtkm::Vec3f& pt)
{
  points.push_back(pt);
  vtkm::Id id = static_cast<vtkm::Id>(points.size());
  //ID is zero-based.
  return id - 1;
}

VTKM_CONT
vtkm::Id DataSetBuilderExplicitIterative::AddPoint(const vtkm::FloatDefault& x,
                                                   const vtkm::FloatDefault& y,
                                                   const vtkm::FloatDefault& z)
{
  points.push_back(vtkm::make_Vec(x, y, z));
  vtkm::Id id = static_cast<vtkm::Id>(points.size());
  //ID is zero-based.
  return id - 1;
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
