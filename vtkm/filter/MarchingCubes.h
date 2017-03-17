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

#include <vtkm/filter/FilterDataSetWithField.h>
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

class MarchingCubes : public vtkm::filter::FilterDataSetWithField<MarchingCubes>
{
public:
  VTKM_CONT
  MarchingCubes();

  VTKM_CONT
  void SetNumberOfIsoValues(vtkm::Id num);

  VTKM_CONT
  vtkm::Id GetNumberOfIsoValues() const;

  VTKM_CONT
  void SetIsoValue(vtkm::Float64 v) { this->SetIsoValue(0, v); }

  VTKM_CONT
  void SetIsoValue(vtkm::Id index, vtkm::Float64);

  VTKM_CONT
  void SetIsoValues(const std::vector<vtkm::Float64>& values);

  VTKM_CONT
  vtkm::Float64 GetIsoValue(vtkm::Id index) const;

  VTKM_CONT
  void SetMergeDuplicatePoints(bool on) { this->Worklet.SetMergeDuplicatePoints(on); }

  VTKM_CONT
  bool GetMergeDuplicatePoints() const  { return this->Worklet.GetMergeDuplicatePoints(); }

  VTKM_CONT
  void SetGenerateNormals(bool on) { this->GenerateNormals = on; }

  VTKM_CONT
  bool GetGenerateNormals() const  { return this->GenerateNormals; }

  VTKM_CONT
  void SetNormalArrayName(const std::string &name) { this->NormalArrayName = name; }

  VTKM_CONT
  const std::string& GetNormalArrayName() const { return this->NormalArrayName; }

  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT
  vtkm::filter::ResultDataSet DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                        const vtkm::filter::FieldMetadata& fieldMeta,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT
  bool DoMapField(vtkm::filter::ResultDataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);

private:
  std::vector<vtkm::Float64> IsoValues;
  bool GenerateNormals;
  std::string NormalArrayName;
  vtkm::worklet::MarchingCubes Worklet;
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
