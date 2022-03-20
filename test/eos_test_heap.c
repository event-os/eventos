
/* include ------------------------------------------------------------------ */
#include "eos_test.h"
#include "eventos.h"
#include "unity.h"
#include "unity_pack.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include "eos_test_def.h"

/* heap function ------------------------------------------------------------ */
void eos_heap_init(eos_heap_t * const me);
void * eos_heap_malloc(eos_heap_t * const me, eos_u32_t size);
void eos_heap_free(eos_heap_t * const me, void * data);
void *eos_heap_get_block(eos_heap_t * const me, eos_u8_t priority);
void eos_heap_gc(eos_heap_t * const me, void *data);

/* test data & function ----------------------------------------------------- */
#define EOS_HEAP_TEST_PRINT_UNIT                10
#define EOS_HEAP_TEST_TIMES                     10
#define EOS_HEAP_TEST_PRINT_EN                  0
static eos_heap_t heap;
uint8_t * p_data;

static void print_heap_list(eos_heap_t * const me, eos_u32_t index);

/* test function ------------------------------------------------------------ */
void eos_test_heap(void)
{
    eos_block_t * block_1st = (eos_block_t *)heap.data;

    eos_heap_init(&heap);
    
    /* Make sure the heap initilization is successful. */
    TEST_ASSERT_EQUAL_UINT16(0, heap.error_id);

    // eos_heap_get_block ------------------------------------------------------
    void *eblock[EOS_MAX_ACTORS];
    void *eb;
    for (int i = 0; i < EOS_MAX_ACTORS; i ++) {
        eos_block_t *block;
        eblock[i] = eos_heap_malloc(&heap, (i + 32));
        TEST_ASSERT_NOT_NULL(eblock[i]);
        eos_event_inner_t *e = (eos_event_inner_t *)eblock[i];
        e->sub = (1 << i);
        TEST_ASSERT_EQUAL_UINT8(0, heap.empty);

        print_heap_list(&heap, i);
    }
    
    eos_u8_t count = EOS_MAX_ACTORS;
    for (int i = 0; i < EOS_MAX_ACTORS; i ++) {
        TEST_ASSERT_EQUAL_UINT8(count, heap.count);
        eos_event_inner_t *e = (eos_event_inner_t *)eblock[i];
        TEST_ASSERT_BIT_HIGH(i, e->sub);
        eb = eos_heap_get_block(&heap, i);
        TEST_ASSERT_NOT_NULL(eb);
        TEST_ASSERT_EQUAL_POINTER(eblock[i], eb);
        TEST_ASSERT_BIT_LOW(i, e->sub);
        eos_heap_gc(&heap, e);
        count --;
        TEST_ASSERT_EQUAL_UINT8(count, heap.count);

        print_heap_list(&heap, i);
    }

    block_1st = (eos_block_t *)heap.data;
    TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, block_1st->next);
    TEST_ASSERT_EQUAL_UINT16((EOS_SIZE_HEAP - sizeof(eos_block_t)), block_1st->size);

    for (int i = 0; i < EOS_HEAP_TEST_TIMES; i ++) {
        eos_u32_t size = ((i + 100) % 10000) + 1;
        eos_u32_t size_adjust = (size % 4 == 0) ? size : (size + 4 - (size % 4));
        p_data = eos_heap_malloc(&heap, size);
        eos_event_inner_t *e = (eos_event_inner_t *)p_data;
        eos_u8_t priority = (size % EOS_MAX_ACTORS);
        e->sub = (1 << priority);
        eos_event_inner_t *e_block = (eos_event_inner_t *)eos_heap_get_block(&heap, priority);
        TEST_ASSERT_EQUAL_POINTER(e, e_block);
        TEST_ASSERT_EQUAL_UINT32(0, heap.error_id);
        TEST_ASSERT(p_data != NULL);
        block_1st = (eos_block_t *)heap.data;
        TEST_ASSERT_EQUAL_UINT32(block_1st->size, size_adjust);
        eos_block_t * block_next = (eos_block_t *)(heap.data + block_1st->next);
        TEST_ASSERT(block_next != NULL);
        TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, block_next->next);

        eos_heap_gc(&heap, p_data);
        TEST_ASSERT_EQUAL_UINT32(0, heap.error_id);
        TEST_ASSERT_EQUAL_POINTER(p_data, (eos_pointer_t)block_1st + (eos_pointer_t)sizeof(eos_block_t));
        TEST_ASSERT(block_1st->next == EOS_HEAP_MAX);
        TEST_ASSERT_EQUAL_UINT32(block_1st->size, EOS_SIZE_HEAP - sizeof(eos_block_t));
    }

    // malloc大小
    uint32_t size_malloc[] = {
        128, 256, 32, 1024, 64, 16, 32, 16, 512, 32
    };
    eos_event_inner_t * data_ptr[10] = {0};

    uint32_t squen_free[10] = {
        8, 1, 0, 4, 5, 2, 6, 9, 3, 7
    };

    uint16_t first_next[10] = {
        size_malloc[0] + sizeof(eos_block_t),
        size_malloc[0] + sizeof(eos_block_t),
        size_malloc[0] + size_malloc[1] + (2 * sizeof(eos_block_t)),
        size_malloc[0] + size_malloc[1] + (2 * sizeof(eos_block_t)),
        size_malloc[0] + size_malloc[1] + (2 * sizeof(eos_block_t)),
        size_malloc[0] + size_malloc[1] + size_malloc[2] + (3 * sizeof(eos_block_t)),
        size_malloc[0] + size_malloc[1] + size_malloc[2] + (3 * sizeof(eos_block_t)),
        size_malloc[0] + size_malloc[1] + size_malloc[2] + (3 * sizeof(eos_block_t)),
        size_malloc[0] + size_malloc[1] + size_malloc[2] + size_malloc[3] +
        size_malloc[4] + size_malloc[5] + size_malloc[6] + (7 * sizeof(eos_block_t)),
        EOS_HEAP_MAX
    };

    block_1st = (eos_block_t *)heap.data;
    for (int m = 0; m < ((EOS_HEAP_TEST_TIMES >= 100) ? 100 : EOS_HEAP_TEST_TIMES); m ++) {
        uint16_t next_1st = 0;
        for (int i = 0; i < 10; i ++) {
            data_ptr[i] = eos_heap_malloc(&heap, size_malloc[i]);
            data_ptr[i]->sub = 0;
            TEST_ASSERT(data_ptr[i] != NULL);
            TEST_ASSERT_EQUAL_UINT16(0, heap.error_id);
            print_heap_list(&heap, i);
        }
        for (int i = 0; i < 10; i ++) {
            eos_heap_gc(&heap, data_ptr[squen_free[i]]);
            TEST_ASSERT_EQUAL_UINT16(0, heap.error_id);
            print_heap_list(&heap, i);

            TEST_ASSERT_EQUAL_UINT16((first_next[i] - sizeof(eos_block_t)), block_1st->size);
            TEST_ASSERT_EQUAL_UINT16(first_next[i], block_1st->next);
        }

        TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, block_1st->next);
        TEST_ASSERT_EQUAL_UINT16((EOS_SIZE_HEAP - sizeof(eos_block_t)), block_1st->size);
    }

    block_1st = (eos_block_t *)heap.data;
    TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, heap.queue);
    TEST_ASSERT_EQUAL_UINT8(1, heap.empty);
    TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, block_1st->next);
    TEST_ASSERT_EQUAL_UINT16((EOS_SIZE_HEAP - sizeof(eos_block_t)), block_1st->size);
    TEST_ASSERT_EQUAL_UINT16(0, heap.count);
    printf("\n");

    /* random test */
    #define HEAP_TETS_MALLOC_QUEUE_SIZE         1024
    eos_event_inner_t * malloc_data[HEAP_TETS_MALLOC_QUEUE_SIZE];
    int malloc_head = 0;
    int malloc_tail = 0;
    int malloc_size = 0;
    int count_malloc = 0;
    int count_free = 0;

    srand(time(0));

    while (count_free < EOS_HEAP_TEST_TIMES) {
        int size = rand() % 256;
        if (size % 2 == 1 && size != 0 && size >= 16) {
            if (count_malloc < EOS_HEAP_TEST_TIMES) {
                malloc_data[malloc_head] = eos_heap_malloc(&heap, size);
                malloc_data[malloc_head]->sub = 0;
                TEST_ASSERT_EQUAL_UINT32(0, heap.error_id);
                count_malloc ++;
                malloc_head = ((malloc_head + 1) % HEAP_TETS_MALLOC_QUEUE_SIZE);
                malloc_size ++;
                TEST_ASSERT(malloc_size <= HEAP_TETS_MALLOC_QUEUE_SIZE);
#if (EOS_HEAP_TEST_PRINT_EN != 0)
                printf("\033[1;31mmalloc: \033[0m");
#else
                if (((count_malloc + 1) % EOS_HEAP_TEST_PRINT_UNIT) == 0)
                    printf("malloc times: %u.\n", (count_malloc + 1));
#endif
                print_heap_list(&heap, count_malloc);
            }
        }
        else {
            if (count_free < count_malloc) {
#if (EOS_HEAP_TEST_PRINT_EN != 0)
                printf("\033[1;33mfree:   \033[0m");
#else
                if (((count_free + 1) % EOS_HEAP_TEST_PRINT_UNIT) == 0)
                    printf("free times: %u.\n", (count_free + 1));
#endif
                eos_heap_gc(&heap, malloc_data[malloc_tail]);
                TEST_ASSERT_EQUAL_UINT32(0, heap.error_id);
                count_free ++;
                malloc_tail = ((malloc_tail + 1) % HEAP_TETS_MALLOC_QUEUE_SIZE);
                malloc_size --;
                TEST_ASSERT(malloc_size >= 0);
                print_heap_list(&heap, count_free);
            }
        }
    }

    block_1st = (eos_block_t *)heap.data;
    TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, heap.queue);
    TEST_ASSERT_EQUAL_UINT8(1, heap.empty);
    TEST_ASSERT_EQUAL_UINT16(EOS_HEAP_MAX, block_1st->next);
    TEST_ASSERT_EQUAL_UINT16((EOS_SIZE_HEAP - sizeof(eos_block_t)), block_1st->size);
    TEST_ASSERT_EQUAL_UINT16(0, heap.count);
}

static void print_heap_list(eos_heap_t * const me, eos_u32_t index)
{
#if (EOS_HEAP_TEST_PRINT_EN == 0)
    (void)me;
#else
    printf("table %6u: ", index);
    eos_block_t * block = (eos_block_t *)me->data;
    eos_u16_t next = 0;
    do {
        block = (eos_block_t *)(me->data + next);
        if (block->free == 1)
            printf("\033[1;32m%u, \033[0m", block->size);
        else
            printf("%u, ", block->size);
        
        next = block->next;
    } while (next != EOS_HEAP_MAX);
    printf("\n");
#endif
}