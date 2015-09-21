//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/Clip.h>

typedef vtkm::Vec<vtkm::Float32, 3> FloatVec3;

template<typename T>
inline void flipEndianness(T* buffer, vtkm::Id size)
{
  for (vtkm::Id i = 0; i < size; ++i)
  {
    T val = buffer[i];
    vtkm::UInt8 *bytes = reinterpret_cast<vtkm::UInt8*>(&val);
    std::reverse(bytes, bytes + sizeof(T));
    buffer[i] = val;
  }
}

template<typename T, typename DeviceAdapter>
inline vtkm::cont::ArrayHandle<vtkm::Float32> LoadBinaryPointDataImpl(
  std::ifstream &fstream, vtkm::Id numPoints, T, DeviceAdapter)
{
  vtkm::cont::ArrayHandle<vtkm::Float32> result;

  std::vector<T> buffer(static_cast<size_t>(numPoints));
  fstream.read(reinterpret_cast<char*>(&buffer[0]),
               numPoints * static_cast<vtkm::Id>(sizeof(T)));
  flipEndianness(&buffer[0], numPoints);

  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(
    vtkm::cont::make_ArrayHandleCast(vtkm::cont::make_ArrayHandle(buffer), vtkm::Float32()),
    result);

  return result;
}

template<typename DeviceAdapter>
inline vtkm::cont::ArrayHandle<vtkm::Float32> LoadBinaryPointData(
  std::ifstream &fstream, vtkm::Id numPoints, std::string type, DeviceAdapter)
{
  if (type == "short")
  {
    return LoadBinaryPointDataImpl(fstream, numPoints, short(), DeviceAdapter());
  }
  else if (type == "float")
  {
    return LoadBinaryPointDataImpl(fstream, numPoints, vtkm::Float32(), DeviceAdapter());
  }
  else
  {
    throw std::runtime_error("only short and float types supported");
  }

  return vtkm::cont::ArrayHandle<vtkm::Float32>();
}

template <typename DeviceAdapter>
vtkm::cont::DataSet LoadVtkLegacyStructuredPoints(const char *fname, DeviceAdapter)
{
  vtkm::Id3 dim;
  FloatVec3 spacing, origin;
  vtkm::cont::ArrayHandle<vtkm::Float32> scalars;

  std::ifstream vtkfile;
  vtkfile.open(fname);

  std::string tag;
  std::getline(vtkfile, tag); // version comment
  std::getline(vtkfile, tag); // datset name
  vtkfile >> tag;
  if (tag != "BINARY")
  {
    throw std::runtime_error("only binary format supported");
  }

  for (;;)
  {
    vtkfile >> tag;
    if (tag == "DATASET")
    {
      std::string dataset;
      vtkfile >> dataset;
      if (dataset != "STRUCTURED_POINTS")
      {
        throw std::runtime_error("expecting structured dataset");
      }
    }
    else if (tag == "DIMENSIONS")
    {
      vtkfile >> dim[0] >> dim[1] >> dim[2];
    }
    else if (tag == "SPACING")
    {
      vtkfile >> spacing[0] >> spacing[1] >> spacing[2];
    }
    else if (tag == "ORIGIN")
    {
      vtkfile >> origin[0] >> origin[1] >> origin[2];
    }
    else if (tag == "POINT_DATA")
    {
      vtkm::Id numPoints;
      std::string type;
      vtkfile >> numPoints;
      vtkfile >> tag;
      if (tag != "SCALARS")
      {
        throw std::runtime_error("only scalars supported for point data");
      }
      vtkfile >> tag >> type >> std::ws;
      std::getline(vtkfile, tag); // LOOKUP_TABLE default
      scalars = LoadBinaryPointData(vtkfile, numPoints, type, DeviceAdapter());
      break;
    }
  }

  vtkfile.close();


  vtkm::cont::CellSetStructured<3> cs("cells");
  cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1], dim[2]));

  vtkm::cont::DataSet ds;
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", 1, dim, origin,
      spacing));
  ds.AddField(vtkm::cont::Field("scalars", 1, vtkm::cont::Field::ASSOC_POINTS,
      scalars));
  ds.AddCellSet(cs);

  return ds;
}

void WriteVtkLegacyUnstructuredGrid(const char *fname, const vtkm::cont::DataSet &ds)
{
  std::ofstream vtkfile;

  vtkfile.open(fname);
  vtkfile << "# vtk DataFile Version 3.0" << std::endl;
  vtkfile << "vtkm_clip_output" << std::endl;
  vtkfile << "BINARY" << std::endl << "DATASET UNSTRUCTURED_GRID" << std::endl;

  vtkm::cont::CoordinateSystem coords = ds.GetCoordinateSystem();
  vtkm::Id numPoints = coords.GetData().GetNumberOfValues();
  vtkfile << "POINTS " << numPoints << " float" << std::endl;
  {
    vtkm::cont::ArrayHandle<FloatVec3> coordinates =
        coords.GetData().CastToArrayHandle(FloatVec3(), VTKM_DEFAULT_STORAGE_TAG());
    std::vector<FloatVec3> buffer(static_cast<size_t>(numPoints));

    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(coordinates.GetPortalConstControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(coordinates.GetPortalConstControl()),
              buffer.begin());

    flipEndianness(reinterpret_cast<vtkm::Float32*>(&buffer[0]), numPoints * 3);
    vtkfile.write(reinterpret_cast<const char*>(&buffer[0]),
                  numPoints * static_cast<vtkm::Id>(sizeof(FloatVec3)));
  }
  vtkfile << std::endl;

  vtkm::cont::CellSetExplicit<> cse =
      ds.GetCellSet().CastTo<vtkm::cont::CellSetExplicit<> >();
  vtkm::Id numCells = cse.GetNumberOfCells();
  {
    std::vector<int> idxBuffer, shapeBuffer;

    idxBuffer.reserve(static_cast<size_t>(numCells * 4));
    shapeBuffer.reserve(static_cast<size_t>(numCells));

    vtkm::Vec<vtkm::Id, 8> pointIndices;
    for (vtkm::Id i = 0; i < numCells; ++i)
    {
      vtkm::Id numCellPoints = cse.GetNumberOfPointsInCell(i);
      idxBuffer.push_back(static_cast<int>(numCellPoints));
      cse.GetIndices(i, pointIndices);
      for (vtkm::IdComponent j = 0; j < numCellPoints; ++j)
      {
        idxBuffer.push_back(static_cast<int>(pointIndices[j]));
      }
      shapeBuffer.push_back(static_cast<int>(cse.GetCellShape(i)));
    }
    vtkm::Id numIndices = static_cast<vtkm::Id>(idxBuffer.size());

    vtkfile << "CELLS " << numCells << " " << numIndices << std::endl;
    flipEndianness(&idxBuffer[0], numIndices);
    vtkfile.write(reinterpret_cast<const char*>(&idxBuffer[0]),
                  numIndices * static_cast<vtkm::Id>(sizeof(idxBuffer[0])));
    vtkfile << std::endl;
    vtkfile << "CELL_TYPES " << numCells << std::endl;
    flipEndianness(&shapeBuffer[0], numCells);
    vtkfile.write(reinterpret_cast<const char*>(&shapeBuffer[0]),
                  numCells * static_cast<vtkm::Id>(sizeof(shapeBuffer[0])));
  }
  vtkfile << std::endl;

  vtkm::cont::Field field = ds.GetField(0);
  vtkfile << "POINT_DATA " << numPoints << std::endl
          << "SCALARS " << field.GetName() << " float" << std::endl
          << "LOOKUP_TABLE default" << std::endl;
  {
    vtkm::cont::ArrayHandle<vtkm::Float32> scalars =
        field.GetData().CastToArrayHandle(vtkm::Float32(), VTKM_DEFAULT_STORAGE_TAG());
    std::vector<vtkm::Float32> buffer(static_cast<size_t>(numPoints));

    std::copy(vtkm::cont::ArrayPortalToIteratorBegin(scalars.GetPortalConstControl()),
              vtkm::cont::ArrayPortalToIteratorEnd(scalars.GetPortalConstControl()),
              buffer.begin());

    flipEndianness(&buffer[0], numPoints);
    vtkfile.write(reinterpret_cast<const char*>(&buffer[0]),
                  numPoints * static_cast<vtkm::Id>(sizeof(vtkm::Float32)));
  }
  vtkfile << std::endl;

  vtkfile.close();
}

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    std::cout << "Usage: " << std::endl
              << "$ " << argv[0]
              << " <vtk_structure_points> <isoval> <vtk_unstructured_grid>"
              << std::endl;
    return 1;
  }

  typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;
  std::cout << "Device Adapter Id: "
            << vtkm::cont::internal::DeviceAdapterTraits<DeviceAdapter>::GetId()
            << std::endl;

  vtkm::cont::DataSet input = LoadVtkLegacyStructuredPoints(argv[1], DeviceAdapter());

  vtkm::Float32 clipValue = boost::lexical_cast<vtkm::Float32>(argv[2]);
  vtkm::worklet::Clip<DeviceAdapter> clip;

  vtkm::cont::Timer<DeviceAdapter> total;
  vtkm::cont::Timer<DeviceAdapter> timer;
  vtkm::cont::CellSetExplicit<> outputCellSet =
      clip.Run(input.GetCellSet(0), input.GetField(0).GetData(), clipValue);
  vtkm::Float64 clipTime = timer.GetElapsedTime();

  timer.Reset();
  vtkm::cont::DynamicArrayHandle coords =
      clip.ProcessField(input.GetCoordinateSystem(0).GetData());
  vtkm::Float64 processCoordinatesTime = timer.GetElapsedTime();
  timer.Reset();
  vtkm::cont::DynamicArrayHandle scalars =
      clip.ProcessField(input.GetField(0).GetData());
  vtkm::Float64 processScalarsTime = timer.GetElapsedTime();

  vtkm::Float64 totalTime = total.GetElapsedTime();

  std::cout << "Timings: " << std::endl
            << "clip: " << clipTime << std::endl
            << "process coordinates: " << processCoordinatesTime << std::endl
            << "process scalars: " << processScalarsTime << std::endl
            << "Total: " << totalTime << std::endl;

  vtkm::cont::DataSet output;
  output.AddField(vtkm::cont::Field("scalars", 1, vtkm::cont::Field::ASSOC_POINTS,
      scalars));
  output.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", 1, coords));
  output.AddCellSet(outputCellSet);

  WriteVtkLegacyUnstructuredGrid(argv[3], output);

  return 0;
}
