#ifndef _LIST_H
#define _LIST_H

#include <stddef.h>

/**
 * Doubly linked list structure, similar to list_head in the Linux kernel
 */
struct list_head {
  struct list_head *next, *prev;
};

/**
 * Initialize list head
 */
#define LIST_HEAD_INIT(name) {&(name), &(name)}

/**
 * Declare and initialize a list_head
 */
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/**
 * Initialize a list_head structure
 */
static inline void INIT_LIST_HEAD(struct list_head *list) {
  list->next = list;
  list->prev = list;
}

/**
 * Add new node after the head
 */
static inline void list_add(struct list_head *new_head,
                            struct list_head *head) {
  new_head->next = head->next;
  new_head->prev = head;
  head->next->prev = new_head;
  head->next = new_head;
}

/**
 * Add new node before the head
 */
static inline void list_add_tail(struct list_head *new_head,
                                 struct list_head *head) {
  new_head->next = head;
  new_head->prev = head->prev;
  head->prev->next = new_head;
  head->prev = new_head;
}

/**
 * Remove an entry from the list
 */
static inline void list_del(struct list_head *entry) {
  entry->next->prev = entry->prev;
  entry->prev->next = entry->next;
  entry->next = NULL;
  entry->prev = NULL;
}

/**
 * Check if the list is empty
 */
#define list_empty(head) ((head)->next == (head))

/**
 * Iterate through the list
 */
#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * Iterate through the list and retrieve the data structure
 */
#define list_entry(ptr, type, member) \
  ((type *)((char *)(ptr) - offsetof(type, member)))

/**
 * Iterate through all elements in the list
 */
#define list_for_each_entry(pos, head, member)               \
  for (pos = list_entry((head)->next, typeof(*pos), member); \
       &pos->member != (head);                               \
       pos = list_entry(pos->member.next, typeof(*pos), member))

/**
 * Iterate through all elements in the list, safe against removal
 */
#define list_for_each_safe(pos, n, head) \
  for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/**
 * Get the first element in the list
 */
#define list_head_entry(head, type, member) \
  list_entry((head)->next, type, member)

/**
 * Get the last element in the list
 */
#define list_tail_entry(head, type, member) \
  list_entry((head)->prev, type, member)

/**
 * Get the element at the given index
 */
#define list_nth_entry(pos, head, n, member)   \
  do {                                         \
    struct list_head *p = (head)->next;        \
    for (int i = 0; i < n; i++) {              \
      p = p->next;                             \
    }                                          \
    pos = list_entry(p, typeof(*pos), member); \
  } while (0)

/**
 * Get list length
 */
static inline int list_len(const struct list_head *head) {
  int len = 0;
  struct list_head *pos;
  list_for_each(pos, head) { len++; }
  return len;
}
#endif  // _LIST_H