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

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/CellSetSingleType.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>

namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
inline VTKM_CONT
MarchingCubes::MarchingCubes():
  vtkm::filter::FilterDataSetWithField<MarchingCubes>(),
  IsoValue(0),
  GenerateNormals(false),
  NormalArrayName("normals"),
  Worklet()
{
  // todo: keep an instance of marching cubes worklet as a member variable
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
vtkm::filter::ResultDataSet MarchingCubes::DoExecute(const vtkm::cont::DataSet& input,
                                                     const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                                     const vtkm::filter::FieldMetadata& fieldMeta,
                                                     const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                     const DeviceAdapter& device)
{
  if(fieldMeta.IsPointField() == false)
  {
    //todo: we need to mark this as a failure of input, not a failure
    //of the algorithm
    return vtkm::filter::ResultDataSet();
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& coords =
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  typedef vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::FloatDefault,3> > Vec3HandleType;
  Vec3HandleType vertices;
  Vec3HandleType normals;

  vtkm::cont::DataSet output;
  vtkm::cont::CellSetSingleType< > outputCells;

  //not sold on this as we have to generate more signatures for the
  //worklet with the design
  //But I think we should get this to compile before we tinker with
  //a more efficient api
  if(this->GenerateNormals)
  {
    outputCells =
      this->Worklet.Run( static_cast<T>(this->IsoValue),
                         vtkm::filter::ApplyPolicy(cells, policy),
                         vtkm::filter::ApplyPolicy(coords, policy),
                         field,
                         vertices,
                         normals,
                         device
                         );
  }
  else
  {
    outputCells =
      this->Worklet.Run( static_cast<T>(this->IsoValue),
                         vtkm::filter::ApplyPolicy(cells, policy),
                         vtkm::filter::ApplyPolicy(coords, policy),
                         field,
                         vertices,
                         device
                         );
  }


  if(this->GenerateNormals)
  {
    vtkm::cont::Field normalField(this->NormalArrayName,
                                  vtkm::cont::Field::ASSOC_POINTS, normals);
    output.AddField( normalField );
  }

  //assign the connectivity to the cell set
  output.AddCellSet( outputCells );


  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem( outputCoords );

  return vtkm::filter::ResultDataSet(output);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
bool MarchingCubes::DoMapField(vtkm::filter::ResultDataSet& result,
                               const vtkm::cont::ArrayHandle<T, StorageType>& input,
                               const vtkm::filter::FieldMetadata& fieldMeta,
                               const vtkm::filter::PolicyBase<DerivedPolicy>&,
                               const DeviceAdapter& device)
{
  if(fieldMeta.IsPointField() == false)
  {
    //not a point field, we can't map it
    return false;
  }

  vtkm::cont::ArrayHandle<T> output;
  this->Worklet.MapFieldOntoIsosurface( input, output, device);

  //use the same meta data as the input so we get the same field name, etc.
  result.GetDataSet().AddField( fieldMeta.AsField(output) );
  return true;

}


}
} // namespace vtkm::filter
