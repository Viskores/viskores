//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <complex>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/filter/geometry_refinement/Tube.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/cont/ColorTable.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/CanvasRayTracer.h>
#include <viskores/rendering/MapperRayTracer.h>
#include <viskores/rendering/Scene.h>
#include <viskores/rendering/View3D.h>

viskores::Vec3f ArchimedeanSpiralToCartesian(viskores::Vec3f const& p)
{
  // p[0] = r, p[1] = theta, p[2] = z:
  viskores::Vec3f xyz;
  auto c = std::polar(p[0], p[1]);
  xyz[0] = c.real();
  xyz[1] = c.imag();
  xyz[2] = p[2];
  return xyz;
}

void TubeThatSpiral(viskores::FloatDefault radius,
                    viskores::Id numLineSegments,
                    viskores::Id numSides)
{
  viskores::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<viskores::Id> ids;

  // The Archimedian spiral is defined by the equation r = a + b*theta.
  // To extend to a 3D curve, use z = t, theta = t, r = a + b t.
  viskores::FloatDefault a = viskores::FloatDefault(0.2);
  viskores::FloatDefault b = viskores::FloatDefault(0.8);
  for (viskores::Id i = 0; i < numLineSegments; ++i)
  {
    viskores::FloatDefault t = 4 * viskores::FloatDefault(3.1415926) * (i + 1) /
      numLineSegments; // roughly two spins around. Doesn't need to be perfect.
    viskores::FloatDefault r = a + b * t;
    viskores::FloatDefault theta = t;
    viskores::Vec3f cylindricalCoordinate{ r, theta, t };
    viskores::Vec3f spiralSample = ArchimedeanSpiralToCartesian(cylindricalCoordinate);
    viskores::Id pid = dsb.AddPoint(spiralSample);
    ids.push_back(pid);
  }
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  viskores::cont::DataSet ds = dsb.Create();

  viskores::filter::geometry_refinement::Tube tubeFilter;
  tubeFilter.SetCapping(true);
  tubeFilter.SetNumberOfSides(numSides);
  tubeFilter.SetRadius(radius);
  viskores::cont::DataSet tubeDataset = tubeFilter.Execute(ds);

  viskores::Bounds coordsBounds = tubeDataset.GetCoordinateSystem().GetBounds();

  viskores::Vec3f_64 totalExtent(
    coordsBounds.X.Length(), coordsBounds.Y.Length(), coordsBounds.Z.Length());
  viskores::Float64 mag = viskores::Magnitude(totalExtent);
  viskores::Normalize(totalExtent);

  // setup a camera and point it to towards the center of the input data
  viskores::rendering::Camera camera;
  camera.ResetToBounds(coordsBounds);

  camera.SetLookAt(totalExtent * (mag * .5f));
  camera.SetViewUp(viskores::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 100.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(totalExtent * (mag * 2.f));
  viskores::cont::ColorTable colorTable("inferno");

  viskores::rendering::Scene scene;
  viskores::rendering::MapperRayTracer mapper;
  viskores::rendering::CanvasRayTracer canvas(2048, 2048);
  viskores::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);


  std::vector<viskores::FloatDefault> v(static_cast<std::size_t>(tubeDataset.GetNumberOfPoints()));
  // The first value is a cap:
  v[0] = 0;
  for (viskores::Id i = 1; i < viskores::Id(v.size()); i += numSides)
  {
    viskores::FloatDefault t = 4 * viskores::FloatDefault(3.1415926) * (i + 1) / numSides;
    viskores::FloatDefault r = a + b * t;
    for (viskores::Id j = i; j < i + numSides && j < viskores::Id(v.size()); ++j)
    {
      v[static_cast<std::size_t>(j)] = r;
    }
  }
  // Point at the end cap should be the same color as the surroundings:
  v[v.size() - 1] = v[v.size() - 2];

  tubeDataset.AddPointField("Spiral Radius", v);
  scene.AddActor(viskores::rendering::Actor(tubeDataset, "Spiral Radius", colorTable));
  viskores::rendering::View3D view(scene, mapper, canvas, camera, bg);
  view.Paint();
  // We can save the file as a .NetBPM:
  std::string output_filename = "tube_output_" + std::to_string(numSides) + "_sides.pnm";
  view.SaveAs(output_filename);
  // Or as a .png:
  output_filename = "tube_output_" + std::to_string(numSides) + "_sides.png";
  view.SaveAs(output_filename);
}



int main()
{
  // Radius of the tube:
  viskores::FloatDefault radius = 0.5f;
  // How many segments is the tube decomposed into?
  viskores::Id numLineSegments = 100;
  // As numSides->infty, the tubes becomes perfectly cylindrical:
  viskores::Id numSides = 50;
  TubeThatSpiral(radius, numLineSegments, numSides);
  // Setting numSides = 4 makes a square around the polyline:
  numSides = 4;
  TubeThatSpiral(radius, numLineSegments, numSides);
  return 0;
}
