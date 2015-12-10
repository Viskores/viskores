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
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#include <cuda.h>
#include <cuda_runtime.h>
#include <iostream>
#include <string>
#include <map>

int main(int argc, char **argv)
{
  std::map< int, std::string > arch_to_compute;
  arch_to_compute[11] = "compute_11";
  arch_to_compute[12] = "compute_12";
  arch_to_compute[13] = "compute_13";
  arch_to_compute[20] = "compute_20";
  arch_to_compute[21] = "compute_20";
  arch_to_compute[30] = "compute_30";
  arch_to_compute[32] = "compute_32";
  arch_to_compute[35] = "compute_35";
  arch_to_compute[37] = "compute_37";
  arch_to_compute[50] = "compute_50";
  arch_to_compute[52] = "compute_52";
  arch_to_compute[53] = "compute_53";

  int nDevices;
  cudaGetDeviceCount(&nDevices);
  if(nDevices == 0)
  { //return failure if no cuda devices found
    return 1;
  }

  //iterate over the devices outputting a string that would be the compile
  //flags needed to target all gpu's on this machine.
  int prev_arch = 0;
  for (int i = 0; i < nDevices; i++)
  {
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, i);

    //convert 2.1 to 21, 3.5 to 35, etc
    int arch = (prop.major * 10) + prop.minor;

    //if we have multiple gpu's make sure they have different arch's
    //instead of adding the same compile options multiple times
    if(prev_arch == arch)
    {
      continue;
    }
    prev_arch = arch;

    //look up the closest virtual architecture, if the arch we are building
    //for is not found
    if(arch_to_compute.find(arch) != arch_to_compute.end() )
    {
    std::string compute_level = arch_to_compute[arch];
    std::cout << "--generate-code arch=" << compute_level << ",code=sm_"<< arch << " ";
    }
    else
    {
    //if not found default to known highest arch, and compile to a virtual arch
    //instead of a known sm.
    std::map< int, std::string >::const_iterator i = arch_to_compute.end();
    --i;
    std::string compute_level = i->second;
    std::cout << "--generate-code arch=" << compute_level << ",code=" << compute_level << " ";
    }
  }
  return 0;
}