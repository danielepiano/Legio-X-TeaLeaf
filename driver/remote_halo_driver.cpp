#include "chunk.h"
#include "comms.h"
#include "drivers.h"
#include "kernel_interface.h"

#include <iostream>

// Attempts to pack buffers
int invoke_pack_or_unpack(Chunk *chunk, Settings &settings, int face, int depth, int offset, bool pack, FieldBufferType buffer) {
  int buffer_len = 0;

  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (!settings.fields_to_exchange[ii]) {
      continue;
    }

    FieldBufferType field;
    switch (ii) {
      case FIELD_DENSITY: field = chunk->density; break;
      case FIELD_ENERGY0: field = chunk->energy0; break;
      case FIELD_ENERGY1: field = chunk->energy; break;
      case FIELD_U: field = chunk->u; break;
      case FIELD_P: field = chunk->p; break;
      case FIELD_SD: field = chunk->sd; break;
      default: die(__LINE__, __FILE__, "Incorrect field provided: %d.\n", ii + 1);
    }

    //    double *offset_buffer = buffer + buffer_len;
    //    buffer_len += depth * offset;
    //
    //    if (settings.kernel_language == Kernel_Language::C) {
    //      run_pack_or_unpack(chunk, settings, depth, face, pack, field, offset_buffer);
    //    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
    //    }

    if (settings.kernel_language == Kernel_Language::C) {
      run_pack_or_unpack(chunk, settings, depth, face, pack, field, buffer, buffer_len);
    } else if (settings.kernel_language == Kernel_Language::FORTRAN) {
    }
    buffer_len += depth * offset;
  }

  return buffer_len;
}

// Invokes the kernels that perform remote halo exchanges
void remote_halo_driver(Chunk *chunks, Settings &settings, int depth) {
#ifndef NO_MPI
  int neighbours_rank[NUM_DIRECTION_NEIGHBOURS];

  // Pack lr buffers and send messages
  get_cart_neighbours_rank(X_AXIS, 1, neighbours_rank);
  if (neighbours_rank[LEFT] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_LEFT, depth, chunks[0].y, true, chunks[0].left_send);
    run_send_recv_halo(&chunks[0], settings,                                     //
                       chunks[0].left_send, chunks[0].left_recv,                 //
                       chunks[0].staging_left_send, chunks[0].staging_left_recv, //
                       buffer_len, neighbours_rank[LEFT], 0, 1);
  }
  if (neighbours_rank[RIGHT] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_RIGHT, depth, chunks[0].y, true, chunks[0].right_send);
    run_send_recv_halo(&chunks[0], settings,                                       //
                       chunks[0].right_send, chunks[0].right_recv,                 //
                       chunks[0].staging_right_send, chunks[0].staging_right_recv, //
                       buffer_len, neighbours_rank[RIGHT], 1, 0);
  }

  int buffer_len = 0;
  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (!settings.fields_to_exchange[ii]) continue;
    buffer_len += depth * chunks[0].y;
  }
  // actually, forked version of TeaLeaf does not allow more than 1 chunk per rank !
  if (neighbours_rank[LEFT] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].left_recv, chunks[0].staging_left_recv, buffer_len);
  }
  if (neighbours_rank[RIGHT] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].right_recv, chunks[0].staging_right_recv, buffer_len);
  }

  // Unpack lr buffers
  // actually, forked version of TeaLeaf does not allow more than 1 chunk per rank !
  if (neighbours_rank[LEFT] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_LEFT, depth, chunks[0].y, false, chunks[0].left_recv);
  }
  if (neighbours_rank[RIGHT] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_RIGHT, depth, chunks[0].y, false, chunks[0].right_recv);
  }


  // Pack tb buffers and send messages
  get_cart_neighbours_rank(Y_AXIS, 1, neighbours_rank);
  if (neighbours_rank[DOWN] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_BOTTOM, depth, chunks[0].x, true, chunks[0].bottom_send);
    run_send_recv_halo(&chunks[0], settings,                                         //
                       chunks[0].bottom_send, chunks[0].bottom_recv,                 //
                       chunks[0].staging_bottom_send, chunks[0].staging_bottom_recv, //
                       buffer_len, neighbours_rank[DOWN], 0, 1);
  }
  if (neighbours_rank[UP] != MPI_PROC_NULL) {
    int buffer_len = invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_TOP, depth, chunks[0].x, true, chunks[0].top_send);
    run_send_recv_halo(&chunks[0], settings,                                   //
                       chunks[0].top_send, chunks[0].top_recv,                 //
                       chunks[0].staging_top_send, chunks[0].staging_top_recv, //
                       buffer_len, neighbours_rank[UP], 1, 0);
  }

  buffer_len = 0;
  for (int ii = 0; ii < NUM_FIELDS; ++ii) {
    if (!settings.fields_to_exchange[ii]) continue;
    buffer_len += depth * chunks[0].x;
  }
  if (neighbours_rank[DOWN] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].bottom_recv, chunks[0].staging_bottom_recv, buffer_len);
  }
  if (neighbours_rank[UP] != MPI_PROC_NULL) {
    run_restore_recv_halo(&chunks[0], settings, chunks[0].top_recv, chunks[0].staging_top_recv, buffer_len);
  }

  // Unpack tb buffers
  if (neighbours_rank[DOWN] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_BOTTOM, depth, chunks[0].x, false, chunks[0].bottom_recv);
  }
  if (neighbours_rank[UP] != MPI_PROC_NULL) {
    invoke_pack_or_unpack(&(chunks[0]), settings, CHUNK_TOP, depth, chunks[0].x, false, chunks[0].top_recv);
  }

#endif
}
