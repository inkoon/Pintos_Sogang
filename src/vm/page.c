#include "vm/page.h"
#include <stdio.h>
#include <stdbool.h>
#include <hash.h>
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "threads/pte.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"

bool
stack_grow(void *fault_addr)
{
	bool writeable = true;
	size_t page_size = (size_t)(PHYS_BASE - pg_round_down(fault_addr));
	int i;
	if(page_size > (1<<23))
			return false;
	page_size = page_size / PGSIZE - 1;
	for(i = 0; page_size--; i += PGSIZE){
		if(pagedir_get_page(thread_current()->pagedir,
								pg_round_down(fault_addr) + i))
			continue;
		void *new = palloc_get_page(PAL_USER | PAL_ZERO);

		if(!new || !pagedir_set_page(thread_current()->pagedir,
								pg_round_down(fault_addr) + i, new, true));
			return false;
	}
}
