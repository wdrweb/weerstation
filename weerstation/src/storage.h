#pragma once
#include "measurements.h"
#include <zephyr/fs/fcb.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>
#include <vector>

#define SECTOR_SZ 4096u
#define N_SECTORS 48u
#define FCB_AREA PARTITION_ID(storage_partition)
#define CACHE_N 15

class Storage
{
public:
    Storage();
    ~Storage();
    int write_storage(measurements* m);
    int read_last_n(std::vector<measurements>& measurementsVec, int n);

private:
    struct flash_sector sectors[N_SECTORS];
    struct fcb fcb;
    struct fcb_entry cache[CACHE_N]; 
    int cache_count = 0;
    int cache_head = 0; // next index to write to, oldest entry if cache is full
};