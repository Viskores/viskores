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

namespace
{

template <typename DeviceTag>
struct CallWorklet
{
  vtkm::Id Stride;
  vtkm::cont::DynamicCellSet& Output;
  vtkm::worklet::Mask& Worklet;

  CallWorklet(vtkm::Id stride, vtkm::cont::DynamicCellSet& output, vtkm::worklet::Mask& worklet)
    : Stride(stride)
    , Output(output)
    , Worklet(worklet)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cells) const
  {
    this->Output = this->Worklet.Run(cells, this->Stride, DeviceTag());
  }
};

} // end anon namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Mask::Mask()
  : vtkm::filter::FilterDataSet<Mask>()
  , Stride(1)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet Mask::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  vtkm::cont::DynamicCellSet cellOut;
  CallWorklet<DeviceAdapter> workletCaller(this->Stride, cellOut, this->Worklet);
  vtkm::filter::ApplyPolicy(cells, policy).CastAndCall(workletCaller);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.AddCellSet(cellOut);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool Mask::DoMapField(vtkm::cont::DataSet& result,
                                       const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                       const vtkm::filter::FieldMetadata& fieldMeta,
                                       const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                       const DeviceAdapter& device)
{
  vtkm::cont::Field output;

  if (fieldMeta.IsPointField())
  {
    output = fieldMeta.AsField(input); // pass through
  }
  else if (fieldMeta.IsCellField())
  {
    output = fieldMeta.AsField(this->Worklet.ProcessCellField(input, device));
  }
  else
  {
    return false;
  }

  result.AddField(output);
  return true;
}
}
}
