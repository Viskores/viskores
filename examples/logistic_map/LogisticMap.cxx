//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cmath>
#include <iostream>
#include <vector>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/io/ImageWriterPNG.h>


int main()
{
  size_t height = 1800;
  size_t width = height * 1.618;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet ds = dsb.Create(vtkm::Id2(width, height));

  std::vector<double> x(width, 0.5);

  double rmin = 2.9;
  for (size_t i = 0; i < width; ++i)
  {
    double r = rmin + (4.0 - rmin) * i / (width - 1);
    int n = 0;
    // 2048 should be enough iterations to be "converged";
    // though of course the iterations actually don't all converge but cycle or are chaotic.
    while (n++ < 2048)
    {
      x[i] = r * x[i] * (1 - x[i]);
    }
  }


  vtkm::Vec4f v(1.0, 0.5, 0.0, 0.0);
  std::vector<vtkm::Vec4f> pixelValues(width * height, vtkm::Vec4f(0, 0, 0, 0));
  size_t iterates = 0;
  // We don't need more iterates than pixels of height,
  // by the pigeonhole principle.
  while (iterates++ < height)
  {
    for (size_t i = 0; i < width; ++i)
    {
      double r = rmin + (4.0 - rmin) * i / (width - 1);
      double y = x[i];
      assert(y >= 0 && y <= 1);
      size_t j = std::round(y * (height - 1));
      pixelValues[j * width + i] = v;
      x[i] = r * x[i] * (1 - x[i]);
    }
  }

  std::string colorFieldName = "pixels";
  ds.AddPointField(colorFieldName, pixelValues);
  std::string filename = "logistic.png";
  vtkm::io::ImageWriterPNG writer(filename);
  writer.WriteDataSet(ds, colorFieldName);
  std::cout << "Now open " << filename << "\n";
}
