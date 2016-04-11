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

#ifndef vtk_m_filter_MarchingCubes_h
#define vtk_m_filter_MarchingCubes_h

#include <vtkm/filter/DataSetWithFieldFilter.h>
#include <vtkm/worklet/MarchingCubes.h>


namespace vtkm {
namespace filter {


/*
* Outstanding issues:
* 1. The output is a proper dataset, which means:
*     It needs a cell set
*     It needs a coordinate system
*
*
*/

class MarchingCubes : public vtkm::filter::DataSetWithFieldFilter<MarchingCubes>
{
public:
  VTKM_CONT_EXPORT
  MarchingCubes();

  VTKM_CONT_EXPORT
  void SetIsoValue(vtkm::Float64 value){ this->IsoValue = value; }

  VTKM_CONT_EXPORT
  vtkm::Float64 GetIsoValue() const    { return this->IsoValue; }

  VTKM_CONT_EXPORT
  void SetMergeDuplicatePoints(bool on) { this->MergeDuplicatePoints = on; }

  VTKM_CONT_EXPORT
  bool GetMergeDuplicatePoints() const  { return this->MergeDuplicatePoints; }

  VTKM_CONT_EXPORT
  void SetGenerateNormals(bool on) { this->GenerateNormals = on; }

  VTKM_CONT_EXPORT
  bool GetGenerateNormals() const  { return this->GenerateNormals; }

  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  vtkm::filter::DataSetResult DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  bool DoMapField(vtkm::filter::DataSetResult& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);

private:
  double IsoValue;
  bool MergeDuplicatePoints;
  bool GenerateNormals;

  vtkm::cont::ArrayHandle<vtkm::IdComponent> EdgeTable;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTable;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> TriangleTable;


  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpolationWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpolationIds;
};

template<>
class FilterTraits<MarchingCubes>
{
public:
  struct TypeListTagMCScalars : vtkm::ListTagBase<vtkm::UInt8, vtkm::Int8,
                                                  vtkm::Float32,vtkm::Float64> { };
  typedef TypeListTagMCScalars InputFieldTypeList;
};

}
} // namespace vtkm::filter


#include <vtkm/filter/MarchingCubes.hxx>

#endif // vtk_m_filter_MarchingCubes_h
