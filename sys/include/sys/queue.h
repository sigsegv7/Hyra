/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(_KERNEL)
#include <stdint.h>
#include <stddef.h>
#else
#include <sys/types.h>
#endif  /* !_KERNEL */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#define _Q_INVALIDATE(a)

/*
 * Tail queue definitions.
 */
#define TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/* first element */			\
	struct type **tqh_last;	/* addr of last next element */		\
    size_t nelem;           /* Number of elements */            \
}

#define TAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).tqh_first }

#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/* next element */			\
	struct type **tqe_prev;	/* address of previous next element */	\
}

/*
 * Tail queue access methods.
 */
#define TAILQ_NELEM(head)       ((head)->nelem)
#define	TAILQ_FIRST(head)		((head)->tqh_first)
#define	TAILQ_END(head)			NULL
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#define TAILQ_LAST(head, headname)					\
	(*(((struct headname *)((head)->tqh_last))->tqh_last))
/* XXX */
#define TAILQ_PREV(elm, headname, field)				\
	(*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#define	TAILQ_EMPTY(head)						\
	(TAILQ_FIRST(head) == TAILQ_END(head))

#define TAILQ_FOREACH(var, head, field)					\
	for((var) = TAILQ_FIRST(head);					\
	  (var) != TAILQ_END(head);					\
	  (var) = TAILQ_NEXT(var, field))

#define	TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST(head);					\
	  (var) != TAILQ_END(head) &&					\
	  ((tvar) = TAILQ_NEXT(var, field), 1);			\
	  (var) = (tvar))

#define TAILQ_FOREACH_REVERSE(var, head, headname, field)		\
	for((var) = TAILQ_LAST(head, headname);				\
	  (var) != TAILQ_END(head);					\
	  (var) = TAILQ_PREV(var, headname, field))

#define	TAILQ_FOREACH_REVERSE_SAFE(var, head, headname, field, tvar)	\
	for ((var) = TAILQ_LAST(head, headname);			\
	  (var) != TAILQ_END(head) &&					\
	  ((tvar) = TAILQ_PREV(var, headname, field), 1);		\
	  (var) = (tvar))

/*
 * Tail queue functions.
 */
#define	TAILQ_INIT(head) do {						\
	(head)->tqh_first = NULL;					\
	(head)->tqh_last = &(head)->tqh_first;				\
    (head)->nelem = 0;                          \
} while (0)

#define TAILQ_INSERT_HEAD(head, elm, field) do {			\
	if (((elm)->field.tqe_next = (head)->tqh_first) != NULL)	\
		(head)->tqh_first->field.tqe_prev =			\
		  &(elm)->field.tqe_next;				\
	else								\
		(head)->tqh_last = &(elm)->field.tqe_next;		\
	(head)->tqh_first = (elm);					\
	(elm)->field.tqe_prev = &(head)->tqh_first;			\
    ++(head)->nelem;                                    \
} while (0)

#define TAILQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.tqe_next = NULL;					\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &(elm)->field.tqe_next;			\
    ++(head)->nelem;                                    \
} while (0)

#define TAILQ_INSERT_AFTER(head, listelm, elm, field) do {		\
	if (((elm)->field.tqe_next = (listelm)->field.tqe_next) != NULL)\
		(elm)->field.tqe_next->field.tqe_prev =			\
		  &(elm)->field.tqe_next;				\
	else								\
		(head)->tqh_last = &(elm)->field.tqe_next;		\
	(listelm)->field.tqe_next = (elm);				\
	(elm)->field.tqe_prev = &(listelm)->field.tqe_next;		\
    ++(head)->nelem;                                        \
} while (0)

#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
    ++(head)->nelem;                                        \
} while (0)

#define TAILQ_REMOVE(head, elm, field) do {				\
	if (((elm)->field.tqe_next) != NULL)				\
		(elm)->field.tqe_next->field.tqe_prev =			\
		  (elm)->field.tqe_prev;				\
	else								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
	*(elm)->field.tqe_prev = (elm)->field.tqe_next;			\
	_Q_INVALIDATE((elm)->field.tqe_prev);				\
	_Q_INVALIDATE((elm)->field.tqe_next);				\
    --(head)->nelem;                                    \
} while (0)

#define TAILQ_REPLACE(head, elm, elm2, field) do {			\
	if (((elm2)->field.tqe_next = (elm)->field.tqe_next) != NULL)	\
		(elm2)->field.tqe_next->field.tqe_prev =		\
		  &(elm2)->field.tqe_next;				\
	else								\
		(head)->tqh_last = &(elm2)->field.tqe_next;		\
	(elm2)->field.tqe_prev = (elm)->field.tqe_prev;			\
	*(elm2)->field.tqe_prev = (elm2);				\
	_Q_INVALIDATE((elm)->field.tqe_prev);				\
	_Q_INVALIDATE((elm)->field.tqe_next);				\
} while (0)

#define TAILQ_CONCAT(head1, head2, field) do {				\
	if (!TAILQ_EMPTY(head2)) {					\
		*(head1)->tqh_last = (head2)->tqh_first;		\
		(head2)->tqh_first->field.tqe_prev = (head1)->tqh_last;	\
		(head1)->tqh_last = (head2)->tqh_last;			\
		TAILQ_INIT((head2));					\
	}								\
} while (0)

/*
 * List definitions.
 */
#define	LIST_HEAD(name, type)						\
struct name {								\
	struct type *lh_first;	/* first element */			\
}

#define	LIST_HEAD_INITIALIZER(head)					\
	{ NULL }

#define	LIST_ENTRY(type)						\
struct {								\
	struct type *le_next;	/* next element */			\
	struct type **le_prev;	/* address of previous next element */	\
}

/*
 * List access methods.
 */
#define	LIST_FIRST(head)		((head)->lh_first)
#define	LIST_END(head)			NULL
#define	LIST_EMPTY(head)		((head)->lh_first == LIST_END(head))
#define	LIST_NEXT(elm, field)		((elm)->field.le_next)

#define	LIST_FOREACH(var, head, field)					\
	for ((var) = ((head)->lh_first);				\
	    (var) != LIST_END(head);					\
	    (var) = ((var)->field.le_next))

#define	LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = LIST_FIRST((head));				\
	    (var) != LIST_END(head) &&					\
	    ((tvar) = LIST_NEXT((var), field), 1);			\
	    (var) = (tvar))

#define	LIST_MOVE(head1, head2, field) do {				\
	LIST_INIT((head2));						\
	if (!LIST_EMPTY((head1))) {					\
		(head2)->lh_first = (head1)->lh_first;			\
		(head2)->lh_first->field.le_prev = &(head2)->lh_first;	\
		LIST_INIT((head1));					\
	}								\
} while (0)

/*
 * List functions.
 */
#if defined(QUEUEDEBUG)
#define	QUEUEDEBUG_LIST_INSERT_HEAD(head, elm, field)			\
	if ((head)->lh_first &&						\
	    (head)->lh_first->field.le_prev != &(head)->lh_first)	\
		QUEUEDEBUG_ABORT("LIST_INSERT_HEAD %p %s:%d", (head),	\
		    __FILE__, __LINE__);
#define	QUEUEDEBUG_LIST_OP(elm, field)					\
	if ((elm)->field.le_next &&					\
	    (elm)->field.le_next->field.le_prev !=			\
	    &(elm)->field.le_next)					\
		QUEUEDEBUG_ABORT("LIST_* forw %p %s:%d", (elm),		\
		    __FILE__, __LINE__);				\
	if (*(elm)->field.le_prev != (elm))				\
		QUEUEDEBUG_ABORT("LIST_* back %p %s:%d", (elm),		\
		    __FILE__, __LINE__);
#define	QUEUEDEBUG_LIST_POSTREMOVE(elm, field)				\
	(elm)->field.le_next = (void *)1L;				\
	(elm)->field.le_prev = (void *)1L;
#else
#define	QUEUEDEBUG_LIST_INSERT_HEAD(head, elm, field)
#define	QUEUEDEBUG_LIST_OP(elm, field)
#define	QUEUEDEBUG_LIST_POSTREMOVE(elm, field)
#endif

#define	LIST_INIT(head) do {						\
	(head)->lh_first = LIST_END(head);				\
} while (0)

#define	LIST_INSERT_AFTER(listelm, elm, field) do {			\
	QUEUEDEBUG_LIST_OP((listelm), field)				\
	if (((elm)->field.le_next = (listelm)->field.le_next) != 	\
	    LIST_END(head))						\
		(listelm)->field.le_next->field.le_prev =		\
		    &(elm)->field.le_next;				\
	(listelm)->field.le_next = (elm);				\
	(elm)->field.le_prev = &(listelm)->field.le_next;		\
} while (0)

#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	QUEUEDEBUG_LIST_OP((listelm), field)				\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	(elm)->field.le_next = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &(elm)->field.le_next;		\
} while (0)

#define	LIST_INSERT_HEAD(head, elm, field) do {				\
	QUEUEDEBUG_LIST_INSERT_HEAD((head), (elm), field)		\
	if (((elm)->field.le_next = (head)->lh_first) != LIST_END(head))\
		(head)->lh_first->field.le_prev = &(elm)->field.le_next;\
	(head)->lh_first = (elm);					\
	(elm)->field.le_prev = &(head)->lh_first;			\
} while (0)

#define	LIST_REMOVE(elm, field) do {					\
	QUEUEDEBUG_LIST_OP((elm), field)				\
	if ((elm)->field.le_next != NULL)				\
		(elm)->field.le_next->field.le_prev = 			\
		    (elm)->field.le_prev;				\
	*(elm)->field.le_prev = (elm)->field.le_next;			\
	QUEUEDEBUG_LIST_POSTREMOVE((elm), field)			\
} while (0)

#define LIST_REPLACE(elm, elm2, field) do {				\
	if (((elm2)->field.le_next = (elm)->field.le_next) != NULL)	\
		(elm2)->field.le_next->field.le_prev =			\
		    &(elm2)->field.le_next;				\
	(elm2)->field.le_prev = (elm)->field.le_prev;			\
	*(elm2)->field.le_prev = (elm2);				\
	QUEUEDEBUG_LIST_POSTREMOVE((elm), field)			\
} while (0)

#endif  /* _QUEUE_H_ */
