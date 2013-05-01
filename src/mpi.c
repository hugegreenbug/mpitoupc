/* 
   MPI Functions

   Hugh Greenberg <hng@lanl.gov>

*/

#include <upc.h>
#include <upc_collective.h>
#include <sys/time.h>
#include "mpi.h"
#include "upc_mpi.h"

/**
 * Exit the program
 */
int MPI_Abort(MPI_Comm comm, int errorcode) {
	upc_global_exit(errorcode);

	return MPI_SUCCESS;
}

//Call upc_barrier
int MPI_Barrier(MPI_Comm comm) {
	upc_barrier;

	return MPI_SUCCESS;
}

/** 
 *  Broadcasts a message to all threads
 *
 *  The root thread will first send a message to all threads.
 *  Then all threads try to receive the message.
*/
int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype,
	      int root, MPI_Comm comm) {
	int i, err;
	
	err = MPI_SUCCESS;
	for (i = 0; i < THREADS && MYTHREAD == root; i++) {
		err = MPI_Send(buffer, count, datatype, i, i, comm);
		if (err) {
			err = MPI_ERR_BUFFER;
			break;
		}
	}	

	err = MPI_Recv(buffer, count, datatype, root, MYTHREAD, comm, MPI_STATUS_IGNORE);
	if (err) {
		err = MPI_ERR_BUFFER;
	} else {
		err = MPI_SUCCESS;
	}

	return err;
}

/**
 * Initialize the shared memory and the MPI_COMM_WORLD communicator
 */

int MPI_Init(int *argc, char ***argv) {
	int ret;

	//Currently ignoring arguments passed to MPI_Init
	ret = upc_all_mpi_init();
	if (!ret)
		ret = MPI_SUCCESS;
	else
		ret = MPI_ERR_BUFFER;

	//Set the size of MPI_COMM_WORLD
	MPI_COMM_WORLD.size = (int) THREADS;

	return ret;
}

/** 
 * Free all of the shared memory used
 */
int MPI_Finalize(void) {
	upc_all_mpi_finalize();

	return MPI_SUCCESS;
}

/**
 * Pack a message
 */
int MPI_Pack(void *inbuf, int incount, MPI_Datatype datatype,
	     void *outbuf, int outsize, int *position, MPI_Comm comm) {
	size_t size;

	//The output buffer size isn't large enough
	if (outsize - *position < incount)
		return MPI_ERR_BUFFER;

	size = sizeof_datatype(datatype);
	memcpy((char *)outbuf + *position, inbuf, incount * size);
	*position += incount * size;
	
	return MPI_SUCCESS;
}

/** 
 * Unpack a message
 */
int MPI_Unpack(void *inbuf, int insize, int *position,
	       void *outbuf, int outcount, MPI_Datatype datatype,
	       MPI_Comm comm) {
	size_t size;

	//The output buffer size isn't large enough
	if (outcount > insize) 
		return MPI_ERR_BUFFER;

	size = sizeof_datatype(datatype);
	memcpy(outbuf, (char *)inbuf + *position, outcount * size);
	*position += outcount * size;
	
	return MPI_SUCCESS;
}

/**
 * Receive a message
 */
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source,
	     int tag, MPI_Comm comm, MPI_Status *status) {
	message_local *recv_msg = NULL; 
	size_t size;

	while(recv_msg == NULL) {
		recv_msg = get_message(source, MYTHREAD, tag);
		usleep(10);
	}

	if (status != NULL) {
		status->MPI_SOURCE = recv_msg->source;
		status->MPI_TAG = recv_msg->tag;
		status->MPI_ERROR = MPI_SUCCESS;
	}

	size = sizeof_datatype(datatype);
	memcpy(buf, recv_msg->data, size * count);
	free(recv_msg->data);
	free(recv_msg);

	return MPI_SUCCESS;
}

/**
 * Send a message
 */
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest,
	     int tag, MPI_Comm comm) {
	size_t size;
	int ret;
	
	size = count * sizeof_datatype(datatype);
	ret = new_message(buf, size, MYTHREAD, dest, tag);
	if (ret) {
		return MPI_ERR_BUFFER;
	}

	return MPI_SUCCESS;
}

/**
 * Get the caller's thread id
 */
int MPI_Comm_rank(MPI_Comm comm, int *rank) {
	*rank = (int) MYTHREAD;

	return MPI_SUCCESS;
}

/**
 * Get the communicator's size
 */
int MPI_Comm_size(MPI_Comm comm, int *size) {
	*size = comm.size;

	return MPI_SUCCESS;
}

/**
 * Check to see a message is available
 */
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag,
	       MPI_Status *status) {
	int found = 0;
	
	found = find_message(source, MYTHREAD, tag);
	*flag = found;

	if (status != NULL) {
		status->MPI_SOURCE = source;
		status->MPI_TAG = tag;
		status->MPI_ERROR = MPI_SUCCESS;
	}

	return MPI_SUCCESS;
}

/**
 * Return the current time in seconds as a double 
 */
double MPI_Wtime() {
	struct timeval tv;
        double time;
	
        gettimeofday(&tv, NULL);
        time = tv.tv_sec + (tv.tv_usec / 1000000.0);

        return(time);
}

/**
 * Perform a reduce on the send buffer according to the op given
 */
int MPI_Reduce(void *sendbuf, void *recvbuf, int count,
	       MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
	int size;
	shared void *dst, *src;

	size = sizeof_datatype(datatype);
	src = upc_all_alloc(1, count * size * THREADS);
	dst = upc_all_alloc(1, count * size);
	upc_barrier;
	if (datatype == MPI_CHAR) {
		upc_memput(((shared char *)src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceC(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_UNSIGNED_CHAR) {
		upc_memput(((shared unsigned char *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceUC(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);

	} else if (datatype == MPI_SHORT_INT) {
		upc_memput(((shared short int *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceS(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_UNSIGNED_SHORT) {
		upc_memput(((shared unsigned short *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceUS(dst, src, op, count * THREADS,
				 0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_INT) {
		upc_memput(((shared int *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceI(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_UNSIGNED) {
		upc_memput(((shared unsigned int *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceUI(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_LONG) {
		upc_memput(((shared long *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceL(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_UNSIGNED_LONG) {
		upc_memput(((shared unsigned long *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceUL(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_FLOAT) {
		upc_memput(((shared float *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceF(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_DOUBLE) {
		upc_memput(((shared double *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceD(dst, src, op, count * THREADS,
				0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else if (datatype == MPI_LONG_DOUBLE) {
		upc_memput(((shared long double *) src) + (MYTHREAD * count), 
			   sendbuf, count * size);
		upc_barrier;
		upc_all_reduceLD(dst, src, op, count * THREADS,
				 0, NULL, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
	} else {
		return MPI_ERR_ARG;
	}

	upc_memget(recvbuf, dst, count * size);
	if (!MYTHREAD) {
	  upc_free(src);
	  upc_free(dst);
	}

	return MPI_SUCCESS;
}

/**
 * Perform an all gather and return the results in the recvbuf
 */
int MPI_Allgather(void *sendbuf, int  sendcount,
		  MPI_Datatype sendtype, void *recvbuf, int recvcount,
		  MPI_Datatype recvtype, MPI_Comm comm) {
	shared char *src_char;	
	shared unsigned char *src_uchar;
	shared short int *src_sint;
	shared unsigned short *src_usint;
	shared int *src_int;
	shared unsigned int *src_uint;
	shared long *src_long;
	shared unsigned long *src_ulong;
	shared float *src_float;
	shared double *src_double;
	shared long double *src_ldouble;

	shared char *dst_char;	
	shared unsigned char *dst_uchar;
	shared short int *dst_sint;
	shared unsigned short *dst_usint;
	shared int *dst_int;
	shared unsigned int *dst_uint;
	shared long *dst_long;
	shared unsigned long *dst_ulong;
	shared float *dst_float;
	shared double *dst_double;
	shared long double *dst_ldouble;

	shared [] char *row_char;	
	shared [] unsigned char *row_uchar;
	shared [] short int *row_sint;
	shared [] unsigned short *row_usint;
	shared [] int *row_int;
	shared [] unsigned int *row_uint;
	shared [] long *row_long;
	shared [] unsigned long *row_ulong;
	shared [] float *row_float;
	shared [] double *row_double;
	shared [] long double *row_ldouble;


	int size;
	
	if (sendtype != recvtype) {
		return MPI_ERR_ARG;
	}
	
	if (recvcount > sendcount) {
		return MPI_ERR_ARG;
	}

	size = sizeof_datatype(sendtype);
	upc_barrier;
	if (sendtype == MPI_CHAR) {
		src_char = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared char *)src_char) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_char = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_char, src_char, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_char = (shared [] char *)&dst_char[MYTHREAD];
		upc_memget(recvbuf, row_char, recvcount * size);
		upc_free(src_char);
		upc_free(dst_char);
	} else if (sendtype == MPI_UNSIGNED_CHAR) {
		src_uchar = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared unsigned char *)src_uchar) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_uchar = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_uchar, src_uchar, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_uchar = (shared [] unsigned char *)&dst_uchar[MYTHREAD];
		upc_memget(recvbuf, row_uchar, recvcount * size);
		upc_free(src_uchar);
		upc_free(dst_uchar);
	} else if (sendtype == MPI_SHORT_INT) {
		src_sint = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared short int *)src_sint) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_sint = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_sint, src_sint, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_sint = (shared [] short int *)&dst_sint[MYTHREAD];
		upc_memget(recvbuf, row_sint, recvcount * size);
		upc_free(src_sint);
		upc_free(dst_sint);
	} else if (sendtype == MPI_UNSIGNED_SHORT) {
		src_usint = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared unsigned short *)src_usint) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_usint = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_usint, src_usint, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_usint = (shared [] unsigned short *)&dst_usint[MYTHREAD];
		upc_memget(recvbuf, row_usint, recvcount * size);
		upc_free(src_usint);
		upc_free(dst_usint);
	} else if (sendtype == MPI_INT) {
		src_int = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared int *)src_int) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_int = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_int, src_int, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_int = (shared [] int *)&dst_int[MYTHREAD];
		upc_memget(recvbuf, row_int, recvcount * size);
		upc_free(src_int);
		upc_free(dst_int);
	} else if (sendtype == MPI_UNSIGNED) {
		src_uint = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared unsigned int *)src_uint) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_uint = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_uint, src_uint, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_uint = (shared [] unsigned int *)&dst_uint[MYTHREAD];
		upc_memget(recvbuf, row_uint, recvcount * size);
		upc_free(src_uint);
		upc_free(dst_uint);
	} else if (sendtype == MPI_LONG) {
		src_long = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared long *)src_long) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_long = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_long, src_long, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_long = (shared [] long *)&dst_long[MYTHREAD];
		upc_memget(recvbuf, row_long, recvcount * size);
		upc_free(src_long);
		upc_free(dst_long);
	} else if (sendtype == MPI_UNSIGNED_LONG) {
		src_ulong = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared unsigned long *)src_ulong) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_ulong = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_ulong, src_ulong, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_ulong = (shared [] unsigned long *)&dst_ulong[MYTHREAD];
		upc_memget(recvbuf, row_ulong, recvcount * size);
		upc_free(src_ulong);
		upc_free(dst_ulong);
	} else if (sendtype == MPI_FLOAT) {
		src_float = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared float *)src_float) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_float = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_float, src_float, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_float = (shared [] float *)&dst_float[MYTHREAD];
		upc_memget(recvbuf, row_float, recvcount * size);
		upc_free(src_float);
		upc_free(dst_float);
	} else if (sendtype == MPI_DOUBLE) {
		src_double = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared double *)src_double) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_double = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_double, src_double, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_double = (shared [] double *)&dst_double[MYTHREAD];
		upc_memget(recvbuf, row_double, recvcount * size);
		upc_free(src_double);
		upc_free(dst_double);
	} else if (sendtype == MPI_LONG_DOUBLE) {
		src_ldouble = upc_all_alloc(sendcount, THREADS * sendcount * size);
		upc_memput(((shared long double *)src_ldouble) + (sendcount), 
			   sendbuf, sendcount * size);
		dst_ldouble = upc_all_alloc(THREADS * THREADS, recvcount * size);
		upc_barrier;
		upc_all_gather_all(dst_ldouble, src_ldouble, recvcount * size, UPC_IN_NOSYNC | UPC_OUT_NOSYNC);
		row_ldouble = (shared [] long double *)&dst_ldouble[MYTHREAD];
		upc_memget(recvbuf, row_ldouble, recvcount * size);
		upc_free(src_ldouble);
		upc_free(dst_ldouble);
	} else {
		return MPI_ERR_ARG;
	}

	return MPI_SUCCESS;
}
