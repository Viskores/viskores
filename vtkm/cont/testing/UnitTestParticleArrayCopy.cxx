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
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ParticleArrayCopy.h>
#include <vtkm/cont/testing/Testing.h>

void TestParticleArrayCopy()
{
  std::random_device device;
  std::default_random_engine generator(static_cast<vtkm::UInt32>(277));
  vtkm::FloatDefault x0(-1), x1(1);
  std::uniform_real_distribution<vtkm::FloatDefault> dist(x0, x1);

  std::vector<vtkm::Particle> particles;
  vtkm::Id N = 17;
  for (vtkm::Id i = 0; i < N; i++)
  {
    auto x = dist(generator);
    auto y = dist(generator);
    auto z = dist(generator);
    particles.push_back(vtkm::Particle(vtkm::Vec3f(x, y, z), i));
  }

  for (vtkm::Id i = 0; i < 2; i++)
  {
    auto particleAH = vtkm::cont::make_ArrayHandle(particles, vtkm::CopyFlag::Off);

    //Test copy position only
    if (i == 0)
    {
      vtkm::cont::ArrayHandle<vtkm::Vec3f> pos;
      vtkm::cont::ParticleArrayCopy<vtkm::Particle>(particleAH, pos);

      auto pPortal = particleAH.ReadPortal();
      for (vtkm::Id j = 0; j < N; j++)
      {
        auto p = pPortal.Get(j);
        auto pt = pos.ReadPortal().Get(j);
        VTKM_TEST_ASSERT(p.GetPosition() == pt, "Positions do not match");
      }
    }
    else //Test copy everything
    {
      vtkm::cont::ArrayHandle<vtkm::Vec3f> pos;
      vtkm::cont::ArrayHandle<vtkm::Id> ids, steps;
      vtkm::cont::ArrayHandle<vtkm::ParticleStatus> status;
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> ptime;

      vtkm::cont::ParticleArrayCopy<vtkm::Particle>(particleAH, pos, ids, steps, status, ptime);

      auto pPortal = particleAH.ReadPortal();
      for (vtkm::Id j = 0; j < N; j++)
      {
        auto p = pPortal.Get(j);
        auto pt = pos.ReadPortal().Get(j);
        VTKM_TEST_ASSERT(p.GetPosition() == pt, "Positions do not match");
        VTKM_TEST_ASSERT(p.GetID() == ids.ReadPortal().Get(j), "IDs do not match");
        VTKM_TEST_ASSERT(p.GetNumberOfSteps() == steps.ReadPortal().Get(j), "Steps do not match");
        VTKM_TEST_ASSERT(p.GetStatus() == status.ReadPortal().Get(j), "Status do not match");
        VTKM_TEST_ASSERT(p.GetTime() == ptime.ReadPortal().Get(j), "Times do not match");
      }
    }
  }

  //Test copying a vector of ArrayHandles.
  std::vector<vtkm::cont::ArrayHandle<vtkm::Particle>> particleVec;
  vtkm::Id totalNumParticles = 0;
  vtkm::Id pid = 0;
  for (vtkm::Id i = 0; i < 4; i++)
  {
    vtkm::Id n = 5 + i;
    std::vector<vtkm::Particle> vec;
    for (vtkm::Id j = 0; j < n; j++)
    {
      auto x = dist(generator);
      auto y = dist(generator);
      auto z = dist(generator);
      vec.push_back(vtkm::Particle(vtkm::Vec3f(x, y, z), pid));
      pid++;
    }
    auto ah = vtkm::cont::make_ArrayHandle(vec, vtkm::CopyFlag::On);
    particleVec.push_back(ah);
    totalNumParticles += ah.GetNumberOfValues();
  }

  vtkm::cont::ArrayHandle<vtkm::Vec3f> res;
  vtkm::cont::ParticleArrayCopy<vtkm::Particle>(particleVec, res);
  VTKM_TEST_ASSERT(res.GetNumberOfValues() == totalNumParticles, "Wrong number of particles");

  vtkm::Id resIdx = 0;
  auto resPortal = res.ReadPortal();
  for (const auto& v : particleVec)
  {
    vtkm::Id n = v.GetNumberOfValues();
    auto portal = v.ReadPortal();
    for (vtkm::Id i = 0; i < n; i++)
    {
      auto p = portal.Get(i);
      auto pRes = resPortal.Get(resIdx);
      VTKM_TEST_ASSERT(p.GetPosition() == pRes, "Positions do not match");
      resIdx++;
    }
  }
}

int UnitTestParticleArrayCopy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleArrayCopy, argc, argv);
}
