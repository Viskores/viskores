//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Tube_h
#define vtk_m_filter_Tube_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Tube.h>

namespace vtkm
{
namespace filter
{
/// \brief generate tube geometry from polylines.

/// Takes as input a set of polylines, radius, num sides and capping flag.
/// Produces tubes along each polyline

class Tube : public vtkm::filter::FilterDataSet<Tube>
{
public:
  VTKM_CONT
  Tube();

  VTKM_CONT
  void SetRadius(vtkm::FloatDefault r) { this->Radius = r; }

  VTKM_CONT
  void SetNumberOfSides(vtkm::Id n) { this->NumberOfSides = n; }

  VTKM_CONT
  void SetCapping(bool v) { this->Capping = v; }

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<Policy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename Policy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<Policy> policy);

private:
  vtkm::worklet::Tube Worklet;
  vtkm::FloatDefault Radius;
  vtkm::Id NumberOfSides;
  bool Capping;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/Tube.hxx>

#endif // vtk_m_filter_Tube_h
