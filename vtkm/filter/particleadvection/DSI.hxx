//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_DSI_hxx
#define vtk_m_filter_DSI_hxx

namespace vtkm
{
namespace filter
{
namespace particleadvection
{

#if 0
namespace internal
{
using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
using SteadyStateGridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
using UnSteadyStateGridEvalType =
  vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;

template <typename GridEvalType,
          typename WorkletType,
          template <typename> class ResultType,
          typename ParticleType>
class AdvectHelper;

 //Steady state
template <typename WorkletType, template <typename> class ResultType, typename ParticleType>
class AdvectHelper<SteadyStateGridEvalType, WorkletType, ResultType, ParticleType>
{
public:
  static void Advect(const FieldType& velField,
                     const vtkm::cont::DataSet& ds,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& integratorType,
                     ResultType<ParticleType>& result)
  {
    using AHType =
      internal::AdvectHelper<SteadyStateGridEvalType, WorkletType, ResultType, ParticleType>;
    if (integratorType == IntegrationSolverType::RK4_TYPE)
      AHType::template DoAdvect<vtkm::worklet::particleadvection::RK4Integrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    else if (integratorType == IntegrationSolverType::EULER_TYPE)
      AHType::template DoAdvect<vtkm::worklet::particleadvection::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <template <typename> class IntType>
  static void DoAdvect(const FieldType& velField,
                       const vtkm::cont::DataSet& ds,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType = vtkm::worklet::particleadvection::Stepper<IntType<SteadyStateGridEvalType>,
                                                                  SteadyStateGridEvalType>;
    SteadyStateGridEvalType eval(ds, velField);
    StepperType stepper(eval, stepSize);
    WorkletType worklet;
    result = worklet.Run(stepper, seedArray, maxSteps);
  }


};

}

void DSI::Meow(const char* func, const int& lineNum) const
{
  if (this->Results.empty())
  {
    //std::cout<<"   no results!"<<std::endl;
    return;
  }

  std::cout << " ******************************";
  std::cout << func << " " << lineNum << " ";
  using ResType = vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>;

  //const auto& R0 = this->Results[0]->Get<RType>();
  //const auto& P0 = R0.Particles.ReadPortal().Get(0);
  //RType* r = static_cast<RType*>(this->Results[0]);
  //const auto& P0 = r->Particles.ReadPortal().Get(0);
  const auto& P0 = this->Results[0].Get<ResType>().Particles.ReadPortal().Get(0);
  std::cout << "   PT0=  " << P0.Pos << std::endl;

  //std::cout<<"   PT0=  "<<P0.Pos<<std::endl;

  /*
  auto& R0 = this->Results[0].Get<RType>();
  auto P0 = R0.Particles.ReadPortal().Get(0);
  std::cout<<"   PT0=  "<<P0.Pos<<std::endl;
  auto ppp = R0.Particles;
  std::cout<<"   ptrs: RP= "<<(void*)&(R0)<<" pPtr= "<<(void*)&ppp<<std::endl;
  */
}

template <typename ParticleType>
VTKM_CONT bool DSI::GetOutput(vtkm::cont::DataSet& ds) const
{
  std::size_t nResults = this->Results.size();
  if (nResults == 0)
    return false;

  std::cout << "Check the variant type!" << std::endl;

  if (this->ResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;

    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);
    for (const auto& vres : this->Results)
      allParticles.push_back(vres.Get<ResType>().Particles);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
    vtkm::cont::ParticleArrayCopy(allParticles, pts);
    std::cout << "DSI::GetOutput() pts= ";
    vtkm::cont::printSummary_ArrayHandle(pts, std::cout);

    vtkm::Id numPoints = pts.GetNumberOfValues();
    if (numPoints > 0)
    {
      //Create coordinate system and vertex cell set.
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", pts));

      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      ds.SetCellSet(cells);
    }
  }
  else if (this->ResType == ParticleAdvectionResultType::STREAMLINE_TYPE)
  {
    using ResType = vtkm::worklet::StreamlineResult<ParticleType>;

    //Easy case with one result.
    if (nResults == 1)
    {
      const auto& res = this->Results[0].Get<ResType>();
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Positions));
      ds.SetCellSet(res.PolyLines);
    }
    else
    {
      std::vector<vtkm::Id> posOffsets(nResults, 0);
      vtkm::Id totalNumCells = 0, totalNumPts = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
        if (i == 0)
          posOffsets[i] = 0;
        else
          posOffsets[i] = totalNumPts;

        totalNumPts += res.Positions.GetNumberOfValues();
        totalNumCells += res.PolyLines.GetNumberOfCells();
      }

      //Append all the points together.
      vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
      appendPts.Allocate(totalNumPts);
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
        // copy all values into appendPts starting at offset.
        vtkm::cont::Algorithm::CopySubRange(
          res.Positions, 0, res.Positions.GetNumberOfValues(), appendPts, posOffsets[i]);
      }
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", appendPts));

      //Create polylines.
      std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
      std::size_t off = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
        vtkm::Id nCells = res.PolyLines.GetNumberOfCells();
        for (vtkm::Id j = 0; j < nCells; j++)
          numPtsPerCell[off++] = static_cast<vtkm::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
      }

      auto numPointsPerCellArray = vtkm::cont::make_ArrayHandle(numPtsPerCell, vtkm::CopyFlag::Off);

      vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
      vtkm::Id connectivityLen =
        vtkm::cont::Algorithm::ScanExclusive(numPointsPerCellArray, cellIndex);
      vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
      vtkm::cont::ArrayCopy(connCount, connectivity);

      vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
      auto polyLineShape = vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(
        vtkm::CELL_SHAPE_POLY_LINE, totalNumCells);
      vtkm::cont::ArrayCopy(polyLineShape, cellTypes);
      auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numPointsPerCellArray);

      vtkm::cont::CellSetExplicit<> polyLines;
      polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
      ds.SetCellSet(polyLines);
    }
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution("Unsupported ParticleAdvectionResultType");
  }

  return true;
}

void DSI::DoAdvect(Bum<vtkm::Particle>& b,
                   vtkm::FloatDefault stepSize,
                   vtkm::Id maxSteps)
{
  using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;

  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
    FieldType velField;
    this->GetSteadyStateVelocityField(velField);

    if (this->ResType == ParticleAdectionResultType::PARTICLE_ADVECT_TYPE)
    {
      using AHType = internal::AdvectHelper<GridEvType,
                                            vtkm::worklet::ParticleAdvection,
                                            vtkm::worklet::ParticleAdvectionResult,
                                            vtkm::Particle>;
      AHType::Advect(result);
    }
    else if (this->ResType == ParticleAdectionResultType::STREAMLINE_TYPE)
    {
    }
  }
    std::cout<<"***************************************************************        DSI::DoAdvect --------- Particle"<<std::endl;
    using FieldType = vtkm::worklet::particleadvection::VelocityField<internal::ArrayType>;
    using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
    using AHType = internal::AdvectHelper<GridEvType,
                                          vtkm::worklet::ParticleAdvection,
                                          vtkm::worklet::ParticleAdvectionResult,
                                          vtkm::Particle>;

    auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
    auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

    const auto& data = this->Data.Get<vtkm::cont::DataSet>();
    FieldType velField;
    this->GetSteadyStateVelocityField(velField);
    vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> result;
    AHType::Advect(velField, data, seedArray, stepSize, maxSteps, this->SolverType, result);
    //this->UpdateResult(result, stuff);
}

void DSI::DoAdvect(Bum<vtkm::ChargedParticle>& b,
                   vtkm::FloatDefault stepSize,
                   vtkm::Id maxSteps)
{
}

template <typename ParticleType>
VTKM_CONT void DSI::Advect(std::vector<ParticleType>& v,
                           vtkm::FloatDefault stepSize,
                           vtkm::Id maxSteps,
                           DSIStuff<ParticleType>& stuff)
{
  //using Association = vtkm::cont::Field::Association;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(v, copyFlag);

  //PA or SL result.
  if (this->ResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
    vtkm::worklet::ParticleAdvectionResult<ParticleType> result;
    vtkm::worklet::ParticleAdvection worklet;

    if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;

      if (this->IsSteadyState())
      {
        using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;

        FieldType velField;
        this->GetSteadyStateVelocityField(velField);

        if (this->SolverType == IntegrationSolverType::RK4_TYPE)
        {
          using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

          GridEvType eval(this->Data.Get<DSI::SteadyStateDataType>(), velField);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
        else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
        {
          using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

          GridEvType eval(this->Data.Get<SteadyStateDataType>(), velField);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
      }
      else if (this->IsUnsteadyState())
      {
        using GridEvType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;

        FieldType velField1, velField2;
        this->GetUnsteadyStateVelocityField(velField1, velField2);
        const auto d = this->Data.Get<UnsteadyStateDataType>();

        if (this->SolverType == IntegrationSolverType::RK4_TYPE)
        {
          using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

          GridEvType eval(d.DataSet1, d.Time1, velField1, d.DataSet2, d.Time2, velField2);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
        else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
        {
          using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

          GridEvType eval(d.DataSet1, d.Time1, velField1, d.DataSet2, d.Time2, velField2);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
      }
    }
    else if (this->VecFieldType == VectorFieldType::ELECTRO_MAGNETIC_FIELD_TYPE)
    {
      using FieldType = vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>;

      if (this->IsSteadyState())
      {
        using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;

        FieldType velField;
        this->GetSteadyStateElectroMagneticField(velField);
        if (this->SolverType == IntegrationSolverType::RK4_TYPE)
        {
          using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

          GridEvType eval(this->Data.Get<DSI::SteadyStateDataType>(), velField);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
        else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
        {
          using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

          GridEvType eval(this->Data.Get<SteadyStateDataType>(), velField);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
      }
      else if (this->IsUnsteadyState())
      {
        using GridEvType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;
        FieldType velField1, velField2;
        this->GetUnsteadyStateElectroMagneticField(velField1, velField2);
        const auto d = this->Data.Get<UnsteadyStateDataType>();

        if (this->SolverType == IntegrationSolverType::RK4_TYPE)
        {
          using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

          GridEvType eval(d.DataSet1, d.Time1, velField1, d.DataSet2, d.Time2, velField2);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
        else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
        {
          using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

          GridEvType eval(d.DataSet1, d.Time1, velField1, d.DataSet2, d.Time2, velField2);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
      }
    }

    this->UpdateResult(result, stuff);
  }
  else if (this->ResType == ParticleAdvectionResultType::STREAMLINE_TYPE)
  {
    vtkm::worklet::StreamlineResult<ParticleType> result;
    vtkm::worklet::Streamline worklet;

    if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;

      if (this->IsSteadyState())
      {
        using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;

        FieldType velField;
        this->GetSteadyStateVelocityField(velField);

        if (this->SolverType == IntegrationSolverType::RK4_TYPE)
        {
          using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

          GridEvType eval(this->Data.Get<DSI::SteadyStateDataType>(), velField);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
        else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
        {
          using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

          GridEvType eval(this->Data.Get<SteadyStateDataType>(), velField);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
      }
      else if (this->IsUnsteadyState())
      {
        using GridEvType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;

        FieldType velField1, velField2;
        this->GetUnsteadyStateVelocityField(velField1, velField2);
        const auto d = this->Data.Get<UnsteadyStateDataType>();

        if (this->SolverType == IntegrationSolverType::RK4_TYPE)
        {
          using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

          GridEvType eval(d.DataSet1, d.Time1, velField1, d.DataSet2, d.Time2, velField2);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
        else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
        {
          using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
          using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

          GridEvType eval(d.DataSet1, d.Time1, velField1, d.DataSet2, d.Time2, velField2);
          StepperType stepper(eval, stepSize);

          result = worklet.Run(stepper, seedArray, maxSteps);
        }
      }
    }
    else if (this->VecFieldType == VectorFieldType::ELECTRO_MAGNETIC_FIELD_TYPE)
    {
      using FieldType = vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>;
    }

    this->UpdateResult(result, stuff);
  }
  else
    throw vtkm::cont::ErrorFilterExecution("Unsupported Data Type in DSI");
}

#endif

template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT void DSI::UpdateResult(const ResultType<ParticleType>& result,
                                 DSIStuff<ParticleType>& stuff)
{
  this->ClassifyParticles(result.Particles, stuff);
  std::cout << "DSI::UpdateResult  TermIdx= " << stuff.TermIdx.size() << std::endl;

  //template this for PA and SL
  if (this->ResType ==
      vtkm::filter::particleadvection::ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
    if (stuff.TermIdx.empty())
      return;

    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;
    auto indicesAH = vtkm::cont::make_ArrayHandle(stuff.TermIdx, vtkm::CopyFlag::Off);
    auto termPerm = vtkm::cont::make_ArrayHandlePermutation(indicesAH, result.Particles);

    vtkm::cont::ArrayHandle<ParticleType> termParticles;
    vtkm::cont::Algorithm::Copy(termPerm, termParticles);

    ResType termRes(termParticles);
    this->Results.push_back(termRes);
  }
  else if (this->ResType ==
           vtkm::filter::particleadvection::ParticleAdvectionResultType::STREAMLINE_TYPE)
  {
    this->Results.push_back(result);
  }
}

template <typename ParticleType>
VTKM_CONT void DSI::ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                      DSIStuff<ParticleType>& stuff) const
{
  stuff.A.clear();
  stuff.I.clear();
  stuff.TermID.clear();
  stuff.TermIdx.clear();
  stuff.IdMapI.clear();
  stuff.IdMapA.clear();

  auto portal = particles.WritePortal();
  vtkm::Id n = portal.GetNumberOfValues();

  for (vtkm::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);
    std::cout << "****** ClassifyParticles: " << p.Pos << " ID= " << p.ID << std::endl;

    if (p.Status.CheckTerminate())
    {
      stuff.TermIdx.push_back(i);
      stuff.TermID.push_back(p.ID);
    }
    else
    {
      const auto& it = stuff.ParticleBlockIDsMap.find(p.ID);
      VTKM_ASSERT(it != stuff.ParticleBlockIDsMap.end());
      auto currBIDs = it->second;
      VTKM_ASSERT(!currBIDs.empty());

      std::vector<vtkm::Id> newIDs;
      if (p.Status.CheckSpatialBounds() && !p.Status.CheckTookAnySteps())
        newIDs.assign(std::next(currBIDs.begin(), 1), currBIDs.end());
      else
        newIDs = stuff.BoundsMap.FindBlocks(p.Pos, currBIDs);

      //reset the particle status.
      p.Status = vtkm::ParticleStatus();

      if (newIDs.empty()) //No blocks, we're done.
      {
        p.Status.SetTerminate();
        stuff.TermIdx.push_back(i);
        stuff.TermID.push_back(p.ID);
      }
      else
      {
        //If we have more than blockId, we want to minimize communication
        //and put any blocks owned by this rank first.
        if (newIDs.size() > 1)
        {
          for (auto idit = newIDs.begin(); idit != newIDs.end(); idit++)
          {
            vtkm::Id bid = *idit;
            if (stuff.BoundsMap.FindRank(bid) == this->Rank)
            {
              newIDs.erase(idit);
              newIDs.insert(newIDs.begin(), bid);
              break;
            }
          }
        }

        int dstRank = stuff.BoundsMap.FindRank(newIDs[0]);
        if (dstRank == this->Rank)
        {
          stuff.A.push_back(p);
          stuff.IdMapA[p.ID] = newIDs;
        }
        else
        {
          stuff.I.push_back(p);
          stuff.IdMapI[p.ID] = newIDs;
        }
      }
      portal.Set(i, p);
    }
  }

  //Make sure we didn't miss anything. Every particle goes into a single bucket.
  VTKM_ASSERT(static_cast<std::size_t>(n) ==
              (stuff.A.size() + stuff.I.size() + stuff.TermIdx.size()));
  VTKM_ASSERT(stuff.TermIdx.size() == stuff.TermID.size());
}

template <typename ParticleType>
VTKM_CONT bool DSI::GetOutput(vtkm::cont::DataSet& ds) const
{
  std::size_t nResults = this->Results.size();
  if (nResults == 0)
    return false;

  std::cout << "Check the variant type!" << std::endl;

  if (this->ResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
  {
    using ResType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;

    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);
    for (const auto& vres : this->Results)
      allParticles.push_back(vres.Get<ResType>().Particles);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
    vtkm::cont::ParticleArrayCopy(allParticles, pts);
    std::cout << "DSI::GetOutput() pts= ";
    vtkm::cont::printSummary_ArrayHandle(pts, std::cout);

    vtkm::Id numPoints = pts.GetNumberOfValues();
    if (numPoints > 0)
    {
      //Create coordinate system and vertex cell set.
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", pts));

      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      ds.SetCellSet(cells);
    }
  }
  else if (this->ResType == ParticleAdvectionResultType::STREAMLINE_TYPE)
  {
    using ResType = vtkm::worklet::StreamlineResult<ParticleType>;

    //Easy case with one result.
    if (nResults == 1)
    {
      const auto& res = this->Results[0].Get<ResType>();
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Positions));
      ds.SetCellSet(res.PolyLines);
    }
    else
    {
      std::vector<vtkm::Id> posOffsets(nResults, 0);
      vtkm::Id totalNumCells = 0, totalNumPts = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
        if (i == 0)
          posOffsets[i] = 0;
        else
          posOffsets[i] = totalNumPts;

        totalNumPts += res.Positions.GetNumberOfValues();
        totalNumCells += res.PolyLines.GetNumberOfCells();
      }

      //Append all the points together.
      vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
      appendPts.Allocate(totalNumPts);
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
        // copy all values into appendPts starting at offset.
        vtkm::cont::Algorithm::CopySubRange(
          res.Positions, 0, res.Positions.GetNumberOfValues(), appendPts, posOffsets[i]);
      }
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", appendPts));

      //Create polylines.
      std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
      std::size_t off = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResType>();
        vtkm::Id nCells = res.PolyLines.GetNumberOfCells();
        for (vtkm::Id j = 0; j < nCells; j++)
          numPtsPerCell[off++] = static_cast<vtkm::Id>(res.PolyLines.GetNumberOfPointsInCell(j));
      }

      auto numPointsPerCellArray = vtkm::cont::make_ArrayHandle(numPtsPerCell, vtkm::CopyFlag::Off);

      vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
      vtkm::Id connectivityLen =
        vtkm::cont::Algorithm::ScanExclusive(numPointsPerCellArray, cellIndex);
      vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
      vtkm::cont::ArrayCopy(connCount, connectivity);

      vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
      auto polyLineShape = vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(
        vtkm::CELL_SHAPE_POLY_LINE, totalNumCells);
      vtkm::cont::ArrayCopy(polyLineShape, cellTypes);
      auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numPointsPerCellArray);

      vtkm::cont::CellSetExplicit<> polyLines;
      polyLines.Fill(totalNumPts, cellTypes, connectivity, offsets);
      ds.SetCellSet(polyLines);
    }
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution("Unsupported ParticleAdvectionResultType");
  }

  return true;
}

using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using VelocityFieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
using SteadyStateGridEvalType = vtkm::worklet::particleadvection::GridEvaluator<VelocityFieldType>;
using UnsteadyStateGridEvalType =
  vtkm::worklet::particleadvection::TemporalGridEvaluator<VelocityFieldType>;

template <typename GridEvalType, typename ParticleType>
class AdvectHelper;

template <typename ParticleType>
class AdvectHelper<SteadyStateGridEvalType, ParticleType>
{
public:
  static void Advect(const VelocityFieldType& velField,
                     const vtkm::cont::DataSet& ds,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::ParticleAdvectionResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::RK4Integrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const VelocityFieldType& velField,
                     const vtkm::cont::DataSet& ds,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::StreamlineResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::RK4Integrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField, ds, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const VelocityFieldType& velField,
                       const vtkm::cont::DataSet& ds,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType =
      vtkm::worklet::particleadvection::Stepper<SolverType<SteadyStateGridEvalType>,
                                                SteadyStateGridEvalType>;

    WorkletType worklet;
    SteadyStateGridEvalType eval(ds, velField);
    StepperType stepper(eval, stepSize);
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};

template <typename ParticleType>
class AdvectHelper<UnsteadyStateGridEvalType, ParticleType>
{
public:
  static void Advect(const VelocityFieldType& velField1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const VelocityFieldType& velField2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::ParticleAdvectionResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::RK4Integrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::ParticleAdvection,
               vtkm::worklet::ParticleAdvectionResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  static void Advect(const VelocityFieldType& velField1,
                     const vtkm::cont::DataSet& ds1,
                     vtkm::FloatDefault t1,
                     const VelocityFieldType& velField2,
                     const vtkm::cont::DataSet& ds2,
                     vtkm::FloatDefault t2,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     const IntegrationSolverType& solverType,
                     vtkm::worklet::StreamlineResult<ParticleType>& result)
  {
    if (solverType == IntegrationSolverType::RK4_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::RK4Integrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else if (solverType == IntegrationSolverType::EULER_TYPE)
    {
      DoAdvect<vtkm::worklet::Streamline,
               vtkm::worklet::StreamlineResult,
               vtkm::worklet::particleadvection::EulerIntegrator>(
        velField1, ds1, t1, velField2, ds2, t2, seedArray, stepSize, maxSteps, result);
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Integrator type");
  }

  template <typename WorkletType,
            template <typename>
            class ResultType,
            template <typename>
            class SolverType>
  static void DoAdvect(const VelocityFieldType& velField1,
                       const vtkm::cont::DataSet& ds1,
                       vtkm::FloatDefault t1,
                       const VelocityFieldType& velField2,
                       const vtkm::cont::DataSet& ds2,
                       vtkm::FloatDefault t2,
                       vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                       vtkm::FloatDefault stepSize,
                       vtkm::Id maxSteps,
                       ResultType<ParticleType>& result)
  {
    using StepperType =
      vtkm::worklet::particleadvection::Stepper<SolverType<UnsteadyStateGridEvalType>,
                                                UnsteadyStateGridEvalType>;

    WorkletType worklet;
    UnsteadyStateGridEvalType eval(ds1, t1, velField1, ds2, t2, velField2);
    StepperType stepper(eval, stepSize);
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};



VTKM_CONT void SteadyStateDSI::DoAdvect(DSIStuff<vtkm::Particle>& b,
                                        vtkm::FloatDefault stepSize,
                                        vtkm::Id maxSteps)
{
  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);
  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
    FieldType velField;
    this->GetVelocityField(velField);

    using AHType = AdvectHelper<SteadyStateGridEvalType, vtkm::Particle>;

    if (this->ResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
    {
      vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(
        velField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
      std::cout << __LINE__ << " ::DoAdvect: " << b.TermIdx.size() << std::endl;
    }
    else if (this->ResType == ParticleAdvectionResultType::STREAMLINE_TYPE)
    {
      vtkm::worklet::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(
        velField, this->DataSet, seedArray, stepSize, maxSteps, this->SolverType, result);
      this->UpdateResult(result, b);
      std::cout << __LINE__ << " ::DoAdvect: " << b.TermIdx.size() << std::endl;
    }
  }
}

VTKM_CONT void SteadyStateDSI::DoAdvect(DSIStuff<vtkm::ChargedParticle>& b,
                                        vtkm::FloatDefault stepSize,
                                        vtkm::Id maxSteps)
{
}


VTKM_CONT void UnsteadyStateDSI::DoAdvect(DSIStuff<vtkm::Particle>& b,
                                          vtkm::FloatDefault stepSize,
                                          vtkm::Id maxSteps)
{
  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(b.V, copyFlag);

  using AHType = AdvectHelper<UnsteadyStateGridEvalType, vtkm::Particle>;

  if (this->VecFieldType == VectorFieldType::VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
    FieldType velField1, velField2;
    this->GetVelocityFields(velField1, velField2);

    if (this->ResType == ParticleAdvectionResultType::PARTICLE_ADVECT_TYPE)
    {
      vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> result;
      AHType::Advect(velField1,
                     this->DataSet1,
                     this->Time1,
                     velField2,
                     this->DataSet2,
                     this->Time2,
                     seedArray,
                     stepSize,
                     maxSteps,
                     this->SolverType,
                     result);
      this->UpdateResult(result, b);
    }
    else if (this->ResType == ParticleAdvectionResultType::STREAMLINE_TYPE)
    {
      vtkm::worklet::StreamlineResult<vtkm::Particle> result;
      AHType::Advect(velField1,
                     this->DataSet1,
                     this->Time1,
                     velField2,
                     this->DataSet2,
                     this->Time2,
                     seedArray,
                     stepSize,
                     maxSteps,
                     this->SolverType,
                     result);
      this->UpdateResult(result, b);
    }
  }
}

VTKM_CONT void UnsteadyStateDSI::DoAdvect(DSIStuff<vtkm::ChargedParticle>& b,
                                          vtkm::FloatDefault stepSize,
                                          vtkm::Id maxSteps)
{
}



}
}
}

#endif //vtk_m_filter_DSI_hxx
