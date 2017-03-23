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

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm {
namespace filter {

struct DistributeCellData : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<> inIndices,
                                FieldOut<> outIndices);
  typedef void ExecutionSignature(_1, _2);

  typedef vtkm::worklet::ScatterCounting ScatterType;

  VTKM_CONT
  ScatterType GetScatter() const { return this->Scatter; }

  template <typename CountArrayType, typename DeviceAdapter>
  VTKM_CONT
  DistributeCellData(const CountArrayType &countArray,
                     DeviceAdapter device) :
                         Scatter(countArray, device) {  }

  template <typename T>
  VTKM_EXEC
  void operator()(T inputIndex,
                  T &outputIndex) const
  {
    outputIndex = inputIndex;
  }
private:
  ScatterType Scatter;
};

//-----------------------------------------------------------------------------
inline VTKM_CONT
Tetrahedralize::Tetrahedralize():
  vtkm::filter::FilterDataSet<Tetrahedralize>()
{
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
vtkm::filter::ResultDataSet Tetrahedralize::DoExecute(
                                                 const vtkm::cont::DataSet& input,
                                                 const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                 const DeviceAdapter& device)
{
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
  typedef vtkm::cont::CellSetStructured<3> CellSetStructuredType;
  typedef vtkm::cont::CellSetExplicit<> CellSetExplicitType;

  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());
  vtkm::Id numberOfCells = cells.GetNumberOfCells();

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::Tetrahedralize<DeviceAdapter> worklet;

  if (cells.IsType<CellSetStructuredType>())
  {
    DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(5, numberOfCells),
                          this->OutCellsPerCell);
    outCellSet = worklet.Run(cells.Cast<CellSetStructuredType>());
  }
  else
  {
    outCellSet = worklet.Run(cells.Cast<CellSetExplicitType>(), this->OutCellsPerCell);
  }

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()) );

  return vtkm::filter::ResultDataSet(output);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
bool Tetrahedralize::DoMapField(
                           vtkm::filter::ResultDataSet& result,
                           const vtkm::cont::ArrayHandle<T, StorageType>& input,
                           const vtkm::filter::FieldMetadata& fieldMeta,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                           const DeviceAdapter& device)
{
  // point data is copied as is because it was not collapsed
  if(fieldMeta.IsPointField())
  {
    result.GetDataSet().AddField(fieldMeta.AsField(input));
    return true;
  }
  
  // cell data must be scattered to the cells created per input cell
  if(fieldMeta.IsCellField())
  {
    vtkm::cont::ArrayHandle<T, StorageType> output;

    DistributeCellData distribute(this->OutCellsPerCell, device);
    vtkm::worklet::DispatcherMapField<DistributeCellData, DeviceAdapter> dispatcher(distribute);
    dispatcher.Invoke(input, output);

    result.GetDataSet().AddField(fieldMeta.AsField(output));
    return true;
  }

  return false;
}

}
}
