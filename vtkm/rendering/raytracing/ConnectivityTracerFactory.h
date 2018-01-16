//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_ConnectivityTracerFactory_h
#define vtk_m_rendering_raytracing_ConnectivityTracerFactory_h

#include <vtkm/rendering/raytracing/ConnectivityBase.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/cont/DynamicCellSet.h>

namespace vtkm
{
namespace cont
{ //forward declares
class CoordinateSystem;
}

namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT ConnectivityTracerFactory
{
public:
  enum TracerType
  {
    Unsupported = 0,
    Structured = 1,
    Unstructured = 2,
    UnstructuredHex = 3,
    UnstructuredTet = 4,
    UnstructuredWedge = 5,
    UnstructuredPyramid = 6
  };

  //----------------------------------------------------------------------------
  static TracerType DetectCellSetType(const vtkm::cont::DynamicCellSet& cellset);

  //----------------------------------------------------------------------------
  static ConnectivityBase* CreateTracer(const vtkm::cont::DynamicCellSet& cellset,
                                        const vtkm::cont::CoordinateSystem& coords);
};
}
}
} // namespace vtkm::rendering::raytracing
#endif
