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

template <typename DerivedPolicy, typename DeviceAdapter>
VTKM_CONT inline vtkm::cont::DataSet Probe::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  this->Worklet.Run(
    vtkm::filter::ApplyPolicy(input.GetCellSet(this->GetActiveCellSetIndex()), policy),
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()),
    this->Geometry.GetCoordinateSystem().GetData(),
    device);

  auto output = this->Geometry;
  auto hpf = this->Worklet.GetHiddenPointsField(device);
  auto hcf = this->Worklet.GetHiddenCellsField(
    vtkm::filter::ApplyPolicy(output.GetCellSet(), policy), device);

  output.AddField(vtkm::cont::Field("HIDDEN", vtkm::cont::Field::Association::POINTS, hpf));
  output.AddField(vtkm::cont::Field(
    "HIDDEN", vtkm::cont::Field::Association::CELL_SET, output.GetCellSet().GetName(), hcf));

  return output;
}

template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
VTKM_CONT inline bool Probe::DoMapField(vtkm::cont::DataSet& result,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                        const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField())
  {
    auto fieldArray =
      this->Worklet.ProcessPointField(input, device, typename DerivedPolicy::AllCellSetList());
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }
  else if (fieldMeta.IsCellField())
  {
    auto fieldArray = this->Worklet.ProcessCellField(input, device);
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }

  return false;
}
}
} // vtkm::filter

#endif // vtk_m_filter_Probe_hxx
