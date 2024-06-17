#pragma once

#ifndef NO_MPI
  // XXX OpenMPI pulls in CXX headers which we don't link against, prevent that:
  #define OMPI_SKIP_MPICXX
  #include <mpi.h>
  #if __has_include("mpi-ext.h") // C23, but everyone supports this already
    #include "mpi-ext.h"         // for CUDA-aware MPI checks
  #endif
#else
  #include "mpi_shim.h"
#endif

enum class RecvFaultToleranceStrategy { STATIC, MIRROR, BRIDGE, INTERPOLATION };

void recover(MPI_Comm communicator, RecvFaultToleranceStrategy recv_ft_strategy, double *send_buffer, double *recv_buffer, int buffer_len);