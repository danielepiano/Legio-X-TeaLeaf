#include "fault_manager.h"
#include "settings.h"

void recover(MPI_Comm communicator,                            //
             enum RecvFaultToleranceStrategy recv_ft_strategy, //
             double *send_buffer, double *recv_buffer, int buffer_len) {
  switch (recv_ft_strategy) {
    case RecvFaultToleranceStrategy::STATIC: {
      for (int ii = 0; ii < buffer_len; ++ii) {
        recv_buffer[ii] = DEF_RECV_FT_STATIC_VALUE;
      }
      break;
    }
    case RecvFaultToleranceStrategy::MIRROR: {
      for (int ii = 0; ii < buffer_len; ++ii) {
        recv_buffer[ii] = send_buffer[ii];
      }
      break;
    }
    case RecvFaultToleranceStrategy::BRIDGE:
    case RecvFaultToleranceStrategy::INTERPOLATION: {
      MPIX_Comm_failure_ack(communicator);
      // Apply MIRROR strategy the first time a fault is detected
      for (int ii = 0; ii < buffer_len; ++ii) {
        recv_buffer[ii] = send_buffer[ii];
      }
      break;
    }
    default: break;
  }
}
