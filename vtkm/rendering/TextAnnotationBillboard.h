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
#ifndef vtkm_rendering_TextAnnotationBillboard_h
#define vtkm_rendering_TextAnnotationBillboard_h

#include <vtkm/rendering/TextAnnotation.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT TextAnnotationBillboard : public TextAnnotation
{
protected:
  vtkm::Vec<vtkm::Float32, 3> Position;
  vtkm::Float32 Angle;

public:
  TextAnnotationBillboard(const std::string& text,
                          const vtkm::rendering::Color& color,
                          vtkm::Float32 scalar,
                          const vtkm::Vec<vtkm::Float32, 3>& position,
                          vtkm::Float32 angleDegrees = 0);

  ~TextAnnotationBillboard();

  void SetPosition(const vtkm::Vec<vtkm::Float32, 3>& position);

  void SetPosition(vtkm::Float32 posx, vtkm::Float32 posy, vtkm::Float32 posz);

  void Render(const vtkm::rendering::Camera& camera,
              const vtkm::rendering::WorldAnnotator& worldAnnotator,
              vtkm::rendering::Canvas& canvas) const override;
};
}
} // namespace vtkm::rendering

#endif //vtkm_rendering_TextAnnotationBillboard_h
