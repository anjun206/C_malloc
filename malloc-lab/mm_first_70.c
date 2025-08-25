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
#define MIN_BLOCK (2*DSIZE)

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* 
 * size = 블록의 전체 크기
 * flags = 하위 비트 플래그로 사용
 * alloc = 할당 상태 비트                0x1
 * prev_alloc = 직전 블록 할당 상태 비트  0x2
 * p = 일반 포인터 (헤더/푸터 가리킬 때 씀)
 * bp = 블록 포인터 (페이로드의 시작 주소)
 */
#define PACK(size, flags) ((size) | (flags))

#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)       (GET(p) & ~0x7)
#define GET_ALLOC(p)      (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) (GET(p) & 0x2)

#define HDRP(bp)          ((char *)(bp) - WSIZE)
#define FTRP(bp)          ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp)     ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)     ((char *)(bp) - GET_SIZE(((char *)(bp)) - DSIZE))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* 
 * PACK = alloc에 0또는 1 넣어서 하위 3비트에 함께저장
 * GET,PUT = p에서 WSIZE 읽기/쓰기
 * GET_SIZE,ALLOC = 헤더/푸터의 워드 값에서 크기와 할당 비트만 추출
 * HDRP = 현재 블록의 헤더 주소
 * FTRP = 현재 블록의 푸터 주소
 * NEXT_BLKP = 다음 블록의 bp
 * PREV_BLKP = 이전 블록의 bp (이전 블록 할당이면 불가)
 * ALIGN = 임의 크기 size를 8바이트 배수에 맞추기 [예) ALIGN(13) = 16]
 * SIZE_T_SIZE = ?
 */


// 지금 당장은 필요없는 것들 오히려 손해
// static char *mem_heap;        /* 힙 영역 첫번째 바이트 포인터 */
// static char *mem_brk;         /* 힙영역 마지막+1 바이트 포인터 */
// static char *mem_max_addr;    /* 사용 가능 최대 합법 주소+1 바이트 포인터 */
//                               /* heap_lo <= 사용가능 합법 주소 < mem_max_addr */



// mm.c top (매크로, 전역 변수들 다음)
static void *heap_listp;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void  place(void *bp, size_t asize);

/*
 * mm_init - initialize the malloc package.
 *
 * 힙을 빈 상태로 초기화
 * 
 * 실패 시 음수반환, 성공시 0
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp + (0*WSIZE), 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));  // 프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));  // 프롤로그 푸터
                                                // 실제 힙 확장하고 사용자 메모리 들어갈 곳
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));      // 에필로그 헤더 (힙 영역 마무리)
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * 힙 익스텐드  ㄱㄱ
 * 사이즈... 어디서 떼오더라?
 * 
 */

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

// init 할때랑 동일
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

// 병합
return coalesce(bp);
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 * 
 * 1. 요청 크기 보정   ✓
 * 2. 적합 블록 탐색   ✓
 * 3. 배치            ✓
 * 4. 힙 확장 (필요시) ✓
 *      
 * asize 계산식 수정
 * asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
 * ↓  ↓  ↓  ↓  ↓
 * if (size <= DSIZE) asize = 2*DSIZE;
 * 최소 16B
 * else asize = DSIZE * ((size + (DSIZE-1)) / DSIZE) + 2*WSIZE;
 * +헤더/푸터
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE-1)) / DSIZE) + 2*WSIZE;

    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    // 힙 익스텐드하고 재할당
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/* 
 * 흐음...
 * 아쉽게도 find_fit은 직접 구현하라고 한다
 * 난 나중에 세그뭐시기 리스트로 구현할거기에
 * firstfit 사용할거다
 * 탐색 완료
 */
static void *find_fit(size_t asize) {
    void *bp;
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= asize)) {
            return bp;
        }
    }
return NULL;
}


/* 
 * 으음...
 * place도 직접 구현하는 거였다
 * 
 * 요청 블록을 가용 블록의 시작 부분에 배치해야 한다
 * 나머지 부분의 크기가 최소블록의 크기와 같거나 크면 분할한다
 * 배치 어렵네...
 * 
 * GET_SIZE는 블록 포인터에서 블록헤더 포인터로 수정
 * if/else문으로 따로 분리해서 분할 가정
 * csize로 블록 크기쓰거나
 * asize와 remainder로 분할해서 asize만 쓰기
 * 
 * ++ 최적화를 위해 가용에만 푸터
 */
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));
    size_t remainder = csize - asize;
    if (remainder >= MIN_BLOCK) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        char *nbp = NEXT_BLKP(bp);
        PUT(HDRP(nbp), PACK(remainder, 0));
        PUT(FTRP(nbp), PACK(remainder, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
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
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/* 
 * 병합도 구현
 * coalesce = 인접 가용 블록 합쳐 큰 가용블록으로 변환
 */

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case 1   앞뒤 둘다 가용 불가
    if (prev_alloc && next_alloc) {
        return bp;
    }

    // case 2  다음 블록 가용가능
        else if (prev_alloc && !next_alloc) {
            size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(bp), PACK(size, 0));
            PUT(FTRP(bp), PACK(size, 0));
        }

    // case 3  이전 블록 가용가능
        else if (!prev_alloc && next_alloc) {
            size += GET_SIZE(HDRP(PREV_BLKP(bp)));
            PUT(FTRP(bp), PACK(size, 0));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
        }

    // case 4  앞뒤 둘다 가용 가능
        else {
            size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                    GET_SIZE(HDRP(NEXT_BLKP(bp)));
            PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
            bp = PREV_BLKP(bp);
    }
    return bp;
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
    if (ptr == NULL) return mm_malloc(size);
    if (size == 0) { mm_free(ptr); return NULL; }

    size_t oldsize = GET_SIZE(HDRP(ptr));
    size_t payload = oldsize - 2*WSIZE;

    size_t asize;
    if (size <= DSIZE) asize = 2*DSIZE;
    else asize = DSIZE * ((size + (DSIZE-1)) / DSIZE) + 2*WSIZE;

    if (asize <= oldsize) {
        return ptr;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL) return NULL;

    size_t copySize = (size < payload) ? size : payload;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}