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

#ifndef vtk_m_filter_DataSetWithFieldFilter_h
#define vtk_m_filter_DataSetWithFieldFilter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/filter/PolicyBase.h>
#include <vtkm/filter/Result.h>

namespace vtkm
{
namespace filter
{

template <class Derived>
class FilterDataSetWithField
{
public:
  VTKM_CONT
  FilterDataSetWithField();

  VTKM_CONT
  ~FilterDataSetWithField();

  VTKM_CONT
  void SetActiveCellSetIndex(vtkm::Id index) { this->CellSetIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCellSetIndex() const { return this->CellSetIndex; }

  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::Id index) { this->CoordinateSystemIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }

  VTKM_CONT
  void SetRuntimeDeviceTracker(const vtkm::cont::RuntimeDeviceTracker& tracker)
  {
    this->Tracker = tracker;
  }

  VTKM_CONT
  const vtkm::cont::RuntimeDeviceTracker& GetRuntimeDeviceTracker() const { return this->Tracker; }

  VTKM_CONT
  Result Execute(const vtkm::cont::DataSet& input, const std::string& inFieldName);

  VTKM_CONT
  Result Execute(const vtkm::cont::DataSet& input, const vtkm::cont::Field& field);

  VTKM_CONT
  Result Execute(const vtkm::cont::DataSet& input, const vtkm::cont::CoordinateSystem& field);

  VTKM_CONT
  std::vector<vtkm::filter::Result> Execute(const vtkm::cont::MultiBlock& input,
                                            const std::string& inFieldName);

  template <typename DerivedPolicy>
  VTKM_CONT std::vector<vtkm::filter::Result> Execute(
    const vtkm::cont::MultiBlock& input,
    const std::string& inFieldName,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT Result Execute(const vtkm::cont::DataSet& input,
                           const std::string& inFieldName,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT Result Execute(const vtkm::cont::DataSet& input,
                           const vtkm::cont::Field& field,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT Result Execute(const vtkm::cont::DataSet& input,
                           const vtkm::cont::CoordinateSystem& field,
                           const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //From the field we can extract the association component
  // ASSOC_ANY -> unable to map
  // ASSOC_WHOLE_MESH -> (I think this is points)
  // ASSOC_POINTS -> map using point mapping
  // ASSOC_CELL_SET -> how do we map this?
  // ASSOC_LOGICAL_DIM -> unable to map?
  VTKM_CONT
  bool MapFieldOntoOutput(Result& result, const vtkm::cont::Field& field);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(Result& result,
                                    const vtkm::cont::Field& field,
                                    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  template <typename DerivedPolicy>
  VTKM_CONT Result PrepareForExecution(const vtkm::cont::DataSet& input,
                                       const vtkm::cont::Field& field,
                                       const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //How do we specify float/double coordinate types?
  template <typename DerivedPolicy>
  VTKM_CONT Result PrepareForExecution(const vtkm::cont::DataSet& input,
                                       const vtkm::cont::CoordinateSystem& field,
                                       const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  std::string OutputFieldName;
  vtkm::Id CellSetIndex;
  vtkm::Id CoordinateSystemIndex;
  vtkm::cont::RuntimeDeviceTracker Tracker;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterDataSetWithField.hxx>

#endif // vtk_m_filter_DataSetWithFieldFilter_h
