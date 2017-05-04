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

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

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

//-----------------------------------------------------------------------------
template <typename ImplicitFunctionType, typename DerivedPolicy>
inline
void ExtractGeometry::SetImplicitFunction(
                       const std::shared_ptr<ImplicitFunctionType> &func,
                       const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  func->ResetDevice(DerivedPolicy::DeviceAdapterList);
  this->Function = func;
}


//-----------------------------------------------------------------------------
inline VTKM_CONT
ExtractGeometry::ExtractGeometry():
  vtkm::filter::FilterDataSet<ExtractGeometry>(),
  ExtractInside(true),
  ExtractBoundaryCells(false),
  ExtractOnlyBoundaryCells(false),
  ValidCellIds()
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
  const vtkm::cont::CoordinateSystem& coords =
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // run the worklet on the cell set
  vtkm::cont::ArrayHandle<bool> passFlags;

  typedef vtkm::worklet::ExtractGeometry::ExtractCellsByVOI ExtractCellsWorklet;
  ExtractCellsWorklet worklet((*this->Function).PrepareForExecution(device),
                              this->ExtractInside,
                              this->ExtractBoundaryCells,
                              this->ExtractOnlyBoundaryCells);

  vtkm::worklet::DispatcherMapTopology<ExtractCellsWorklet, DeviceAdapter> dispatcher(worklet);
  dispatcher.Invoke(vtkm::filter::ApplyPolicy(cells, policy),
                    vtkm::filter::ApplyPolicy(coords, policy),
                    passFlags);

  vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
        vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>
        ::CopyIf(indices, passFlags, this->ValidCellIds);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // generate the cellset
  AddPermutationCellSet addCellSet(output, this->ValidCellIds);
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
    typedef vtkm::cont::ArrayHandlePermutation<
                    vtkm::cont::ArrayHandle<vtkm::Id>,
                    vtkm::cont::ArrayHandle<T, StorageType> > PermutationType;

    PermutationType permutation =
          vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input);

    result.GetDataSet().AddField( fieldMeta.AsField(permutation) );
    return true;
  }
  return false;
}

}
}
