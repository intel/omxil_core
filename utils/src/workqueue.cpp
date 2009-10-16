/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#include <workqueue.h>

WorkQueue::WorkQueue()
{
    stop = false;
    works = NULL;

    pthread_mutex_init(&wlock, NULL);
    pthread_cond_init(&wcond, NULL);
}

WorkQueue::~WorkQueue()
{
    StopWork();

    pthread_mutex_lock(&wlock);
    while (works)
        works = __list_delete(works, works);
    pthread_mutex_unlock(&wlock);

    pthread_cond_destroy(&wcond);
    pthread_mutex_destroy(&wlock);
}

int WorkQueue::StartWork(void)
{
    return Start();
}

void WorkQueue::StopWork(void)
{
    FlushWork();

    pthread_mutex_lock(&wlock);
    stop = true;
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);

    Join();
}

void WorkQueue::Run(void)
{
    while (!stop) {
        pthread_mutex_lock(&wlock);
        /*
         * sleeps until works're available.
         * wokeup by ScheduleWork() or FlushWork() or ~WorkQueue()
         */
        if (!works)
            pthread_cond_wait(&wcond, &wlock);

        while (works) {
            struct list *entry = works;
            WorkableInterface *wi =
                static_cast<WorkableInterface *>(entry->data);

            works = __list_delete(works, entry);
            pthread_mutex_unlock(&wlock);
            DoWork(wi);
            pthread_mutex_lock(&wlock);
        }
        pthread_mutex_unlock(&wlock);
    }
}

void WorkQueue::DoWork(WorkableInterface *wi)
{
    if (wi)
        wi->Work();
}

void WorkQueue::Work(void)
{
    return;
}

void WorkQueue::ScheduleWork(void)
{
    pthread_mutex_lock(&wlock);
    works = list_add_tail(works, static_cast<WorkableInterface *>(this));
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);
}

void WorkQueue::ScheduleWork(WorkableInterface *wi)
{
    pthread_mutex_lock(&wlock);
    if (wi)
        works = list_add_tail(works, wi);
    else
        works = list_add_tail(works, static_cast<WorkableInterface *>(this));
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);
}

void WorkQueue::CancelScheduledWork(WorkableInterface *wi)
{
    pthread_mutex_lock(&wlock);
    works = list_delete_all(works, wi);
    pthread_mutex_unlock(&wlock);
}

void WorkQueue::FlushWork(void)
{
    FlushBarrier fb;
    bool needtowait = false;

    pthread_mutex_lock(&wlock);
    if (works) {
        list_add_tail(works, &fb);
        pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */

        needtowait = true;
    }
    pthread_mutex_unlock(&wlock);

    if (needtowait)
        fb.WaitCompletion(); /* wokeup by FlushWork::Work() */
}

WorkQueue::FlushBarrier::FlushBarrier()
{
    pthread_mutex_init(&cplock, NULL);
    pthread_cond_init(&complete, NULL);
}

WorkQueue::FlushBarrier::~FlushBarrier()
{
    pthread_cond_destroy(&complete);
    pthread_mutex_destroy(&cplock);
}

void WorkQueue::FlushBarrier::WaitCompletion(void)
{
    pthread_mutex_lock(&cplock);
    pthread_cond_wait(&complete, &cplock);
    pthread_mutex_unlock(&cplock);
}

void WorkQueue::FlushBarrier::Work(void)
{
    pthread_mutex_lock(&cplock);
    pthread_cond_signal(&complete); /* wakeup WaitCompletion() */
    pthread_mutex_unlock(&cplock);
}
