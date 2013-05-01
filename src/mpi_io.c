#include "mpi.h"
#include "plfs.h"

/**
 * Opens a file
 */
int MPI_File_open(MPI_Comm comm, char *filename,
		  int amode, MPI_Info info, MPI_File *fh) {
	upcio_file_t *fd;

	if (!fh) {
		return MPI_ERR_ARG;
	}

	fd = upc_all_fopen(filename, amode, 0644);
	upc_barrier;
	
	if (!fd) {
		return MPI_ERR_UNKNOWN;
	}

	*fh = malloc(sizeof(struct MPI_File));
	(*fh)->fd = fd;
	(*fh)->info = info;

	return MPI_SUCCESS;
}

/**
 *  Writes to the given file
 *  Not non-blocking as there isn't a good async implementation yet 
 */

int MPI_File_iwrite(MPI_File fh, void *buf, int count,
		    MPI_Datatype datatype, MPI_Request *request) {
	ssize_t size, data_size;
	int ret, error_code;
	struct __struct_thread_upc_file_t *fhp;
	Plfs_fd *fd;
	
	data_size = sizeof_datatype(datatype);
	size = count * data_size;

	fhp = (struct __struct_thread_upc_file_t *)fh->fd->th[MYTHREAD];
	fd = (Plfs_fd *)fhp->adio_fd;

	if( fhp->flags & UPC_RDONLY )
		return MPI_ERR_OTHER;

	if( fhp->async_flag == 1 )
		return MPI_ERR_OTHER;

	ret = UPC_ADIO_WriteContig( fd, buf, size, fhp->private_pointer, 
				    &error_code );

	if (error_code == UPC_ADIO_FAILURE) {
		ret = MPI_ERR_OTHER;
	} else {
		ret = MPI_SUCCESS;
	}

	return ret;
}

/**
 * Reads from the given file
 * Not non-blocking as there isn't a good async implementation yet 
 */

int MPI_File_iread(MPI_File fh, void  *buf, int count,
		   MPI_Datatype  datatype, MPI_Request *request) {
	ssize_t size, data_size;
	int ret, error_code;
	struct __struct_thread_upc_file_t *fhp;
	Plfs_fd *fd;
	
	data_size = sizeof_datatype(datatype);
	size = count * data_size;

	fhp = (struct __struct_thread_upc_file_t *)fh->fd->th[MYTHREAD];
	fd = (Plfs_fd *)fhp->adio_fd;

	if( fhp->flags & UPC_RDONLY )
		return MPI_ERR_OTHER;

	if( fhp->async_flag == 1 )
		return MPI_ERR_OTHER;

	ret = UPC_ADIO_ReadContig( fd, buf, size, fhp->private_pointer, 
				   &error_code );

	request->done = 1;
	if (error_code == UPC_ADIO_FAILURE) {
		ret = MPI_ERR_OTHER;
	} else {
		ret = MPI_SUCCESS;
	}

	return ret;
}


/**
 * Reads from the given file
 */

int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset,
			 void *buf, int count, MPI_Datatype datatype,
			 MPI_Status *status) {
	ssize_t size, data_size;
	int ret;

	data_size = sizeof_datatype(datatype);
	size = count * data_size;
	data_size = upc_all_fread_local(fh->fd, buf, size, 1, 0);
	if (data_size < 0)
		ret = MPI_ERR_OTHER;
	else
		ret = MPI_SUCCESS;

	status->MPI_ERROR = ret;

	return ret;
}

/**
 * Reads from the given file
 */
int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, void *buf,			  
			  int count, MPI_Datatype datatype, MPI_Status *status) {
	ssize_t size, data_size;
	int ret;
	
	data_size = sizeof_datatype(datatype);
	size = count * data_size;
	data_size = upc_all_fwrite_local(fh->fd, buf, size, 1, 0);
	if (data_size < 0)
		ret = MPI_ERR_OTHER;
	else
		ret = MPI_SUCCESS;
	
	status->MPI_ERROR = ret;

	return ret;
}

/**
 * Seeks within the given file
 */
int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence) {
	upc_off_t ret;

	ret = upc_all_fseek(fh->fd, offset, whence);
	if (ret < 0)
		return MPI_ERR_OTHER;

	return MPI_SUCCESS;
}

/**
 * Sets the file size
 */
int MPI_File_set_size(MPI_File fh, MPI_Offset size) {
	int ret;

	ret = upc_all_fset_size(fh->fd, size);
	if (ret < 0)
		return MPI_ERR_OTHER;

	return MPI_SUCCESS;
}

/**
 * Closes the given file
 */
int MPI_File_close(MPI_File *fh) {
	int ret = MPI_SUCCESS;

	if (!fh || !*fh)
		return MPI_ERR_ARG;

	upc_barrier;
	if(upc_all_fclose((*fh)->fd) != 0) {
		ret = MPI_ERR_OTHER;
	}

	if (fh && *fh) {
		free(*fh);
	}

	return ret;
}

/**
 * Deletes the given file
 */
int MPI_File_delete(char *filename, MPI_Info info) {
	int ret;

	ret = unlink(filename);
	if (ret < 0)
		return MPI_ERR_OTHER;

	return MPI_SUCCESS;
}

/**
 * Returns the MPI_Info associated with the file
 */
int MPI_File_get_info(MPI_File fh, MPI_Info *info_used) {
	if(!info_used)
		return MPI_ERR_ARG;

	*info_used = fh->info;

	return MPI_SUCCESS;
}

/**
 * Flushes the file to disk
 */
int MPI_File_sync(MPI_File fh) {
	int ret;

	ret = upc_all_fsync(fh->fd);
	if (ret < 0)
		return MPI_ERR_OTHER;

	return MPI_SUCCESS;
}

/**
 * Returns the file size of the file
 */
int MPI_File_get_size(MPI_File fh, MPI_Offset *size) {
	*size =  upc_all_fget_size(fh->fd);

	if (*size < 0)
		return MPI_ERR_OTHER;

	return MPI_SUCCESS;
}

/**
 * Waits for all asynchronous operations to complete
 */
int MPIO_Wait(MPIO_Request *request, MPI_Status *status) {
	status->MPI_ERROR = 0;
	while (!request->done) {
		usleep(100);
	}

	return MPI_SUCCESS;
}
