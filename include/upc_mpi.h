#ifndef _UPC_MPI_H
#define _UPC_MPI_H 1

//A shared message 
typedef shared struct message_shared message_shared;
struct message_shared {
	int source;
	int dest;
	int tag;
	int data_size;
	shared char *data;
};

//A message in local memory
typedef struct message_local message_local;
struct message_local {
	int source;
	int dest;
	int tag;
	int data_size;
	char *data;
};

//The message array
message_shared *message_list;

int upc_all_mpi_init();
int upc_all_mpi_finalize();
int new_message(void *data, size_t data_size, int source, int dest, int tag);
int delete_message(int off);
message_local *get_message(int source, int dest, int tag);
int find_message(int source, int dest, int tag);

#endif /* _UPC_MPI_H */
