#include "mpi.h"

//Return the size, in bytes, of the MPI datatype
size_t sizeof_datatype(int datatype) {
	size_t ret = 0;

	if (datatype == MPI_BYTE || datatype == MPI_CHAR || 
	    datatype == MPI_SIGNED_CHAR) {
		ret = sizeof(char);
	} else if (datatype == MPI_SHORT || datatype == MPI_SHORT_INT || 
		   datatype == MPI_UNSIGNED_SHORT ) {
		ret = sizeof(short int);
	} else if (datatype == MPI_INT || datatype == MPI_UNSIGNED) {
		ret = sizeof(int);
	} else if (datatype == MPI_LONG) {
		ret = sizeof(long);
	} else if (datatype == MPI_FLOAT) {
		ret = sizeof(float);
	} else if (datatype == MPI_DOUBLE) {
		ret = sizeof(double);
	} else if (datatype == MPI_PACKED) {
		ret = 1;
	} else {
		ret = sizeof(uint64_t);
	}

	return ret;	
}

int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler) {
	//Currently just a stub

	return MPI_SUCCESS;
}
