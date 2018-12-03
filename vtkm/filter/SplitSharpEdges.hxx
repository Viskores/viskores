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
//============================================================================

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT SplitSharpEdges::SplitSharpEdges()
  : vtkm::filter::FilterDataSetWithField<SplitSharpEdges>()
  , FeatureAngle(30.0)
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet SplitSharpEdges::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& vtkmNotUsed(fieldMeta),
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // Get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> newCoords;
  vtkm::cont::CellSetExplicit<> newCellset;

  this->Worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                    this->FeatureAngle,
                    field,
                    input.GetCoordinateSystem().GetData(),
                    newCoords,
                    newCellset);

  vtkm::cont::DataSet output;
  output.AddCellSet(newCellset);
  output.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem(input.GetCoordinateSystem().GetName(), newCoords));
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool SplitSharpEdges::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    // We copy the input handle to the result dataset, reusing the metadata
    vtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessPointField(input);
    result.AddField(fieldMeta.AsField(out));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    result.AddField(fieldMeta.AsField(input));
    return true;
  }
  else
  {
    return false;
  }
}
}
}
