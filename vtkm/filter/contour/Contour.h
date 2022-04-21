//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_Contour_h
#define vtk_m_filter_contour_Contour_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief generate isosurface(s) from a Volume

/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
/// @warning
/// This filter is currently only supports 3D volumes.
class VTKM_FILTER_CONTOUR_EXPORT Contour : public vtkm::filter::NewFilterField
{
public:
  void SetNumberOfIsoValues(vtkm::Id num)
  {
    if (num >= 0)
    {
      this->IsoValues.resize(static_cast<std::size_t>(num));
    }
  }

  vtkm::Id GetNumberOfIsoValues() const { return static_cast<vtkm::Id>(this->IsoValues.size()); }

  void SetIsoValue(vtkm::Float64 v) { this->SetIsoValue(0, v); }

  void SetIsoValue(vtkm::Id index, vtkm::Float64 v)
  {
    std::size_t i = static_cast<std::size_t>(index);
    if (i >= this->IsoValues.size())
    {
      this->IsoValues.resize(i + 1);
    }
    this->IsoValues[i] = v;
  }

  void SetIsoValues(const std::vector<vtkm::Float64>& values) { this->IsoValues = values; }

  vtkm::Float64 GetIsoValue(vtkm::Id index) const
  {
    return this->IsoValues[static_cast<std::size_t>(index)];
  }

  /// Set/Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  VTKM_CONT
  void SetMergeDuplicatePoints(bool on);

  VTKM_CONT
  bool GetMergeDuplicatePoints() const;

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

private:
  VTKM_CONT

  std::vector<vtkm::Float64> IsoValues;
  bool GenerateNormals = false;
  bool AddInterpolationEdgeIds = false;
  bool ComputeFastNormalsForStructured = false;
  bool ComputeFastNormalsForUnstructured = true;
  bool MergeDuplicatedPoints = true;
  std::string NormalArrayName = "normals";
  std::string InterpolationEdgeIdsArrayName = "edgeIds";

protected:
  // Needed by the subclass Slice
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& result) override;
};
} // namespace contour
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::contour::Contour.") Contour
  : public vtkm::filter::contour::Contour
{
  using contour::Contour::Contour;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_Contour_h
