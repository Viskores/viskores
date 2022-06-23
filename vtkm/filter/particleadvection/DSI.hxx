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

template <typename ParticleType>
VTKM_CONT vtkm::cont::DataSet DSI::GetOutput() const
{
  vtkm::cont::DataSet ds;

  std::size_t nResults = this->Results.size();
  if (nResults == 0)
    return ds;

  std::cout << "Check the variant type!" << std::endl;

  if (this->ResType == PARTICLE_ADVECT_TYPE)
  {
    using RType = vtkm::worklet::ParticleAdvectionResult<ParticleType>;

    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);

    //Get all the particles and put them into a single ArrayHandle: pts
    for (const auto& vres : this->Results)
      allParticles.push_back(vres.Get<RType>().Particles);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
    vtkm::cont::ParticleArrayCopy(allParticles, pts);

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
    using ResultType = vtkm::worklet::StreamlineResult<ParticleType>;

    //Easy case with one result.
    if (nResults == 1)
    {
      const auto& res = this->Results[0].Get<ResultType>();
      ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Positions));
      ds.SetCellSet(res.PolyLines);
    }
    else
    {
      std::vector<vtkm::Id> posOffsets(nResults, 0);
      vtkm::Id totalNumCells = 0, totalNumPts = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = this->Results[i].Get<ResultType>();
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
        const auto& res = this->Results[i].Get<ResultType>();
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
        const auto& res = this->Results[i].Get<ResultType>();
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

  return ds;
}

template <typename ParticleType, template <typename> class ResultType>
VTKM_CONT void DSI::UpdateResult(ResultType<ParticleType>& result, DSIStuff<ParticleType>& stuff)
{
  this->ClassifyParticles(result.Particles, stuff);
  this->Results.push_back(result);
}

template <typename ParticleType>
VTKM_CONT void DSI::ClassifyParticles(const vtkm::cont::ArrayHandle<ParticleType>& particles,
                                      DSIStuff<ParticleType>& stuff) const
{
  stuff.A.clear();
  stuff.I.clear();
  stuff.TermID.clear();
  stuff.IdMapI.clear();
  stuff.IdMapA.clear();

  auto portal = particles.WritePortal();
  vtkm::Id n = portal.GetNumberOfValues();

  for (vtkm::Id i = 0; i < n; i++)
  {
    auto p = portal.Get(i);

    if (p.Status.CheckTerminate())
      stuff.TermID.push_back(p.ID);
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
              (stuff.A.size() + stuff.I.size() + stuff.TermID.size()));
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

  if (this->SolverType == IntegrationSolverType::RK4_TYPE)
  {
    if (this->VecFieldType == VELOCITY_FIELD_TYPE) //vtkm::Particle, VelocityField
    {
      using FieldType = vtkm::worklet::particleadvection::VelocityField<ArrayType>;
      using GridEvType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
      using RK4_Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvType>;
      using StepperType = vtkm::worklet::particleadvection::Stepper<RK4_Type, GridEvType>;

      FieldType velField;
      this->GetVelocityField(velField);

      GridEvType eval(this->DataSet, velField);
      StepperType stepper(eval, stepSize);

      //make this a template
      if (this->ResType == PARTICLE_ADVECT_TYPE)
      {
        vtkm::worklet::ParticleAdvection Worklet;
        auto r = Worklet.Run(stepper, seedArray, maxSteps);
        this->UpdateResult(r, stuff);
        // /*result =*/ Worklet.Run(stepper, seedArray, maxSteps);
      }
      else
      {
        vtkm::worklet::Streamline Worklet;
        auto r = Worklet.Run(stepper, seedArray, maxSteps);
        this->UpdateResult(r, stuff);
        //Put results in unknown array??
      }
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
}

}
}
}

#endif //vtk_m_filter_DSI_hxx
