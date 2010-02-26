/*
 * workqueue.cpp, workqueue class
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <workqueue.h>

WorkQueue::WorkQueue()
{
    stop = false;
    executing = true;
    works = NULL;

    pthread_mutex_init(&wlock, NULL);
    pthread_cond_init(&wcond, NULL);

    pthread_mutex_init(&executing_lock, NULL);
    pthread_cond_init(&executing_wait, NULL);
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

int WorkQueue::StartWork(bool executing)
{
    if (!executing)
        PauseWork();

    return Start();
}

void WorkQueue::StopWork(void)
{
    ResumeWork();

    FlushWork();

    pthread_mutex_lock(&wlock);
    stop = true;
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);

    Join();
}

void WorkQueue::PauseWork(void)
{
    pthread_mutex_lock(&executing_lock);
    executing = false;
    pthread_mutex_unlock(&executing_lock);
}

void WorkQueue::ResumeWork(void)
{
    pthread_mutex_lock(&executing_lock);
    executing = true;
    pthread_cond_signal(&executing_wait);
    pthread_mutex_unlock(&executing_lock);
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
    if (wi) {
        pthread_mutex_lock(&executing_lock);
        if (!executing)
            pthread_cond_wait(&executing_wait, &executing_lock);

        wi->Work();

        pthread_mutex_unlock(&executing_lock);
    }
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
