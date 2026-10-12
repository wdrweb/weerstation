#include "storage.h"
#include <zephyr/kernel.h>

Storage::Storage()
{

	int rc;
	for (unsigned int i = 0; i < N_SECTORS; i++)
	{
		sectors[i].fs_off = i * SECTOR_SZ; //(ptrdiff_t)(i * SECTOR_SZ);
		sectors[i].fs_size = SECTOR_SZ;
	}

	fcb.f_magic = 0x57a17e01;
	fcb.f_version = 1;
	fcb.f_sectors = sectors;
	fcb.f_sector_cnt = N_SECTORS;
	fcb.f_scratch_cnt = 1; // maybe not needed.

	rc = fcb_init(FCB_AREA, &fcb);
	if (rc)
	{
		printk("fcb_init failed: %d\n", rc);
	}
}

Storage::~Storage()
{
	if (fcb.fap != NULL)
	{
		flash_area_close(fcb.fap);
		fcb.fap = NULL;
	}
}

int Storage::write_storage(measurements *m)
{
	struct fcb_entry loc;
	int rc;
	rc = fcb_append(&fcb, sizeof(*m), &loc);
	if (rc)
	{
		fcb_rotate(&fcb);
		rc = fcb_append(&fcb, sizeof(*m), &loc);
	}
	if (rc)
	{
		printk("fcb_append failed: %d\n", rc);
		return rc;
	}

	rc = flash_area_write(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc),
						  (const uint8_t *)m, sizeof(*m));
	if (rc)
	{
		printk("flash_area_write failed: %d\n", rc);
		return rc;
	}
	rc = fcb_append_finish(&fcb, &loc);
	cache[cache_head] = loc;
	cache_head = (cache_head + 1) % CACHE_N;
	if (cache_count < CACHE_N)
	{
		cache_count++;
	}
	return rc;
}

int Storage::read_last_n(std::vector<measurements> &measurementsVec, int n)
{
	if (n <= 0)
	{
		return -EINVAL;
	}
	if (n <= cache_count)
	{
		// read from cache
		measurementsVec.reserve(measurementsVec.size() + n);
		int start = (cache_head - n + CACHE_N) % CACHE_N;
		for (int i = 0; i < n; i++)
		{
			struct fcb_entry loc = cache[(start + i) % CACHE_N];
			measurements m;
			int rc = flash_area_read(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), &m, sizeof(m));
			if (rc)
			{
				printk("flash_area_read failed: %d\n", rc);
				return rc;
			}
			measurementsVec.push_back(m);
		}
		return 0;
	}
	else
	{
		// count the items in the fcb
		struct fcb_entry scan = {0};
		int total = 0;
		while (fcb_getnext(&fcb, &scan) == 0)
		{
			total++;
		}

		if (total == 0)
		{
			return -ENOENT;
		}

		if (total < n)
		{
			n = total;
		}
		int skip = total - n;
		measurementsVec.reserve(measurementsVec.size() + n);

		// skip first items
		struct fcb_entry loc = {0};
		for (int i = 0; i < skip + 1; i++)
		{
			if (fcb_getnext(&fcb, &loc) != 0)
			{
				return -ENOENT;
			}
		}

		// read the last n items
		for (int i = 0; i < n; i++)
		{
			measurements m;
			int rc = flash_area_read(fcb.fap, FCB_ENTRY_FA_DATA_OFF(loc), &m, sizeof(m));
			if (rc)
			{
				printk("flash_area_read failed: %d\n", rc);
				return rc;
			}
			measurementsVec.push_back(m);

			if (fcb_getnext(&fcb, &loc))
			{
				// Reached the newest entry.
				break;
			}
		}
		return 0;
	}
}