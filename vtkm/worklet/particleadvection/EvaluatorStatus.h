//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtk_m_worklet_particleadvection_EvaluatorStatus_h
#define vtk_m_worklet_particleadvection_EvaluatorStatus_h

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{
enum class EvaluatorStatus
{
  SUCCESS = 0,
  OUTSIDE_SPATIAL_BOUNDS,
  OUTSIDE_TEMPORAL_BOUNDS
};

} //namespace particleadvection
} //namespace worklet
} //namespace vtkm


#endif // vtk_m_worklet_particleadvection_EvaluatorStatus_h
