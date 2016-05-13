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

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

class ThresholdRange
{
public:
  VTKM_CONT_EXPORT
  ThresholdRange(const vtkm::Float64& lower,
                 const vtkm::Float64& upper) :
    Lower(lower),
    Upper(upper)
  { }

  template<typename T>
  VTKM_EXEC_EXPORT
  bool operator()(const T& value) const
  {
    return value >= static_cast<T>(this->Lower) &&
           value <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

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
Threshold::Threshold():
  vtkm::filter::FilterDataSetWithField<Threshold>(),
  LowerValue(0),
  UpperValue(0),
  ValidCellIds()
{

}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::ResultDataSet Threshold::DoExecute(const vtkm::cont::DataSet& input,
                                                 const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                                 const vtkm::filter::FieldMetadata& fieldMeta,
                                                 const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                 const DeviceAdapter&)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  ThresholdRange predicate( this->GetLowerThreshold(), this->GetUpperThreshold() );
  vtkm::cont::ArrayHandle<bool> passFlags;
  if(fieldMeta.IsPointField())
  {
    typedef vtkm::worklet::Threshold Worklets;
    typedef Worklets::ThresholdByPointField< ThresholdRange > ThresholdWorklet;
    ThresholdWorklet worklet(predicate);
    vtkm::worklet::DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(vtkm::filter::ApplyPolicy(cells, policy), field, passFlags);
  }
  else if(fieldMeta.IsCellField())
  {
    typedef vtkm::worklet::Threshold Worklets;
    typedef Worklets::ThresholdByCellField< ThresholdRange > ThresholdWorklet;
    ThresholdWorklet worklet(predicate);
    vtkm::worklet::DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(vtkm::filter::ApplyPolicy(cells, policy), field, passFlags);
  }
  else
  {
    //todo: we need to mark this as a failure of input, not a failure
    //of the algorithm
    return vtkm::filter::ResultDataSet();
  }

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

  Algorithm::StreamCompact(passFlags, this->ValidCellIds);

  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(
          input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()) );

  //now generate the output cellset. We are going to need to do a cast
  //and call to generate the correct form of output.
  //todo: see if we can make this into a helper class that other filters
  //can use to reduce code duplication, and make it easier to write filters
  //that return complex dataset types.
  AddPermutationCellSet addCellSet(output, this->ValidCellIds);
  vtkm::filter::ApplyPolicy(cells, policy).CastAndCall(addCellSet);

  //todo: We need to generate a new output policy that replaces
  //the original storage tag with a new storage tag where everything is
  //wrapped in CellSetPermutation.
  return output;
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
bool Threshold::DoMapField(vtkm::filter::ResultDataSet& result,
                           const vtkm::cont::ArrayHandle<T, StorageType>& input,
                           const vtkm::filter::FieldMetadata& fieldMeta,
                           const vtkm::filter::PolicyBase<DerivedPolicy>&,
                           const DeviceAdapter&)
{
  if(fieldMeta.IsPointField())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.GetDataSet().AddField( fieldMeta.AsField(input) );
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
          vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input);

    result.GetDataSet().AddField( fieldMeta.AsField(permutation) );
    return true;
  }

  return false;
}

}
}
