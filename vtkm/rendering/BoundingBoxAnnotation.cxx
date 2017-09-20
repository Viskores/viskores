//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/BoundingBoxAnnotation.h>

namespace vtkm
{
namespace rendering
{

BoundingBoxAnnotation::BoundingBoxAnnotation()
  : Color(0.5, 0.5, 0.5)
  , Extents(-1, 1, -1, 1, -1, 1)
{
}

BoundingBoxAnnotation::~BoundingBoxAnnotation()
{
}

void BoundingBoxAnnotation::Render(const vtkm::rendering::Camera&, const WorldAnnotator& annotator)
{
  //win->SetupForWorldSpace();

  vtkm::Float32 linewidth = 1.0;

  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);

  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);

  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    this->Extents.X.Max,
                    this->Extents.Y.Min,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Min,
                    linewidth,
                    this->Color);
  annotator.AddLine(this->Extents.X.Min,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    this->Extents.X.Max,
                    this->Extents.Y.Max,
                    this->Extents.Z.Max,
                    linewidth,
                    this->Color);
}
}
} // namespace vtkm::rendering
