//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_flow_FlowTypes_h
#define vtk_m_filter_flow_FlowTypes_h

namespace vtkm
{
namespace filter
{
namespace flow
{
enum class IntegrationSolverType
{
  RK4_TYPE = 0,
  EULER_TYPE,
};

enum class VectorFieldType
{
  VELOCITY_FIELD_TYPE = 0,
  ELECTRO_MAGNETIC_FIELD_TYPE,
};

enum FlowResultType
{
  UNKNOWN_TYPE = -1,
  PARTICLE_ADVECT_TYPE,
  STREAMLINE_TYPE,
};

}
}
}

#endif // vtk_m_filter_flow_FlowTypes_h
