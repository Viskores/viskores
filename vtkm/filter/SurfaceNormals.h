//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
  /// Create SurfaceNormals filter. This calls
  /// this->SetUseCoordinateSystemAsField(true) since that is the most common
  /// use-case for surface normals.
  SurfaceNormals();

  /// Set/Get if cell normals should be generated. Default is off.
  void SetGenerateCellNormals(bool value) { this->GenerateCellNormals = value; }
  bool GetGenerateCellNormals() const { return this->GenerateCellNormals; }

  /// Set/Get if the cell normals should be normalized. Default value is true.
  /// The intended use case of this flag is for faster, approximate point
  /// normals generation by skipping the normalization of the face normals.
  /// Note that when set to false, the result cell normals will not be unit
  /// length normals and the point normals will be different.
  void SetNormalizeCellNormals(bool value) { this->NormalizeCellNormals = value; }
  bool GetNormalizeCellNormals() const { return this->NormalizeCellNormals; }

  /// Set/Get if the point normals should be generated. Default is on.
  void SetGeneratePointNormals(bool value) { this->GeneratePointNormals = value; }
  bool GetGeneratePointNormals() const { return this->GeneratePointNormals; }

  /// Set/Get the name of the cell normals field. Default is "Normals".
  void SetCellNormalsName(const std::string& name) { this->CellNormalsName = name; }
  const std::string& GetCellNormalsName() const { return this->CellNormalsName; }

  /// Set/Get the name of the point normals field. Default is "Normals".
  void SetPointNormalsName(const std::string& name) { this->PointNormalsName = name; }
  const std::string& GetPointNormalsName() const { return this->PointNormalsName; }

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
                                const vtkm::filter::FieldMetadata& fieldMeta,
                                const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                const DeviceAdapter& device);

private:
  bool GenerateCellNormals;
  bool NormalizeCellNormals;
  bool GeneratePointNormals;

  std::string CellNormalsName;
  std::string PointNormalsName;
};

template <>
class FilterTraits<SurfaceNormals>
{
public:
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
}
} // vtkm::filter

#include <vtkm/filter/SurfaceNormals.hxx>

#endif // vtk_m_filter_SurfaceNormals_h
