//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>

#include <vtkm/VectorAnalysis.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/source/PerlinNoise.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <time.h>

namespace
{

struct PerlinNoiseWorklet : public vtkm::worklet::WorkletVisitPointsWithCells
{
  using ControlSignature = void(CellSetIn, FieldInPoint, WholeArrayIn, FieldOut noise);
  using ExecutionSignature = void(_2, _3, _4);

  VTKM_CONT PerlinNoiseWorklet(vtkm::Id repeat)
    : Repeat(repeat)
  {
  }

  // Adapted from https://adrianb.io/2014/08/09/perlinnoise.html
  // Archive link: https://web.archive.org/web/20210329174559/https://adrianb.io/2014/08/09/perlinnoise.html
  template <typename PointVecType, typename PermsPortal, typename OutType>
  VTKM_EXEC void operator()(const PointVecType& pos, const PermsPortal& perms, OutType& noise) const
  {
    vtkm::Id xi = static_cast<vtkm::Id>(pos[0]) % this->Repeat;
    vtkm::Id yi = static_cast<vtkm::Id>(pos[1]) % this->Repeat;
    vtkm::Id zi = static_cast<vtkm::Id>(pos[2]) % this->Repeat;
    vtkm::FloatDefault xf = static_cast<vtkm::FloatDefault>(pos[0] - xi);
    vtkm::FloatDefault yf = static_cast<vtkm::FloatDefault>(pos[1] - yi);
    vtkm::FloatDefault zf = static_cast<vtkm::FloatDefault>(pos[2] - zi);
    vtkm::FloatDefault u = this->Fade(xf);
    vtkm::FloatDefault v = this->Fade(yf);
    vtkm::FloatDefault w = this->Fade(zf);

    vtkm::Id aaa, aba, aab, abb, baa, bba, bab, bbb;
    aaa = perms.Get(perms.Get(perms.Get(xi) + yi) + zi);
    aba = perms.Get(perms.Get(perms.Get(xi) + this->Increment(yi)) + zi);
    aab = perms.Get(perms.Get(perms.Get(xi) + yi) + this->Increment(zi));
    abb = perms.Get(perms.Get(perms.Get(xi) + this->Increment(yi)) + this->Increment(zi));
    baa = perms.Get(perms.Get(perms.Get(this->Increment(xi)) + yi) + zi);
    bba = perms.Get(perms.Get(perms.Get(this->Increment(xi)) + this->Increment(yi)) + zi);
    bab = perms.Get(perms.Get(perms.Get(this->Increment(xi)) + yi) + this->Increment(zi));
    bbb = perms.Get(perms.Get(perms.Get(this->Increment(xi)) + this->Increment(yi)) +
                    this->Increment(zi));

    vtkm::FloatDefault x1, x2, y1, y2;
    x1 = vtkm::Lerp(this->Gradient(aaa, xf, yf, zf), this->Gradient(baa, xf - 1, yf, zf), u);
    x2 =
      vtkm::Lerp(this->Gradient(aba, xf, yf - 1, zf), this->Gradient(bba, xf - 1, yf - 1, zf), u);
    y1 = vtkm::Lerp(x1, x2, v);

    x1 =
      vtkm::Lerp(this->Gradient(aab, xf, yf, zf - 1), this->Gradient(bab, xf - 1, yf, zf - 1), u);
    x2 = vtkm::Lerp(
      this->Gradient(abb, xf, yf - 1, zf - 1), this->Gradient(bbb, xf - 1, yf - 1, zf - 1), u);
    y2 = vtkm::Lerp(x1, x2, v);

    noise = (vtkm::Lerp(y1, y2, w) + OutType(1.0f)) * OutType(0.5f);
  }

  VTKM_EXEC vtkm::FloatDefault Fade(vtkm::FloatDefault t) const
  {
    return t * t * t * (t * (t * 6 - 15) + 10);
  }

  VTKM_EXEC vtkm::Id Increment(vtkm::Id n) const { return (n + 1) % this->Repeat; }

  VTKM_EXEC vtkm::FloatDefault Gradient(vtkm::Id hash,
                                        vtkm::FloatDefault x,
                                        vtkm::FloatDefault y,
                                        vtkm::FloatDefault z) const
  {
    switch (hash & 0xF)
    {
      case 0x0:
        return x + y;
      case 0x1:
        return -x + y;
      case 0x2:
        return x - y;
      case 0x3:
        return -x - y;
      case 0x4:
        return x + z;
      case 0x5:
        return -x + z;
      case 0x6:
        return x - z;
      case 0x7:
        return -x - z;
      case 0x8:
        return y + z;
      case 0x9:
        return -y + z;
      case 0xA:
        return y - z;
      case 0xB:
        return -y - z;
      case 0xC:
        return y + x;
      case 0xD:
        return -y + z;
      case 0xE:
        return y - x;
      case 0xF:
        return -y - z;
      default:
        return 0; // never happens
    }
  }

  vtkm::Id Repeat;
};

class PerlinNoiseField : public vtkm::filter::Filter
{
public:
  VTKM_CONT PerlinNoiseField(vtkm::IdComponent tableSize, vtkm::IdComponent seed)
    : TableSize(tableSize)
    , Seed(seed)
  {
    this->GeneratePermutations();
    this->SetUseCoordinateSystemAsField(true);
  }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> noise;
    PerlinNoiseWorklet worklet{ this->TableSize };
    this->Invoke(
      worklet, input.GetCellSet(), input.GetCoordinateSystem(), this->Permutations, noise);
    return this->CreateResultFieldPoint(input, this->GetOutputFieldName(), noise);
  }

  VTKM_CONT void GeneratePermutations()
  {
    std::mt19937_64 rng;
    rng.seed(this->Seed);
    std::uniform_int_distribution<vtkm::IdComponent> distribution(0, this->TableSize - 1);

    vtkm::cont::ArrayHandle<vtkm::Id> perms;
    perms.Allocate(this->TableSize);
    auto permsPortal = perms.WritePortal();
    for (auto i = 0; i < permsPortal.GetNumberOfValues(); ++i)
    {
      permsPortal.Set(i, distribution(rng));
    }
    this->Permutations.Allocate(2 * this->TableSize);
    auto permutations = this->Permutations.WritePortal();
    for (auto i = 0; i < permutations.GetNumberOfValues(); ++i)
    {
      permutations.Set(i, permsPortal.Get(i % this->TableSize));
    }
  }

  vtkm::IdComponent TableSize;
  vtkm::IdComponent Seed;
  vtkm::cont::ArrayHandle<vtkm::Id> Permutations;
};

} // anonymous namespace

namespace vtkm
{
namespace source
{

PerlinNoise::PerlinNoise(vtkm::Id3 dims)
  : PerlinNoise()
{
  this->SetCellDimensions(dims);
}

PerlinNoise::PerlinNoise(vtkm::Id3 dims, vtkm::IdComponent seed)
  : PerlinNoise()
{
  this->SetCellDimensions(dims);
  this->SetSeed(seed);
}

PerlinNoise::PerlinNoise(vtkm::Id3 dims, vtkm::Vec3f origin)
  : PerlinNoise()
{
  this->SetCellDimensions(dims);
  this->SetOrigin(origin);
}

PerlinNoise::PerlinNoise(vtkm::Id3 dims, vtkm::Vec3f origin, vtkm::IdComponent seed)
{
  this->SetCellDimensions(dims);
  this->SetOrigin(origin);
  this->SetSeed(seed);
}

vtkm::cont::DataSet PerlinNoise::DoExecute() const
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::DataSet dataSet;
  const vtkm::Vec3f cellDims = this->GetCellDimensions();
  const vtkm::Vec3f spacing(1.0f / cellDims[0], 1.0f / cellDims[1], 1.0f / cellDims[2]);


  vtkm::cont::CellSetStructured<3> cellSet;
  cellSet.SetPointDimensions(this->PointDimensions);
  dataSet.SetCellSet(cellSet);
  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(
    this->PointDimensions, this->Origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  auto tableSize =
    static_cast<vtkm::IdComponent>(vtkm::Max(cellDims[0], vtkm::Max(cellDims[1], cellDims[2])));

  vtkm::IdComponent seed = this->Seed;
  if (!this->SeedSet)
  {
    // If a seed has not been chosen, create a unique seed here. It is done here instead
    // of the `PerlinNoise` source constructor for 2 reasons. First, `std::random_device`
    // can be slow. If the user wants to specify a seed, it makes no sense to spend
    // time generating a random seed only to overwrite it. Second, creating the seed
    // here allows subsequent runs of the `PerlinNoise` source to have different random
    // results if a seed is not specified.
    //
    // It is also worth noting that the current time is added to the random number.
    // This is because the spec for std::random_device allows it to be deterministic
    // if nondeterministic hardware is unavailable and the deterministic numbers can
    // be the same for every execution of the program. Adding the current time is
    // a fallback for that case.
    seed = static_cast<vtkm::IdComponent>(std::random_device{}() + time(NULL));
  }

  PerlinNoiseField noiseGenerator(tableSize, seed);
  noiseGenerator.SetOutputFieldName("perlinnoise");
  dataSet = noiseGenerator.Execute(dataSet);

  return dataSet;
}

} // namespace source
} // namespace vtkm
