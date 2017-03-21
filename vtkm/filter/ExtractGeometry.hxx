//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

namespace
{
class AddPermutationCellSet
{
  vtkm::cont::DataSet* Output;
  vtkm::cont::ArrayHandle<vtkm::Id>* ValidIds;
public:
  AddPermutationCellSet(vtkm::cont::DataSet& data,
                        vtkm::cont::ArrayHandle<vtkm::Id>& validIds):
    Output(&data),
    ValidIds(&validIds)
  { }

  template<typename CellSetType>
  void operator()(const CellSetType& cellset ) const
  {
    typedef vtkm::cont::CellSetPermutation<CellSetType> PermutationCellSetType;

    PermutationCellSetType permCellSet(*this->ValidIds, cellset,
                                       cellset.GetName());

    this->Output->AddCellSet(permCellSet);
  }
};
}

namespace vtkm {
namespace filter {

const int BY_IDS = 0;
const int BY_VOI = 1;

//-----------------------------------------------------------------------------
inline VTKM_CONT
void ExtractGeometry::SetCellIds(vtkm::cont::ArrayHandle<vtkm::Id> &cellIds)
{
  this->CellIds = cellIds;
  this->ExtractType = BY_IDS;
}

/*
inline VTKM_CONT
void ExtractGeometry::SetVolumeOfInterest(vtkm::ImplicitFunction &implicitFunction) 
{
  this->VolumeOfInterest = implicitFunction;
  this->ExtractType = BY_VOI;
}
*/

//-----------------------------------------------------------------------------
inline VTKM_CONT
ExtractGeometry::ExtractGeometry():
  vtkm::filter::FilterDataSet<ExtractGeometry>(),
  CellIds(),
  CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
vtkm::filter::ResultDataSet ExtractGeometry::DoExecute(
                                                 const vtkm::cont::DataSet& input,
                                                 const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                 const DeviceAdapter& device)
{
  // extract the input cell set and coordinates
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());
/*
  const vtkm::cont::CoordinateSystem& coords =
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
*/

  switch (this->ExtractType)
  {
  case BY_IDS:
  {
    break;
  }
  case BY_VOI:
  {
/*
    vtkm::cont::ArrayHandle<bool> passFlags;

    // Worklet output will be a boolean passFlag array
    typedef vtkm::worklet::ExtractGeometry::ExtractCellsByVOI<ImplicitFunction> ExtractCellsWorklet;
    ExtractCellsWorklet worklet(implicitFunction);
    DispatcherMapTopology<ExtractCellsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet,
                      coordinates,
                      passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>
        ::CopyIf(indices, passFlags, this->ValidCellIds);
*/
    break;
  }
  default:
    break;
  }

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  AddPermutationCellSet addCellSet(output, this->CellIds);
  vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicy(cells, policy),
                          addCellSet);

  return output;
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
bool ExtractGeometry::DoMapField(
                           vtkm::filter::ResultDataSet& result,
                           const vtkm::cont::ArrayHandle<T, StorageType>& input,
                           const vtkm::filter::FieldMetadata& fieldMeta,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                           const DeviceAdapter&)
{
  // point data is copied as is because it was not collapsed
  if(fieldMeta.IsPointField())
  {
    result.GetDataSet().AddField(fieldMeta.AsField(input));
    return true;
  }

  if(fieldMeta.IsCellField())
  {
    //todo: We need to generate a new output policy that replaces
    //the original storage tag with a new storage tag where everything is
    //wrapped in ArrayHandlePermutation.
    typedef vtkm::cont::ArrayHandlePermutation<
                    vtkm::cont::ArrayHandle<vtkm::Id>,
                    vtkm::cont::ArrayHandle<T, StorageType> > PermutationType;

    PermutationType permutation =
          vtkm::cont::make_ArrayHandlePermutation(this->CellIds, input);

    result.GetDataSet().AddField( fieldMeta.AsField(permutation) );
    return true;
  }

  // cell data does not apply
  return false;
}

}
}
