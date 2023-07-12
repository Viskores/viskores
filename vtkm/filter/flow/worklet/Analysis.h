//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#ifndef vtkm_worklet_particleadvection_analysis
#define vtkm_worklet_particleadvection_analysis

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/ParticleArrayCopy.h>

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename ParticleType>
class NoAnalysisExec
{
public:
  VTKM_EXEC_CONT
  NoAnalysisExec() {}

  VTKM_EXEC void PreStepAnalyze(const vtkm::Id index, const ParticleType& particle)
  {
    (void)index;
    (void)particle;
  }

  //template <typename ParticleType>
  VTKM_EXEC void Analyze(const vtkm::Id index,
                         const ParticleType& oldParticle,
                         const ParticleType& newParticle)
  {
    // Do nothing
    (void)index;
    (void)oldParticle;
    (void)newParticle;
  }
};

template <typename ParticleType>
class NoAnalysis : public vtkm::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  vtkm::cont::ArrayHandle<ParticleType> Particles;

  VTKM_CONT
  NoAnalysis()
    : Particles()
  {
  }

  VTKM_CONT
  void UseAsTemplate(const NoAnalysis& other) { (void)other; }

  VTKM_CONT
  //template <typename ParticleType>
  void InitializeAnalysis(const vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    (void)particles;
  }

  VTKM_CONT
  //template <typename ParticleType>
  void FinalizeAnalysis(vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    this->Particles = particles; //, vtkm::CopyFlag::Off);
  }

  VTKM_CONT NoAnalysisExec<ParticleType> PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                             vtkm::cont::Token& token) const
  {
    (void)device;
    (void)token;
    return NoAnalysisExec<ParticleType>();
  }

  VTKM_CONT bool SupportPushOutOfBounds() const { return true; }

  VTKM_CONT static bool MakeDataSet(vtkm::cont::DataSet& dataset,
                                    const std::vector<NoAnalysis>& results)
  {
    size_t nResults = results.size();
    std::vector<vtkm::cont::ArrayHandle<ParticleType>> allParticles;
    allParticles.reserve(nResults);
    for (const auto& vres : results)
      allParticles.emplace_back(vres.Particles);

    vtkm::cont::ArrayHandle<vtkm::Vec3f> pts;
    vtkm::cont::ParticleArrayCopy(allParticles, pts);

    vtkm::Id numPoints = pts.GetNumberOfValues();
    if (numPoints > 0)
    {
      //Create coordinate system and vertex cell set.
      dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", pts));

      vtkm::cont::CellSetSingleType<> cells;
      vtkm::cont::ArrayHandleIndex conn(numPoints);
      vtkm::cont::ArrayHandle<vtkm::Id> connectivity;

      vtkm::cont::ArrayCopy(conn, connectivity);
      cells.Fill(numPoints, vtkm::CELL_SHAPE_VERTEX, 1, connectivity);
      dataset.SetCellSet(cells);
    }
    return true;
  }
};

template <typename ParticleType>
class StreamlineAnalysisExec
{
public:
  VTKM_EXEC_CONT
  StreamlineAnalysisExec()
    : NumParticles(0)
    , MaxSteps(0)
    , Streams()
    , StreamLengths()
    , Validity()
  {
  }

  VTKM_CONT
  StreamlineAnalysisExec(vtkm::Id numParticles,
                         vtkm::Id maxSteps,
                         const vtkm::cont::ArrayHandle<vtkm::Vec3f>& streams,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& streamLengths,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& validity,
                         vtkm::cont::DeviceAdapterId device,
                         vtkm::cont::Token& token)
    : NumParticles(numParticles)
    , MaxSteps(maxSteps + 1)
  {
    Streams = streams.PrepareForOutput(this->NumParticles * this->MaxSteps, device, token);
    StreamLengths = streamLengths.PrepareForInPlace(device, token);
    Validity = validity.PrepareForInPlace(device, token);
  }

  VTKM_EXEC void PreStepAnalyze(const vtkm::Id index, const ParticleType& particle)
  {
    vtkm::Id streamLength = this->StreamLengths.Get(index);
    if (streamLength == 0)
    {
      this->StreamLengths.Set(index, 1);
      vtkm::Id loc = index * MaxSteps;
      this->Streams.Set(loc, particle.GetPosition());
      this->Validity.Set(loc, 1);
    }
  }

  //template <typename ParticleType>
  VTKM_EXEC void Analyze(const vtkm::Id index,
                         const ParticleType& oldParticle,
                         const ParticleType& newParticle)
  {
    (void)oldParticle;
    vtkm::Id streamLength = this->StreamLengths.Get(index);
    vtkm::Id loc = index * MaxSteps + streamLength;
    this->StreamLengths.Set(index, ++streamLength);
    this->Streams.Set(loc, newParticle.GetPosition());
    this->Validity.Set(loc, 1);
  }

private:
  using IdPortal = typename vtkm::cont::ArrayHandle<vtkm::Id>::WritePortalType;
  using VecPortal = typename vtkm::cont::ArrayHandle<vtkm::Vec3f>::WritePortalType;

  vtkm::Id NumParticles;
  vtkm::Id MaxSteps;
  VecPortal Streams;
  IdPortal StreamLengths;
  IdPortal Validity;
};

namespace detail
{
class GetSteps : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  GetSteps() {}
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename ParticleType>
  VTKM_EXEC void operator()(const ParticleType& p, vtkm::Id& numSteps) const
  {
    numSteps = p.GetNumberOfSteps();
  }
};

class ComputeNumPoints : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  ComputeNumPoints() {}
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  // Offset is number of points in streamline.
  // 1 (inital point) + number of steps taken (p.NumSteps - initalNumSteps)
  template <typename ParticleType>
  VTKM_EXEC void operator()(const ParticleType& p,
                            const vtkm::Id& initialNumSteps,
                            vtkm::Id& diff) const
  {
    diff = 1 + p.GetNumberOfSteps() - initialNumSteps;
  }
};

} // namespace detail

template <typename ParticleType>
class StreamlineAnalysis : public vtkm::cont::ExecutionObjectBase
{
public:
  // Intended to store advected particles after Finalize
  vtkm::cont::ArrayHandle<ParticleType> Particles;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Streams;
  vtkm::cont::CellSetExplicit<> PolyLines;

  //Helper functor for compacting history
  struct IsOne
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

  VTKM_CONT
  StreamlineAnalysis()
    : Particles()
    , MaxSteps(0)
  {
  }

  VTKM_CONT
  StreamlineAnalysis(vtkm::Id maxSteps)
    : Particles()
    , MaxSteps(maxSteps)
  {
  }

  VTKM_CONT
  void UseAsTemplate(const StreamlineAnalysis& other) { this->MaxSteps = other.MaxSteps; }

  VTKM_CONT StreamlineAnalysisExec<ParticleType> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return StreamlineAnalysisExec<ParticleType>(this->NumParticles,
                                                this->MaxSteps,
                                                this->Streams,
                                                this->StreamLengths,
                                                this->Validity,
                                                device,
                                                token);
  }

  VTKM_CONT bool SupportPushOutOfBounds() const { return true; }

  VTKM_CONT
  //template <typename ParticleType>
  void InitializeAnalysis(const vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    this->NumParticles = particles.GetNumberOfValues();

    //Create ValidPointArray initialized to zero.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> validity(0,
                                                       this->NumParticles * (this->MaxSteps + 1));
    vtkm::cont::ArrayCopy(validity, this->Validity);
    //Create StepCountArray initialized to zero.
    vtkm::cont::ArrayHandleConstant<vtkm::Id> streamLengths(0, this->NumParticles);
    vtkm::cont::ArrayCopy(streamLengths, this->StreamLengths);

    // Initialize InitLengths
    vtkm::Id numSeeds = static_cast<vtkm::Id>(particles.GetNumberOfValues());
    vtkm::cont::ArrayHandleIndex idxArray(numSeeds);
    vtkm::cont::Invoker invoker;
    invoker(detail::GetSteps{}, particles, this->InitialLengths);
  }

  VTKM_CONT
  //template <typename ParticleType>
  void FinalizeAnalysis(vtkm::cont::ArrayHandle<ParticleType>& particles)
  {
    vtkm::Id numSeeds = particles.GetNumberOfValues();
    vtkm::cont::ArrayHandle<vtkm::Vec3f> positions;
    vtkm::cont::Algorithm::CopyIf(this->Streams, this->Validity, positions, IsOne());
    vtkm::cont::Algorithm::Copy(positions, this->Streams);

    // Create the cells
    vtkm::cont::ArrayHandle<vtkm::Id> numPoints;
    vtkm::cont::Invoker invoker;
    invoker(detail::ComputeNumPoints{}, particles, this->InitialLengths, numPoints);

    vtkm::cont::ArrayHandle<vtkm::Id> cellIndex;
    vtkm::Id connectivityLen = vtkm::cont::Algorithm::ScanExclusive(numPoints, cellIndex);
    vtkm::cont::ArrayHandleIndex connCount(connectivityLen);
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayCopy(connCount, connectivity);

    vtkm::cont::ArrayHandle<vtkm::UInt8> cellTypes;
    auto polyLineShape =
      vtkm::cont::make_ArrayHandleConstant<vtkm::UInt8>(vtkm::CELL_SHAPE_POLY_LINE, numSeeds);
    vtkm::cont::ArrayCopy(polyLineShape, cellTypes);

    auto offsets = vtkm::cont::ConvertNumComponentsToOffsets(numPoints);

    this->PolyLines.Fill(this->Streams.GetNumberOfValues(), cellTypes, connectivity, offsets);
    this->Particles = particles;
  }


  VTKM_CONT static bool MakeDataSet(vtkm::cont::DataSet& dataset,
                                    const std::vector<StreamlineAnalysis>& results)
  {
    size_t nResults = results.size();
    if (nResults == 1)
    {
      const auto& res = results[0];
      dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", res.Streams));
      dataset.SetCellSet(res.PolyLines);
    }
    else
    {
      std::vector<vtkm::Id> posOffsets(nResults, 0);
      vtkm::Id totalNumCells = 0, totalNumPts = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = results[i];
        if (i == 0)
          posOffsets[i] = 0;
        else
          posOffsets[i] = totalNumPts;

        totalNumPts += res.Streams.GetNumberOfValues();
        totalNumCells += res.PolyLines.GetNumberOfCells();
      }

      //Append all the points together.
      vtkm::cont::ArrayHandle<vtkm::Vec3f> appendPts;
      appendPts.Allocate(totalNumPts);
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = results[i];
        // copy all values into appendPts starting at offset.
        vtkm::cont::Algorithm::CopySubRange(
          res.Streams, 0, res.Streams.GetNumberOfValues(), appendPts, posOffsets[i]);
      }
      dataset.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", appendPts));

      //Create polylines.
      std::vector<vtkm::Id> numPtsPerCell(static_cast<std::size_t>(totalNumCells));
      std::size_t off = 0;
      for (std::size_t i = 0; i < nResults; i++)
      {
        const auto& res = results[i];
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
      dataset.SetCellSet(polyLines);
    }
    return true;
  }

private:
  vtkm::Id NumParticles;
  vtkm::Id MaxSteps;

  vtkm::cont::ArrayHandle<vtkm::Id> StreamLengths;
  vtkm::cont::ArrayHandle<vtkm::Id> InitialLengths;
  vtkm::cont::ArrayHandle<vtkm::Id> Validity;
};

} // namespace particleadvection
} // namespace worklet
} // namespace vtkm

#endif //vtkm_worklet_particleadvection_analysis
