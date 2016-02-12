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
#ifndef vtk_m_rendering_SceneRendererOSMesa_h
#define vtk_m_rendering_SceneRendererOSMesa_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/ColorTable.h>
#include <vtkm/rendering/View.h>
#include <vtkm/rendering/Triangulator.h>

#include <GL/osmesa.h>
#include <GL/gl.h>
#include <stdio.h>
#include <iostream>
#include <fstream>


namespace vtkm {
namespace rendering {

template<typename DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class SceneRendererOSMesa : public SceneRenderer
{
public:
  VTKM_CONT_EXPORT
  SceneRendererOSMesa()
  {
      ctx = NULL;
      width = 1024;
      height = 1024;
      rgba.resize(width*height*4);
  }

  VTKM_CONT_EXPORT
  virtual void RenderCells(const vtkm::cont::DynamicCellSet &cellset,
			   const vtkm::cont::CoordinateSystem &coords,
                           vtkm::cont::Field &scalarField,
                           const vtkm::rendering::ColorTable &colorTable,
                           vtkm::Float64 *scalarBounds)
  {
      //Doesn't work if you take the Init() out......
      //Init();
      SetupView();
      
      vtkm::cont::DynamicArrayHandleCoordinateSystem dcoords = coords.GetData();
      if (!dcoords.IsSameType(vtkm::cont::ArrayHandleUniformPointCoordinates()))
      {
	  std::cout<<"NOT UNIFORM!"<<std::endl;
	  return;
      }

      std::cout<<"RenderCells()"<<std::endl;
      vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Id, 4> > indices;
      vtkm::Id numTri;
      Triangulator<DeviceAdapter> triangulator;
      triangulator.run(cellset, indices, numTri);
      std::cout<<"NumTris= "<<numTri<<std::endl;
      //printSummary_ArrayHandle(indices, std::cout);

      vtkm::cont::ArrayHandle<vtkm::Float32> sf;
      sf = scalarField.GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Float32> >();
      //printSummary_ArrayHandle(sf, std::cout);

      vtkm::cont::ArrayHandleUniformPointCoordinates uVerts;
      uVerts = dcoords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      vtkm::rendering::ColorTable ct = colorTable;
      vtkm::Float32 sMin = vtkm::Float32(scalarBounds[0]);
      vtkm::Float32 sMax = vtkm::Float32(scalarBounds[1]);
      vtkm::Float32 sDiff = sMax-sMin;
      
      glBegin(GL_TRIANGLES);
      for (int i = 0; i < numTri; i++)
      {
	  vtkm::Vec<vtkm::Id, 4> idx = indices.GetPortalConstControl().Get(i);
	  vtkm::Id si = indices.GetPortalConstControl().Get(i)[0];
	  vtkm::Id i1 = indices.GetPortalConstControl().Get(i)[1];
	  vtkm::Id i2 = indices.GetPortalConstControl().Get(i)[2];
	  vtkm::Id i3 = indices.GetPortalConstControl().Get(i)[3];

	  vtkm::Vec<vtkm::Float32, 3> p1 = uVerts.GetPortalConstControl().Get(i1);
	  vtkm::Vec<vtkm::Float32, 3> p2 = uVerts.GetPortalConstControl().Get(i2);
	  vtkm::Vec<vtkm::Float32, 3> p3 = uVerts.GetPortalConstControl().Get(i3);
	  
	  vtkm::Float32 s = sf.GetPortalConstControl().Get(si);
          vtkm::Float32 sn = (s-sMin)/sDiff;
	  //Color color = ct.MapRGB(s);
	  Color color = colorTable.MapRGB(s);
	  std::cout<<i<<": "<<i1<<" "<<i2<<" "<<i3<<" si= "<<si<<" sn= "<<sn<<std::endl;
	  //std::cout<<"  color= "<<color.Components[0]<<" "<<color.Components[1]<<" "<<color.Components[2]<<std::endl;
          
	  s = sf.GetPortalConstControl().Get(i1);
          s = (s-sMin)/sDiff;

	  color = ct.MapRGB(s);
	  color.Components[0] = 0;
	  glColor3fv(color.Components);
	  glVertex3f(p1[0],p1[1],p1[2]);
	  
	  s = sf.GetPortalConstControl().Get(i2);
          s = (s-sMin)/sDiff;
	  color = ct.MapRGB(s);
	  color.Components[0] = 0;
	  glColor3fv(color.Components);
	  glVertex3f(p2[0],p2[1],p2[2]);

	  s = sf.GetPortalConstControl().Get(i3);
          s = (s-sMin)/sDiff;
	  color = ct.MapRGB(s);
	  color.Components[0] = 0;
	  glColor3fv(color.Components);
	  glVertex3f(p3[0],p3[1],p3[2]);
      }
      glEnd();
      glFinish();
      glFlush();
  }

private:
  VTKM_CONT_EXPORT
  virtual void Init()
  {
      ctx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, NULL);
      if (!ctx) std::cout<<"ERROR 0"<<std::endl;
      rgba.resize(width*height*4);
      if (!OSMesaMakeCurrent(ctx, &rgba[0], GL_UNSIGNED_BYTE, width, height))
	  std::cout<<"ERROR 1"<<std::endl;

      SetupView();
  }

  VTKM_CONT_EXPORT
  void SetupView()
  {
      vtkm::Float32 oglV[16], oglP[16];
      vtkm::Matrix<vtkm::Float32,4,4> vm = GetView().CreateViewMatrix();
      vtkm::Matrix<vtkm::Float32,4,4> pm = GetView().CreateProjectionMatrix();
      oglP[ 0] = pm[0][0];
      oglP[ 1] = pm[1][0];
      oglP[ 2] = pm[2][0];
      oglP[ 3] = pm[3][0];
      oglP[ 4] = pm[0][1];
      oglP[ 5] = pm[1][1];
      oglP[ 6] = pm[2][1];
      oglP[ 7] = pm[3][1];
      oglP[ 8] = pm[0][2];
      oglP[ 9] = pm[1][2];
      oglP[10] = pm[2][2];
      oglP[11] = pm[3][2];
      oglP[12] = pm[0][3];
      oglP[13] = pm[1][3];
      oglP[14] = pm[2][3];
      oglP[15] = pm[3][3];
      glMatrixMode(GL_PROJECTION);
      glLoadMatrixf(oglP);

      oglV[ 0] = vm[0][0];
      oglV[ 1] = vm[1][0];
      oglV[ 2] = vm[2][0];
      oglV[ 3] = vm[3][0];
      oglV[ 4] = vm[0][1];
      oglV[ 5] = vm[1][1];
      oglV[ 6] = vm[2][1];
      oglV[ 7] = vm[3][1];
      oglV[ 8] = vm[0][2];
      oglV[ 9] = vm[1][2];
      oglV[10] = vm[2][2];
      oglV[11] = vm[3][2];
      oglV[12] = vm[0][3];
      oglV[13] = vm[1][3];
      oglV[14] = vm[2][3];
      oglV[15] = vm[3][3];
      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixf(oglV);
  }

  VTKM_CONT_EXPORT
  void DumpImage()
  {
      std::ofstream of("output.pnm");
      of<<"P6"<<std::endl<<width<<" "<<height<<std::endl<<255<<std::endl;
      for (int i=height-1; i>=0; i--)
	  for (int j=0; j < width; j++)
	  { 
              const unsigned char *tuple = &(rgba[i*width*4 + j*4]);
	      of<<tuple[0]<<tuple[1]<<tuple[2];
	  }
      of.close();
  }
    
  OSMesaContext ctx;
  std::vector<unsigned char> rgba;
  std::vector<float> zbuff;
  int width, height;
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_SceneRendererOSMesa_h
