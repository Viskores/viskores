//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_particleadvection_ParticleAdvectionTypes_h
#define vtk_m_filter_particleadvection_ParticleAdvectionTypes_h

namespace vtkm
{
namespace filter
{
namespace particleadvection
{
enum IntegrationSolverType
{
  RK4_TYPE = 0,
  EULER_TYPE,
};

enum VectorFieldType
{
  VELOCITY_FIELD_TYPE = 0,
  ELECTRO_MAGNETIC_FIELD_TYPE,
};

enum ParticleAdvectionResultType
{
  PARTICLE_ADVECT_TYPE = 0,
  STREAMLINE_TYPE,
};

}
}
}


#endif // vtk_m_filter_particleadvection_ParticleAdvectionTypes_h
