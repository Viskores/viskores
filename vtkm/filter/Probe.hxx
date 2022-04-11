//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Probe_hxx
#define vtk_m_filter_Probe_hxx

#include <vtkm/filter/Probe.h>

#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/cont/internal/CastInvalidValue.h>

namespace vtkm
{
namespace filter
{

VTKM_CONT
inline void Probe::SetGeometry(const vtkm::cont::DataSet& geometry)
{
  this->Geometry = vtkm::cont::DataSet();
  this->Geometry.SetCellSet(geometry.GetCellSet());
  this->Geometry.AddCoordinateSystem(geometry.GetCoordinateSystem());
}

VTKM_CONT
inline const vtkm::cont::DataSet& Probe::GetGeometry() const
{
  return this->Geometry;
}

template <typename DerivedPolicy>
VTKM_CONT inline vtkm::cont::DataSet Probe::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  this->Worklet.Run(vtkm::filter::ApplyPolicyCellSet(input.GetCellSet(), policy, *this),
                    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
                    this->Geometry.GetCoordinateSystem().GetData());

  auto output = this->Geometry;
  auto hpf = this->Worklet.GetHiddenPointsField();
  auto hcf = this->Worklet.GetHiddenCellsField(
    vtkm::filter::ApplyPolicyCellSet(output.GetCellSet(), policy, *this));

  output.AddField(vtkm::cont::make_FieldPoint("HIDDEN", hpf));
  output.AddField(vtkm::cont::make_FieldCell("HIDDEN", hcf));

  return output;
}

template <typename DerivedPolicy>
VTKM_CONT inline bool Probe::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                const vtkm::cont::Field& field,
                                                vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (field.IsFieldPoint())
  {
    // If the field is a point field, then we need to do a custom interpolation of the points.
    // In this case, we need to call the superclass's MapFieldOntoOutput, which will in turn
    // call our DoMapField.
    return this->FilterDataSet<Probe>::MapFieldOntoOutput(result, field, policy);
  }
  else if (field.IsFieldCell())
  {
    vtkm::cont::Field outField;
    if (vtkm::filter::MapFieldPermutation(
          field, this->Worklet.GetCellIds(), outField, this->InvalidValue))
    {
      // output field should be associated with points
      outField = vtkm::cont::Field(
        field.GetName(), vtkm::cont::Field::Association::Points, outField.GetData());
      result.AddField(outField);
      return true;
    }
    return false;
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}

template <typename T, typename StorageType, typename DerivedPolicy>
VTKM_CONT inline bool Probe::DoMapField(vtkm::cont::DataSet& result,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        vtkm::filter::PolicyBase<DerivedPolicy>)
{
  VTKM_ASSERT(fieldMeta.IsPointField());
  auto fieldArray =
    this->Worklet.ProcessPointField(input,
                                    vtkm::cont::internal::CastInvalidValue<T>(this->InvalidValue),
                                    typename DerivedPolicy::AllCellSetList());
  result.AddField(fieldMeta.AsField(fieldArray));
  return true;
}
}
} // vtkm::filter
#endif
