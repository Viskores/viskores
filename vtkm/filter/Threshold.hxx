//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Threshold_hxx
#define vtk_m_filter_Threshold_hxx

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

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

  //Needed to work with ArrayHandleVirtual
  template <typename PortalType>
  VTKM_EXEC bool operator()(
    const vtkm::internal::ArrayPortalValueReference<PortalType>& value) const
  {
    using T = typename PortalType::ValueType;
    return value.Get() >= static_cast<T>(this->Lower) && value.Get() <= static_cast<T>(this->Upper);
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
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Threshold::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  vtkm::cont::DynamicCellSet cellOut = this->Worklet.Run(
    vtkm::filter::ApplyPolicyCellSet(cells, policy), field, fieldMeta.GetAssociation(), predicate);

  vtkm::cont::DataSet output;
  output.SetCellSet(cellOut);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  return output;
}
}
}
#endif
