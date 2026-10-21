#pragma once
#include "measurements.h"
#include <zephyr/kernel.h>
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <vector>

#define SECTOR_SZ 4096u
#define N_SECTORS 48u
#define FCB_AREA PARTITION_ID(storage_partition)
// one hour cached
#define CACHE_N 61

class Storage
{
public:
    Storage();
    ~Storage();
    int write_storage(measurements* m);
    int read_last_n(std::vector<measurements>& measurementsVec, int n);

private:
    int write_locked(measurements* m);
    int read_last_n_locked(std::vector<measurements>& measurementsVec, int n);

    struct flash_sector sectors[N_SECTORS];
    struct fcb fcb;
    struct fcb_entry cache[CACHE_N]; 
    int cache_count = 0;
    int cache_head = 0; // next index to write to, oldest entry if cache is full
    struct k_mutex lock;
};
