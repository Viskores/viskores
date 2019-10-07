//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Tetrahedralize_hxx
#define vtk_m_filter_Tetrahedralize_hxx

namespace
{
struct DeduceCellSet
{
  template <typename CellSetType>
  void operator()(const CellSetType& cellset,
                  vtkm::worklet::Tetrahedralize& worklet,
                  vtkm::cont::CellSetSingleType<>& outCellSet) const
  {
    outCellSet = worklet.Run(cellset);
  }
};
}

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Tetrahedralize::Tetrahedralize()
  : vtkm::filter::FilterDataSet<Tetrahedralize>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Tetrahedralize::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::cont::CastAndCall(
    vtkm::filter::ApplyPolicyCellSet(cells, policy), DeduceCellSet{}, this->Worklet, outCellSet);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Tetrahedralize::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    result.AddField(fieldMeta.AsField(input));
    return true;
  }

  // cell data must be scattered to the cells created per input cell
  if (fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T> output = this->Worklet.ProcessCellField(input);

    result.AddField(fieldMeta.AsField(output));
    return true;
  }

  return false;
}
}
}
#endif
