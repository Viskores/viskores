//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_ScalarRenderer_h
#define vtk_m_rendering_ScalarRenderer_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/Camera.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT ScalarRenderer
{
public:
  ScalarRenderer();

  ~ScalarRenderer();

  void SetInput(vtkm::cont::DataSet& dataSet);

  void SetWidth(const vtkm::Int32 width);
  void SetHeight(const vtkm::Int32 height);
  void SetDefaultValue(vtkm::Float32 value);

  struct VTKM_RENDERING_EXPORT Result
  {
    vtkm::Int32 Width;
    vtkm::Int32 Height;
    vtkm::cont::ArrayHandle<vtkm::Float32> Depths;
    std::vector<vtkm::cont::ArrayHandle<vtkm::Float32>> Scalars;
    std::vector<std::string> ScalarNames;
    std::map<std::string, vtkm::Range> Ranges;

    vtkm::cont::DataSet ToDataSet();
  };

  ScalarRenderer::Result Render(const vtkm::rendering::Camera& camera);


private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_ScalarRenderer_h
