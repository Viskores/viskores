//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_StreamSurface_h
#define vtk_m_filter_StreamSurface_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/StreamSurface.h>

namespace vtkm
{
namespace filter
{
/// \brief generate stream surface geometry from polylines.

/// Takes as input a set of polylines.
/// Produces tubes along each polyline

class StreamSurface : public vtkm::filter::FilterDataSet<StreamSurface>
{
public:
  VTKM_CONT
  StreamSurface();

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
  vtkm::worklet::StreamSurface Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/StreamSurface.hxx>

#endif // vtk_m_filter_StreamSurface_h
