/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * 
 * mm.c 파일외에는 손댈 필요 없다고 한다
 * 잘 구현되었는지는 mdriver.c 실행해서 확인하면 된다
 * memlib.c의 경우는 sbrk처럼 힙을 흉내낸거라고 한다
 * 힙을 늘릴땐 mem_sbrk등을 쓰면 된단다
 * 말곤 다 시간/사이클 측정용이니 냅두면 OK
 * 
 * memlib에서 지원해주는 함수
 * - mem_sbrk(int incr) incr만큼 증가 (단 sbrk랑 달리 양수만)
 * - mem_reset_brk 모의 힙을 시작으로 되돌림
 * - mem_heap_lo 모의 힙의 첫 바이트 주소 반환
 * - mem_heap_hi 모의 힙의 마지막 바이트 주소 반환 (확장 후 바뀜)
 * - meme_pagesize 시스템 크기 반환
 * 
 * 걍 mem_sbrk만 생각하면 될듯?
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p)        (*(unsigned int *)(p))
#define PUT(p, val)   (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)   (GET(p) & ~0x7)
#define GET_ALLOC(p)  (GET(p) & 0x1)

#define HDRP(bp)      ((char *)(bp) - WSIZE)
#define FTRP(bp)      ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)) - DSIZE))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 *
 * 힙을 빈 상태로 초기화
 * 
 * 실패 시 음수반환, 성공시 0
 */
int mm_init(void)
{

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 * 
 * 1. 요청 크기 보정
 * 2. 적합 블록 탐색
 * 3. 배치
 * 4. 힙 확장 (필요시)
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 *
 * 블록의 할당 비트 가용으로 변경
 * 인접 블록과 병합
 * 가용 리스트 삽입
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 *
 * 특수 케이스 : 포인터 NULL
 * 
 * 같은 자리 블록 축소/분할
 * 뒤 블록 가용이면 병합
 * 불가하면 새 블록 받아 복사 후 기존 블록 mm_free
 * 
 * 복사 크기는 기존 페이로드 최솟값
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}