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

#include <vtkm/worklet/ContourTreeUniform.h>

namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
ContourTreeMesh2D::ContourTreeMesh2D()
{
  this->SetOutputFieldName("saddlePeak");
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::ResultField
ContourTreeMesh2D::DoExecute(const vtkm::cont::DataSet &input,
                             const vtkm::cont::ArrayHandle<T,StorageType> &field,
                             const vtkm::filter::FieldMetadata &fieldMeta,
                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                             const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField() == false) {
    std::cout << "ERROR: Point field expected" << std::endl;
    return vtkm::filter::ResultField();
  }

  // Collect sizing information from the dataset
  vtkm::cont::CellSetStructured<2> cellSet;
  input.GetCellSet(0).CopyTo(cellSet);

  // How should policy be used?
  vtkm::filter::ApplyPolicy(cellSet, policy);

  vtkm::Id2 pointDimensions = cellSet.GetPointDimensions();
  vtkm::Id nRows = pointDimensions[0];
  vtkm::Id nCols = pointDimensions[1];

  vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;

  vtkm::worklet::ContourTreeMesh2D worklet;
  worklet.Run(field, nRows, nCols, saddlePeak, device);

  return vtkm::filter::ResultField(input,
                                   saddlePeak,
                                   this->GetOutputFieldName(),
                                   fieldMeta.GetAssociation(),
                                   fieldMeta.GetCellSetName());
}
//-----------------------------------------------------------------------------
ContourTreeMesh3D::ContourTreeMesh3D()
{
  this->SetOutputFieldName("saddlePeak");
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::ResultField
ContourTreeMesh3D::DoExecute(const vtkm::cont::DataSet &input,
                             const vtkm::cont::ArrayHandle<T,StorageType> &field,
                             const vtkm::filter::FieldMetadata &fieldMeta,
                             const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                             const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField() == false) {
    std::cout << "ERROR: Point field expected" << std::endl;
    return vtkm::filter::ResultField();
  }

  // Collect sizing information from the dataset
  vtkm::cont::CellSetStructured<3> cellSet;
  input.GetCellSet(0).CopyTo(cellSet);

  // How should policy be used?
  vtkm::filter::ApplyPolicy(cellSet, policy);

  vtkm::Id3 pointDimensions = cellSet.GetPointDimensions();
  vtkm::Id nRows   = pointDimensions[0];
  vtkm::Id nCols   = pointDimensions[1];
  vtkm::Id nSlices = pointDimensions[2];

  vtkm::cont::ArrayHandle<vtkm::Pair<vtkm::Id, vtkm::Id> > saddlePeak;

  vtkm::worklet::ContourTreeMesh3D worklet;
  worklet.Run(field, nRows, nCols, nSlices, saddlePeak, device);

  return vtkm::filter::ResultField(input,
                                   saddlePeak,
                                   this->GetOutputFieldName(),
                                   fieldMeta.GetAssociation(),
                                   fieldMeta.GetCellSetName());
}

}
} // namespace vtkm::filter
