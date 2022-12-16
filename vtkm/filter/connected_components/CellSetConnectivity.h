//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_connected_components_CellSetConnectivity_h
#define vtk_m_filter_connected_components_CellSetConnectivity_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/connected_components/vtkm_filter_connected_components_export.h>

namespace vtkm
{
namespace filter
{
namespace connected_components
{

/// \brief Finds groups of cells that are connected together through their topology.
///
/// Finds and labels groups of cells that are connected together through their topology.
/// Two cells are considered connected if they share an edge. CellSetConnectivity identifies some
/// number of components and assigns each component a unique integer.
/// The result of the filter is a cell field of type vtkm::Id with the default name of 'component'.
/// Each entry in the cell field will be a number that identifies to which component the cell belongs.
class VTKM_FILTER_CONNECTED_COMPONENTS_EXPORT CellSetConnectivity : public vtkm::filter::FilterField
{
public:
  VTKM_CONT CellSetConnectivity() { this->SetOutputFieldName("component"); }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace connected_components

} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_connected_components_CellSetConnectivity_h
