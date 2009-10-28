/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __QUEUE_H
#define __QUEUE_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct queue {
	struct list *head;
	struct list *tail;
	int  length;
};

void __queue_init(struct queue *queue);
struct queue *queue_alloc(void);
/* FIXME */
inline void __queue_free(struct queue *queue);
/* FIXME */
void queue_free_all(struct queue *queue);

void __queue_push_head(struct queue *queue, struct list *entry);
int queue_push_head(struct queue *queue, void *data);
void __queue_push_tail(struct queue *queue, struct list *entry);
int queue_push_tail(struct queue *queue, void *data);

struct list *__queue_pop_head(struct queue *queue);
void *queue_pop_head(struct queue *queue);
struct list *__queue_pop_tail(struct queue *queue);
void *queue_pop_tail(struct queue *queue);

inline struct list *__queue_peek_head(struct queue *queue);
inline struct list *__queue_peek_tail(struct queue *queue);
inline void *queue_peek_head(struct queue *queue);
inline void *queue_peek_tail(struct queue *queue);

int queue_length(struct queue *queue);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __QUEUE_H */
