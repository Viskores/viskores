## Workletize flow particle advection

Flow particle advection now keeps more scheduling and classification work in
worklets and `ArrayHandle`s. This reduces host-side particle movement, improves
multi-block routing, and re-enables the threaded flow scheduler.
