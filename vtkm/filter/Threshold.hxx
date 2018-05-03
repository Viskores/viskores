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

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

class ThresholdRange
{
public:
  VTKM_CONT
  ThresholdRange(const vtkm::Float64& lower, const vtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename T>
  VTKM_EXEC bool operator()(const T& value) const
  {
    return value >= static_cast<T>(this->Lower) && value <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

} // end anon namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Threshold::Threshold()
  : vtkm::filter::FilterDataSetWithField<Threshold>()
  , LowerValue(0)
  , UpperValue(0)
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet Threshold::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  DeviceAdapter)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  vtkm::cont::DynamicCellSet cellOut = this->Worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                                                         field,
                                                         fieldMeta.GetAssociation(),
                                                         predicate,
                                                         DeviceAdapter());

  vtkm::cont::DataSet output;
  output.AddCellSet(cellOut);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool Threshold::DoMapField(vtkm::cont::DataSet& result,
                                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                            const vtkm::filter::FieldMetadata& fieldMeta,
                                            const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                            DeviceAdapter device)
{
  if (fieldMeta.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(fieldMeta.AsField(input));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> out = this->Worklet.ProcessCellField(input, device);
    result.AddField(fieldMeta.AsField(out));
    return true;
  }
  else
  {
    return false;
  }
}
}
}
