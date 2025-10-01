// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

//changes
struct {
  struct spinlock lock;
  int count[PHYSTOP / PGSIZE];
} ref_counts;

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
// changes
void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&ref_counts.lock, "ref_counts");
  freerange(end, (void*)PHYSTOP);
}
//helperfunctions
// pa is physical address
static inline int
pa2idx(uint64 pa) {
  return (pa - KERNBASE) / PGSIZE;
}

// Helper function to increment the reference count of a physical page
void inc_ref_count(void *pa) {
  acquire(&ref_counts.lock);
  ref_counts.count[(uint64)pa / PGSIZE]++;
  release(&ref_counts.lock);
}

int get_ref_count(void *pa) {
  acquire(&ref_counts.lock);
  return ref_counts.count[(uint64)pa / PGSIZE];
  release(&ref_counts.lock);
}

// Helper function to decrement the reference count of a physical page
void dec_ref_count(void *pa) {
  acquire(&ref_counts.lock);
  ref_counts.count[(uint64)pa / PGSIZE]--;
  release(&ref_counts.lock);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&ref_counts.lock);
  if(--ref_counts.count[(uint64)pa / PGSIZE] > 0) {
    release(&ref_counts.lock);
    return;
  }
  release(&ref_counts.lock);

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&ref_counts.lock);
    ref_counts.count[(uint64)r / PGSIZE] = 1;
    release(&ref_counts.lock);
  }
  return (void*)r;
}
