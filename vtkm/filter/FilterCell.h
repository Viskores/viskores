//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CellFilter_h
#define vtk_m_filter_CellFilter_h

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{

template <class Derived>
class FilterCell : public vtkm::filter::FilterField<Derived>
{
public:
  VTKM_CONT
  FilterCell();

  VTKM_CONT
  ~FilterCell();

  VTKM_CONT
  void SetActiveCellSetIndex(vtkm::Id index) { this->CellSetIndex = index; }

  VTKM_CONT
  vtkm::Id GetActiveCellSetIndex() const { return this->CellSetIndex; }

protected:
  vtkm::Id CellSetIndex;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/FilterCell.hxx>

#endif // vtk_m_filter_CellFilter_h
