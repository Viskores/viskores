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

namespace internal
{
template <typename GridEvalType,
          typename WorkletType,
          template <typename>
          class ResultType,
          typename ParticleType,
          template <typename>
          class IntType>
class AdvectHelper;

using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
using TempGridEvType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;


//Steady state
template <typename WorkletType,
          template <typename>
          class ResultType,
          typename ParticleType,
          template <typename>
          class IntType>
class AdvectHelper<GridEvType, WorkletType, ResultType, ParticleType, IntType>
{
public:
  static void Advect(const FieldType& velField,
                     const vtkm::cont::DataSet& ds,
                     //const vtkm::filter::particleadvection::DSI::SteadyStateDataType& data,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     ResultType<ParticleType>& result)

  {
    std::cout << "ADVECT HELPER!!!!!!!" << std::endl;
    using StepperType = vtkm::worklet::particleadvection::Stepper<IntType<GridEvType>, GridEvType>;

    GridEvType eval(ds, velField);
    StepperType stepper(eval, stepSize);

    WorkletType worklet;
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};


//unSteady state
template <typename WorkletType,
          template <typename>
          class ResultType,
          typename ParticleType,
          template <typename>
          class IntType>
class AdvectHelper<TempGridEvType, WorkletType, ResultType, ParticleType, IntType>
{
public:
  static void Advect(const FieldType& velField1,
                     const FieldType& velField2,
                     const vtkm::filter::particleadvection::DSI::UnsteadyStateDataType& data,
                     vtkm::cont::ArrayHandle<ParticleType>& seedArray,
                     vtkm::FloatDefault stepSize,
                     vtkm::Id maxSteps,
                     ResultType<ParticleType>& result)

  {
    std::cout << "TEMP ADVECT HELPER!!!!!!!" << std::endl;
    using StepperType =
      vtkm::worklet::particleadvection::Stepper<IntType<TempGridEvType>, TempGridEvType>;

    TempGridEvType eval(data.DataSet1, data.Time1, velField1, data.DataSet2, data.Time2, velField2);
    StepperType stepper(eval, stepSize);

    WorkletType worklet;
    result = worklet.Run(stepper, seedArray, maxSteps);
  }
};

}

#if 0
namespace internal
{
//Helper class to store the different result types.
//template <typename ResultType>
template <template <typename> class ResultType, typename ParticleType>
class ResultHelper2;

template <typename ParticleType>
class ResultHelper2<vtkm::worklet::ParticleAdvectionResult, ParticleType>
{
  using PAType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;

public:
  static void Store(const PAType& result)
  {
  }

  static vtkm::cont::DataSet
  GetOutput()
  {
    vtkm::cont::DataSet ds;

    return ds;
  }
};

template <typename ParticleType>
class ResultHelper2<vtkm::worklet::StreamlineResult, ParticleType>
{
  using PAType = vtkm::worklet::StreamlineResult<ParticleType>;

public:
  static void Store(const PAType& result)
  {
  }

  static vtkm::cont::DataSet
  GetOutput()
  {
    vtkm::cont::DataSet ds;

    return ds;
  }
};

}
#endif

//template <typename ParticleType>
void
//DSI<ParticleType>::Meow() const
DSI::Meow(const char* func, const int& lineNum) const
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

  if (this->ResType == PARTICLE_ADVECT_TYPE)
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
  else if (this->ResType == STREAMLINE_TYPE)
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

namespace internal
{
using ResultVariantType =
  vtkm::cont::internal::Variant<vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>,
                                vtkm::worklet::ParticleAdvectionResult<vtkm::ChargedParticle>,
                                vtkm::worklet::StreamlineResult<vtkm::Particle>,
                                vtkm::worklet::StreamlineResult<vtkm::ChargedParticle>>;

template <template <typename> class ResultType, typename ParticleType>
class ResultHelper
{
public:
  static void Store(const vtkm::worklet::ParticleAdvectionResult<ParticleType>& res,
                    std::vector<ResultVariantType>& results)
  {
    results.push_back(res);
  }
};


/*
template <typename ParticleType>
class ResultHelper<vtkm::worklet::ParticleAdvectionResult, ParticleType>
{
public:
  static void Store(const vtkm::worklet::ParticleAdvectionResult<ParticleType>>& res,
                    std::vector<ResultVariantType>& results)
  {
    results.push_back(res);
  }
};
  */
};

template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT void DSI::UpdateResult(const ResultType<ParticleType>& result,
                                 DSIStuff<ParticleType>& stuff)
{
  this->ClassifyParticles(result.Particles, stuff);

  //this->Meow(__PRETTY_FUNCTION__, __LINE__);

  //template this for PA and SL
  if (this->ResType == vtkm::filter::particleadvection::PARTICLE_ADVECT_TYPE)
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
  else if (this->ResType == vtkm::filter::particleadvection::STREAMLINE_TYPE)
  {
    this->Results.push_back(result);
  }

  //this->Meow(__PRETTY_FUNCTION__, __LINE__);
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
VTKM_CONT void DSI::Advect(std::vector<ParticleType>& v,
                           vtkm::FloatDefault stepSize,
                           vtkm::Id maxSteps,
                           DSIStuff<ParticleType>& stuff)
{
  //using Association = vtkm::cont::Field::Association;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  auto copyFlag = (this->CopySeedArray ? vtkm::CopyFlag::On : vtkm::CopyFlag::Off);
  auto seedArray = vtkm::cont::make_ArrayHandle(v, copyFlag);

  std::cout << "DSI::Advect() " << v.size() << std::endl;


  //Assume all RK4.
  if (this->VecFieldType == VELOCITY_FIELD_TYPE)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
    if (this->IsSteadyState())
    {
      const auto& data = this->Data.Get<SteadyStateDataType>();
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      FieldType velField;
      this->GetSteadyStateVelocityField(velField);

      if (this->ResType == PARTICLE_ADVECT_TYPE)
      {
        using AHType = internal::AdvectHelper<GridEvType,
                                              vtkm::worklet::ParticleAdvection,
                                              vtkm::worklet::ParticleAdvectionResult,
                                              ParticleType,
                                              vtkm::worklet::particleadvection::RK4Integrator>;
        vtkm::worklet::ParticleAdvectionResult<ParticleType> result;
        AHType::Advect(velField, data, seedArray, stepSize, maxSteps, result);
        this->UpdateResult(result, stuff);
      }
      else
      {
        using AHType = internal::AdvectHelper<GridEvType,
                                              vtkm::worklet::Streamline,
                                              vtkm::worklet::StreamlineResult,
                                              ParticleType,
                                              vtkm::worklet::particleadvection::RK4Integrator>;
        vtkm::worklet::StreamlineResult<ParticleType> result;
        AHType::Advect(velField, data, seedArray, stepSize, maxSteps, result);
        this->UpdateResult(result, stuff);
      }
    }
    else if (this->IsUnsteadyState())
    {
      const auto& data = this->Data.Get<UnsteadyStateDataType>();
      using GridEvType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;

      FieldType velField1, velField2;
      this->GetUnsteadyStateVelocityField(velField1, velField2);

      if (this->ResType == PARTICLE_ADVECT_TYPE)
      {
        using AHType = internal::AdvectHelper<GridEvType,
                                              vtkm::worklet::ParticleAdvection,
                                              vtkm::worklet::ParticleAdvectionResult,
                                              ParticleType,
                                              vtkm::worklet::particleadvection::RK4Integrator>;
        vtkm::worklet::ParticleAdvectionResult<ParticleType> result;

        AHType::Advect(velField1, velField2, data, seedArray, stepSize, maxSteps, result);
        this->UpdateResult(result, stuff);
      }
      else
      {
        using AHType = internal::AdvectHelper<GridEvType,
                                              vtkm::worklet::Streamline,
                                              vtkm::worklet::StreamlineResult,
                                              ParticleType,
                                              vtkm::worklet::particleadvection::RK4Integrator>;
        vtkm::worklet::StreamlineResult<ParticleType> result;

        AHType::Advect(velField1, velField2, data, seedArray, stepSize, maxSteps, result);
        this->UpdateResult(result, stuff);
      }
    }
    else
      throw vtkm::cont::ErrorFilterExecution("Unsupported Data Type in DSI");
  }


#if 0
  if (this->SolverType == IntegrationSolverType::RK4_TYPE)
  {
    if (this->VecFieldType == VELOCITY_FIELD_TYPE) //vtkm::Particle, VelocityField
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
      //using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using GridEvType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldType>;

      FieldType velField;
      this->GetVelocityField(velField);

      if (this->ResType == PARTICLE_ADVECT_TYPE)
      {
        using AHType = internal::AdvectHelper<GridEvType,
                                              vtkm::worklet::ParticleAdvection,
                                              vtkm::worklet::ParticleAdvectionResult,
                                              ParticleType,
                                              vtkm::worklet::particleadvection::RK4Integrator>;
        vtkm::worklet::ParticleAdvectionResult<ParticleType> result;
        //AHType::Advect(velField, this->DataSet, seedArray, stepSize, maxSteps, result);
        AHType::Advect(velField, velField, this->DataSet, this->DataSet, 0.0, 1.0, seedArray, stepSize, maxSteps, result);
        this->UpdateResult(result, stuff);
      }
      else
      {
        using AHType = internal::AdvectHelper<GridEvType,
                                              vtkm::worklet::Streamline,
                                              vtkm::worklet::StreamlineResult,
                                              ParticleType,
                                              vtkm::worklet::particleadvection::RK4Integrator>;
        vtkm::worklet::StreamlineResult<ParticleType> result;
        //AHType::Advect(velField, this->DataSet, seedArray, stepSize, maxSteps, result);
        AHType::Advect(velField, velField, this->DataSet, this->DataSet, 0.0, 1.0, seedArray, stepSize, maxSteps, result);
        this->UpdateResult(result, stuff);
      }

      /*
      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);

      //make this a template
      if (this->ResType == PARTICLE_ADVECT_TYPE)
      {
        vtkm::worklet::ParticleAdvection Worklet;
        auto result = Worklet.Run(stepper, seedArray, maxSteps);
        this->UpdateResult(result, stuff);
      }
      else
      {
        vtkm::worklet::Streamline Worklet;
        auto r = Worklet.Run(stepper, seedArray, maxSteps);
        this->UpdateResult(r, stuff);
        //Put results in unknown array??
      }
      */
    }
    else if (this->VecFieldType == ELECTRO_MAGNETIC_FIELD_TYPE) //vtkm::ChargedParticle
    {
      using FieldType = vtkm::worklet::particleadvection::ElectroMagneticField<ArrayType>;
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
      using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

      FieldType velField;
      this->GetElectroMagneticField(velField);

      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);

      if (this->ResType == PARTICLE_ADVECT_TYPE)
      {
        vtkm::worklet::ParticleAdvection Worklet;
        auto r = Worklet.Run(stepper, seedArray, maxSteps);
        this->UpdateResult(r, stuff);
        ///*result =*/ auto r = Worklet.Run(stepper, seedArray, maxSteps);
      }
      else
      {
        vtkm::worklet::Streamline Worklet;
        auto r = Worklet.Run(stepper, seedArray, maxSteps);
        this->UpdateResult(r, stuff);
        ///*result =*/ Worklet.Run(stepper, seedArray, maxSteps);
      }
    }
  }

  else if (this->SolverType == IntegrationSolverType::EULER_TYPE)
  {
    if (this->VecFieldType == VELOCITY_FIELD_TYPE) //vtkm::Particle, VelocityField
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using EulerType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvType>;
      using StepperType = vtkm::worklet::particleadvection::Stepper<EulerType, GridEvType>;

      FieldType velField;
      this->GetVelocityField(velField);

      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);
//      vtkm::worklet::ParticleAdvection Worklet;
//      result = Worklet.Run(stepper, seedArray, maxSteps);
      //Put results in unknown array??
    }
  }
#endif
}


}
}
}

#endif //vtk_m_filter_DSI_hxx
