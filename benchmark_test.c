
#include <autoconf.h>
#include <sel4test-vbtalloc/gen_config.h>

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include <sel4runtime.h>

#include <allocman/bootstrap.h>
#include <allocman/vka.h>

#include <sel4utils/vspace.h>
#include <sel4utils/stack.h>
#include <sel4utils/process.h>
#include <sel4test/test.h>
#include <simple/simple.h>
#include <simple-default/simple-default.h>
#include <utils/util.h>
#include <vka/object.h>
#include <vka/capops.h>
#include <vspace/vspace.h>
#include <sel4platsupport/platsupport.h>

#include "wrapper.h"
#include "aimbench.h"
#include <sel4/benchmark_utilisation_types.h>

struct unienv {
    vka_t vka;
    vspace_t vspace;
    simple_t simple;
};
typedef struct unienv *unienv_t;

#define ALLOCATOR_VIRTUAL_POOL_SIZE ((1ul << (seL4_PageBits + 22)))
#define ALLOCATOR_STATIC_POOL_SIZE ((1ul << seL4_PageBits) * 400)
#define BRK_VIRTUAL_DEFAULT_SIZE ((1ul << (seL4_PageBits + 16)))

struct unienv env;
static sel4utils_alloc_data_t data;
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];
static allocman_t * __alloc;

extern vspace_t *muslc_this_vspace;
extern reservation_t muslc_brk_reservation;
extern void *muslc_brk_reservation_start;

static void init_env(unienv_t env)
{
    allocman_t *allocman;
    reservation_t virtual_reservation;
    int error;

    allocman = bootstrap_use_current_simple(&env->simple, ALLOCATOR_STATIC_POOL_SIZE, allocator_mem_pool);
    if (allocman == NULL) {
        ZF_LOGF("Failed to create allocman");
    }

    allocman_make_vka(&env->vka, allocman);

    error = sel4utils_bootstrap_vspace_with_bootinfo_leaky(&env->vspace,
                                                           &data, simple_get_pd(&env->simple),
                                                           &env->vka, platsupport_get_bootinfo());
    if (error) {
        ZF_LOGF("Failed to bootstrap vspace");
    }

    sel4utils_res_t *muslc_brk_reservation_memory = allocman_mspace_alloc(allocman, sizeof(sel4utils_res_t), &error);
    if (error) {
        ZF_LOGE("Failed to alloc virtual memory for muslc heap describing metadata");
    }

    error = sel4utils_reserve_range_no_alloc(&env->vspace, muslc_brk_reservation_memory,
                                             BRK_VIRTUAL_DEFAULT_SIZE, seL4_AllRights, 1, &muslc_brk_reservation_start);
    if (error) {
        ZF_LOGE("Failed to reserve range for muslc heap initialization");
    }

    muslc_this_vspace = &env->vspace;
    muslc_brk_reservation.res = muslc_brk_reservation_memory;

    void *vaddr;
    virtual_reservation = vspace_reserve_range(&env->vspace,
                                               ALLOCATOR_VIRTUAL_POOL_SIZE, seL4_AllRights, 1, &vaddr);
    if (virtual_reservation.res == 0) {
        ZF_LOGF("Failed to provide virtual memory for allocator");
    }

    bootstrap_configure_virtual_pool(allocman, vaddr,
                                     ALLOCATOR_VIRTUAL_POOL_SIZE, simple_get_pd(&env->simple));
                                
    __alloc = allocman;

}

#if 1

#define ITER 20000

typedef struct mem_block {
    size_t time;
    int frame_num;
    vka_object_t blk;
} mblk_t;

mblk_t mem[ITER];

int get_random_size_1()
{
    int a = random() % 1024;
        while (a == 0) {
        a = random() % 1024;
    }
    return a;
}

int get_random_size_2()
{
    int a[11] = {1,2,4,8,16,32,64,128,256,512,1024};
    int b = random() % 11;
    return a[b];
}

int get_random_size_3()
{
    int a[11] = {1,2,4,8,16,32,64,128,256,512,1024};
    int b = (random() % 1024) + 1;
    if(b == 1){
    return a[10];
    }
    else if(b == 2){
        return a[9];
    }
    else if(2<b && b<=4){
    return a[8];
    }
    else if(4<b && b<=8){
    return a[7];
    }
    else if(8<b && b<=16){
    return a[6];
    }
    else if(16<b && b<=32){
    return a[5];
    }
    else if(32<b && b<=64){
    return a[4];
    }
    else if(64<b && b<=128){
    return a[3];
    }
    else if(128<b && b<=256){
    return a[2];
    }
    else if(256<b && b<=512){
    return a[1];
    }
    else if(512<b && b<=1024){
    return a[0];
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

typedef struct {
    unsigned long long start_address;
    unsigned int size;
} MemoryBlock;

unsigned int findLargestContiguousMemory(MemoryBlock *memory_blocks, unsigned int num_blocks) {

    for(int i = 0; i < num_blocks; i++) {
        printf("[%016llx, %x]\n", memory_blocks[i].start_address, memory_blocks[i].size);
    }

    for (unsigned int i = 0; i < num_blocks - 1; i++) {
        for (unsigned int j = i + 1; j < num_blocks; j++) {
            if (memory_blocks[i].start_address > memory_blocks[j].start_address) {
                MemoryBlock temp = memory_blocks[i];
                memory_blocks[i] = memory_blocks[j];
                memory_blocks[j] = temp;
            }
        }
    }

    unsigned int largest_contiguous_memory = 0;
    unsigned int current_contiguous_memory = memory_blocks[0].size;

    for (unsigned int i = 1; i < num_blocks; i++) {
        MemoryBlock current_block = memory_blocks[i];
        MemoryBlock previous_block = memory_blocks[i - 1];

        if (current_block.start_address == previous_block.start_address + previous_block.size) {
            current_contiguous_memory += current_block.size;
        } else {
            if (current_contiguous_memory > largest_contiguous_memory) {
                largest_contiguous_memory = current_contiguous_memory;
            }
            current_contiguous_memory = current_block.size;
        }
    }

    if (current_contiguous_memory > largest_contiguous_memory) {
        largest_contiguous_memory = current_contiguous_memory;
    }

    return largest_contiguous_memory;
}

vka_object_t barray[1024 * ITER];
static int buddy_test()
{
    int error = 0;
    vka_t *vka = &env.vka;
    MemoryBlock memory_blocks[50];
    
    vka_object_t ut_container;

    int bitmap_tree_test = 1;

    printf("\n >>> start <<< \n");

    int size = 0;
    int time = 0;
    for(int i = 0; i < ITER; i++)
    {
        printf("--------------\n");
    printf("current time: %d\n",i);
    size = get_random_size_2();
    time = get_random_time(12000);

        mem[i].time = time + i;
        mem[i].frame_num = size;

        printf("alloc_block{size: %d, time: %d}\n", CTZL(size) + 12, time);

        if (bitmap_tree_test) {
        for (int j = 0; j < size; ++j) {
                error = vka_alloc_frame(&env.vka, 12, &barray[i * 1024 + j]);
            }                   
        } 
        else {
            error = vka_alloc_untyped(vka, CTZL(size) + 12, &mem[i].blk);
        }

        if (error) {
        int z = 0;
        for (int i = 0; i < 11; ++i) {   
                printf("treelist[%d]: ", i);
            for (struct vbt_tree *t = __alloc->frame_pool.mem_treeList[i]; t; t = t->next) {
                if (t) {
                    printf(" < capPtr: [%ld] ", t->pool_range.capPtr);
                if (t->prev) {
                    printf(" prev: {%ld} ", t->prev->pool_range.capPtr);
                }
                if (t->next) {
                    printf(" next: {%ld} ", t->next->pool_range.capPtr);
                }
                printf("paddr: %016lx>\n", t->paddr);
                }   
            }
                printf("\n");
            }
    
            printf("\n [EmptyList] >>>> : \n");
        int xx = 0;
            for (struct vbt_tree *tree = __alloc->frame_pool.empty; tree; tree = tree->next) {
                if (tree) {
                    ++xx;
                    printf("   < capPtr: [%ld] ", tree->pool_range.capPtr);
                    if (tree->prev) {
                        printf(" prev: {%ld} ", tree->prev->pool_range.capPtr);
                    }
                    if (tree->next) {
                        printf(" next: {%ld} ", tree->next->pool_range.capPtr);
                    }
                    printf("paddr: %016lx > \n", tree->paddr);        
                }
            }
            printf("%d\n", xx);
    
            ZF_LOGE("Failed to alloc contiguous frames of size: %d", size);
            exit(0);
        }
        
        printf(" cptr: %ld\n", mem[i].blk.cptr);
        printf("-----------------------\n");

    for(int j = 0; j < i; j++)
    {
        if (mem[j].time == i) {
        printf("free_block{size: %ld, time: %ld}\n", CTZL(mem[j].frame_num) + 12, mem[j].time);
            printf("==============================\n\n");
            for (int k = 0; k < mem[j].frame_num; ++k) {
            vka_free_object(&env.vka, &barray[j * 1024 + k]);
            }
        }
    }
    }
    printf("\n >>> tend <<< \n");

    int s;
    for (s = 24; s >= 22; s--) {
        if (!vka_alloc_untyped(vka, s, &ut_container)) {
            printf("Current avail size: %d\n", s);
            break;
        }
    }
    printf("Down to the bitmap\n");
    
    int z = 0;
    for (int i = 0; i < 11; ++i) {
        printf("treelist[%d]: ", i);
        for (struct vbt_tree *t = __alloc->frame_pool.mem_treeList[i]; t; t = t->next) {
            if (t) {
                printf(" < capPtr: [%ld] ", t->pool_range.capPtr);
                if (t->prev) {
                     printf(" prev: {%ld} ", t->prev->pool_range.capPtr);
                }
                if (t->next) {
                    printf(" next: {%ld} ", t->next->pool_range.capPtr);
                }
                printf("paddr: %016lx>\n", t->paddr);
                if(i==8 ){
                    memory_blocks[z].start_address = t->paddr;
                    memory_blocks[z].size = 0x100000;
                    z = z + 1;  
                } 
                if(i==9 ){
                    memory_blocks[z].start_address = t->paddr;
                    memory_blocks[z].size = 0x200000;
                    z = z + 1;
                } 
                if(i==10 ){
                    memory_blocks[z].start_address = t->paddr;
                    memory_blocks[z].size = 0x200000;
                    z = z + 1;
                } 
            }
        }
        printf("\n");
    }

    unsigned int largest_contiguous_memory = findLargestContiguousMemory(memory_blocks, z);
    printf("the largest free block size：%uM\n", largest_contiguous_memory / 1048576);
    
    printf("\n [EmptyList] >>>> : \n");
    int xx = 0;
    for (struct vbt_tree *tree = __alloc->frame_pool.empty; tree; tree = tree->next) {
        if (tree) {
            ++xx;
            printf("   < capPtr: [%ld] ", tree->pool_range.capPtr);
            if (tree->prev) {
                printf(" prev: {%ld} ", tree->prev->pool_range.capPtr);
            }
            if (tree->next) {
                printf(" next: {%ld} ", tree->next->pool_range.capPtr);
            }
            printf("paddr: %016lx > \n", tree->paddr);
        }
    }
    printf("%d\n", xx);
    
    return 0;
}

#endif

#define EPOCH 1

vka_object_t rarray[1024 * EPOCH];

void *__posix_entry(void *arg UNUSED)
{

    printf("\n>>>>>>>> __posix_entry__ <<<<<<<\n");
    printf("*********** Benchmark ***********\n\n");
    
    int error;
    int frame_num = 0;
    int sum[30];
    vka_object_t res[EPOCH];
    vka_object_t es[30*30];
    unsigned long long time[50];
    unsigned long long start = 0, end = 0;
    int loop = 50;

    uint64_t *ipcbuffer = (uint64_t*) &(seL4_GetIPCBuffer()->msg[0]);
        
    for(int i = 30; i >= 4; i--){
        for(int j = 0; ; j++){
            error = vka_alloc_untyped(&env.vka, i, &es[i*30 + j]);
            if(error == 1) {
                sum[i] = j;
            break;
            }
        }
    }
    for(int i = 30;  i >= 4; i--){
        printf("sum[%d]: %d\n", i, sum[i]);
    }
        
    error = buddy_test();
        
    seL4_BenchmarkResetThreadUtilisation(simple_get_tcb(&env.simple));
    seL4_BenchmarkResetLog();
    
    for(int k=0; k < loop; k++){
        for (int i = 0; i < EPOCH; ++i) {
            if (1) {
                error = vka_alloc_frame_contiguous(&env.vka, 22, &frame_num, &res[i]);
            } 
            else {
                for (int j = 0; j < 256; ++j) {
                    error = vka_alloc_frame(&env.vka, 12, &rarray[i * 1024 + j]);
                }
            }           
        if (error) {
            assert(0);
        }        
        }
         
    for (int i = 0; i < EPOCH; ++i) {
        if (1) {
        vka_free_object(&env.vka, &res[i]);
        } 
        else {
            for (int j = 0; j < 256; ++j) {
            vka_free_object(&env.vka, &rarray[i * 1024 + j]);
            }
        }
    }
        
    seL4_BenchmarkFinalizeLog();
    seL4_BenchmarkGetThreadUtilisation(simple_get_tcb(&env.simple));
    time[k] = ipcbuffer[BENCHMARK_TCB_UTILISATION];
    printf("\n CPU cycles spent: %ld\n", ipcbuffer[BENCHMARK_TCB_UTILISATION]);
  
    }
        
    unsigned long long all = 0;
    for(int i=0; i < loop; i++){
        all = all + time[i];
    }
    all = all/loop;
    
    printf("\n*********** Benchmark ***********\n");
    printf("\nall: %ld\n", all);
    printf("\nCPU cycles spent: %ld\n", ipcbuffer[BENCHMARK_TCB_UTILISATION]);
}

#define POSIX_ENTRY __posix_entry

static void sel4test_exit(int code)
{
    seL4_TCB_Suspend(seL4_CapInitThreadTCB);
}

int main(void)
{
    sel4runtime_set_exit(sel4test_exit);

    seL4_BootInfo *info = platsupport_get_bootinfo();
    simple_default_init_bootinfo(&env.simple, info);
    simple_print(&env.simple);
    init_env(&env);

    void *res;
    int error;

    error = sel4utils_run_on_stack(&env.vspace, POSIX_ENTRY, NULL, &res);
    
    test_assert_fatal(error == 0);
    test_assert_fatal(res == 0);

    return 0;
}