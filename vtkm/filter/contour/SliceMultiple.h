//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_contour_Slice_Multi_h
#define vtk_m_filter_contour_Slice_Multi_h

#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

#include <vtkm/ImplicitFunction.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief This filter can accept multiple implicit functions used by the slice filter.
/// It returns a merged data set that contains multiple results returned by the slice filter.
class VTKM_FILTER_CONTOUR_EXPORT SliceMultiple : public vtkm::filter::contour::Contour
{
public:
  /// Set/Get the implicit function that is used to perform the slicing.
  ///
  VTKM_CONT
  void AddImplicitFunction(const vtkm::ImplicitFunctionGeneral& func)
  {
    FunctionList.push_back(func);
  }
  VTKM_CONT
  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction(vtkm::Id id) const
  {
    VTKM_ASSERT(id < static_cast<vtkm::Id>(FunctionList.size()));
    return FunctionList[id];
  }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  std::vector<vtkm::ImplicitFunctionGeneral> FunctionList;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_Slice_Multi_h
