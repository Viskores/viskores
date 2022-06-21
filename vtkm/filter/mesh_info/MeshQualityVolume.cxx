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

#include <vtkm/filter/mesh_info/MeshQualityVolume.h>

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/ErrorCode.h>

#include <vtkm/exec/CellMeasure.h>

namespace
{

struct VolumeWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellset,
                                FieldInPoint pointCoords,
                                FieldOutCell metricOut);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);

  template <typename CellShapeType, typename PointCoordVecType, typename OutType>
  VTKM_EXEC void operator()(CellShapeType shape,
                            const vtkm::IdComponent& numPoints,
                            const PointCoordVecType& pts,
                            OutType& metricValue) const
  {
    vtkm::UInt8 thisId = shape.Id;
    switch (thisId)
    {
      vtkmGenericCellShapeMacro(metricValue =
                                  this->ComputeMetric<OutType>(numPoints, pts, CellShapeTag()));
      default:
        this->RaiseError(vtkm::ErrorString(vtkm::ErrorCode::CellNotFound));
        metricValue = OutType(0.0);
    }
  }

private:
  template <typename OutType, typename PointCoordVecType, typename CellShapeType>
  VTKM_EXEC OutType ComputeMetric(const vtkm::IdComponent& numPts,
                                  const PointCoordVecType& pts,
                                  CellShapeType tag) const
  {
    constexpr vtkm::IdComponent dims = vtkm::CellTraits<CellShapeType>::TOPOLOGICAL_DIMENSIONS;
    vtkm::ErrorCode errorCode{ vtkm::ErrorCode::Success };

    if (dims == 3)
    {
      OutType outValue = vtkm::exec::CellMeasure<OutType>(numPts, pts, tag, errorCode);
      if (errorCode != vtkm::ErrorCode::Success)
      {
        this->RaiseError(vtkm::ErrorString(errorCode));
      }
      return outValue;
    }
    else
    {
      return 0;
    }
  }
};

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

MeshQualityVolume::MeshQualityVolume()
{
  this->SetUseCoordinateSystemAsField(true);
  this->SetOutputFieldName("volume");
}

vtkm::cont::DataSet MeshQualityVolume::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorBadValue("Active field for MeshQuality must be point coordinates. "
                                    "But the active field is not a point field.");
  }

  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
    vtkm::cont::ArrayHandle<T> result;
    this->Invoke(VolumeWorklet{}, input.GetCellSet(), concrete, result);
    outArray = result;
  };
  this->CastAndCallVecField<3>(field, resolveType);

  return this->CreateResultFieldCell(input, this->GetOutputFieldName(), outArray);
}

} // namespace mesh_info
} // namespace filter
} // namespace vtkm
