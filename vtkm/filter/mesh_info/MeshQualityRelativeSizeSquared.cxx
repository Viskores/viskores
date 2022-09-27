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

#include <vtkm/filter/mesh_info/MeshQualityArea.h>
#include <vtkm/filter/mesh_info/MeshQualityRelativeSizeSquared.h>
#include <vtkm/filter/mesh_info/MeshQualityVolume.h>

#include <vtkm/filter/mesh_info/worklet/MeshQualityWorklet.h>
#include <vtkm/filter/mesh_info/worklet/cellmetrics/CellRelativeSizeSquaredMetric.h>

#include <vtkm/CellTraits.h>

namespace
{

struct RelativeSizeSquaredWorklet : MeshQualityWorklet<RelativeSizeSquaredWorklet>
{
  vtkm::Float64 AverageArea;
  vtkm::Float64 AverageVolume;

  VTKM_CONT RelativeSizeSquaredWorklet(vtkm::Float64 averageArea, vtkm::Float64 averageVolume)
    : AverageArea(averageArea)
    , AverageVolume(averageVolume)
  {
  }

  VTKM_EXEC vtkm::Float64 GetAverageSize(vtkm::CellTopologicalDimensionsTag<2>) const
  {
    return this->AverageArea;
  }
  VTKM_EXEC vtkm::Float64 GetAverageSize(vtkm::CellTopologicalDimensionsTag<3>) const
  {
    return this->AverageVolume;
  }
  template <vtkm::IdComponent Dimension>
  VTKM_EXEC vtkm::Float64 GetAverageSize(vtkm::CellTopologicalDimensionsTag<Dimension>) const
  {
    return 1;
  }

  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VTKM_EXEC OutType ComputeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType shape,
                                  vtkm::ErrorCode& ec) const
  {
    using DimensionTag = typename vtkm::CellTraits<CellShapeType>::TopologicalDimensionsTag;
    return vtkm::worklet::cellmetrics::CellRelativeSizeSquaredMetric<OutType>(
      numPts, pts, static_cast<OutType>(this->GetAverageSize(DimensionTag{})), shape, ec);
  }
};

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

MeshQualityRelativeSizeSquared::MeshQualityRelativeSizeSquared()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName("relativeSizeSquared");
}

vtkm::cont::DataSet MeshQualityRelativeSizeSquared::DoExecute(const vtkm::cont::DataSet& input)
{
  RelativeSizeSquaredWorklet worklet(
    vtkm::filter::mesh_info::MeshQualityArea{}.ComputeAverageArea(input),
    vtkm::filter::mesh_info::MeshQualityVolume{}.ComputeAverageVolume(input));
  vtkm::cont::UnknownArrayHandle outArray = worklet.Run(input, this->GetFieldFromDataSet(input));

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), outArray);
}

} // namespace mesh_info
} // namespace filter
} // namespace vtkm
