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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

template <class Derived>
class FilterDataSetWithField : public vtkm::filter::Filter<Derived>
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

  //@{
  /// Choose the field to operate on. Note, if
  /// `this->UseCoordinateSystemAsField` is true, then the active field is not used.
  VTKM_CONT
  void SetActiveField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->ActiveFieldName = name;
    this->ActiveFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetActiveFieldName() const { return this->ActiveFieldName; }
  VTKM_CONT vtkm::cont::Field::Association GetActiveFieldAssociation() const
  {
    return this->ActiveFieldAssociation;
  }
  //@}

  //@{
  /// To simply use the active coordinate system as the field to operate on, set
  /// UseCoordinateSystemAsField to true.
  VTKM_CONT
  void SetUseCoordinateSystemAsField(bool val) { this->UseCoordinateSystemAsField = val; }
  VTKM_CONT
  bool GetUseCoordinateSystemAsField() const { return this->UseCoordinateSystemAsField; }
  //@}

  //From the field we can extract the association component
  // Association::ANY -> unable to map
  // Association::WHOLE_MESH -> (I think this is points)
  // Association::POINTS -> map using point mapping
  // Association::CELL_SET -> how do we map this?
  // Association::LOGICAL_DIM -> unable to map?
  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(
    const vtkm::cont::DataSet& input,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::Field& field,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //How do we specify float/double coordinate types?
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::CoordinateSystem& field,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  std::string OutputFieldName;
  vtkm::Id CellSetIndex;
  vtkm::Id CoordinateSystemIndex;
  std::string ActiveFieldName;
  vtkm::cont::Field::Association ActiveFieldAssociation;
  bool UseCoordinateSystemAsField;

  friend class vtkm::filter::Filter<Derived>;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterDataSetWithField.hxx>

#endif // vtk_m_filter_DataSetWithFieldFilter_h
