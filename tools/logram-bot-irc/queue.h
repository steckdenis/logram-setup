#ifndef __SYS_QUEUE_H
#define __SYS_QUEUE_H

#define SLIST_HEAD(name,type)        \
struct name {                        \
	struct type *slh_first;          \
}

#define SLIST_HEAD_INITIALIZER(head) \
	{ NULL }

#define SLIST_ENTRY(type)            \
struct {                             \
	struct type *sle_next;           \
}

#define SLIST_EMPTY(head)        ((head)->slh_first == NULL)
#define SLIST_FIRST(head)        ((head)->slh_first)

#define SLIST_INSERT_HEAD(head,elm,field)            \
do {                                                 \
	SLIST_NEXT((elm),field) = SLIST_FIRST((head));   \
	SLIST_FIRST((head)) = (elm);                     \
} while (/* CONSTCOND */ 0)

#define SLIST_NEXT(elm,field)    ((elm)->field.sle_next)

#define SLIST_REMOVE_HEAD(head,field) do {                        \
	SLIST_FIRST((head)) = SLIST_NEXT(SLIST_FIRST((head)), field); \
} while (/* CONSTCOND */ 0)

#endif /* __SYS_QUEUE_H */
