#include "vm/frame.h"

/* Lock for frame table. */
struct lock frameLock;

/* Frame table. */
struct list frameTable;

/* Init frame */
void
init_frame ()
{
	list_init(&frameTable);
	lock_init(&frameLock);
}
