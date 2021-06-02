//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Slice_h
#define vtk_m_filter_Slice_h

#include <vtkm/filter/vtkm_filter_contour_export.h>

#include <vtkm/filter/Contour.h>
#include <vtkm/filter/FilterDataSet.h>

#include <vtkm/ImplicitFunction.h>

namespace vtkm
{
namespace filter
{

class VTKM_FILTER_CONTOUR_EXPORT Slice : public vtkm::filter::FilterDataSet<Slice>
{
public:
  /// Set/Get the implicit function that is used to perform the slicing.
  ///
  VTKM_CONT
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }
  VTKM_CONT
  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  /// Set/Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  VTKM_CONT
  void SetMergeDuplicatePoints(bool on) { this->ContourFilter.SetMergeDuplicatePoints(on); }
  VTKM_CONT
  bool GetMergeDuplicatePoints() const { return this->ContourFilter.GetMergeDuplicatePoints(); }

  /// Set/Get whether normals should be generated. Off by default. If enabled,
  /// the default behaviour is to generate high quality normals for structured
  /// datasets, using gradients, and generate fast normals for unstructured
  /// datasets based on the result triangle mesh.
  ///
  VTKM_CONT
  void SetGenerateNormals(bool on) { this->ContourFilter.SetGenerateNormals(on); }
  VTKM_CONT
  bool GetGenerateNormals() const { return this->ContourFilter.GetGenerateNormals(); }

  /// Set/Get whether to append the ids of the intersected edges to the vertices of the isosurface
  /// triangles. Off by default.
  VTKM_CONT
  void SetAddInterpolationEdgeIds(bool on) { this->ContourFilter.SetAddInterpolationEdgeIds(on); }
  VTKM_CONT
  bool GetAddInterpolationEdgeIds() const
  {
    return this->ContourFilter.GetAddInterpolationEdgeIds();
  }

  /// Set/Get whether the fast path should be used for normals computation for
  /// structured datasets. Off by default.
  VTKM_CONT
  void SetComputeFastNormalsForStructured(bool on)
  {
    this->ContourFilter.SetComputeFastNormalsForStructured(on);
  }
  VTKM_CONT
  bool GetComputeFastNormalsForStructured() const
  {
    return this->ContourFilter.GetComputeFastNormalsForStructured();
  }

  /// Set/Get whether the fast path should be used for normals computation for
  /// unstructured datasets. On by default.
  VTKM_CONT
  void SetComputeFastNormalsForUnstructured(bool on)
  {
    this->ContourFilter.SetComputeFastNormalsForUnstructured(on);
  }
  VTKM_CONT
  bool GetComputeFastNormalsForUnstructured() const
  {
    return this->ContourFilter.GetComputeFastNormalsForUnstructured();
  }

  VTKM_CONT
  void SetNormalArrayName(const std::string& name) { this->ContourFilter.SetNormalArrayName(name); }

  VTKM_CONT
  const std::string& GetNormalArrayName() const { return this->ContourFilter.GetNormalArrayName(); }

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    return this->ContourFilter.MapFieldOntoOutput(result, field, policy);
  }

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    return this->ContourFilter.DoMapField(result, input, fieldMeta, policy);
  }

private:
  vtkm::ImplicitFunctionGeneral Function;
  vtkm::filter::Contour ContourFilter;
};

#ifndef vtk_m_filter_Slice_cxx
extern template VTKM_FILTER_CONTOUR_TEMPLATE_EXPORT vtkm::cont::DataSet Slice::DoExecute(
  const vtkm::cont::DataSet&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
#endif

}
} // vtkm::filter

#endif // vtk_m_filter_Slice_h
