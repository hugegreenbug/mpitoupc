#include "mpi.h"
#include "mpi_info.h"

/**
 * Create the MPI_Info structure
 */
int MPI_Info_create(MPI_Info *info) {
	int i = 0;

	if (info == NULL)
	  return MPI_ERR_ARG;

	*info = malloc(sizeof(struct MPI_Info));
	(*info)->entries = (info_entry **) malloc(sizeof(info_entry *) * BUCKETS);
	(*info)->num_entries = 0;

	for (i = 0; i < BUCKETS; i++) {
		(*info)->entries[i] = NULL;
	}

	return MPI_SUCCESS;
}

/**
 * Free the MPI_Info structure
 */
int MPI_Info_free(MPI_Info *info) {
	int i;
	info_entry *p, *t;

	if (!info || !*info)
		return MPI_ERR_ARG;

	for (i = 0; i < BUCKETS; i++) {
		p = (*info)->entries[i];
		while (p) {
			t = p->next;
			free(p->key);
			free(p->value);
			free(p);
			p = t;
		}

		(*info)->entries[i] = NULL;
	}

	free(*info);
	return MPI_SUCCESS;
}


/**
 * Sets nkeys to be the number of keys in the given info
 */
int MPI_Info_get_nkeys(MPI_Info info, int *nkeys) {
	if (!info)
		return MPI_ERR_ARG;

	*nkeys = info->num_entries;

	return MPI_SUCCESS;
}

/**
 * Sets the key parameter to be the nth key in the given info
 */
int MPI_Info_get_nthkey(MPI_Info info, int n, char *key) {
	int cur_key;
	int i;
	info_entry *p;
	char *ret_key;

	cur_key = -1;
	ret_key = NULL;

	if (info->num_entries == 0)
		return MPI_ERR_ARG;

	for (i = 0; i < BUCKETS; i++) {
		p = info->entries[i];
		while (p) {
			cur_key++;
			if (cur_key == n) {
				ret_key = p->key;
				break;
			}
			
			p = p->next;
		}

		info->entries[i] = NULL;
	}

	if (ret_key == NULL)
		return MPI_ERR_OTHER;
	
	return MPI_SUCCESS;
}

/**
 * Adds a key/value pair in the given info
 */
int MPI_Info_set(MPI_Info info, char *key, char *value) {
	int bucket;
	info_entry *p, *n;

	if (key == NULL || value == NULL)
		return MPI_ERR_ARG;

	bucket = hash_key(key);
	if (bucket < 0 || bucket >= BUCKETS)
		return MPI_ERR_UNKNOWN;
	
	n = malloc(sizeof(info_entry));
	memset(n, 0, sizeof(info_entry));
	n->key = malloc(strlen(key) + 1);
	n->value = malloc(strlen(value) + 1);
	memcpy(n->key, key, strlen(key) + 1);
	memcpy(n->value, key, strlen(key) + 1);
	
	if (info->entries[bucket]) {
		info->entries[bucket] = n;
		return MPI_SUCCESS;
	}

	p = info->entries[bucket];
	while (p) {
		if (p->next) {
			p = p->next;
			continue;
		}

		p->next = n;
		n->prev = p;
		break;
	}

	return MPI_SUCCESS;
}

/**
 * Sets the value parameter to be the value for the given key
 */
int MPI_Info_get(MPI_Info info, char *key, int valuelen, 
		 char *value, int *flag) {
	int cur_key;
	int i;
	info_entry *p;

	*flag = 0;
	if (info->num_entries == 0)
		return MPI_ERR_ARG;

	for (i = 0; i < BUCKETS; i++) {
		p = info->entries[i];
		while (p) {
			if (strncmp(p->key, key, MPI_MAX_INFO_KEY)) {
				*flag = 1;
				memcpy(value, p->value, (strlen(p->value) + 1 < valuelen) ? 
				       strlen(p->value) + 1 : valuelen);
				break;
			}
			
			p = p->next;
		}

		info->entries[i] = NULL;
	}

	return MPI_SUCCESS;
}

/**
 * Hashes the given key to get the bucket number
 */
int hash_key(char *key) {
	int len, sum;
	int i;

	len = strlen(key);
	sum = 0;
	if (len == 0)
		return -1;

	for (i = 0; i < len; i++) {
		sum += (int) key[i];
	}

	sum %= BUCKETS;
	return sum;
}
