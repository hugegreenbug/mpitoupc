#ifndef _MPI_IO_H
#define _MPI_IO_H 1

#include <upc.h>
#include "upc_io.h"

/*
 * MPI_File
 */

#define MPI_MODE_APPEND          UPC_APPEND
#define MPI_MODE_CREATE          UPC_CREATE
#define MPI_MODE_EXCL            UPC_EXCL
#define MPI_MODE_RDONLY          UPC_RDONLY
#define MPI_MODE_RDWR            UPC_RDWR
#define MPI_MODE_WRONLY          UPC_WRONLY

/* Currently not available in pupc-io */
#define MPI_MODE_UNIQUE_OPEN     0

#define MPI_SEEK_SET             UPC_SEEK_SET
#define MPI_SEEK_CUR             UPC_SEEK_CUR
#define MPI_SEEK_END             UPC_SEEK_END

#define MPI_Offset               upc_off_t

typedef MPI_Request MPIO_Request;
typedef struct MPI_File *MPI_File;
struct MPI_File {
        upcio_file_t *fd;
        MPI_Info info;
};

int MPI_File_open(MPI_Comm comm, char *filename,
		  int amode, MPI_Info info, MPI_File *fh);
int MPI_File_iwrite(MPI_File fh, void *buf, int count,
		    MPI_Datatype datatype, MPI_Request *request);
int MPI_File_iread(MPI_File fh, void  *buf, int  count,
		   MPI_Datatype  datatype, MPI_Request  *request);
int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset,
			 void *buf, int count, MPI_Datatype datatype,
			 MPI_Status *status);
int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, void *buf,			  
			  int count, MPI_Datatype datatype, MPI_Status *status);
int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence);
int MPI_File_set_size(MPI_File fh, MPI_Offset size);
int MPI_File_close(MPI_File *fh);
int MPI_File_delete(char *filename, MPI_Info info);
int MPI_File_get_info(MPI_File fh, MPI_Info *info_used);
int MPI_File_sync(MPI_File fh);
int MPI_File_get_size(MPI_File fh, MPI_Offset *size);
int MPIO_Wait(MPIO_Request *request, MPI_Status *status);

#endif /* End _MPI_IO_H */ 
