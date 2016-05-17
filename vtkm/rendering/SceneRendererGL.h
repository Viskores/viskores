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
#ifndef vtk_m_rendering_SceneRendererGL_h
#define vtk_m_rendering_SceneRendererGL_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Triangulator.h>

#include <GL/gl.h>
#include <stdio.h>
#include <iostream>
#include <fstream>


namespace vtkm {
namespace rendering {

template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class SceneRendererGL : public SceneRenderer
{
public:
  VTKM_CONT_EXPORT
  SceneRendererGL() {}

  VTKM_CONT_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
                           const vtkm::cont::CoordinateSystem &coords,
                           vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &colorTable,
                           vtkm::rendering::View &,
                           vtkm::Float64 *scalarBounds)
  {
    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > indices;
    vtkm::Id numTri;
    Triangulator<DeviceAdapter> triangulator;
    triangulator.run(cellset, indices, numTri);

    vtkm::cont::ArrayHandle<vtkm::Float32> sf;
    sf = scalarField.GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32> >();

    vtkm::cont::DynamicArrayHandleCoordinateSystem dcoords = coords.GetData();
    vtkm::cont::ArrayHandleUniformPointCoordinates uVerts;
    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > eVerts;

    if(dcoords.IsSameType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
    {
      uVerts = dcoords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      RenderTriangles(numTri, uVerts, indices, sf, colorTable, scalarBounds);
    }
    else if(dcoords.IsSameType(vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> >()))
    {
      eVerts = dcoords.Cast<vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > > ();
      RenderTriangles(numTri, eVerts, indices, sf, colorTable, scalarBounds);
    }
    else if(dcoords.IsSameType(vtkm::cont::ArrayHandleCartesianProduct<
                               vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                               vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                               vtkm::cont::ArrayHandle<vtkm::FloatDefault> >()))
    {
      vtkm::cont::ArrayHandleCartesianProduct<
          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
          vtkm::cont::ArrayHandle<vtkm::FloatDefault> > rVerts;
      rVerts = dcoords.Cast<vtkm::cont::ArrayHandleCartesianProduct<
                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                vtkm::cont::ArrayHandle<vtkm::FloatDefault> > > ();
      RenderTriangles(numTri, rVerts, indices, sf, colorTable, scalarBounds);
    }
    glFinish();
    glFlush();
  }

  template <typename PtType>
  VTKM_CONT_EXPORT
  void RenderTriangles(vtkm::Id numTri, const PtType &verts,
                       const vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > &indices,
                       const vtkm::cont::ArrayHandle<vtkm::Float32> &scalar,
                       const vtkm::rendering::ColorTable &ct,
                       vtkm::Float64 *scalarBounds)
  {
    vtkm::Float32 sMin = vtkm::Float32(scalarBounds[0]);
    vtkm::Float32 sMax = vtkm::Float32(scalarBounds[1]);
    vtkm::Float32 sDiff = sMax-sMin;

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < numTri; i++)
    {
      vtkm::Vec<vtkm::Id, 4> idx = indices.GetPortalConstControl().Get(i);
      vtkm::Id i1 = idx[1];
      vtkm::Id i2 = idx[2];
      vtkm::Id i3 = idx[3];

      vtkm::Vec<vtkm::Float32, 3> p1 = verts.GetPortalConstControl().Get(idx[1]);
      vtkm::Vec<vtkm::Float32, 3> p2 = verts.GetPortalConstControl().Get(idx[2]);
      vtkm::Vec<vtkm::Float32, 3> p3 = verts.GetPortalConstControl().Get(idx[3]);

      vtkm::Float32 s = scalar.GetPortalConstControl().Get(i1);
      s = (s-sMin)/sDiff;

      Color color = ct.MapRGB(s);
      glColor3fv(color.Components);
      glVertex3f(p1[0],p1[1],p1[2]);

      s = scalar.GetPortalConstControl().Get(i2);
      s = (s-sMin)/sDiff;
      color = ct.MapRGB(s);
      glColor3fv(color.Components);
      glVertex3f(p2[0],p2[1],p2[2]);

      s = scalar.GetPortalConstControl().Get(i3);
      s = (s-sMin)/sDiff;
      color = ct.MapRGB(s);
      glColor3fv(color.Components);
      glVertex3f(p3[0],p3[1],p3[2]);
    }
    glEnd();
  }
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_SceneRendererGL_h
