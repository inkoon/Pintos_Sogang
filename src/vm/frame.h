#ifdef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/thread.h"
#include "vm/page.h"

struct frameEntry
{
	void *paddr;						// Physical address.
	struct thread *t;				// Thread using this frame.
	struct list_elem elem;	// List elem for frame table.
};

#endif
