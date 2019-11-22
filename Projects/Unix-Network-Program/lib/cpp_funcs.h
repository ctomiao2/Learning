#ifdef __cplusplus
extern "C" {
#endif

long guid();

void* unordered_map_create();

void unordered_map_insert(void* p_map, int key, void *value);

void* unordered_map_find(void* p_map, int key);

int unordered_map_items(void* p_map, int** key_list, void*** val_list);


#ifdef __cplusplus
}
#endif
