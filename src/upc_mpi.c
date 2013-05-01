/*
  Functions for manipulating shared memory

  Hugh Greenberg <hng@lanl.gov>
*/

#include <upc.h>
#include "mpi.h"
#include "upc_mpi.h"

//The shared lock
upc_lock_t *message_lock;

//Initialize the shared array and shared lock
int upc_all_mpi_init() {
	int ret = 0;
	int i;

	message_list = upc_all_alloc(1, sizeof(message_shared) * THREADS);
	message_lock = upc_all_lock_alloc();
	upc_memset(message_list, -1, sizeof(message_shared) * THREADS);
	if (!message_lock) {
		ret = 1;
		return ret;
	}

	upc_barrier;
	
	return ret;
}

//Free the shared array and lock
int upc_all_mpi_finalize() {
	int i;

	upc_barrier;
	if (MYTHREAD)
		return 0;

	//Free each message in the list
	if (!MYTHREAD)
		upc_free(message_list);

	message_list = NULL;
	upc_lock_free(message_lock);

	return 0;
}

//Create a new message and add it to the array
int new_message(void *data, size_t data_size, int source, int dest, int tag) {
	message_local *new_msg;
	int found = 1;

	if (dest < 0 || dest >= THREADS) {
		return 1;
	}
	
	while (found) {
		upc_lock(message_lock);
		found = find_message_nolock(MPI_ANY_SOURCE, dest, MPI_ANY_TAG);
		if (found) {
			upc_unlock(message_lock);
			usleep(10);
		}
	}

	new_msg = malloc(sizeof(message_local));
	if (!new_msg) {
		//Error allocating memory for the new message
		upc_unlock(message_lock);
		return 1;
	}

	new_msg->data_size = data_size;
	new_msg->source = source;
	new_msg->tag = tag;
	new_msg->dest = dest;
	upc_memput(&message_list[dest], new_msg, sizeof(message_shared));
	message_list[dest].data = upc_alloc(data_size);
	upc_memput(message_list[dest].data, data, data_size);
	upc_unlock(message_lock);

	return 0;
}

//Delete a message from the shared array
int delete_message(int off) {
	message_local *lmsg;

	int i, ret;
	
	ret = 1;	
	lmsg = malloc(sizeof(message_local));
	upc_memget(lmsg, &message_list[off], sizeof(message_local));
	if (off < 0 || off >= THREADS) {
		free(lmsg);
		return ret;
	}

	upc_free(message_list[off].data);

	lmsg->data_size = -1;
	lmsg->dest = -1;
	lmsg->data = NULL;
	upc_memput(&message_list[off], lmsg, sizeof(message_local));
	free(lmsg);

	return 0;
}

//Retrieve a message from the shared array
message_local *get_message(int source, int dest, int tag) {
	message_local *ret, *lmsg;
	int i;
	int found = 0;

	ret = NULL;
	if (dest < 0 || dest >= THREADS) {
		return ret;
	}

	while (!found) {
		upc_lock(message_lock);
		found = find_message_nolock(source, dest, tag);
		if (!found) {
			upc_unlock(message_lock);
			usleep(10);
		}
	}

	lmsg = malloc(sizeof(message_local));
	upc_memget(lmsg, &message_list[dest], sizeof(message_local));
	if (lmsg->dest != dest) {
		free(lmsg);
		upc_unlock(message_lock);
		return NULL;
	}
	
	if (source != MPI_ANY_SOURCE && source != lmsg->source) {
		free(lmsg);
		upc_unlock(message_lock);
		return NULL;
	}

	if (lmsg->tag != tag && tag != MPI_ANY_TAG) {
		free(lmsg);
		upc_unlock(message_lock);
		return NULL;
	} 
	
	lmsg->data = malloc(lmsg->data_size);
	upc_memget(lmsg->data, message_list[dest].data, lmsg->data_size);

	delete_message(dest);
	upc_unlock(message_lock);	
	ret = lmsg;
	
	return ret;
}

//Look for a message in the shared array
int find_message(int source, int dest, int tag) {
  	message_local *lmsg;
	int ret;

	ret = 0;
	if (dest < 0 || dest >= THREADS) {
		return ret;
	}

	//Find msg in the list
	upc_lock(message_lock);
	lmsg = malloc(sizeof(message_local));
	upc_memget(lmsg, &message_list[dest], sizeof(message_local));
	if (lmsg->dest != dest) {
		goto done;
	}
	
	if (source != MPI_ANY_SOURCE && source != lmsg->source) {
		goto done;
	}

	if (lmsg->tag == tag || tag == MPI_ANY_TAG) {
		ret = 1;
	} 


done:
	upc_unlock(message_lock);
	free(lmsg);
	return ret;
}

//Same as find_message, but without the upc_lock calls
int find_message_nolock(int source, int dest, int tag) {
	message_local *lmsg;
	int ret;

	ret = 0;
	if (dest < 0 || dest >= THREADS) {
		return ret;
	}

	//Find msg in the list
	lmsg = malloc(sizeof(message_local));
	upc_memget(lmsg, &message_list[dest], sizeof(message_local));
	if (lmsg->dest != dest) {
		free(lmsg);
		return ret;
	}
	
	if (source != MPI_ANY_SOURCE && source != lmsg->source) {
		free(lmsg);
		return ret;
	}

	if (lmsg->tag == tag || tag == MPI_ANY_TAG) {
		ret = 1;
	} 

	free(lmsg);

	return ret;
}
