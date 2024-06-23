#include "fault_manager.h"
#include "settings.h"

void recover_on_first_fault(MPI_Comm communicator,                                                 //
                            enum RecvFaultToleranceStrategy ft_recv_strategy, double static_value, //
                            double *send_buffer, double *recv_buffer, int buffer_len) {
  switch (ft_recv_strategy) {
    case RecvFaultToleranceStrategy::STATIC: {
      for (int ii = 0; ii < buffer_len; ++ii) {
        recv_buffer[ii] = static_value;
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

void recover_on_fault(MPI_Comm communicator, int rank, int neighbour_rank, int rc,                                        //
                      enum RecvFaultToleranceStrategy ft_recv_strategy, double static_value, double interpolation_factor, //
                      double *send_buffer, double *recv_buffer, int buffer_len) {
  if (rc == MPIX_ERR_PROC_FAILED) {
    recover_on_first_fault(communicator, ft_recv_strategy, static_value, send_buffer, recv_buffer, buffer_len);
  }
  if (ft_recv_strategy == RecvFaultToleranceStrategy::INTERPOLATION) {
    // Fetch our and neighbour's coordinates
    int send_coords[2], recv_coords[2];
    MPI_Cart_coords(communicator, rank, 2, send_coords);
    MPI_Cart_coords(communicator, neighbour_rank, 2, recv_coords);

    // Calc the number of dead neighbours
    int dead_neighbours = 0;
    for (int dd = 0; dd < 2; ++dd) {
      int delta_coords = send_coords[dd] - recv_coords[dd];
      if (delta_coords < 0) delta_coords *= -1;
      if (dead_neighbours < delta_coords - 1) {
        dead_neighbours = delta_coords - 1;
      }
    }
    if (!dead_neighbours) return;

    // Interpolate halo values
    for (int ii = 0; ii < buffer_len; ++ii) {
      recv_buffer[ii] = send_buffer[ii] + (recv_buffer[ii] - send_buffer[ii]) * interpolation_factor / dead_neighbours;
    }
  }
}
