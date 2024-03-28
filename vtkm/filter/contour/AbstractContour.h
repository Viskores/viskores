//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_AbstractContour_h
#define vtk_m_filter_contour_AbstractContour_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>
#include <vtkm/filter/vector_analysis/SurfaceNormals.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief Contour filter interface
///
/// Provides common configuration & execution methods for contour filters
/// Only the method `DoExecute` executing the contour algorithm needs to be implemented
class VTKM_FILTER_CONTOUR_EXPORT AbstractContour : public vtkm::filter::Filter
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

  /// @brief Set a field value on which to extract a contour.
  ///
  /// This form of the method is usually used when only one contour is being extracted.
  void SetIsoValue(vtkm::Float64 v) { this->SetIsoValue(0, v); }

  /// @brief Set a field value on which to extract a contour.
  ///
  /// This form is used to specify multiple contours. The method is called
  /// multiple times with different @a index parameters.
  void SetIsoValue(vtkm::Id index, vtkm::Float64 v)
  {
    std::size_t i = static_cast<std::size_t>(index);
    if (i >= this->IsoValues.size())
    {
      this->IsoValues.resize(i + 1);
    }
    this->IsoValues[i] = v;
  }

  /// @brief Set multiple iso values at once.
  ///
  /// The iso values can be specified as either a `std::vector` or an initializer list.
  /// So, both
  ///
  /// @code{.cpp}
  /// std::vector<vtkm::Float64> isovalues = { 0.2, 0.5, 0.7 };
  /// contour.SetIsoValues(isovalues);
  /// @endcode
  ///
  /// and
  ///
  /// @code{.cpp}
  /// contour.SetIsoValues({ 0.2, 0.5, 0.7 });
  /// @endcode
  ///
  /// work.
  void SetIsoValues(const std::vector<vtkm::Float64>& values) { this->IsoValues = values; }

  /// @brief Return a value used to contour the mesh.
  vtkm::Float64 GetIsoValue(vtkm::Id index = 0) const
  {
    return this->IsoValues[static_cast<std::size_t>(index)];
  }

  /// @brief Set whether normals should be generated.
  ///
  /// Normals are used in shading calculations during rendering and can make the
  /// surface appear more smooth.
  ///
  /// Off by default.
  VTKM_CONT
  void SetGenerateNormals(bool flag) { this->GenerateNormals = flag; }
  /// Get whether normals should be generated.
  VTKM_CONT
  bool GetGenerateNormals() const { return this->GenerateNormals; }

  /// Set whether to append the ids of the intersected edges to the vertices of the isosurface
  /// triangles. Off by default.
  VTKM_CONT
  void SetAddInterpolationEdgeIds(bool flag) { this->AddInterpolationEdgeIds = flag; }
  /// Get whether to append the ids of the intersected edges to the vertices of the isosurface
  /// triangles.
  VTKM_CONT
  bool GetAddInterpolationEdgeIds() const { return this->AddInterpolationEdgeIds; }

  /// @brief Set whether the fast path should be used for normals computation.
  ///
  /// When this flag is off (the default), the generated normals are based on
  /// the gradient of the field being contoured and can be quite expensive to compute.
  /// When the flag is on, a faster method that computes the normals based on the faces
  /// of the isosurface mesh is used, but the normals do not look as good as the
  /// gradient based normals.
  ///
  /// This flag has no effect if `SetGenerateNormals` is false.
  VTKM_CONT
  void SetComputeFastNormals(bool flag) { this->ComputeFastNormals = flag; }
  /// Get whether the fast path should be used for normals computation.
  VTKM_CONT
  bool GetComputeFastNormals() const { return this->ComputeFastNormals; }

  /// Set the name of the field for the generated normals.
  VTKM_CONT
  void SetNormalArrayName(const std::string& name) { this->NormalArrayName = name; }

  /// Get the name of the field for the generated normals.
  VTKM_CONT
  const std::string& GetNormalArrayName() const { return this->NormalArrayName; }

  /// Set whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together. Duplicate points are identified
  /// by the unique edge it was generated from.
  ///
  /// Because the contour filter (like all filters in VTK-m) runs in parallel, parallel
  /// threads can (and often do) create duplicate versions of points. When this flag is
  /// set to true, a secondary operation will find all duplicated points and combine
  /// them together. If false, points will be duplicated. In addition to requiring more
  /// storage, duplicated points mean that triangles next to each other will not be
  /// considered adjecent to subsequent filters.
  ///
  VTKM_CONT
  void SetMergeDuplicatePoints(bool on) { this->MergeDuplicatedPoints = on; }

  /// Get whether the points generated should be unique for every triangle
  /// or will duplicate points be merged together.
  VTKM_CONT
  bool GetMergeDuplicatePoints() { return this->MergeDuplicatedPoints; }

protected:
  /// \brief Map a given field to the output \c DataSet , depending on its type.
  ///
  /// The worklet needs to implement \c ProcessPointField to process point fields as arrays
  /// and \c GetCellIdMap function giving the cell id mapping from input to output
  template <typename WorkletType>
  VTKM_CONT static bool DoMapField(vtkm::cont::DataSet& result,
                                   const vtkm::cont::Field& field,
                                   WorkletType& worklet)
  {
    if (field.IsPointField())
    {
      vtkm::cont::UnknownArrayHandle inputArray = field.GetData();
      vtkm::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();

      auto functor = [&](const auto& concrete) {
        using ComponentType = typename std::decay_t<decltype(concrete)>::ValueType::ComponentType;
        auto fieldArray = outputArray.ExtractArrayFromComponents<ComponentType>();
        worklet.ProcessPointField(concrete, fieldArray);
      };
      inputArray.CastAndCallWithExtractedArray(functor);
      result.AddPointField(field.GetName(), outputArray);
      return true;
    }
    else if (field.IsCellField())
    {
      // Use the precompiled field permutation function.
      vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetCellIdMap();
      return vtkm::filter::MapFieldPermutation(field, permutation, result);
    }
    else if (field.IsWholeDataSetField())
    {
      result.AddField(field);
      return true;
    }
    return false;
  }

  VTKM_CONT void ExecuteGenerateNormals(vtkm::cont::DataSet& output,
                                        const vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals)
  {
    if (this->GenerateNormals)
    {
      if (this->GetComputeFastNormals())
      {
        vtkm::filter::vector_analysis::SurfaceNormals surfaceNormals;
        surfaceNormals.SetPointNormalsName(this->NormalArrayName);
        surfaceNormals.SetGeneratePointNormals(true);
        output = surfaceNormals.Execute(output);
      }
      else
      {
        output.AddField(vtkm::cont::make_FieldPoint(this->NormalArrayName, normals));
      }
    }
  }

  template <typename WorkletType>
  VTKM_CONT void ExecuteAddInterpolationEdgeIds(vtkm::cont::DataSet& output, WorkletType& worklet)
  {
    if (this->AddInterpolationEdgeIds)
    {
      vtkm::cont::Field interpolationEdgeIdsField(this->InterpolationEdgeIdsArrayName,
                                                  vtkm::cont::Field::Association::Points,
                                                  worklet.GetInterpolationEdgeIds());
      output.AddField(interpolationEdgeIdsField);
    }
  }

  VTKM_CONT
  virtual vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& result) = 0; // Needs to be overridden by contour implementations

  std::vector<vtkm::Float64> IsoValues;
  bool GenerateNormals = true;
  bool ComputeFastNormals = false;

  bool AddInterpolationEdgeIds = false;
  bool MergeDuplicatedPoints = true;
  std::string NormalArrayName = "normals";
  std::string InterpolationEdgeIdsArrayName = "edgeIds";
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_AbstractContour_h
