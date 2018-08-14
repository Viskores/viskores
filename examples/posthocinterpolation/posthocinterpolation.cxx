//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkAppendFilter.h>
#include <vtkDelaunay3D.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkProbeFilter.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/io/reader/VTKDataSetReader.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <time.h>


using namespace std;

vtkSmartPointer<vtkUnstructuredGrid> generateRandomSeeds(float xmin,
                                                         float xmax,
                                                         float ymin,
                                                         float ymax,
                                                         float zmin,
                                                         float zmax,
                                                         int num_seeds,
                                                         string out_path)
{
  vtkSmartPointer<vtkPoints> seed_points = vtkSmartPointer<vtkPoints>::New();
  float x, y, z;

  srand(1);
  //  srand(time(NULL));

  int xdiff = (int)(xmax - xmin) * 100000;
  int ydiff = (int)(ymax - ymin) * 100000;
  int zdiff = (int)(zmax - zmin) * 100000;

  for (int i = 0; i < num_seeds; i++)
  {
    if (xdiff != 0)
      x = (float)((rand() % (xdiff)) / 100000.0) + xmin;
    else
      x = 0.0;
    if (ydiff != 0)
      y = (float)((rand() % (ydiff)) / 100000.0) + ymin;
    else
      y = 0.0;
    if (zdiff != 0)
      z = (float)((rand() % (zdiff)) / 100000.0) + zmin;
    else
      z = 0.0;
    seed_points->InsertNextPoint(x, y, z);

    stringstream file;
    file << out_path << "/particle" << (i + 1) << ".lines";
    ofstream ofs;
    ofs.open(file.str().c_str(), ofstream::out);
    ofs << x << "," << y << "," << z << "\n";
    ofs.close();
  }

  vtkSmartPointer<vtkUnstructuredGrid> particleMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
  particleMesh->SetPoints(seed_points);
  return particleMesh;
}

vtkSmartPointer<vtkUnstructuredGrid> readSeedInputFile(string in_path, string out_path)
{
  vtkSmartPointer<vtkPoints> seed_points = vtkSmartPointer<vtkPoints>::New();
  float x, y, z, t0, tn;
  ifstream seed_stream(in_path);
  int i = 0;
  while (seed_stream >> x)
  {
    seed_stream >> y;
    seed_stream >> z;
    seed_stream >> t0;
    seed_stream >> tn;

    seed_points->InsertNextPoint(x, y, z);

    stringstream file;
    file << out_path << "/particle" << (i + 1) << ".lines";
    ofstream ofs;
    ofs.open(file.str().c_str(), ofstream::out);
    ofs << x << "," << y << "," << z << "\n";
    ofs.close();
    cout << "Read point : " << x << " " << y << " " << z << endl;

    stringstream filetxt;
    filetxt << out_path << "/particle" << (i + 1) << ".txt";
    ofstream ofstxt;
    ofstxt.open(filetxt.str().c_str(), ofstream::out);
    ofstxt << x << " " << y << " " << z << " "
           << "0"
           << "\n";
    ofstxt.close();

    i++;
  }

  vtkSmartPointer<vtkUnstructuredGrid> particleMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
  particleMesh->SetPoints(seed_points);
  return particleMesh;
}



int main(int argc, char* argv[])
{
  if (argc != 17)
  {
    cout << " Usage: <input_directory> <file_name> <start> <end> <interval> <output_directory> "
            "<number_of_seeds> <xmin> <xmax> <ymin> <ymax> <zmin> <zmax> <input_seeds> <seed_file> "
            "<num_nodes>"
         << endl;
    return 1;
  }

  string in_path(argv[1]);
  string file_name(argv[2]);
  int start = atoi(argv[3]);
  int end = atoi(argv[4]);
  int interval = atoi(argv[5]);
  string out_path(argv[6]);
  int num_seeds = atoi(argv[7]);
  float xmin = (float)atof(argv[8]);
  float xmax = (float)atof(argv[9]);
  float ymin = (float)atof(argv[10]);
  float ymax = (float)atof(argv[11]);
  float zmin = (float)atof(argv[12]);
  float zmax = (float)atof(argv[13]);
  int input_seeds = atoi(argv[14]);
  string seed_path(argv[15]);
  int num_nodes = atoi(argv[16]);

  vtkSmartPointer<vtkUnstructuredGrid> particleMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();

  if (input_seeds == 0)
  {
    particleMesh = generateRandomSeeds(xmin, xmax, ymin, ymax, zmin, zmax, num_seeds, out_path);
  }
  else
  {
    particleMesh = readSeedInputFile(seed_path, out_path);
  }

  // Write out starting location along with output file creation.
  // Maybe don't create a mesh directly.
  //
  int* validSeeds = new int[num_seeds];
  for (int i = 0; i < num_seeds; i++)
    validSeeds[i] = 1;

  for (int i = start; i <= end; i += interval)
  {
    int total_num_flows = 0;

    for (int n = 0; n < num_nodes; n++)
    {
      stringstream file;
      file << in_path << "/" << file_name << "_" << n << "_" << i << ".vtk";
      vtkm::io::reader::VTKDataSetReader reader(file.str().c_str());
      vtkm::cont::DataSet input = reader.ReadDataSet();
      int num_flows = input.GetCellSet().GetNumberOfCells();
      total_num_flows += num_flows;
    }

    vtkSmartPointer<vtkPoints> meshpoints = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDoubleArray> xvec = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> yvec = vtkSmartPointer<vtkDoubleArray>::New();
    vtkSmartPointer<vtkDoubleArray> zvec = vtkSmartPointer<vtkDoubleArray>::New();

    xvec->SetNumberOfValues(total_num_flows);
    yvec->SetNumberOfValues(total_num_flows);
    zvec->SetNumberOfValues(total_num_flows);

    cout << "The total number of flows is : " << total_num_flows << endl;

    xvec->SetName("xvec");
    yvec->SetName("yvec");
    zvec->SetName("zvec");
    int flow_counter = 0;

    for (int n = 0; n < num_nodes; n++)
    {
      stringstream file;
      file << in_path << "/" << file_name << "_" << n << "_" << i << ".vtk";

      cout << "Loading file : " << file.str() << endl;
      vtkm::io::reader::VTKDataSetReader reader(file.str().c_str());
      vtkm::cont::DataSet input = reader.ReadDataSet();

      auto pointArray = input.GetCoordinateSystem().GetData();
      int num_flows = input.GetCellSet(0).GetNumberOfCells();

      for (int j = 0; j < num_flows; j++)
      {
        auto pt1 = pointArray.GetPortalConstControl().Get(j * 2 + 0);
        auto pt2 = pointArray.GetPortalConstControl().Get(j * 2 + 1);

        meshpoints->InsertNextPoint(pt1[0], pt1[1], pt1[2]);
        xvec->SetValue(flow_counter, pt2[0]);
        yvec->SetValue(flow_counter, pt2[1]);
        zvec->SetValue(flow_counter, pt2[2]);
        flow_counter++;
      }
    }

    vtkSmartPointer<vtkUnstructuredGrid> mesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    mesh->SetPoints(meshpoints);
    mesh->GetPointData()->AddArray(xvec);
    mesh->GetPointData()->AddArray(yvec);
    mesh->GetPointData()->AddArray(zvec);

    cout << "Added arrays and points to unstructured grid" << endl;

    vtkSmartPointer<vtkDelaunay3D> triangulation = vtkSmartPointer<vtkDelaunay3D>::New();
    triangulation->SetInputData(mesh);
    triangulation->Update();

    cout << "Completed triangulation" << endl;

    vtkSmartPointer<vtkUnstructuredGrid> flowMesh = vtkSmartPointer<vtkUnstructuredGrid>::New();
    flowMesh = triangulation->GetOutput();

    vtkSmartPointer<vtkProbeFilter> probe = vtkSmartPointer<vtkProbeFilter>::New();

    probe->SetSourceData(flowMesh);
    probe->SetInputData(particleMesh);
    probe->Update();

    vtkSmartPointer<vtkIntArray> validInterpolations = vtkSmartPointer<vtkIntArray>::New();
    validInterpolations->DeepCopy(
      probe->GetOutput()->GetPointData()->GetArray(probe->GetValidPointMaskArrayName()));

    vtkSmartPointer<vtkDoubleArray> xlocation =
      vtkDoubleArray::SafeDownCast(probe->GetOutput()->GetPointData()->GetArray("xvec"));
    vtkSmartPointer<vtkDoubleArray> ylocation =
      vtkDoubleArray::SafeDownCast(probe->GetOutput()->GetPointData()->GetArray("yvec"));
    vtkSmartPointer<vtkDoubleArray> zlocation =
      vtkDoubleArray::SafeDownCast(probe->GetOutput()->GetPointData()->GetArray("zvec"));

    vtkSmartPointer<vtkPoints> new_locations = vtkSmartPointer<vtkPoints>::New();

    for (int k = 0; k < num_seeds; k++)
    {
      if (validSeeds[k])
      {
        if (validInterpolations->GetValue(k))
        {
          {
            stringstream filetxt;
            filetxt << out_path << "/particle" << (k + 1) << ".txt";
            ofstream ofstxt;
            ofstxt.open(filetxt.str().c_str(), ofstream::out | ofstream::app);
            ofstxt << xlocation->GetValue(k) << " " << ylocation->GetValue(k) << " "
                   << zlocation->GetValue(k) << " " << i << "\n";
            ofstxt.close();
          }
          // Write location to file.
          {
            stringstream file2;
            file2 << out_path << "/particle" << (k + 1) << ".lines";
            ofstream ofs;
            ofs.open(file2.str().c_str(), ofstream::out | ofstream::app);
            ofs << xlocation->GetValue(k) << "," << ylocation->GetValue(k) << ","
                << zlocation->GetValue(k) << "\n";
            ofs.close();
          }
        }
        else
        {
          validSeeds[k] = 0;
        }
      }
      new_locations->InsertNextPoint(
        xlocation->GetValue(k), ylocation->GetValue(k), zlocation->GetValue(k));
    }
    particleMesh->SetPoints(new_locations);
  } // Loop over all input files

  return 0;
}
