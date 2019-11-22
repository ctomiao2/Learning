#include <iostream>
#include <unordered_map>
#include <bits/stdc++.h>
#include "cpp_funcs.h"

long guid() {
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist6(1, INT32_MAX);
    return (long) dist6(rng);
}

void* unordered_map_create() {
    std::unordered_map<int, void*> *p = new std::unordered_map<int, void*>;
    return (void*) p;
}

void unordered_map_insert(void* p_map, int key, void *value) {
    std::unordered_map<int, void*> *p = (std::unordered_map<int, void*> *) p_map;
    (*p)[key] = value;
}

void* unordered_map_find(void* p_map, int key) {
    
    if (p_map == NULL)
        return NULL;

    std::unordered_map<int, void*> *p = (std::unordered_map<int, void*> *) p_map;
    if (p->find(key) == p->end())
        return NULL;
    return (*p)[key];
}

int unordered_map_items(void* p_map, int** key_list, void*** val_list) {
    
    if (p_map == NULL)
        return 0;

    int n = 0;
    std::unordered_map<int, void*> *p = (std::unordered_map<int, void*> *) p_map;
    n = p->size();
    *key_list = (int*) malloc(n * sizeof(int));
    *val_list = (void**) malloc(n * sizeof(void*));
    std::unordered_map<int, void*>::iterator it;
    int i = 0;
    for (it = p->begin(); it != p->end(); ++it) {
        (*key_list)[i] = it->first;
        (*val_list)[i++] = it->second;
    }
    return n;
}

