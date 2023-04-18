/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <autoconf.h>
#include <sel4/sel4.h>
#include <stdlib.h>

#ifdef CONFIG_ARCH_X86
#include <platsupport/arch/tsc.h>
#endif

#define N_ASID_POOLS ((int)BIT(seL4_NumASIDPoolsBits))
#define ASID_POOL_SIZE ((int)BIT(seL4_ASIDPoolIndexBits))

#include "../helpers.h"

struct mem_block {
	int size;
	int time;
	int *p;
};

int get_random_size_1()
{
	int a = random() % 50;
	while (a == 0) {
		a = random() % 50;
	}
	return a;
}

int get_random_size_2()
{
	int a[10] = {1,2,4,6,8,10,20,30,40,50};
	int b = random() % 10;
	
	return a[b];
}

int get_random_size_3()
{
	int a[6] = {1,2,4,8,16,32};
	int b = (random() % 32) + 1;
	printf("b is %d\n",b);
	if(b <= 16){
		return a[0];
	}
	else if(16<b && b<=24){
		return a[1];
	}
	else if(24<b && b<=28){
		return a[2];
	}
	else if(28<b && b<=30){
		return a[3];
	}
	else if(b==31){
		return a[4];
	}
	else if(b==32){
		return a[5];
	}
	
	
}

int get_random_time(int a)
{
	int b = random() % a;
	while (b == 0){
		b = random() % a;
	}
	return b;
}

static int buddy_test()
{
	int size = 0;
	int time = 0;
	int i,j = 0;
	struct mem_block mem[2000];
	for(i = 0; i < 4000; i++)
	{
		printf("current time: %d\n",i);
		size = get_random_size_3();
		time = get_random_time(10);
		mem[i].size = size;
		mem[i].time = time + i; 
		mem[i].p = (int*)malloc(size * 4096);
		printf("alloc_block{size:%d, time:%d}\n",size, time);
		for(j = 0; j < i; j++)
		{
			if (mem[j].time == i) {
				free(mem[j].p);
				printf("free_block{size:%d, time:%d}\n", mem[j].size, mem[j].time);
			}
		}
		
	}
	
	return 0;
}
DEFINE_TEST(BUDDY0000, "TEST BUDDY", buddy_test, true)

