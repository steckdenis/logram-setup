#include "local.h"

static SLIST_HEAD (stack, stack_t) mstack = SLIST_HEAD_INITIALIZER (&mstack);

void *xmalloc (size_t size)
{
	struct stack_t *el;
	void *ret;

	if (!(ret = malloc (size)))
		return NULL;

	el = malloc (sizeof (struct stack_t));
	el->adr = ret;
	SLIST_INSERT_HEAD (&mstack, el, next);

	return ret;
}

void stack_delete (void)
{
	struct stack_t *el;

	while (!SLIST_EMPTY (&mstack))
	{
		el = SLIST_FIRST (&mstack);
		SLIST_REMOVE_HEAD (&mstack, next);
		free (el->adr);
		free (el);
	}
}
