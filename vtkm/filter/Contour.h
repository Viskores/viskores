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

#include <vtkm/filter/vtkm_filter_contour_export.h>

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/filter/Instantiations.h>
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
class VTKM_FILTER_CONTOUR_EXPORT Contour : public vtkm::filter::FilterDataSetWithField<Contour>
{
public:
  using SupportedTypes = vtkm::List<vtkm::UInt8, vtkm::Int8, vtkm::Float32, vtkm::Float64>;

  VTKM_CONT
  Filter* Clone() const override
  {
    Contour* clone = new Contour();
    clone->CopyStateFrom(this);
    return clone;
  }

  VTKM_CONT
  bool CanThread() const override { return true; }

  Contour();

  void SetNumberOfIsoValues(vtkm::Id num);

  vtkm::Id GetNumberOfIsoValues() const;

  void SetIsoValue(vtkm::Float64 v) { this->SetIsoValue(0, v); }

  void SetIsoValue(vtkm::Id index, vtkm::Float64);

  void SetIsoValues(const std::vector<vtkm::Float64>& values);

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

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    if (field.IsFieldPoint())
    {
      // If the field is a point field, then we need to do a custom interpolation of the points.
      // In this case, we need to call the superclass's MapFieldOntoOutput, which will in turn
      // call our DoMapField.
      return this->FilterDataSetWithField<Contour>::MapFieldOntoOutput(result, field, policy);
    }
    else if (field.IsFieldCell())
    {
      // Use the precompiled field permutation function.
      vtkm::cont::ArrayHandle<vtkm::Id> permutation = this->Worklet.GetCellIdMap();
      return vtkm::filter::MapFieldPermutation(field, permutation, result);
    }
    else if (field.IsFieldGlobal())
    {
      result.AddField(field);
      return true;
    }
    else
    {
      return false;
    }
  }

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    // All other conditions should be handled by MapFieldOntoOutput directly.
    VTKM_ASSERT(fieldMeta.IsPointField());

    vtkm::cont::ArrayHandle<T> fieldArray;

    fieldArray = this->Worklet.ProcessPointField(input);

    //use the same meta data as the input so we get the same field name, etc.
    result.AddField(fieldMeta.AsField(fieldArray));
    return true;
  }

protected:
  VTKM_CONT
  void CopyStateFrom(const Contour* contour)
  {
    this->FilterDataSetWithField<Contour>::CopyStateFrom(contour);

    this->IsoValues = contour->IsoValues;
    this->GenerateNormals = contour->GenerateNormals;
    this->AddInterpolationEdgeIds = contour->AddInterpolationEdgeIds;
    this->ComputeFastNormalsForStructured = contour->ComputeFastNormalsForStructured;
    this->ComputeFastNormalsForUnstructured = contour->ComputeFastNormalsForUnstructured;
    this->NormalArrayName = contour->NormalArrayName;
    this->InterpolationEdgeIdsArrayName = contour->InterpolationEdgeIdsArrayName;
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

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Int8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Float32>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Float64>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

}
} // namespace vtkm::filter

#endif // vtk_m_filter_Contour_h
