#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct ll_node_struct
{
	struct ll_node_struct* next;
	struct ll_node_struct* prev;
} ll_node_t;

void ll_init(ll_node_t* n)
{
	n->next = n;
	n->prev = n;
}

void ll_insert_internal(ll_node_t* new, ll_node_t* prev, ll_node_t* next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;

	*((volatile uintptr_t*)&prev->next) = *((uintptr_t*)&new);
}

void ll_insert(ll_node_t* head, ll_node_t* n)
{
	ll_insert_internal(n, head, head->next);
}

typedef struct
{
	ll_node_t node;
	uintptr_t guest_addr;
	uintptr_t host_addr;
	uint64_t size;
} memslot_ll_t;

typedef struct
{
	uintptr_t guest_addr;
	uintptr_t host_addr;
	uint64_t size;
} memslot_arr_t;

uint64_t __attribute__((noinline)) find_slot_arr(uint64_t addr, uint64_t size, memslot_arr_t* slots)
{
	for(int i = 0; i < 10; i++)
	{
		if(slots[i].guest_addr <= addr &&
			(addr + size) <= (slots[i].guest_addr + slots[i].size))
		{
			return slots[i].host_addr;
		}
	}

	return 0;
}

uint64_t __attribute__((noinline)) find_slot_ll(uint64_t addr, uint64_t size, ll_node_t* slots)
{
	ll_node_t* pos;
	for(pos = slots->next; pos != slots; pos = pos->next)
	{
		memslot_ll_t* slot = (memslot_ll_t*)pos;
		if(slot->guest_addr <= addr &&
			(addr + size) <= (slot->guest_addr + slot->size))
		{
			return slot->host_addr;
		}
	}

	return 0;
}

uint64_t get_ticks()
{
	uint32_t lo, hi;
	asm volatile ("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

uint64_t example_guest_addrs[] = {0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x10000, 0x12000,  0x200000, 0x40000000, 0x80000000};
uint64_t example_host_addrs[] =  {0x1000, 0x2000, 0x3000, 0x4000, 0x5000, 0x10000, 0x12000,  0x200000, 0x40000000, 0x80000000};
uint64_t example_sizes[] = 	 {0x1000, 0x1000, 0x1000, 0x1000, 0x5000, 0x2000,  0x1EE000, 0x8F0000, 0x40000000, 0x40000000};

int main(int argc, char** argv)
{
	// generate array
	memslot_arr_t* arr = malloc(sizeof(*arr) * 10);
	for(int i = 0; i < 10; i++)
	{
		arr[i].guest_addr = example_guest_addrs[i];
		arr[i].host_addr = example_host_addrs[i];
		arr[i].size = example_sizes[i];
	}

	// generate linked list
	ll_node_t head;
	ll_init(&head);
	for(int i = 0; i < 10; i++)
	{
		memslot_ll_t* node = malloc(sizeof(*node));
		node->guest_addr = example_guest_addrs[i];
		node->host_addr = example_host_addrs[i];
		node->size = example_sizes[i];
		ll_insert(&head, &node->node);
	}

#if 0
	// testing
	printf("Array\n");
	for(int i = 0; i < 10; i++)
	{
		printf("%#lx, %#lx -> %#lx\n", arr[i].guest_addr, arr[i].size, arr[i].host_addr);
	}

	printf("LL\n");
	ll_node_t* pos, *start;
	start = &head;
	for(pos = start->next; pos != start; pos = pos->next)
	{
		memslot_ll_t* slot = (memslot_ll_t*)pos;
		printf("%#lx, %#lx -> %#lx\n", slot->guest_addr, slot->size, slot->host_addr);
	}
#endif

#define NUM_RUNS 1000000000
	uint64_t sum = 0;
	uint64_t min = ULONG_MAX;
	uint64_t max = 0;
	uint64_t d = 0;
	for(int i = 0; i < NUM_RUNS; i++)
	{
		uint64_t a = rand();
		uint64_t r_s = rand();

		uint64_t s = get_ticks();
#ifdef ARRAY
		find_slot_arr(a, r_s, arr);
#else
		find_slot_ll(a, r_s, &head);
#endif
		uint64_t e = get_ticks();

		d = e -s;
		sum += d;
		min = (d < min) ? d : min;
		max = (d > max) ? d : max;
	}

	printf("min: %lu\n", min);
	printf("max: %lu\n", max);
	printf("sum: %lu\n", sum);
	printf("avg: %lu\n", sum / NUM_RUNS);

	return 0;
}
