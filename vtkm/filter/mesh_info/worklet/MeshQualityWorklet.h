//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_mesh_info_worklet_MeshQualityWorklet_h
#define vtk_m_filter_mesh_info_worklet_MeshQualityWorklet_h

#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/ErrorCode.h>
#include <vtkm/TypeList.h>

namespace
{

/**
  * Worklet that computes mesh quality metric values for each cell in
  * the input mesh. A metric is specified per cell type in the calling filter,
  * and this metric is invoked over all cells of that cell type. An array of
  * the computed metric values (one per cell) is returned as output.
  */
template <typename Derived>
struct MeshQualityWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
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
    if (shape.Id == vtkm::CELL_SHAPE_POLYGON)
    {
      if (numPoints == 3)
        thisId = vtkm::CELL_SHAPE_TRIANGLE;
      else if (numPoints == 4)
        thisId = vtkm::CELL_SHAPE_QUAD;
    }

    const Derived* self = reinterpret_cast<const Derived*>(this);
    vtkm::ErrorCode errorCode = vtkm::ErrorCode::Success;
    switch (thisId)
    {
      vtkmGenericCellShapeMacro(metricValue = self->template ComputeMetric<OutType>(
                                  numPoints, pts, CellShapeTag{}, errorCode));
      default:
        errorCode = vtkm::ErrorCode::CellNotFound;
        metricValue = OutType(0.0);
    }

    if (errorCode != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(errorCode));
    }
  }

  VTKM_CONT vtkm::cont::UnknownArrayHandle Run(const vtkm::cont::DataSet& input,
                                               const vtkm::cont::Field& field) const
  {
    if (!field.IsPointField())
    {
      throw vtkm::cont::ErrorBadValue("Active field for MeshQuality must be point coordinates. "
                                      "But the active field is not a point field.");
    }

    vtkm::cont::UnknownArrayHandle outArray;
    vtkm::cont::Invoker invoke;

    auto resolveType = [&](const auto& concrete) {
      using T = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
      vtkm::cont::ArrayHandle<T> result;
      invoke(*reinterpret_cast<const Derived*>(this), input.GetCellSet(), concrete, result);
      outArray = result;
    };
    field.GetData()
      .CastAndCallForTypesWithFloatFallback<vtkm::TypeListFieldVec3, VTKM_DEFAULT_STORAGE_LIST>(
        resolveType);

    return outArray;
  }
};

} // anonymous namespace

#endif //vtk_m_filter_mesh_info_worklet_MeshQualityWorklet_h
