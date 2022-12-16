//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_mesh_info_GhostCellClassify_h
#define vtk_m_filter_mesh_info_GhostCellClassify_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{
class VTKM_FILTER_MESH_INFO_EXPORT GhostCellClassify : public vtkm::filter::Filter
{
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
  std::string GhostCellName;

public:
  VTKM_CONT GhostCellClassify()
    : Filter()
    , GhostCellName(vtkm::cont::GetGlobalGhostCellFieldName())
  {
  }

  VTKM_CONT void SetGhostCellName(const std::string& fieldName) { this->GhostCellName = fieldName; }
  VTKM_CONT const std::string& GetGhostCellName() { return this->GhostCellName; }
};
} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_GhostCellClassify_h
