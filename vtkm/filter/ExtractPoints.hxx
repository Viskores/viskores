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

// Needed to CompactPoints
template <typename BasePolicy>
struct CellSetSingleTypePolicy : public BasePolicy
{
  using AllCellSetList = vtkm::cont::CellSetListTagUnstructured;
};

template <typename DerivedPolicy>
inline vtkm::filter::PolicyBase<CellSetSingleTypePolicy<DerivedPolicy>>
GetCellSetSingleTypePolicy(const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  return vtkm::filter::PolicyBase<CellSetSingleTypePolicy<DerivedPolicy>>();
}

}

namespace vtkm {
namespace filter {

const int BY_IDS = 0;
const int BY_VOI = 1;

//-----------------------------------------------------------------------------
inline VTKM_CONT
void ExtractPoints::SetPointIds(vtkm::cont::ArrayHandle<vtkm::Id> &pointIds)
{
  this->PointIds = pointIds;
  this->ExtractType = BY_IDS;
}

/*
inline VTKM_CONT
void ExtractPoints::SetVolumeOfInterest(vtkm::ImplicitFunction &implicitFunction) 
{
  this->VolumeOfInterest = implicitFunction;
  this->ExtractType = BY_VOI;
}
*/

//-----------------------------------------------------------------------------
inline VTKM_CONT
ExtractPoints::ExtractPoints():
  vtkm::filter::FilterDataSet<ExtractPoints>(),
  CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
vtkm::filter::ResultDataSet ExtractPoints::DoExecute(
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

  // run the worklet on the cell set
  vtkm::cont::CellSetSingleType<> outCellSet(cells.GetName());
  vtkm::worklet::ExtractPoints worklet;

  switch (this->ExtractType)
  {
  case BY_IDS:
  {
    outCellSet = worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                             this->PointIds,
                             device);
    break;
  }
  case BY_VOI:
  {
/*
    outCellSet = worklet.Run(vtkm::filter::ApplyPolicy(cells, policy),
                             vtkm::filter::ApplyPolicy(coords, policy),
                             this->implicitFunction,
                             device);
*/
    break;
  }
  default:
    break;
  }

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    return this->Compactor.DoExecute(output, 
                                     GetCellSetSingleTypePolicy(policy),
                                     DeviceAdapter());
  }
  else
  {
    return vtkm::filter::ResultDataSet(output);
  }
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
bool ExtractPoints::DoMapField(
                           vtkm::filter::ResultDataSet& result,
                           const vtkm::cont::ArrayHandle<T, StorageType>& input,
                           const vtkm::filter::FieldMetadata& fieldMeta,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                           const DeviceAdapter&)
{
  // point data is copied as is because it was not collapsed
  if(fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy, DeviceAdapter());
    }
    else
    {
      result.GetDataSet().AddField(fieldMeta.AsField(input));
      return true;
    }
  }
  
  // cell data does not apply
  return false;
}

}
}
