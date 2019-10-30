//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Mask_hxx
#define vtk_m_filter_Mask_hxx

namespace
{

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
    this->Output = this->Worklet.Run(cells, this->Stride);
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
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Mask::DoExecute(const vtkm::cont::DataSet& input,
                                                     vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  vtkm::cont::DynamicCellSet cellOut;
  CallWorklet workletCaller(this->Stride, cellOut, this->Worklet);
  vtkm::filter::ApplyPolicyCellSet(cells, policy).CastAndCall(workletCaller);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(cellOut);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Mask::DoMapField(vtkm::cont::DataSet& result,
                                       const vtkm::cont::ArrayHandle<T, StorageType>& input,
                                       const vtkm::filter::FieldMetadata& fieldMeta,
                                       vtkm::filter::PolicyBase<DerivedPolicy>)
{
  vtkm::cont::Field output;

  if (fieldMeta.IsPointField())
  {
    output = fieldMeta.AsField(input); // pass through
  }
  else if (fieldMeta.IsCellField())
  {
    output = fieldMeta.AsField(this->Worklet.ProcessCellField(input));
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
#endif
