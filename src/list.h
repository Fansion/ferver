#ifndef __FV_LIST_H__
#define __FV_LIST_H__

#ifndef NULL
#define NULL 0
#endif

/*liste head, refer to linux kernel implementation*/
typedef struct list_head {
    struct list_head *pre, *next;
} list_head;

#define INIT_LIST_HEAD(ptr) do {                        \
    list_head *_ptr = (list_head *)ptr;   \
    (_ptr)->next = (_ptr); (_ptr)->pre = (_ptr);        \
} while (0)

/*
 * Insert a new entry to two consecutive entries
 */
static inline void __list_add(list_head *_new, list_head *pre, list_head *next)
{
    _new->next = next;
    next->pre = _new;
    pre->next = _new;
    _new->pre = pre;
};

/*
 * insert at beginning after head node
 */
static inline void list_add(list_head *_new, list_head *head)
{
    __list_add(_new, head, head->next);
}

/*
 * to be modifeid
 */
static inline void list_add_tail(list_head *_new, list_head *head)
{
    __list_add(_new, head, head->next);
}

/*
 * delete an entry of two consecutive entries
 * actually combine two list_head
 */
static inline void __list_del(list_head *pre, list_head *next)
{
    pre->next = next;
    next->pre = pre;
}

static inline void list_del(list_head *entry)
{
    __list_del(entry->pre, entry->next);
    // here should free entry, but for the use of function del_node in list_test.c free is commented
    // free(entry);
}

/*
 * check whether the list is empty
 */
static inline int list_empty(list_head *head)
{
    return (head->next == head) && (head->pre == head);
}


#define offsetof(TYPE, MEMBER)      ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type, member) );    \
})
#define list_entry(ptr, type, member)                       \
    container_of(ptr, type, member)
#define list_for_each(pos, head)                            \
    for (pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_prev(pos, head)                       \
    for (pos = (head)->pre; pos != (head); pos = pos->pre)

#endif
