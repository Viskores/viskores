//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DataSetFilter_h
#define vtk_m_filter_DataSetFilter_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/PartitionedDataSet.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/PolicyBase.h>

namespace vtkm
{
namespace filter
{

VTKM_DEPRECATED_SUPPRESS_BEGIN
template <class Derived>
class VTKM_DEPRECATED(1.9, "Use vtkm::filter::NewFilter.") FilterDataSet
  : public vtkm::filter::Filter<Derived>
{
public:
  VTKM_CONT
  FilterDataSet();

  VTKM_CONT
  ~FilterDataSet();

  VTKM_CONT
  void SetActiveCoordinateSystem(vtkm::Id index) { this->CoordinateSystemIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCoordinateSystemIndex() const { return this->CoordinateSystemIndex; }

  /// These are provided to satisfy the Filter API requirements.

  //From the field we can extract the association component
  // Association::Any -> unable to map
  // Association::WholeDataSet -> (I think this is points)
  // Association::Points -> map using point mapping
  // Association::Cells -> how do we map this?
  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet PrepareForExecution(const vtkm::cont::DataSet& input,
                                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

protected:
  vtkm::filter::FilterDataSet<Derived>& operator=(const vtkm::filter::FilterDataSet<Derived>&) =
    default;
  VTKM_CONT
  void CopyStateFrom(const FilterDataSet<Derived>* filter) { *this = *filter; }

private:
  vtkm::Id CoordinateSystemIndex;

  friend class vtkm::filter::Filter<Derived>;
};
VTKM_DEPRECATED_SUPPRESS_END
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterDataSet.hxx>

#endif // vtk_m_filter_DataSetFilter_h
