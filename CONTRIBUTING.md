# Contributing to Viskores

We are currently in the process of adopting software from [VTK-m]. During
this transitional period, we have no formal method of accepting
contributions outside of our core contributors. For the time being, new
contributions should be directed at the [VTK-m] project.

When Viskores is ready to accept its own features, the contributing process
will be similar to [that of
VTK-m's](https://gitlab.kitware.com/vtk/vtk-m/-/blob/master/CONTRIBUTING.md).

The author of all contributions must agree to the [Developer Certificate of
Origin] sign-off. Any developer who submits a contribution to Viskores
implicitly agrees to these conditions. That said, contributors remain as
the copyright holder for independent works of authorship and that no
contributor or copyright holder will be required to assign copyrights to
Viskores.



# Governance of Viskores

The above part of this document describes the process of contributing to
Viskores. The remainder of this document describes the governance of the
Viskores project. That is, it describes the roles and responsibilities of
the people who manage Viskores and the process for making decisions about
the project.


## Technical Charter

See the [technical charter] for the official structure of this project.
This document provides a subset of the technical charter for clarity of the
roles and responsibility of members and developers. In the case there is
disagreement between this document and the technical charter, the technical
charter takes precedence.


## Technical Steering Committee

The technical steering committee (TSC) is a collection of individuals
responsible for all technical oversight of Viskores. The TSC comprises
developers and stakeholders of Viskores.

The TSC voting members are listed below along with their GitHub usernames.

  * Kenneth Moreland (@kmorel) -- Chair
  * Mark Bolstad (@renderdude)
  * Hank Childs (@hankchilds)
  * Berk Geveci (@berkgeveci)
  * Li-Ta "Ollie" Lo (@ollielo)
  * David Pugmire (@dpugmire)

A TSC voting member may be added by a majority approval of the existing
members. A TSC member may be removed by self-request, they have remained
inactive for a year or more, or by a majority approval of the other
existing TSC voting members. The TSC may choose an alternative approach for
determining the voting members of the TSC, and any such alternative
approach will be documented in this file. Any meetings of the TSC are
intended to be open to the public, and can be conducted electronically, via
teleconference, or in person.

The TSC may (1) establish work flow procedures for the submission,
approval, and closure/archiving of projects, (2) set requirements for the
promotion of contributors to committer status, as applicable, and (3)
amend, adjust, refine and/or eliminate the roles of contributors, and
committers, and create new roles, and publicly document any TSC roles, as
it sees fit.

The TSC has a TSC Chair, who will preside over meetings of the TSC and will
serve until their resignation or replacement by the TSC. The TSC Chair, or
any other TSC member so designated by the TSC, will serve as the primary
communication contact between the Project and High Performance Software
Foundation, a directed fund of The Linux Foundation.

### Responsibilities

The TSC will be responsible for all aspects of oversight relating to the
Project, which may include:

  * Coordinating the technical direction of the Project.
  * Approving project or system proposals.
  * Organizing sub-projects and removing sub-projects.
  * Creating sub-committees or working groups to focus on cross-project
    technical issues and requirements.
  * Appointing representatives to work with other open source or open
    standards communities.
  * Establishing community norms, workflows, issuing releases, and security
    issue reporting policies.
  * Approving and implementing policies and processes for contributing and
    coordinating with the series manager of the Project to resolve matters
    or concerns that may arise.
  * Discussions, seeking consensus, and where necessary, voting on
    technical matters relating to the code base that affect multiple
    projects.
  * Coordinating any marketing, events, or communications regarding the
    Project.

Individually, each TSC member agrees to perform the following functions:

  * Participate in the responsibilities of the TSC.
  * Regularly attend meetings of the TSC.
  * Participate in on-line TSC voting.
  * Perform review and/or commits of contributions.


### Voting

Although Viskores aims to operate as a consensus-based community, if any
TSC decision requires a vote to move the project forward, the voting
members of the TSC will vote on a one vote per voting member basis.

Quorum for TSC meetings requires at least fifty percent of all voting
members of the TSC to be present. The TSC may continue to meet if quorum is
not met but will be prevented from making any decisions at the meeting.

Unless otherwise stated, decisions by vote at a meeting require a majority
vote of those in attendance, provided quorum is met. Decisions made by
electronic vote without a meeting require a majority vote of all voting
members of the TSC.

Attendees of TSC meetings who are not voting members are encouraged to
discuss voting decisions along with the TSC.


## Committers and Contributors

TSC projects generally involve committers and contributors.

_Contributors_ include anyone in the technical community that contributes
code, documentation, or other technical artifacts to the Project.

_Committers_ are contributors who have earned the ability to modify
("commit") source code, documentation or other technical artifacts in a
project's repository.

A contributor may become a committer by a majority approval of the TSC
voting members. A committer may be removed by self-request, if it has been
at least one year since their last commit, or by a majority approval of the
TSC.

Committers are expected to uphold the contributing guidelines of Viskores.
Furthermore, committers will be expected to review and facilitate in pull
requests where appropriate. Committers are encouraged to attend the regular
Viskores meetings.

It should be noted that anyone can choose to implicitly be a contributor by
agreeing to the [Developer Certificate of Origin] sign-off and submitting a
pull request to the repository (see the top of this document guide).
Contributors do not have or need membership within the GitHub repository.
Any GitHub user may submit a pull request from their own fork.

## Amendments

The technical charter (and by extension this governance) may be amended by
a two-thirds vote of the entire TSC and may be subject to approval by LF
Projects.


[VTK-m]: https://m.vtk.org/
[Developer Certificate of Origin]: http://developercertificate.org
[technical charter]: docs/technical-charter.pdf
