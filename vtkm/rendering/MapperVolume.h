//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_MapperVolume_h
#define vtk_m_rendering_MapperVolume_h

#include <vtkm/rendering/Mapper.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/shared_ptr.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace rendering {

class MapperVolume : public Mapper
{
public:
  VTKM_RENDERING_EXPORT
  MapperVolume();

  VTKM_RENDERING_EXPORT
  void SetCanvas(vtkm::rendering::Canvas *canvas) VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           const vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &, //colorTable
                           const vtkm::rendering::Camera &camera,
                           const vtkm::Range &scalarRange) VTKM_OVERRIDE;

  VTKM_RENDERING_EXPORT
  virtual void StartScene() VTKM_OVERRIDE;
  VTKM_RENDERING_EXPORT
  virtual void EndScene() VTKM_OVERRIDE;

private:
  struct InternalsType;
  boost::shared_ptr<InternalsType> Internals;

  struct RenderFunctor;
};

}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperVolume_h
