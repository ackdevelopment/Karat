#include<stdio.h>
#include<alloc.h>
#include<smap.h>
#include<ktypes.h>

#define FNV_PRIME 16777619
#define FNV_OFFSET_BASIS 2166136261

static inline uint32_t FNV1a_hash(register const wchar_t *data)
{
	register uint32_t hash = FNV_OFFSET_BASIS;
	while(*data){
		#define hash_octet(h, o, p) h = (h ^ o) * p;
		hash_octet(hash, *(char *)data, FNV_PRIME);
		hash_octet(hash, *(char *)(data + 1), FNV_PRIME);
		++data;
		#undef hash_octet
	}
	return hash;
}

static inline size_t hash_to_index(const wchar_t *key)
{
	return FNV1a_hash(key) % HASH_BUCKETS;
}

struct smap *smap_create(void(*destroy_node)(void *))
{
	struct smap *ret = s_alloc(struct smap);
	ret->destroy_node_f = destroy_node;
	return ret;
}

struct smap *smap_create_d()
{
	return smap_create(s_free);
}

void *smap_lookup(struct smap *map, const wchar_t *key)
{
	struct smap_node *head = map->map[hash_to_index(key)];
	if(!head){
		return NULL;
	}else if(head->next){
		while(head && wcscmp(head->key, key) && (head = head->next));
		return head->value;
	}else{
		return head->value;
	}
}

static inline struct smap_node *make_node(const wchar_t *key, void *val)
{
	struct smap_node *n = s_calloc(1, sizeof(struct smap_node));
	n->key = wcsdup(key);
	n->value = val;
	n->next = NULL;
	return n;
}

void smap_insert(struct smap *map, const wchar_t *key, void *val)
{
	struct smap_node **head = &map->map[hash_to_index(key)];
	while(*head && wcscmp((*head)->key, key) && (head = &(*head)->next));
	if(!*head){
		*head = make_node(key, val);
	}else{
		map->destroy_node_f((*head)->value);
		(*head)->value = val;
	}
}

static void destroy_chain(struct smap *map, struct smap_node *node)
{
	struct smap_node *save;
	while((save = node)){
		node = node->next;
		if(map->destroy_node_f){
			map->destroy_node_f(save->value);
		}
		s_free(save->key);
		s_free(save);
	}
}

void smap_destroy(struct smap *map)
{
	for(size_t i = 0; i < HASH_BUCKETS; ++i){
		if(map->map[i]){
			destroy_chain(map, map->map[i]);
		}
	}
	s_free(map);
}
