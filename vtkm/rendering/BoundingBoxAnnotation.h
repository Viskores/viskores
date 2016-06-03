//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_BoundingBoxAnnotation_h
#define vtk_m_rendering_BoundingBoxAnnotation_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/WorldAnnotator.h>

namespace vtkm {
namespace rendering {

class BoundingBoxAnnotation
{
private:
  vtkm::rendering::Color Color;
  vtkm::Bounds Extents;

public:
  BoundingBoxAnnotation()
    : Color(0.5, 0.5, 0.5), Extents(-1, 1, -1, 1, -1, 1)
  {
  }
  virtual ~BoundingBoxAnnotation()
  {
  }
  vtkm::Bounds GetExtents() const
  {
    return this->Extents;
  }
  void SetExtents(const vtkm::Bounds &extents)
  {
    this->Extents = extents;
  }
  const vtkm::rendering::Color &GetColor() const
  {
    return this->Color;
  }
  void SetColor(vtkm::rendering::Color c)
  {
    this->Color = c;
  }
  virtual void Render(vtkm::rendering::Camera &,
                      WorldAnnotator &annotator)
  {
    //win->SetupForWorldSpace();

    vtkm::Float32 linewidth = 1.0;

    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Min,this->Extents.Z.Min,
                      this->Extents.X.Min,this->Extents.Y.Min,this->Extents.Z.Max,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Max,this->Extents.Z.Min,
                      this->Extents.X.Min,this->Extents.Y.Max,this->Extents.Z.Max,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Max,this->Extents.Y.Min,this->Extents.Z.Min,
                      this->Extents.X.Max,this->Extents.Y.Min,this->Extents.Z.Max,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Max,this->Extents.Y.Max,this->Extents.Z.Min,
                      this->Extents.X.Max,this->Extents.Y.Max,this->Extents.Z.Max,
                      linewidth, this->Color);

    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Min,this->Extents.Z.Min,
                      this->Extents.X.Min,this->Extents.Y.Max,this->Extents.Z.Min,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Min,this->Extents.Z.Max,
                      this->Extents.X.Min,this->Extents.Y.Max,this->Extents.Z.Max,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Max,this->Extents.Y.Min,this->Extents.Z.Min,
                      this->Extents.X.Max,this->Extents.Y.Max,this->Extents.Z.Min,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Max,this->Extents.Y.Min,this->Extents.Z.Max,
                      this->Extents.X.Max,this->Extents.Y.Max,this->Extents.Z.Max,
                      linewidth, this->Color);

    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Min,this->Extents.Z.Min,
                      this->Extents.X.Max,this->Extents.Y.Min,this->Extents.Z.Min,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Min,this->Extents.Z.Max,
                      this->Extents.X.Max,this->Extents.Y.Min,this->Extents.Z.Max,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Max,this->Extents.Z.Min,
                      this->Extents.X.Max,this->Extents.Y.Max,this->Extents.Z.Min,
                      linewidth, this->Color);
    annotator.AddLine(this->Extents.X.Min,this->Extents.Y.Max,this->Extents.Z.Max,
                      this->Extents.X.Max,this->Extents.Y.Max,this->Extents.Z.Max,
                      linewidth, this->Color);
  }
};


}} //namespace vtkm::rendering

#endif // vtk_m_rendering_BoundingBoxAnnotation_h

