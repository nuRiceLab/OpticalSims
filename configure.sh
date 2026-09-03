#!/bin/bash
# NOTE: no `set -u` around the Spack sourcing. The CVMFS setup scripts read
# unset vars ($USER at setup-env.sh:367) and can return non-zero benignly.

: "${USER:=$(id -un)}"; export USER
: "${LOGNAME:=$USER}"; export LOGNAME

set +eu
source /opt/opticks-dev-env.sh
set -e

VDT_PREFIX=$(ls -d /cvmfs/dune.opensciencegrid.org/spack/v1.2.2/opt/spack/linux-x86_64_v3/vdt-* 2>/dev/null | head -1)
[ -n "$VDT_PREFIX" ] || VDT_PREFIX=$(ls -d /cvmfs/dune.opensciencegrid.org/spack/v1.1.1/opt/spack/linux-x86_64_v3/vdt-* 2>/dev/null | head -1)
[ -n "$VDT_PREFIX" ] || { echo "FATAL: no vdt-* under either spack tree"; exit 1; }

export VDT_INCLUDE_DIR=$VDT_PREFIX/include
export VDT_LIBRARY=$(ls "$VDT_PREFIX"/lib*/libvdt.so* 2>/dev/null | head -1)
test -f "$VDT_INCLUDE_DIR/vdt/vdtMath.h" || { echo "FATAL: no vdt headers at $VDT_INCLUDE_DIR"; exit 1; }
test -f "$VDT_LIBRARY" || { echo "FATAL: no libvdt.so under $VDT_PREFIX"; exit 1; }
echo "VDT: $VDT_PREFIX"

rm -rf build && mkdir build && cd build
cmake \
  -DWith_Opticks=true \
  -DWITH_GEANT4_VIS=false \
  -DGeant4_DIR="$Geant4_DIR" \
  -DROOT_DIR="$ROOT_DIR" \
  -DBCM_DIR="$BCM_DIR" \
  -DCUDAToolkit_ROOT=/usr/local/cuda \
  -DOptiX_INSTALL_DIR=/opt/optix/current \
  -DVDT_INCLUDE_DIR="$VDT_INCLUDE_DIR" \
  -DVDT_LIBRARY="$VDT_LIBRARY" \
  -DCMAKE_EXE_LINKER_FLAGS="-lQt5Core" \
  -DCMAKE_INSTALL_PREFIX="$HOME/opticalsims" \
  ..
