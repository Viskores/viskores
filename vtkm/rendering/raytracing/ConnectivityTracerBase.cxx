//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <vtkm/rendering/raytracing/ConnectivityTracerBase.h>

#include <vtkm/VectorAnalysis.h>

#include <vtkm/rendering/raytracing/Logger.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{
ConnectivityTracerBase::ConnectivityTracerBase()
  : ConnectivityBase()
{
}

ConnectivityTracerBase::~ConnectivityTracerBase()
{
}

void ConnectivityTracerBase::Init()
{
  //
  // Check to see if a sample distance was set
  //
  if (SampleDistance <= 0)
  {
    const vtkm::Float32 defaultSampleRate = 200.f;
    // We need to set some default sample distance
    vtkm::Vec<vtkm::Float32, 3> extent;
    extent[0] = BoundingBox[1] - BoundingBox[0];
    extent[1] = BoundingBox[3] - BoundingBox[2];
    extent[2] = BoundingBox[5] - BoundingBox[4];
    SampleDistance = vtkm::Magnitude(extent) / defaultSampleRate;
  }
}

void ConnectivityTracerBase::SetColorMap(
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 4>>& colorMap)
{
  ColorMap = colorMap;
}

void ConnectivityTracerBase::SetVolumeData(const vtkm::cont::Field& scalarField,
                                           const vtkm::Range& scalarBounds)
{
  //TODO: Need a way to tell if we have been updated
  ScalarField = scalarField;
  ScalarBounds = scalarBounds;

  bool isSupportedField =
    (ScalarField.GetAssociation() == vtkm::cont::Field::Association::POINTS ||
     ScalarField.GetAssociation() == vtkm::cont::Field::Association::CELL_SET);
  if (!isSupportedField)
    throw vtkm::cont::ErrorBadValue("Field not accociated with cell set or points");
  FieldAssocPoints = ScalarField.GetAssociation() == vtkm::cont::Field::Association::POINTS;

  this->Integrator = Volume;
}

void ConnectivityTracerBase::SetEnergyData(const vtkm::cont::Field& absorption,
                                           const vtkm::Int32 numBins,
                                           const vtkm::cont::Field& emission)
{
  bool isSupportedField = absorption.GetAssociation() == vtkm::cont::Field::Association::CELL_SET;
  if (!isSupportedField)
    throw vtkm::cont::ErrorBadValue("Absorption Field '" + absorption.GetName() +
                                    "' not accociated with cells");
  ScalarField = absorption;
  // Check for emission
  HasEmission = false;

  if (emission.GetAssociation() != vtkm::cont::Field::Association::ANY)
  {
    if (emission.GetAssociation() != vtkm::cont::Field::Association::CELL_SET)
      throw vtkm::cont::ErrorBadValue("Emission Field '" + emission.GetName() +
                                      "' not accociated with cells");
    HasEmission = true;
    EmissionField = emission;
  }
  // Do some basic range checking
  if (numBins < 1)
    throw vtkm::cont::ErrorBadValue("Number of energy bins is less than 1");
  vtkm::Id binCount = ScalarField.GetData().GetNumberOfValues();
  vtkm::Id cellCount = this->GetNumberOfMeshCells();
  if (cellCount != (binCount / vtkm::Id(numBins)))
  {
    std::stringstream message;
    message << "Invalid number of absorption bins\n";
    message << "Number of cells: " << cellCount << "\n";
    message << "Number of field values: " << binCount << "\n";
    message << "Number of bins: " << numBins << "\n";
    throw vtkm::cont::ErrorBadValue(message.str());
  }
  if (HasEmission)
  {
    binCount = EmissionField.GetData().GetNumberOfValues();
    if (cellCount != (binCount / vtkm::Id(numBins)))
    {
      std::stringstream message;
      message << "Invalid number of emission bins\n";
      message << "Number of cells: " << cellCount << "\n";
      message << "Number of field values: " << binCount << "\n";
      message << "Number of bins: " << numBins << "\n";
      throw vtkm::cont::ErrorBadValue(message.str());
    }
  }
  //TODO: Need a way to tell if we have been updated
  this->Integrator = Energy;
}

void ConnectivityTracerBase::SetBackgroundColor(const vtkm::Vec<vtkm::Float32, 4>& backgroundColor)
{
  BackgroundColor = backgroundColor;
}

void ConnectivityTracerBase::SetSampleDistance(const vtkm::Float32& distance)
{
  if (distance <= 0.f)
    throw vtkm::cont::ErrorBadValue("Sample distance must be positive.");
  SampleDistance = distance;
}

void ConnectivityTracerBase::ResetTimers()
{
  IntersectTime = 0.;
  IntegrateTime = 0.;
  SampleTime = 0.;
  LostRayTime = 0.;
  MeshEntryTime = 0.;
}

void ConnectivityTracerBase::LogTimers()
{
  Logger* logger = Logger::GetInstance();
  logger->AddLogData("intersect ", IntersectTime);
  logger->AddLogData("integrate ", IntegrateTime);
  logger->AddLogData("sample_cells ", SampleTime);
  logger->AddLogData("lost_rays ", LostRayTime);
  logger->AddLogData("mesh_entry", LostRayTime);
}
}
}
}
