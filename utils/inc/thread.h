/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __THREAD_H
#define __THREAD_H

#include <pthread.h>

class RunnableInterface {
public:
    RunnableInterface() {};
    virtual ~RunnableInterface() {};

    virtual void Run(void) = 0;
};

class Thread : public RunnableInterface {
public:
    Thread();
    Thread(RunnableInterface *r);
    ~Thread();

    int Start(void);
    int Join(void);

protected:
    /*
     * overriden by the derived class
     * when the class is derived from Thread class
     */
    virtual void Run(void);

private:
    static void *Instance(void *);

    RunnableInterface *r;
    pthread_t id;
    bool created;

    pthread_mutex_t lock;
};

#endif /* __THREAD_H */
