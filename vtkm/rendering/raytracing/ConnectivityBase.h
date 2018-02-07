//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Connectivity_Base_h
#define vtk_m_rendering_raytracing_Connectivity_Base_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/raytracing/Ray.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT ConnectivityBase
{
public:
  enum IntegrationMode
  {
    Volume,
    Energy
  };

  ConnectivityBase();
  virtual ~ConnectivityBase();

  virtual void Trace(Ray<vtkm::Float64>& rays) = 0;

  virtual void Trace(Ray<vtkm::Float32>& rays) = 0;

  virtual void SetVolumeData(const vtkm::cont::Field& scalarField,
                             const vtkm::Range& scalarBounds) = 0;

  virtual void SetEnergyData(const vtkm::cont::Field& absorption,
                             const vtkm::Int32 numBins,
                             const vtkm::cont::Field& emission = vtkm::cont::Field()) = 0;

  virtual void SetBackgroundColor(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor) = 0;

  virtual void SetSampleDistance(const vtkm::Float32& distance) = 0;

  virtual void SetColorMap(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap) = 0;

}; // class ConnectivityBase
}
}
} // namespace vtkm::rendering::raytracing
#endif
