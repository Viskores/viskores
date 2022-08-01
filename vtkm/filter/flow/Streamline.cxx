//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/Streamline.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT vtkm::filter::flow::FlowResultType Streamline::GetResultType() const
{
  return vtkm::filter::flow::FlowResultType::STREAMLINE_TYPE;
}

}
}
} // namespace vtkm::filter::flow
