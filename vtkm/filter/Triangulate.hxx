//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Triangulate_hxx
#define vtk_m_filter_Triangulate_hxx

namespace
{

class DeduceCellSet
{
  mutable vtkm::worklet::Triangulate Worklet;
  vtkm::cont::CellSetSingleType<>& OutCellSet;

public:
  DeduceCellSet(vtkm::worklet::Triangulate worklet, vtkm::cont::CellSetSingleType<>& outCellSet)
    : Worklet(worklet)
    , OutCellSet(outCellSet)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& vtkmNotUsed(cellset)) const
  {
  }
};
template <>
void DeduceCellSet::operator()(const vtkm::cont::CellSetExplicit<>& cellset) const
{
  this->OutCellSet = Worklet.Run(cellset);
}
template <>
void DeduceCellSet::operator()(const vtkm::cont::CellSetStructured<2>& cellset) const
{
  this->OutCellSet = Worklet.Run(cellset);
}
template <>
void DeduceCellSet::operator()(const vtkm::cont::CellSetStructured<3>& cellset) const
{
  this->OutCellSet = Worklet.Run(cellset);
}
}

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Triangulate::Triangulate()
  : vtkm::filter::FilterDataSet<Triangulate>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Triangulate::DoExecute(
  const vtkm::cont::DataSet& input,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  vtkm::cont::CellSetSingleType<> outCellSet;
  DeduceCellSet triangulate(this->Worklet, outCellSet);

  vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicyCellSet(cells, policy), triangulate);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Triangulate::DoMapField(vtkm::cont::DataSet& result,
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
