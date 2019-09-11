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
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>
#include <vtkm/worklet/Tube.h>

#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>

vtkm::Vec3f ArchimedeanSpiralToCartesian(vtkm::Vec3f const& p)
{
  // p[0] = r, p[1] = theta, p[2] = z:
  vtkm::Vec3f xyz;
  auto c = std::polar(p[0], p[1]);
  xyz[0] = c.real();
  xyz[1] = c.imag();
  xyz[2] = p[2];
  return xyz;
}

void TubeThatSpiral(vtkm::FloatDefault radius, vtkm::Id numLineSegments, vtkm::Id numSides)
{
  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;

  // The Archimedian spiral is defined by the equation r = a + b*theta.
  // To extend to a 3D curve, use z = t, theta = t, r = a + b t.
  vtkm::FloatDefault a = vtkm::FloatDefault(0.2);
  vtkm::FloatDefault b = vtkm::FloatDefault(0.8);
  for (vtkm::Id i = 0; i < numLineSegments; ++i)
  {
    vtkm::FloatDefault t = 4 * vtkm::FloatDefault(3.1415926) * (i + 1) /
      numLineSegments; // roughly two spins around. Doesn't need to be perfect.
    vtkm::FloatDefault r = a + b * t;
    vtkm::FloatDefault theta = t;
    vtkm::Vec3f cylindricalCoordinate{ r, theta, t };
    vtkm::Vec3f spiralSample = ArchimedeanSpiralToCartesian(cylindricalCoordinate);
    vtkm::Id pid = dsb.AddPoint(spiralSample);
    ids.push_back(pid);
  }
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  vtkm::cont::DataSet ds = dsb.Create();

  vtkm::worklet::Tube tubeWorklet(
    /*capEnds = */ true,
    /* how smooth the cylinder is; infinitely smooth as n->infty */ numSides,
    radius);

  // You added lines, but you need to extend it to a tube.
  // This generates a new pointset, and new cell set.
  vtkm::cont::ArrayHandle<vtkm::Vec3f> tubePoints;
  vtkm::cont::CellSetSingleType<> tubeCells;
  tubeWorklet.Run(ds.GetCoordinateSystem().GetData().Cast<vtkm::cont::ArrayHandle<vtkm::Vec3f>>(),
                  ds.GetCellSet(),
                  tubePoints,
                  tubeCells);

  vtkm::cont::DataSet tubeDataset;
  tubeDataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coords", tubePoints));
  tubeDataset.SetCellSet(tubeCells);

  vtkm::Bounds coordsBounds = tubeDataset.GetCoordinateSystem().GetBounds();

  vtkm::Vec3f_64 totalExtent(
    coordsBounds.X.Length(), coordsBounds.Y.Length(), coordsBounds.Z.Length());
  vtkm::Float64 mag = vtkm::Magnitude(totalExtent);
  vtkm::Normalize(totalExtent);

  // setup a camera and point it to towards the center of the input data
  vtkm::rendering::Camera camera;
  camera.ResetToBounds(coordsBounds);

  camera.SetLookAt(totalExtent * (mag * .5f));
  camera.SetViewUp(vtkm::make_Vec(0.f, 1.f, 0.f));
  camera.SetClippingRange(1.f, 100.f);
  camera.SetFieldOfView(60.f);
  camera.SetPosition(totalExtent * (mag * 2.f));
  vtkm::cont::ColorTable colorTable("inferno");

  vtkm::rendering::Scene scene;
  vtkm::rendering::MapperRayTracer mapper;
  vtkm::rendering::CanvasRayTracer canvas(2048, 2048);
  vtkm::rendering::Color bg(0.2f, 0.2f, 0.2f, 1.0f);


  vtkm::cont::DataSetFieldAdd dsfa;
  std::vector<vtkm::FloatDefault> v(static_cast<std::size_t>(tubePoints.GetNumberOfValues()));
  // The first value is a cap:
  v[0] = 0;
  for (vtkm::Id i = 1; i < vtkm::Id(v.size()); i += numSides)
  {
    vtkm::FloatDefault t = 4 * vtkm::FloatDefault(3.1415926) * (i + 1) / numSides;
    vtkm::FloatDefault r = a + b * t;
    for (vtkm::Id j = i; j < i + numSides && j < vtkm::Id(v.size()); ++j)
    {
      v[static_cast<std::size_t>(j)] = r;
    }
  }
  // Point at the end cap should be the same color as the surroundings:
  v[v.size() - 1] = v[v.size() - 2];

  dsfa.AddPointField(tubeDataset, "Spiral Radius", v);
  scene.AddActor(vtkm::rendering::Actor(tubeDataset.GetCellSet(),
                                        tubeDataset.GetCoordinateSystem(),
                                        tubeDataset.GetField("Spiral Radius"),
                                        colorTable));
  vtkm::rendering::View3D view(scene, mapper, canvas, camera, bg);
  view.Initialize();
  view.Paint();
  std::string output_filename = "tube_output_" + std::to_string(numSides) + "_sides.pnm";
  view.SaveAs(output_filename);
}



int main()
{
  // Radius of the tube:
  vtkm::FloatDefault radius = 0.5f;
  // How many segments is the tube decomposed into?
  vtkm::Id numLineSegments = 100;
  // As numSides->infty, the tubes becomes perfectly cylindrical:
  vtkm::Id numSides = 50;
  TubeThatSpiral(radius, numLineSegments, numSides);
  // Setting numSides = 4 makes a square around the polyline:
  numSides = 4;
  TubeThatSpiral(radius, numLineSegments, numSides);
  return 0;
}
