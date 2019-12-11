//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_GhostCellRemove_h
#define vtk_m_filter_GhostCellRemove_h

#include <vtkm/CellClassification.h>
#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/filter/Threshold.h>

namespace vtkm
{
namespace filter
{

struct GhostCellRemovePolicy : vtkm::filter::PolicyBase<GhostCellRemovePolicy>
{
  using FieldTypeList = vtkm::List<vtkm::UInt8>;
};

/// \brief Removes ghost cells
///
class GhostCellRemove : public vtkm::filter::FilterDataSetWithField<GhostCellRemove>
{
public:
  //currently the GhostCellRemove filter only works on uint8 data.
  using SupportedTypes = vtkm::List<vtkm::UInt8>;

  VTKM_CONT
  GhostCellRemove();

  VTKM_CONT
  void RemoveGhostField() { this->RemoveField = true; }
  VTKM_CONT
  void RemoveAllGhost() { this->RemoveAll = true; }
  VTKM_CONT
  void RemoveByType(const vtkm::UInt8& vals)
  {
    this->RemoveAll = false;
    this->RemoveVals = vals;
  }
  VTKM_CONT
  bool GetRemoveGhostField() { return this->RemoveField; }
  VTKM_CONT
  bool GetRemoveAllGhost() const { return this->RemoveAll; }

  VTKM_CONT
  bool GetRemoveByType() const { return !this->RemoveAll; }
  VTKM_CONT
  vtkm::UInt8 GetRemoveType() const { return this->RemoveVals; }

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
  bool RemoveField;
  vtkm::UInt8 RemoveVals;
  vtkm::worklet::Threshold Worklet;
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_GhostCellRemove_hxx
#include <vtkm/filter/GhostCellRemove.hxx>
#endif

#endif // vtk_m_filter_GhostCellRemove_h
