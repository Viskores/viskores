//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Contour_h
#define vtk_m_filter_Contour_h

#include <vtkm/filter/vtkm_filter_export.h>

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/Contour.h>

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
class VTKM_ALWAYS_EXPORT Contour : public vtkm::filter::FilterDataSetWithField<Contour>
{
public:
  using SupportedTypes = vtkm::List<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>;

  VTKM_FILTER_EXPORT
  Contour();

  VTKM_FILTER_EXPORT
  void SetNumberOfIsoValues(vtkm::Id num);

  VTKM_FILTER_EXPORT
  vtkm::Id GetNumberOfIsoValues() const;

  VTKM_FILTER_EXPORT
  void SetIsoValue(vtkm::Float64 v) { this->SetIsoValue(0, v); }

  VTKM_FILTER_EXPORT
  void SetIsoValue(vtkm::Id index, vtkm::Float64);

  VTKM_FILTER_EXPORT
  void SetIsoValues(const std::vector<vtkm::Float64>& values);

  VTKM_FILTER_EXPORT
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

  /// Set/Get whether to append the ids of the intersected edges to the vertices of the isosurface triangles. Off by default.
  VTKM_CONT
  void SetAddInterpolationEdgeIds(bool on) { this->AddInterpolationEdgeIds = on; }
  VTKM_CONT
  bool GetAddInterpolationEdgeIds() const { return this->AddInterpolationEdgeIds; }

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

  template <typename T, typename StorageType, typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                const vtkm::filter::FieldMetadata& fieldMeta,
                                vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    vtkm::cont::ArrayHandle<T> fieldArray;

    if (fieldMeta.IsPointField())
    {
      fieldArray = this->Worklet.ProcessPointField(input);
    }
    else if (fieldMeta.IsCellField())
    {
      fieldArray = this->Worklet.ProcessCellField(input);
    }
    else
    {
      return false;
    }

    //use the same meta data as the input so we get the same field name, etc.
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }

private:
  std::vector<vtkm::Float64> IsoValues;
  bool GenerateNormals;
  bool AddInterpolationEdgeIds;
  bool ComputeFastNormalsForStructured;
  bool ComputeFastNormalsForUnstructured;
  std::string NormalArrayName;
  std::string InterpolationEdgeIdsArrayName;
  vtkm::worklet::Contour Worklet;
};

#ifndef vtkm_filter_Contour_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(Contour);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/Contour.hxx>

#endif // vtk_m_filter_Contour_h
