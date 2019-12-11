//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Lagrangian_hxx
#define vtk_m_filter_Lagrangian_hxx

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

#include <cstring>
#include <sstream>
#include <string.h>

static vtkm::Id cycle = 0;
static vtkm::cont::ArrayHandle<vtkm::Particle> BasisParticles;
static vtkm::cont::ArrayHandle<vtkm::Particle> BasisParticlesOriginal;
static vtkm::cont::ArrayHandle<vtkm::Id> BasisParticlesValidity;

namespace
{
class ValidityCheck : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn end_point, FieldInOut output);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  ValidityCheck(vtkm::Bounds b)
    : bounds(b)
  {
  }

  template <typename ValidityType>
  VTKM_EXEC void operator()(const vtkm::Particle& end_point, ValidityType& res) const
  {
    vtkm::Id steps = end_point.NumSteps;
    if (steps > 0 && res == 1)
    {
      if (end_point.Pos[0] >= bounds.X.Min && end_point.Pos[0] <= bounds.X.Max &&
          end_point.Pos[1] >= bounds.Y.Min && end_point.Pos[1] <= bounds.Y.Max &&
          end_point.Pos[2] >= bounds.Z.Min && end_point.Pos[2] <= bounds.Z.Max)
      {
        res = 1;
      }
      else
      {
        res = 0;
      }
    }
    else
    {
      res = 0;
    }
  }

private:
  vtkm::Bounds bounds;
};
}

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT Lagrangian::Lagrangian()
  : vtkm::filter::FilterDataSetWithField<Lagrangian>()
  , rank(0)
  , initFlag(true)
  , extractFlows(false)
  , resetParticles(true)
  , stepSize(1.0f)
  , x_res(0)
  , y_res(0)
  , z_res(0)
  , cust_res(0)
  , SeedRes(vtkm::Id3(1, 1, 1))
  , writeFrequency(0)
{
}

//-----------------------------------------------------------------------------
inline void Lagrangian::WriteDataSet(vtkm::Id cycle,
                                     const std::string& filename,
                                     vtkm::cont::DataSet dataset)
{
  std::stringstream file_stream;
  file_stream << filename << cycle << ".vtk";
  vtkm::io::writer::VTKDataSetWriter writer(file_stream.str());
  writer.WriteDataSet(dataset);
}

//-----------------------------------------------------------------------------
inline void Lagrangian::UpdateSeedResolution(const vtkm::cont::DataSet input)
{
  vtkm::cont::DynamicCellSet cell_set = input.GetCellSet();

  if (cell_set.IsSameType(vtkm::cont::CellSetStructured<1>()))
  {
    vtkm::cont::CellSetStructured<1> cell_set1 = cell_set.Cast<vtkm::cont::CellSetStructured<1>>();
    vtkm::Id dims1 = cell_set1.GetPointDimensions();
    this->SeedRes[0] = dims1;
    if (this->cust_res)
    {
      this->SeedRes[0] = dims1 / this->x_res;
    }
  }
  else if (cell_set.IsSameType(vtkm::cont::CellSetStructured<2>()))
  {
    vtkm::cont::CellSetStructured<2> cell_set2 = cell_set.Cast<vtkm::cont::CellSetStructured<2>>();
    vtkm::Id2 dims2 = cell_set2.GetPointDimensions();
    this->SeedRes[0] = dims2[0];
    this->SeedRes[1] = dims2[1];
    if (this->cust_res)
    {
      this->SeedRes[0] = dims2[0] / this->x_res;
      this->SeedRes[1] = dims2[1] / this->y_res;
    }
  }
  else if (cell_set.IsSameType(vtkm::cont::CellSetStructured<3>()))
  {
    vtkm::cont::CellSetStructured<3> cell_set3 = cell_set.Cast<vtkm::cont::CellSetStructured<3>>();
    vtkm::Id3 dims3 = cell_set3.GetPointDimensions();
    this->SeedRes[0] = dims3[0];
    this->SeedRes[1] = dims3[1];
    this->SeedRes[2] = dims3[2];
    if (this->cust_res)
    {
      this->SeedRes[0] = dims3[0] / this->x_res;
      this->SeedRes[1] = dims3[1] / this->y_res;
      this->SeedRes[2] = dims3[2] / this->z_res;
    }
  }
}


//-----------------------------------------------------------------------------
inline void Lagrangian::InitializeUniformSeeds(const vtkm::cont::DataSet& input)
{
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  Lagrangian::UpdateSeedResolution(input);

  vtkm::Float64 x_spacing = 0.0, y_spacing = 0.0, z_spacing = 0.0;
  if (this->SeedRes[0] > 1)
    x_spacing = (double)(bounds.X.Max - bounds.X.Min) / (double)(this->SeedRes[0] - 1);
  if (this->SeedRes[1] > 1)
    y_spacing = (double)(bounds.Y.Max - bounds.Y.Min) / (double)(this->SeedRes[1] - 1);
  if (this->SeedRes[2] > 1)
    z_spacing = (double)(bounds.Z.Max - bounds.Z.Min) / (double)(this->SeedRes[2] - 1);
  // Divide by zero handling for 2D data set. How is this handled

  BasisParticles.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
  BasisParticlesValidity.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);

  auto portal1 = BasisParticles.GetPortalControl();
  auto portal2 = BasisParticlesValidity.GetPortalControl();

  vtkm::Id count = 0, id = 0;
  for (int x = 0; x < this->SeedRes[0]; x++)
  {
    vtkm::FloatDefault xi = static_cast<vtkm::FloatDefault>(x * x_spacing);
    for (int y = 0; y < this->SeedRes[1]; y++)
    {
      vtkm::FloatDefault yi = static_cast<vtkm::FloatDefault>(y * y_spacing);
      for (int z = 0; z < this->SeedRes[2]; z++)
      {
        vtkm::FloatDefault zi = static_cast<vtkm::FloatDefault>(z * z_spacing);
        portal1.Set(count,
                    vtkm::Particle(Vec3f(static_cast<vtkm::FloatDefault>(bounds.X.Min) + xi,
                                         static_cast<vtkm::FloatDefault>(bounds.Y.Min) + yi,
                                         static_cast<vtkm::FloatDefault>(bounds.Z.Min) + zi),
                                   id));
        portal2.Set(count, 1);
        count++;
        id++;
      }
    }
  }
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet Lagrangian::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>)
{

  if (cycle == 0)
  {
    InitializeUniformSeeds(input);
    BasisParticlesOriginal.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
    vtkm::cont::ArrayCopy(BasisParticles, BasisParticlesOriginal);
  }

  if (!fieldMeta.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  if (this->writeFrequency == 0)
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Write frequency can not be 0. Use SetWriteFrequency().");
  }
  vtkm::cont::ArrayHandle<vtkm::Particle> basisParticleArray;
  vtkm::cont::ArrayCopy(BasisParticles, basisParticleArray);

  cycle += 1;
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());
  vtkm::Bounds bounds = input.GetCoordinateSystem().GetBounds();

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  vtkm::worklet::ParticleAdvection particleadvection;
  vtkm::worklet::ParticleAdvectionResult res;

  GridEvalType gridEval(coords, cells, field);
  RK4Type rk4(gridEval, static_cast<vtkm::Float32>(this->stepSize));

  res = particleadvection.Run(rk4, basisParticleArray, 1); // Taking a single step

  auto particles = res.Particles;
  auto particlePortal = particles.GetPortalControl();

  auto start_position = BasisParticlesOriginal.GetPortalControl();
  auto portal_validity = BasisParticlesValidity.GetPortalControl();

  vtkm::cont::DataSet outputData;
  vtkm::cont::DataSetBuilderExplicit dataSetBuilder;

  if (cycle % this->writeFrequency == 0)
  {
    int connectivity_index = 0;
    std::vector<vtkm::Id> connectivity;
    std::vector<vtkm::Vec<T, 3>> pointCoordinates;
    std::vector<vtkm::UInt8> shapes;
    std::vector<vtkm::IdComponent> numIndices;

    for (vtkm::Id index = 0; index < particlePortal.GetNumberOfValues(); index++)
    {
      auto start_point = start_position.Get(index);
      auto end_point = particlePortal.Get(index).Pos;
      auto steps = particlePortal.Get(index).NumSteps;

      if (steps > 0 && portal_validity.Get(index) == 1)
      {
        if (bounds.Contains(end_point))
        {
          connectivity.push_back(connectivity_index);
          connectivity.push_back(connectivity_index + 1);
          connectivity_index += 2;
          pointCoordinates.push_back(
            vtkm::Vec3f(static_cast<vtkm::FloatDefault>(start_point.Pos[0]),
                        static_cast<vtkm::FloatDefault>(start_point.Pos[1]),
                        static_cast<vtkm::FloatDefault>(start_point.Pos[2])));
          pointCoordinates.push_back(vtkm::Vec3f(static_cast<vtkm::FloatDefault>(end_point[0]),
                                                 static_cast<vtkm::FloatDefault>(end_point[1]),
                                                 static_cast<vtkm::FloatDefault>(end_point[2])));
          shapes.push_back(vtkm::CELL_SHAPE_LINE);
          numIndices.push_back(2);
        }
        else
        {
          portal_validity.Set(index, 0);
        }
      }
      else
      {
        portal_validity.Set(index, 0);
      }
    }

    outputData = dataSetBuilder.Create(pointCoordinates, shapes, numIndices, connectivity);
    std::stringstream file_path;
    file_path << "output/basisflows_" << this->rank << "_";
    auto f_path = file_path.str();
    WriteDataSet(cycle, f_path, outputData);
    if (this->resetParticles)
    {
      InitializeUniformSeeds(input);
      BasisParticlesOriginal.Allocate(this->SeedRes[0] * this->SeedRes[1] * this->SeedRes[2]);
      vtkm::cont::ArrayCopy(BasisParticles, BasisParticlesOriginal);
    }
    else
    {
      vtkm::cont::ArrayCopy(particles, BasisParticles);
    }
  }
  else
  {
    ValidityCheck check(bounds);
    this->Invoke(check, particles, BasisParticlesValidity);
    vtkm::cont::ArrayCopy(particles, BasisParticles);
  }

  return outputData;
}

//---------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool Lagrangian::DoMapField(vtkm::cont::DataSet&,
                                             const vtkm::cont::ArrayHandle<T, StorageType>&,
                                             const vtkm::filter::FieldMetadata&,
                                             const vtkm::filter::PolicyBase<DerivedPolicy>)
{
  return false;
}
}
} // namespace vtkm::filter

#endif
