#ifndef _MPI_INFO_H
#define _MPI_INFO_H 1

#include "string.h"

#define BUCKETS 101
#define MPI_MAX_INFO_KEY 256
#define MPI_MAX_INFO_VAL 256

typedef struct info_entry info_entry;
struct info_entry {
	char *key;
	char *value;
	info_entry *next;
	info_entry *prev;
};

typedef struct MPI_Info *MPI_Info;
struct MPI_Info {
	info_entry **entries;
	int num_entries;
};

#define MPI_INFO_NULL ((MPI_Info) 0)

int MPI_Info_create(MPI_Info *info);
int MPI_Info_free(MPI_Info *info);
int MPI_Info_get(MPI_Info info, char *key, int valuelen, 
		 char *value, int *flag);
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys);
int MPI_Info_get_nthkey(MPI_Info info, int n, char *key);
int MPI_Info_set(MPI_Info info, char *key, char *value);
int hash_key(char *key);

#endif /*_MPI_INFO_H*/
