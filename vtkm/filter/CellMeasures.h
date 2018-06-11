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

#ifndef vtk_m_filter_CellMeasures_h
#define vtk_m_filter_CellMeasures_h

#include "vtkm/filter/FilterCell.h"
#include "vtkm/worklet/CellMeasure.h"

namespace vtkm
{
namespace filter
{

/// \brief Compute the measure of each (3D) cell in a dataset.
///
/// CellMeasures is a filter that generates a new cell data array (i.e., one value
/// specified per cell) holding the signed measure of the cell
/// or 0 (if measure is not well defined or the cell type is unsupported).
///
/// By default, the new cell-data array is named "measure".
template <typename IntegrationType>
class CellMeasures : public vtkm::filter::FilterCell<CellMeasures<IntegrationType>>
{
public:
  VTKM_CONT
  CellMeasures();

  /// Set/Get the name of the cell measure field. If empty, "measure" is used.
  void SetCellMeasureName(const std::string& name) { this->SetOutputFieldName(name); }
  const std::string& GetCellMeasureName() const { return this->GetOutputFieldName(); }

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter& tag);
};

template <typename IntegrationType>
class FilterTraits<CellMeasures<IntegrationType>>
{
public:
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/CellMeasures.hxx>

#endif // vtk_m_filter_CellMeasures_h
