//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Triangulate_h
#define vtk_m_filter_Triangulate_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Triangulate.h>

namespace vtkm
{
namespace filter
{

class Triangulate : public vtkm::filter::FilterDataSet<Triangulate>
{
public:
  VTKM_CONT
  Triangulate();

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  // Map new field onto the resulting dataset after running the filter
  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::Triangulate Worklet;
};
}
} // namespace vtkm::filter

#ifndef vtk_m_filter_Triangulate_hxx
#include <vtkm/filter/Triangulate.hxx>
#endif

#endif // vtk_m_filter_Triangulate_h
