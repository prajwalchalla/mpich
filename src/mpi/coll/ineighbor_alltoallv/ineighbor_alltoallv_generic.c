/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2012 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "mpiimpl.h"

#undef FUNCNAME
#define FUNCNAME MPIR_Ineighbor_alltoallv_generic_sched
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
int MPIR_Ineighbor_alltoallv_generic_sched(const void *sendbuf, const int sendcounts[], const int sdispls[], MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int rdispls[], MPI_Datatype recvtype, MPIR_Comm *comm_ptr, MPIR_Sched_t s)
{
    int mpi_errno = MPI_SUCCESS;
    int indegree, outdegree, weighted;
    int i, k,l;
    int *srcs, *dsts;
    int comm_size;
    MPI_Aint sendtype_extent, recvtype_extent;
    MPIR_CHKLMEM_DECL(2);

    comm_size = comm_ptr->local_size;

    MPIR_Datatype_get_extent_macro(sendtype, sendtype_extent);
    MPIR_Datatype_get_extent_macro(recvtype, recvtype_extent);

    for (i = 0; i < comm_size; ++i) {
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT sendbuf +
                                         (sdispls[i] * sendtype_extent));
        MPIR_Ensure_Aint_fits_in_pointer(MPIR_VOID_PTR_CAST_TO_MPI_AINT recvbuf +
                                         (rdispls[i] * recvtype_extent));
    }

    mpi_errno = MPIR_Topo_canon_nhb_count(comm_ptr, &indegree, &outdegree, &weighted);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    MPIR_CHKLMEM_MALLOC(srcs, int *, indegree*sizeof(int), mpi_errno, "srcs", MPL_MEM_COMM);
    MPIR_CHKLMEM_MALLOC(dsts, int *, outdegree*sizeof(int), mpi_errno, "dsts", MPL_MEM_COMM);
    mpi_errno = MPIR_Topo_canon_nhb(comm_ptr,
                                    indegree, srcs, MPI_UNWEIGHTED,
                                    outdegree, dsts, MPI_UNWEIGHTED);
    if (mpi_errno) MPIR_ERR_POP(mpi_errno);

    for (k = 0; k < outdegree; ++k) {
        char *sb = ((char *)sendbuf) + sdispls[k] * sendtype_extent;
        mpi_errno = MPIR_Sched_send(sb, sendcounts[k], sendtype, dsts[k], comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    for (l = 0; l < indegree; ++l) {
        char *rb = ((char *)recvbuf) + rdispls[l] * recvtype_extent;
        mpi_errno = MPIR_Sched_recv(rb, recvcounts[l], recvtype, srcs[l], comm_ptr, s);
        if (mpi_errno) MPIR_ERR_POP(mpi_errno);
    }

    MPIR_SCHED_BARRIER(s);

fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
fn_fail:
    goto fn_exit;
}
