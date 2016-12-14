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

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace {

template<typename DerivedPolicy, typename T, typename S>
struct PointGrad
{
  PointGrad(const vtkm::cont::CoordinateSystem& coords,
            const vtkm::cont::ArrayHandle< T, S>& field,
            vtkm::cont::ArrayHandle< vtkm::Vec<T,3> >* result):
  Points(&coords),
  InField(&field),
  Result(result)
  {
  }

  template<typename CellSetType>
  void operator()(const CellSetType& cellset ) const
  {
    vtkm::worklet::DispatcherMapTopology<vtkm::worklet::PointGradient> dispatcher;
    dispatcher.Invoke(cellset, //topology to iterate on a per point basis
                      cellset, //whole cellset in
                      vtkm::filter::ApplyPolicy(*this->Points, this->Policy),
                      *this->InField,
                      *this->Result);
  }

  vtkm::filter::PolicyBase<DerivedPolicy> Policy;
  const vtkm::cont::CoordinateSystem* const Points;
  const vtkm::cont::ArrayHandle<T,S>* const InField;
  vtkm::cont::ArrayHandle< vtkm::Vec<T,3> >* Result;
};


}

namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline
vtkm::filter::ResultField Gradient::DoExecute(
    const vtkm::cont::DataSet &input,
    const vtkm::cont::ArrayHandle<T, StorageType> &inField,
    const vtkm::filter::FieldMetadata &fieldMetadata,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter&)
{
  if(!fieldMetadata.IsPointField())
  {
    //we currently only support point fields, as we need to write the
    //worklet to efficiently map a cell field to the points of a cell
    //without doing a memory explosion
    return vtkm::filter::ResultField();
  }

  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coords =
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());


  vtkm::cont::Field::AssociationEnum fieldAssociation;
  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    outputName = "Gradients";
  }

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  vtkm::cont::ArrayHandle< vtkm::Vec<T,3> > outArray;
  if(this->ComputePointGradient)
  {
    PointGrad<DerivedPolicy,T,StorageType> func(coords, inField, &outArray);
    vtkm::cont::CastAndCall( vtkm::filter::ApplyPolicy(cells, policy),
                             func);
    fieldAssociation = vtkm::cont::Field::ASSOC_POINTS;
  }
  else
  {
    vtkm::worklet::DispatcherMapTopology<
                                       vtkm::worklet::CellGradient,
                                       DeviceAdapter > dispatcher;
    dispatcher.Invoke( vtkm::filter::ApplyPolicy(cells, policy),
                       vtkm::filter::ApplyPolicy(coords, policy),
                       inField,
                       outArray);
    fieldAssociation = vtkm::cont::Field::ASSOC_CELL_SET;
  }

  return vtkm::filter::ResultField(input,
                                   outArray,
                                   outputName,
                                   fieldAssociation,
                                   cells.GetName());


}

}
} // namespace vtkm::filter
