//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/flow/PathParticle.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

VTKM_CONT vtkm::filter::flow::FlowResultType PathParticle::GetResultType() const
{
  return vtkm::filter::flow::FlowResultType::PARTICLE_ADVECT_TYPE;
}

}
}
} // namespace vtkm::filter::flow
