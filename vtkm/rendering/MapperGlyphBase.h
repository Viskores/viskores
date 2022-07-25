//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperGlyphBase_h
#define vtk_m_rendering_MapperGlyphBase_h

#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

class CanvasRayTracer;

class VTKM_RENDERING_EXPORT MapperGlyphBase : public Mapper
{
public:
  MapperGlyphBase();

  virtual ~MapperGlyphBase();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  virtual bool GetUseCells() const;
  virtual void SetUseCells();
  virtual bool GetUseNodes() const;
  virtual void SetUseNodes();

  virtual bool GetUseStride() const;
  virtual void SetUseStride(bool on);
  virtual vtkm::Id GetStride() const;
  virtual void SetStride(vtkm::Id stride);

  virtual vtkm::Float32 GetBaseSize() const;
  virtual void SetBaseSize(vtkm::Float32 size);
  virtual bool GetScaleByValue() const;
  virtual void SetScaleByValue(bool on);
  virtual vtkm::Float32 GetScaleDelta() const;
  virtual void SetScaleDelta(vtkm::Float32 delta);

  virtual void SetCompositeBackground(bool on);

protected:
  virtual vtkm::cont::DataSet FilterPoints(const vtkm::cont::UnknownCellSet& cellSet,
                                           const vtkm::cont::CoordinateSystem& coords,
                                           const vtkm::cont::Field& scalarField) const;


  vtkm::rendering::CanvasRayTracer* Canvas;
  bool CompositeBackground;

  bool UseNodes;

  bool UseStride;
  vtkm::Id Stride;

  bool ScaleByValue;
  vtkm::Float32 BaseSize;
  vtkm::Float32 ScaleDelta;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperGlyphBase_h
