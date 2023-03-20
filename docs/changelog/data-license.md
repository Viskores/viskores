# Clarified license of test data

The VTK-m source comes distributed with several data files used for
regression testing. Some of these are generated specifically by VTK-m
developers and are released as part of the VTK-m license, but some come
from external sources. For those that come from external sources, we have
clarified the license and attribution of those files. In particular, the
following files originate from external sources.

* **internet.egr**: Distributed as part of a graph data set paper. The
  license of this data is compatible with VTK-m's license. The file is
  placed in the third-party data directory and the information has been
  updated to clearly document the correct license for this data.
* **example.vtk** and **example_temp.bov**: Distributed as part of the
  VisIt tutorials. This data is provided under the VisIt license (per Eric
  Brugger), which is compatible with VTK-m's license. The files are moved
  to the third-party data directory and the license and attribution is
  clarified. (These files were previously named "noise" but were changed to
  match the VisIt tutorial files they came from.)
* **vanc.vtk** Data derived from a digital elevation map of Vancouver that
  comes from GTOPO30. This data is in the public domain, so it is valid for
  us to use, modify, and redistribute the data under our license.

The fishtank and fusion/magField datasets were removed. These are standard
flow testing data sets that are commonly distributed. However, we could not
track down the original source and license, so to be cautious these data
sets have been removed and replaced with some generated in house.

For some of the other data sets, we have traced down the original author
and verified that they propery contribute the data to VTK-m and agree to
allow it to be distributed under VTK-m's license. Not counting the most
trivial examples, here are the originators of the non-trivial data
examples.

* **5x6_&_MC*.ctm** and **8x9test_HierarchicalAugmentedTree*.dat**: Hamish
  Carr
* **warpXfields.vtk** and **warpXparticles.vtk**: Axel Huebl
* **amr_wind_flowfield.vtk**: James Kress
* **DoubleGyre*.vtk**: James Kress
* **venn250.vtk**: Abhishek Yenpure
* **wedge_cells.vtk**: Chris Laganella
* **kitchen.vtk**: Copyright owned by Kitware, Inc. (who shares the
  copyright of VTK-m)

