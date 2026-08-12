# Upstream References

This project maintains upstream source trees exclusively as immutable
technical and historical references.

No project development is performed inside `upstream/`.

## Quake III Arena

Repository:

https://github.com/id-Software/Quake-III-Arena

Reference revision:

dbe4ddb10315479fc00086f08e25d968b4b43c49

Purpose:

- Original id Software GPL source release.
- Historical reference for id Tech 3 architecture.
- Reference for BSP, renderer, collision, game VM, networking and platform architecture.

## ioquake3

Repository:

https://github.com/ioquake/ioq3

Reference revision:

588393618dbc82e7207c21c6ddecca229944a03a

Purpose:

- Modern evolution of the Quake III Arena source.
- Reference for modern Linux support.
- SDL integration.
- OpenAL integration.
- Modern compiler and platform compatibility.
- CMake-based build system.

## Project Rule

The contents of `upstream/` are considered read-only.

Any code adopted or adapted into the project must be incorporated outside
the upstream trees and its origin and license must be documented.
