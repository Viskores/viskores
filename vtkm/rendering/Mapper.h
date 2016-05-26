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
#ifndef vtk_m_rendering_Mapper_h
#define vtk_m_rendering_Mapper_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/ColorTable.h>
namespace vtkm {
namespace rendering {

class Mapper
{
public:
  VTKM_CONT_EXPORT
  Mapper()
  {
  }

  VTKM_CONT_EXPORT
  virtual ~Mapper()
  {}

  VTKM_CONT_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           const vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &colorTable,
                           const vtkm::rendering::Camera &camera,
                           const vtkm::Range &scalarRange) = 0;

  VTKM_CONT_EXPORT
  virtual void SetActiveColorTable(const ColorTable &ct)
  {
      ct.Sample(1024, ColorMap);
  }

    // needed for volume... Can we have a volume render surface??
  VTKM_CONT_EXPORT
  virtual void SetBackgroundColor(const vtkm::Vec<vtkm::Float32,4> &backgroundColor)
  {
      BackgroundColor = backgroundColor;
  }
  VTKM_CONT_EXPORT
  virtual void SetBackgroundColor(const vtkm::rendering::Color &backgroundColor)
  {
      BackgroundColor[0] = backgroundColor.Components[0];
      BackgroundColor[1] = backgroundColor.Components[1];
      BackgroundColor[2] = backgroundColor.Components[2];
      BackgroundColor[3] = backgroundColor.Components[3];
  }

    VTKM_CONT_EXPORT
    virtual void Render() {}
    VTKM_CONT_EXPORT
    virtual void Finish() {}
    VTKM_CONT_EXPORT
    virtual void StartScene()
    {
    }
    VTKM_CONT_EXPORT
    virtual void EndScene()
    {
    }
    virtual void SetCanvas(Canvas *vtkmNotUsed(surface))
    {
    }
protected:
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32,4> > ColorMap;
    vtkm::Vec<vtkm::Float32,4> BackgroundColor;
};
}} //namespace vtkm::rendering
#endif //vtk_m_rendering_Mapper_h
