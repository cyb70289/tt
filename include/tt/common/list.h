/* Circular double linked list
 *
 * Copyright(C) 2014 Yibo Cai
 */
#pragma once

struct tt_list
{
	struct tt_list *prev;
	struct tt_list *next;
};

/* Initialize and empty list header */
static inline void tt_list_init(struct tt_list *list)
{
	list->next = list->prev = list;
}

/* Add a new entry between two known consecutive entries */
static inline void __tt_list_add(struct tt_list *entry,
		struct tt_list *prev, struct tt_list *next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

/* Insert to head */
static inline void tt_list_insert(struct tt_list *entry, struct tt_list *head)
{
	__tt_list_add(entry, head, head->next);
}

/* Append to tail */
static inline void tt_list_add(struct tt_list *entry, struct tt_list *head)
{
	__tt_list_add(entry, head->prev, head);
}

/* Delete a list entry between two consecutive entries */
static inline void __tt_list_del(struct tt_list *prev, struct tt_list *next)
{
	next->prev = prev;
	prev->next = next;
}

/* Delete a list entry */
static inline void tt_list_del(struct tt_list *entry)
{
	__tt_list_del(entry->prev, entry->next);
	entry->next = entry->prev = 0;
}

/* Move a list entry from one list to another list's head */
static inline void tt_list_move_head(struct tt_list *entry,
		struct tt_list *head)
{
	__tt_list_del(entry->prev, entry->next);
	tt_list_insert(entry, head);
}

/* Move a list entry from one list to another list's tail */
static inline void tt_list_move(struct tt_list *entry, struct tt_list *head)
{
	__tt_list_del(entry->prev, entry->next);
	tt_list_add(entry, head);
}

/* Test if a list is empty */
static inline int tt_list_isempty(const struct tt_list *head)
{
	return head->next == head;
}

/* Iterate over a list */
#define tt_list_for_each(pos, head)	\
	for (pos = (head)->next; pos != (head); pos = pos->next)

/* Iterate over a list in reverse order */
#define	tt_list_for_each_prev(pos, head)	\
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/* Iterate over a list, safe against removal of list entry */
#define tt_list_for_each_safe(pos, n, head)	\
	for (pos = (head)->next, n = pos->next; pos != (head);	\
			pos = n, n = pos->next)
