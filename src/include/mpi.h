#ifndef __MPI_H
#define __MPI_H

#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <upc_collective.h>

/*
 * Miscellaneous constants
 */
#define MPI_ANY_SOURCE         -1                      /* match any source rank */
#define MPI_ANY_TAG            -1                      /* match any message tag */
#define MPI_WTIME_IS_GLOBAL     0

/* Error codes */
#define MPI_SUCCESS                   0
#define MPI_ERR_BUFFER                1
#define MPI_ERR_COUNT                 2
#define MPI_ERR_TYPE                  3
#define MPI_ERR_TAG                   4
#define MPI_ERR_COMM                  5
#define MPI_ERR_RANK                  6
#define MPI_ERR_REQUEST               7
#define MPI_ERR_ROOT                  8
#define MPI_ERR_GROUP                 9
#define MPI_ERR_OP                    10
#define MPI_ERR_TOPOLOGY              11
#define MPI_ERR_DIMS                  12
#define MPI_ERR_ARG                   13
#define MPI_ERR_UNKNOWN               14
#define MPI_ERR_TRUNCATE              15
#define MPI_ERR_OTHER                 16
#define MPI_ERR_INTERN                17
#define MPI_ERR_LASTCODE              18

/* C datatypes */
#define MPI_BYTE                      0
#define MPI_PACKED                    1
#define MPI_CHAR                      2
#define MPI_SHORT                     3
#define MPI_INT                       4
#define MPI_LONG                      5
#define MPI_FLOAT                     6
#define MPI_DOUBLE                    7
#define MPI_LONG_DOUBLE               8
#define MPI_UNSIGNED_CHAR             9
#define MPI_SIGNED_CHAR               10
#define MPI_UNSIGNED_SHORT            11
#define MPI_UNSIGNED_LONG             12
#define MPI_UNSIGNED                  13
#define MPI_LONG_INT                  14
#define MPI_SHORT_INT                 15
#define MPI_LONG_LONG_INT             16
#define MPI_LONG_LONG                 17
#define MPI_UNSIGNED_LONG_LONG        18
#define MPI_DOUBLE_INT                19

/* MPI_Reduce constants */
#define MPI_MAX                       UPC_MAX
#define MPI_MIN                       UPC_MIN
#define MPI_SUM                       UPC_ADD

#define MPI_ERRORS_RETURN             1

typedef int MPI_Errhandler;
typedef int MPI_Datatype;
typedef int MPI_Op;

/*
 * MPI_Comm
 */

typedef struct MPI_Comm MPI_Comm;
struct MPI_Comm {
    int size;
};

MPI_Comm MPI_COMM_WORLD;
#define MPI_COMM_SELF MPI_COMM_WORLD

/*
 * MPI_Status
 */
typedef struct MPI_Status MPI_Status;
struct MPI_Status {
    int MPI_SOURCE;
    int MPI_TAG;
    int MPI_ERROR;
};

/*
 * MPI_Request
 */

typedef struct MPI_Request MPI_Request;
struct MPI_Request {
    int done;
};


#include "mpi_info.h"
#include "mpi_io.h"
#include "mpi_utils.h"

#define MPI_STATUS_IGNORE ((MPI_Status *) 0)
#define MPI_MAX_PROCESSOR_NAME 256

/* Functions */
int MPI_Abort(MPI_Comm comm, int errorcode);
int MPI_Barrier(MPI_Comm comm);
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype,
	      int root, MPI_Comm comm);
int MPI_Init(int *argc, char ***argv);
int MPI_Finalize(void);
int MPI_Pack(void *inbuf, int incount, MPI_Datatype datatype,
	      void *outbuf, int outsize, int *position, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source,
	     int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest,
	     int tag, MPI_Comm comm);
int MPI_Unpack(void *inbuf, int insize, int *position,
	       void *outbuf, int outcount, MPI_Datatype datatype,
	       MPI_Comm comm);
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
	       MPI_Status *status);
double MPI_Wtime();
int MPI_Reduce(void *sendbuf, void *recvbuf, int count,
	       MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
int MPI_Allgather(void *sendbuf, int  sendcount,
		  MPI_Datatype sendtype, void *recvbuf, int recvcount,
		  MPI_Datatype recvtype, MPI_Comm comm);

int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);

#endif /* End _MPI_H */ 
