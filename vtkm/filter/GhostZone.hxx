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

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

class RemoveAllGhosts
{
public:
  VTKM_CONT
  RemoveAllGhosts() {}

  VTKM_EXEC bool operator()(const vtkm::UInt8& value) const { return (value == 0); }
};

class RemoveGhostByType
{
public:
  VTKM_CONT
  RemoveGhostByType()
    : RemoveType(0)
  {
  }

  VTKM_CONT
  RemoveGhostByType(const vtkm::UInt8& val)
    : RemoveType(static_cast<vtkm::UInt8>(~val))
  {
  }

  VTKM_EXEC bool operator()(const vtkm::UInt8& value) const
  {
    return value == 0 || (value & RemoveType);
  }

private:
  vtkm::UInt8 RemoveType;
};

} // end anon namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT GhostZone::GhostZone()
  : vtkm::filter::FilterDataSetWithField<GhostZone>()
  , RemoveAll(false)
  , ConvertToUnstructured(false)
  , RemoveVals(0)
{
  this->SetActiveField("vtkmGhostCells");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet GhostZone::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  vtkm::cont::DynamicCellSet cellOut;

  if (this->GetRemoveAllGhost())
    cellOut = this->Worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                                field,
                                fieldMeta.GetAssociation(),
                                RemoveAllGhosts());
  else if (this->GetRemoveByType())
    cellOut = this->Worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                                field,
                                fieldMeta.GetAssociation(),
                                RemoveGhostByType(this->GetRemoveType()));
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported ghost cell removal type");

  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  if (this->GetConvertOutputToUnstructured())
  {
    vtkm::cont::CellSetExplicit<> explicitCells;
    explicitCells = this->ConvertOutputToUnstructured(cellOut);
    output.AddCellSet(explicitCells);
  }
  else
    output.AddCellSet(cellOut);

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool GhostZone::DoMapField(vtkm::cont::DataSet& result,
                                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                            const vtkm::filter::FieldMetadata& fieldMeta,
                                            vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(fieldMeta.AsField(input));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessCellField(input);
    result.AddField(fieldMeta.AsField(out));
    return true;
  }
  else
  {
    return false;
  }
}

inline VTKM_CONT vtkm::cont::CellSetExplicit<> GhostZone::ConvertOutputToUnstructured(
  vtkm::cont::DynamicCellSet& inCells)
{
  using PermStructured2d = vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<2>>;
  using PermStructured3d = vtkm::cont::CellSetPermutation<vtkm::cont::CellSetStructured<3>>;
  using PermExplicit = vtkm::cont::CellSetPermutation<vtkm::cont::CellSetExplicit<>>;
  using PermExplicitSingle = vtkm::cont::CellSetPermutation<vtkm::cont::CellSetSingleType<>>;

  vtkm::cont::CellSetExplicit<> explicitCells;

  if (inCells.IsSameType(PermStructured2d()))
  {
    PermStructured2d perm = inCells.Cast<PermStructured2d>();
    vtkm::worklet::CellDeepCopy::Run(perm, explicitCells);
  }
  else if (inCells.IsSameType(PermStructured3d()))
  {
    PermStructured3d perm = inCells.Cast<PermStructured3d>();
    vtkm::worklet::CellDeepCopy::Run(perm, explicitCells);
  }
  else if (inCells.IsSameType(PermExplicit()))
  {
    PermExplicit perm = inCells.Cast<PermExplicit>();
    vtkm::worklet::CellDeepCopy::Run(perm, explicitCells);
  }
  else if (inCells.IsSameType(PermExplicitSingle()))
  {
    PermExplicitSingle perm = inCells.Cast<PermExplicitSingle>();
    vtkm::worklet::CellDeepCopy::Run(perm, explicitCells);
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported permutation cell type");

  return explicitCells;
}
}
}
