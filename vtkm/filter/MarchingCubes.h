//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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

namespace vtkm
{
namespace filter
{
/// \brief generate isosurface(s) from a Volume

/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
/// @warning
/// This filter is currently only supports 3D volumes.
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

  /// Set/Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  VTKM_CONT
  void SetMergeDuplicatePoints(bool on) { this->Worklet.SetMergeDuplicatePoints(on); }

  VTKM_CONT
  bool GetMergeDuplicatePoints() const { return this->Worklet.GetMergeDuplicatePoints(); }

  /// Set/Get whether normals should be generated. Off by default. If enabled,
  /// the default behaviour is to generate high quality normals for structured
  /// datasets, using gradients, and generate fast normals for unstructured
  /// datasets based on the result triangle mesh.
  ///
  VTKM_CONT
  void SetGenerateNormals(bool on) { this->GenerateNormals = on; }
  VTKM_CONT
  bool GetGenerateNormals() const { return this->GenerateNormals; }

  /// Set/Get whether the fast path should be used for normals computation for
  /// structured datasets. Off by default.
  VTKM_CONT
  void SetComputeFastNormalsForStructured(bool on) { this->ComputeFastNormalsForStructured = on; }
  VTKM_CONT
  bool GetComputeFastNormalsForStructured() const { return this->ComputeFastNormalsForStructured; }

  /// Set/Get whether the fast path should be used for normals computation for
  /// unstructured datasets. On by default.
  VTKM_CONT
  void SetComputeFastNormalsForUnstructured(bool on)
  {
    this->ComputeFastNormalsForUnstructured = on;
  }
  VTKM_CONT
  bool GetComputeFastNormalsForUnstructured() const
  {
    return this->ComputeFastNormalsForUnstructured;
  }

  VTKM_CONT
  void SetNormalArrayName(const std::string& name) { this->NormalArrayName = name; }

  VTKM_CONT
  const std::string& GetNormalArrayName() const { return this->NormalArrayName; }

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                          const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            const DeviceAdapter& tag);

private:
  std::vector<vtkm::Float64> IsoValues;
  bool GenerateNormals;
  bool ComputeFastNormalsForStructured;
  bool ComputeFastNormalsForUnstructured;
  std::string NormalArrayName;
  vtkm::worklet::MarchingCubes Worklet;
};

template <>
class FilterTraits<MarchingCubes>
{
public:
  struct TypeListTagMCScalars
    : vtkm::ListTagBase<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>
  {
  };
  using InputFieldTypeList = TypeListTagMCScalars;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/MarchingCubes.hxx>

#endif // vtk_m_filter_MarchingCubes_h
