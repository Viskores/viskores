# Viskores 1.0.0 Release Notes

## Table of Contents
1. [Core](#core)
   - Rebranding of the source code to Viskores
   - New DCO.txt
   - Transition to GitHub
2. [Build](#build)
   - UO Frank cluster CI

## Core

### Rebranding of the source code to Viskores
The project has been officially rebranded from VTK-m to Viskores. This
comprehensive rebranding effort involved replacing all occurrences of "vtk-m"
and similar references throughout the codebase with "viskores". This change
aligns with the project's new identity as part of the High Performance Software
Foundation (HPSF) requirements.

All these changes collectively ensure that Viskores now fully conforms to the
High Performance Software Foundation's Developer Certificate of Origin
(DCO.txt) standards, setting the foundation for sustainable, community-driven
development under the HPSF umbrella.

### New DCO.txt
As part of the transition to HPSF standards, the project now implements the
Developer Certificate of Origin (DCO) as documented in the new DCO.txt file.
This conforms to the HPSF requirements for transparent contribution tracking
and ensures all contributions are properly certified regarding their origin and
licensing.

### Transition to GitHub
The project has been migrated from Kitware's GitLab instance to GitHub. This
move enhances project visibility and accessibility within the broader
open-source community and aligns with HPSF's preferred collaborative platform
as an independent and accessible platform for software development.

## Build

### UO Frank cluster CI
The Continuous Integration (CI) system has been migrated to utilize the
University of Oregon's Frank Cluster CI through the GitLab Spack instance. This
setup connects to GitHub using the Spack CI bridge, enabling efficient testing
and validation of the codebase. This infrastructure change supports the
project's integration into the HPSF ecosystem and leverages high-performance
computing resources for testing and validation.
