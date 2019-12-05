//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_SurfaceNormals_h
#define vtk_m_filter_SurfaceNormals_h

#include <vtkm/filter/FilterCell.h>

namespace vtkm
{
namespace filter
{

/// \brief compute normals for polygonal mesh
///
/// Compute surface normals on points and/or cells of a polygonal dataset.
/// The cell normals are faceted and are computed based on the plane where a
/// face lies. The point normals are smooth normals, computed by averaging
/// the face normals of incident cells.
class SurfaceNormals : public vtkm::filter::FilterCell<SurfaceNormals>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;

  /// Create SurfaceNormals filter. This calls
  /// this->SetUseCoordinateSystemAsField(true) since that is the most common
  /// use-case for surface normals.
  SurfaceNormals();

  /// Set/Get if cell normals should be generated. Default is off.
  /// @{
  void SetGenerateCellNormals(bool value) { this->GenerateCellNormals = value; }
  bool GetGenerateCellNormals() const { return this->GenerateCellNormals; }
  /// @}

  /// Set/Get if the cell normals should be normalized. Default value is true.
  /// The intended use case of this flag is for faster, approximate point
  /// normals generation by skipping the normalization of the face normals.
  /// Note that when set to false, the result cell normals will not be unit
  /// length normals and the point normals will be different.
  /// @{
  void SetNormalizeCellNormals(bool value) { this->NormalizeCellNormals = value; }
  bool GetNormalizeCellNormals() const { return this->NormalizeCellNormals; }
  /// @}

  /// Set/Get if the point normals should be generated. Default is on.
  /// @{
  void SetGeneratePointNormals(bool value) { this->GeneratePointNormals = value; }
  bool GetGeneratePointNormals() const { return this->GeneratePointNormals; }
  /// @}

  /// Set/Get the name of the cell normals field. Default is "Normals".
  /// @{
  void SetCellNormalsName(const std::string& name) { this->CellNormalsName = name; }
  const std::string& GetCellNormalsName() const { return this->CellNormalsName; }
  /// @}

  /// Set/Get the name of the point normals field. Default is "Normals".
  /// @{
  void SetPointNormalsName(const std::string& name) { this->PointNormalsName = name; }
  const std::string& GetPointNormalsName() const { return this->PointNormalsName; }
  /// @}

  /// If true, the normals will be oriented to face outwards from the surface.
  /// This requires a closed manifold surface or the behavior is undefined.
  /// This option is expensive but necessary for rendering.
  /// To make the normals point inward, set FlipNormals to true.
  /// Default is off.
  /// @{
  void SetAutoOrientNormals(bool v) { this->AutoOrientNormals = v; }
  bool GetAutoOrientNormals() const { return this->AutoOrientNormals; }
  /// @}

  /// Reverse the normals to point inward when AutoOrientNormals is true.
  /// Default is false.
  /// @{
  void SetFlipNormals(bool v) { this->FlipNormals = v; }
  bool GetFlipNormals() const { return this->FlipNormals; }
  /// @}

  /// Ensure that polygon winding is consistent with normal orientation.
  /// Triangles are wound such that their points are counter-clockwise around
  /// the generated cell normal. Default is true.
  /// @note This currently only affects triangles.
  /// @note This is only applied when cell normals are generated.
  /// @{
  void SetConsistency(bool v) { this->Consistency = v; }
  bool GetConsistency() const { return this->Consistency; }
  /// @}

  template <typename T, typename StorageType, typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
                                const vtkm::filter::FieldMetadata& fieldMeta,
                                vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  bool GenerateCellNormals;
  bool NormalizeCellNormals;
  bool GeneratePointNormals;
  bool AutoOrientNormals;
  bool FlipNormals;
  bool Consistency;

  std::string CellNormalsName;
  std::string PointNormalsName;
};
}
} // vtkm::filter

#include <vtkm/filter/SurfaceNormals.hxx>

#endif // vtk_m_filter_SurfaceNormals_h
