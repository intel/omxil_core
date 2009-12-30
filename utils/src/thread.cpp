/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#include <pthread.h>
#include <thread.h>

Thread::Thread()
{
    r = NULL;
    created = false;

    pthread_mutex_init(&lock, NULL);
}

Thread::Thread(RunnableInterface *r)
{
    this->r = r;
    created = false;

    pthread_mutex_init(&lock, NULL);
}

Thread::~Thread()
{
    Join();

    pthread_mutex_destroy(&lock);
}

int Thread::Start(void)
{
    int ret = 0;

    pthread_mutex_lock(&lock);
    if (!created) {
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        ret = pthread_create(&id, &attr, Instance, this);
        if (!ret)
            created = true;
    }
    pthread_mutex_unlock(&lock);

    return ret;
}

int Thread::Join(void)
{
    int ret = 0;

    pthread_mutex_lock(&lock);
    if (created) {
        ret = pthread_join(id, NULL);
        created = false;
    }
    pthread_mutex_unlock(&lock);

    return ret;
}

void *Thread::Instance(void *p)
{
    Thread *t = static_cast<Thread *>(p);

    t->Run();

    return NULL;
}

void Thread::Run(void)
{
    if (r)
        r->Run();
    else
        return;
}
