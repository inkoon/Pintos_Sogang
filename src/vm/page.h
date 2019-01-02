#ifdef VM_PAGE_H
#define VM_PAGE_H


struct supEntry
{
	void *uvaddr;
	struct hash_elem elem;
}

bool stack_grow(void *fault_addr);

#endif
