#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (head)
        INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *head)
{
    if (!head)
        return;

    struct list_head *cur = NULL, *safe = NULL;
    list_for_each_safe (cur, safe, head)
        q_release_element(container_of(cur, element_t, list));
    free(head);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *element = malloc(sizeof(element_t));
    if (!element)
        return false;
    element->value = strdup(s);
    if (!element->value) {
        free(element);
        return false;
    }
    list_add(&element->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    return q_insert_head(head->prev, s);
}

/* Remove an element from queue */
static inline element_t *q_remove(struct list_head *head,
                                  char *sp,
                                  size_t bufsize,
                                  struct list_head *rm_node)
{
    element_t *ele = list_entry(rm_node, element_t, list);
    if (sp && ele->value) {
        strncpy(sp, ele->value, bufsize);
        sp[bufsize - 1] = 0;
    }
    list_del(rm_node);
    return ele;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (head == NULL || list_empty(head))
        return NULL;

    return q_remove(head, sp, bufsize, head->next);
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    return q_remove(head, sp, bufsize, head->prev);
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;

    size_t size = 0;
    struct list_head *p = NULL;
    list_for_each (p, head)
        ++size;
    return size;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head *slow, *fast;
    slow = fast = head->next;
    for (; fast != head && (fast = fast->next) != head; fast = fast->next)
        slow = slow->next;
    element_t *element = list_entry(slow, element_t, list);
    list_del(&element->list);
    q_release_element(element);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head || list_empty(head))
        return false;

    struct list_head *node = NULL, *safe = NULL;
    bool last_dup = false;
    list_for_each_safe (node, safe, head) {
        element_t *cur = list_entry(node, element_t, list);
        const bool match =
            node->next != head &&
            !strcmp(cur->value, list_entry(node->next, element_t, list)->value);
        if (match || last_dup) {
            list_del(node);
            q_release_element(cur);
        }
        last_dup = match;
    }

    return true;
}

static inline void swap_pair(struct list_head **head)
{
    struct list_head *first = *head, *second = first->next;
    first->next = second->next;
    second->next = first;
    second->prev = first->prev;
    first->prev = second;
    *head = second;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head)
        return;
    for (struct list_head **it = &head->next;
         *it != head && (*it)->next != head; it = &(*it)->next->next)
        swap_pair(it);
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    struct list_head *it, *safe;
    list_for_each_safe (it, safe, head)
        list_move(it, head);
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
    if (!head || list_empty(head))
        return;
    struct list_head *it, *safe, *cut;
    int count = k;
    cut = head;
    list_for_each_safe (it, safe, head) {
        if (--count)
            continue;
        LIST_HEAD(tmp);
        count = k;
        list_cut_position(&tmp, cut, it);
        q_reverse(&tmp);
        list_splice(&tmp, cut);
        cut = safe->prev;
    }
}

static int q_merge_two(struct list_head *first, struct list_head *second)
{
    if (!first || !second)
        return 0;

    int count = 0;
    LIST_HEAD(tmp);
    while (!list_empty(first) && !list_empty(second)) {
        element_t *f = list_first_entry(first, element_t, list);
        element_t *s = list_first_entry(second, element_t, list);
        if (strcmp(f->value, s->value) <= 0)
            list_move_tail(&f->list, &tmp);
        else
            list_move_tail(&s->list, &tmp);
        count++;
    }
    count += q_size(first) + q_size(second);
    list_splice(&tmp, first);
    list_splice_tail_init(second, first);

    return count;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    /* Try to use merge sort*/
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    /* Find middle point */
    struct list_head *mid, *left, *right;
    left = right = head;
    do {
        left = left->next;
        right = right->prev;
    } while (left != right && left->next != right);
    mid = left;

    /* Divide into two part */
    LIST_HEAD(second);
    list_cut_position(&second, mid, head->prev);

    /* Conquer */
    q_sort(head, descend);
    q_sort(&second, descend);

    /* Merge */
    q_merge_two(head, &second);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    element_t *entry = NULL;
    list_for_each_entry (entry, head, list) {
        struct list_head *prev = entry->list.prev, *safe = prev->prev;
        for (; prev != head; prev = safe, safe = safe->prev) {
            element_t *prev_entry = list_entry(prev, element_t, list);
            if (strcmp(prev_entry->value, entry->value) <= 0)
                break;
            list_del(prev);
            q_release_element(prev_entry);
        }
    }
    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    element_t *entry = NULL;
    list_for_each_entry (entry, head, list) {
        struct list_head *prev = entry->list.prev, *safe = prev->prev;
        for (; prev != head; prev = safe, safe = safe->prev) {
            element_t *prev_entry = list_entry(prev, element_t, list);
            if (strcmp(prev_entry->value, entry->value) >= 0)
                break;
            list_del(prev);
            q_release_element(prev_entry);
        }
    }
    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head || list_empty(head))
        return 0;

    if (list_is_singular(head))
        return list_first_entry(head, queue_contex_t, chain)->size;

    /* Use 2-merge algorithm in https://arxiv.org/pdf/1801.04641.pdf */
    LIST_HEAD(pending);
    LIST_HEAD(empty);
    int n = 0;
    while (!list_empty(head)) {
        list_move(head->next, &pending);
        n++;

        while (n > 3) {
            queue_contex_t *x, *y, *z;
            x = list_first_entry(&pending, queue_contex_t, chain);
            y = list_first_entry(&x->chain, queue_contex_t, chain);
            z = list_first_entry(&x->chain, queue_contex_t, chain);

            if (y->size >= z->size << 1)
                break;

            if (x->size < z->size) {
                y->size = q_merge_two(y->q, x->q);
                list_move(&x->chain, &empty);
                n--;
            } else {
                z->size = q_merge_two(z->q, y->q);
                list_move(&y->chain, &empty);
                n--;
            }
        }
    }

    /* Merge remaining list */
    while (n > 1) {
        queue_contex_t *x, *y;
        x = list_first_entry(&pending, queue_contex_t, chain);
        y = list_first_entry(&x->chain, queue_contex_t, chain);
        y->size = q_merge_two(y->q, x->q);
        list_move(&x->chain, &empty);
        n--;
    }

    /* Move the last queue and empty queue to head */
    list_splice(&empty, head);
    list_splice(&pending, head);

    return list_first_entry(head, queue_contex_t, chain)->size;
}
