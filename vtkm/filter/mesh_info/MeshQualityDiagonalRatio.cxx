//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//=========================================================================

#include <vtkm/filter/mesh_info/MeshQualityDiagonalRatio.h>

#include <vtkm/filter/mesh_info/worklet/MeshQualityWorklet.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellDiagonalRatioMetric.h>

namespace
{

struct DiagonalRatioWorklet : MeshQualityWorklet<DiagonalRatioWorklet>
{
  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VTKM_EXEC OutType ComputeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType shape,
                                  vtkm::ErrorCode& ec) const
  {
    return vtkm::worklet::cellmetrics::CellDiagonalRatioMetric<OutType>(numPts, pts, shape, ec);
  }
};

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

MeshQualityDiagonalRatio::MeshQualityDiagonalRatio()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName("diagonalRatio");
}

vtkm::cont::DataSet MeshQualityDiagonalRatio::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::cont::UnknownArrayHandle outArray =
    DiagonalRatioWorklet{}.Run(input, this->GetFieldFromDataSet(input));

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), outArray);
}

} // namespace mesh_info
} // namespace filter
} // namespace vtkm
