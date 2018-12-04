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

#ifndef vtk_m_filter_GhostZone_h
#define vtk_m_filter_GhostZone_h

#include <vtkm/GhostCell.h>
#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/Threshold.h>

namespace vtkm
{
namespace filter
{

struct GhostZonePolicy : vtkm::filter::PolicyBase<GhostZonePolicy>
{
  using FieldTypeList = vtkm::ListTagBase<vtkm::UInt8>;
};

/// \brief Removes ghost zones
///
class GhostZone : public vtkm::filter::FilterDataSetWithField<GhostZone>
{
public:
  VTKM_CONT
  GhostZone();

  VTKM_CONT
  void RemoveAllGhost() { this->RemoveAll = true; }
  VTKM_CONT
  void RemoveByType(const vtkm::UInt8& vals)
  {
    this->RemoveAll = false;
    this->RemoveVals = vals;
  }
  VTKM_CONT
  bool GetRemoveAllGhost() const { return this->RemoveAll; }

  VTKM_CONT
  void ConvertOutputToUnstructured() { this->ConvertToUnstructured = true; }

  VTKM_CONT
  vtkm::UInt8 GetRemoveByType() const { return !this->RemoveAll; }
  VTKM_CONT
  vtkm::UInt8 GetRemoveType() const { return this->RemoveVals; }
  VTKM_CONT
  bool GetConvertOutputToUnstructured() { return this->ConvertToUnstructured; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  bool RemoveAll;
  bool ConvertToUnstructured;
  vtkm::UInt8 RemoveVals;
  vtkm::worklet::Threshold Worklet;

  VTKM_CONT vtkm::cont::CellSetExplicit<> ConvertOutputToUnstructured(
    vtkm::cont::DynamicCellSet& inCells);
};

template <>
class FilterTraits<GhostZone>
{ //currently the ghostzone filter only works on uint8 data.
public:
  using InputFieldTypeList = vtkm::ListTagBase<vtkm::UInt8>;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/GhostZone.hxx>

#endif // vtk_m_filter_GhostZone_h
