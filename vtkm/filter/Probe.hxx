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

namespace vtkm
{
namespace filter
{

VTKM_CONT
inline void Probe::SetGeometry(const vtkm::cont::DataSet& geometry)
{
  this->Geometry = vtkm::cont::DataSet();
  this->Geometry.AddCellSet(geometry.GetCellSet());
  this->Geometry.AddCoordinateSystem(geometry.GetCoordinateSystem());
}

template <typename DerivedPolicy>
VTKM_CONT inline vtkm::cont::DataSet Probe::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  this->Worklet.Run(
    vtkm::filter::ApplyPolicy(input.GetCellSet(this->GetActiveCellSetIndex()), policy),
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
    this->Geometry.GetCoordinateSystem().GetData());

  auto output = this->Geometry;
  auto hpf = this->Worklet.GetHiddenPointsField();
  auto hcf =
    this->Worklet.GetHiddenCellsField(vtkm::filter::ApplyPolicy(output.GetCellSet(), policy));

  output.AddField(vtkm::cont::Field("HIDDEN", vtkm::cont::Field::Association::POINTS, hpf));
  output.AddField(vtkm::cont::Field(
    "HIDDEN", vtkm::cont::Field::Association::CELL_SET, output.GetCellSet().GetName(), hcf));

  return output;
}

template <typename T, typename StorageType, typename DerivedPolicy>
VTKM_CONT inline bool Probe::DoMapField(vtkm::cont::DataSet& result,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        vtkm::filter::PolicyBase<DerivedPolicy>)
{
  if (fieldMeta.IsPointField())
  {
    auto fieldArray =
      this->Worklet.ProcessPointField(input, typename DerivedPolicy::AllCellSetList());
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    auto fieldArray = this->Worklet.ProcessCellField(input);
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }

  return false;
}
}
} // vtkm::filter

#endif // vtk_m_filter_Probe_hxx
