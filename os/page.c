#include "os.h"

extern void memset(void *ptr, uint8_t val, uint32_t size);

extern uint32_t TEXT_START;
extern uint32_t TEXT_END;
extern uint32_t DATA_START;
extern uint32_t DATA_END;
extern uint32_t RODATA_START;
extern uint32_t RODATA_END;
extern uint32_t BSS_START;
extern uint32_t BSS_END;
extern uint32_t HEAP_START;
extern uint32_t HEAP_SIZE;

/*
 * _alloc_start points to the actual start address of heap pool
 * _alloc_end points to the actual end address of heap pool
 * _num_pages holds the actual max number of pages we can allocate.
 */
static uint32_t _alloc_start = 0;
static uint32_t _alloc_end = 0;
static uint32_t _num_pages = 0;
static uint32_t _num_chunks = 0;

#define CHUNK_SIZE 256
#define PAGE_SIZE 4096

#define PAGE_ORDER 12
#define INDEX_PAGE_NUMS 128
#define CHUNK_NUMS_A_PAGE (PAGE_SIZE / CHUNK_SIZE)

#define TAKEN (uint8_t)(1 << 0)
#define LAST  (uint8_t)(1 << 1)


#define chunk_num(x) (((x) / ((CHUNK_SIZE) + 1)) + 1)

/*
 * Chunk Descriptor 
 * chunks:
 * - bit 0: flag if this chunk is taken(allocated)
 * - bit 1: flag if this chunk is the last chunk of the memory chunk allocated
 */

struct Chunk {
	uint8_t flags;
};


static inline int is_last(struct Chunk *e)
{
	return e->flags & LAST ? 1 : 0;
}


static inline int is_free(struct Chunk *e)
{
	return e->flags & TAKEN ? 0 : 1;
}


static inline void set_flag(struct Chunk *e, uint8_t flags)
{
	e->flags |= flags;
}


static inline void clear(struct Chunk *e)
{
	e->flags = 0;
}


/*
 * align the address to the border of page(4K)
 */

static inline uint32_t _align_page(uint32_t address)
{
	uint32_t order = (1 << PAGE_ORDER) - 1;
	return (address + order) & (~order);
}


static inline int is_page_free(struct Chunk *chunk)
{
	int flag = 1;
	for (int i = 0; i < CHUNK_NUMS_A_PAGE; i++) {
		if (!is_free(chunk)) {
			flag = 0;
			break;
		}
		chunk += 1;
	}

	return flag;
}


static void *mp_alloc(int total, int nums, int SIZE, int STEPS, int(*func)(struct Chunk *))
{
	if (nums <= 0) {
		return NULL;
	}

	struct Chunk *chunk = (struct Chunk *)HEAP_START;
	int found;
	struct Chunk *t;

	for (int i = 0; i <= total - nums; i++) {
		found = 0;
		if (func(chunk)) {
			found = 1;
			t = chunk;
			for (int j = 0; j < nums; j++) {
				if (!func(t)) {
					found = 0;
					break;
				}
				t += STEPS;
			}
			if (found) {
				for (int j = 0; j < nums * STEPS; j++) {
					set_flag(chunk, TAKEN);
					chunk++;
				}
				chunk -= 1;
				set_flag(chunk, LAST);			

				return (void *)(_alloc_start + i * SIZE);
			}
		}

		chunk += STEPS;
	}

	return NULL;

}

static void mp_free(void *p, int SIZE, int STEPS)
{
	if (!p || (uint32_t)p >= _alloc_end) {
		return;
	}
	struct Chunk *chunk = (struct Chunk *)HEAP_START;
	int pos = ((uint32_t)p - _alloc_start) / SIZE;

	chunk += pos * STEPS;

	while (!is_last(chunk)) {
		clear(chunk);
		chunk += 1;
	}
	clear(chunk);
}

void page_init()
{
	/* 
	 * The size of page is 4096 Byte.
	 * And each CHUNK's size is 256 Byte, we use 128 Page to manage (128 x 4096 x 256)
	 */
	_num_pages = (HEAP_SIZE / PAGE_SIZE) - INDEX_PAGE_NUMS;
	_num_chunks = _num_pages * CHUNK_NUMS_A_PAGE;
	
	printf("HEAP_START = %x, HEAP_SIZE = %x, num of pages = %d\n", \
			HEAP_START, HEAP_SIZE, _num_pages);

	uint8_t *ptr = (uint8_t *)HEAP_START;
	memset(ptr, 0, _num_pages * CHUNK_NUMS_A_PAGE);


	_alloc_start = _align_page(HEAP_START + INDEX_PAGE_NUMS * PAGE_SIZE);
	_alloc_end = _alloc_start + (PAGE_SIZE * _num_pages);

	printf("TEXT:   0x%x -> 0x%x\n", TEXT_START, TEXT_END);
	printf("RODATA: 0x%x -> 0x%x\n", RODATA_START, RODATA_END);
	printf("DATA:   0x%x -> 0x%x\n", DATA_START, DATA_END);
	printf("BSS:    0x%x -> 0x%x\n", BSS_START, BSS_END);
	printf("HEAP:   0x%x -> 0x%x\n", _alloc_start, _alloc_end);

}

/*
 * Allocate a memory page which is composed of contiguous physical pages
 * - npages: the number of PAGE_SIZE pages to allocate
 */
void *page_alloc(int npages)
{	
	return mp_alloc(_num_pages, npages, PAGE_SIZE, CHUNK_NUMS_A_PAGE, is_page_free);
}

/*
 * Free the memory
 * - p: start address of the memory
 */
void page_free(void *p)
{
	mp_free(p, PAGE_SIZE, CHUNK_NUMS_A_PAGE);
}


void *malloc(size_t size)
{
	return mp_alloc(_num_chunks, chunk_num(size), CHUNK_SIZE, 1, is_free);
}


void free(void *p)
{
	mp_free(p, CHUNK_SIZE, 1);
}


void page_test()
{
	
	void *p0 = page_alloc(1);
	void *p1 = malloc(257);
	free(p1);

	void *p2 = page_alloc(1);
	void *p3 = malloc(258);
	page_free(p0);
	void *p4 = malloc(1);
	void *p5 = page_alloc(1);
	void *p6 = malloc(256 * 3);

	printf("p0 = 0x%x\np1 = 0x%x\np2 = 0x%x\np3 = 0x%x\np4 = 0x%x\np5 = 0x%x\np6 = 0x%x\n", \
	p0, p1, p2, p3, p4, p5, p6);
}

