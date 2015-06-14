#include "list.h"
#include "dbg.h"

typedef struct {
    void *arg;
    list_head list;
} st_header;

typedef struct {
    void *arg1;
    void *arg2;
    void *arg3;
    list_head list;
} st_struct;

void add_node(void *arg, list_head *head)
{
    st_struct *s = (st_struct *)malloc(sizeof(st_struct));
    s->arg1 = arg;
    s->arg2 = arg;
    s->arg3 = arg;
    list_add(&s->list, head);
}

void del_node(list_head *entry)
{
    list_del(entry);                                    // 删除当前结点
    st_struct *s = list_entry(entry, st_struct, list);
    printf("del node%d\n", (int)s->arg1);
    free(s);
}

void display(list_head *head)
{
    list_head *pos;
    st_struct *et;
    list_for_each(pos, head) {                          // 遍历双链表的每一个结点
        et = list_entry(pos, st_struct, list);          // 将每一个结点强制转化成原有类型st_struct
        printf("arg = %d\n", (int)et->arg1);            // 访问st_struct的其他属性
    }
}

int main()
{
    st_header st_h;
    INIT_LIST_HEAD(&st_h.list);

    int isEmpty = list_empty(&st_h.list);
    check(isEmpty == 1, "list should be empty, isEmpty = %d", isEmpty);

    int i;
    for (i = 0; i < 100; ++i)
    {
        add_node((void *)i, &st_h.list);
    }

    isEmpty = list_empty(&st_h.list);
    check(isEmpty == 0, "list should not be empty, isEmpty = %d", isEmpty);

    printf("before del:\n");
    display(&st_h.list);

    for (i = 0; i < 32; ++i)
    {
        del_node(st_h.list.next);
    }
    printf("after del:\n");
    display(&st_h.list);

    return 0;
}
