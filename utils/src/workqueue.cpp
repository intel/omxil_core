#include <workqueue.h>

WorkQueue::WorkQueue()
{
    stop = false;
    __list_init(&works); /* head, works->data is always NULL */

    pthread_mutex_init(&wlock, NULL);
    pthread_cond_init(&wcond, NULL);

    Start();
}

WorkQueue::~WorkQueue()
{
    struct list *entry, *temp;

    stop = true;

    pthread_mutex_lock(&wlock);
    /* race condition against Run() */
    /*
    list_foreach_safe(works.next, entry, temp)
        __list_delete(works.next, entry);
    */
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);

    Join();

    pthread_cond_destroy(&wcond);
    pthread_mutex_destroy(&wlock);
}

void WorkQueue::Run(void)
{
    while (!stop) {
        struct list *entry, *temp;

        pthread_mutex_lock(&wlock);
        /*
         * sleeps until works're available.
         * wokeup by ScheduleWork() or FlushWork() or ~WorkQueue()
         */
        if (!works.next)
            pthread_cond_wait(&wcond, &wlock);

        list_foreach_safe(works.next, entry, temp) {
            WorkableInterface *wi =
                static_cast<WorkableInterface *>(entry->data);

            __list_delete(works.next, entry);
            pthread_mutex_unlock(&wlock);
            DoWork(wi);
            pthread_mutex_lock(&wlock);
        }
        pthread_mutex_unlock(&wlock);
    }
}

void WorkQueue::DoWork(WorkableInterface *wi)
{
    wi->Work();
}

void WorkQueue::Work(void)
{
    return;
}

void WorkQueue::ScheduleWork(void)
{
    pthread_mutex_lock(&wlock);
    list_add_tail(&works, static_cast<WorkableInterface *>(this));
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);
}

void WorkQueue::ScheduleWork(WorkableInterface *wi)
{
    pthread_mutex_lock(&wlock);
    if (wi)
        list_add_tail(&works, wi);
    else
        list_add_tail(&works, static_cast<WorkableInterface *>(this));
    pthread_cond_signal(&wcond); /* wakeup Run() if it's sleeping */
    pthread_mutex_unlock(&wlock);
}

void WorkQueue::FlushWork(void)
{
    FlushBarrier fb;
    bool needtowait = false;

    pthread_mutex_lock(&wlock);
    if (works.next) {
        list_add_tail(&works, &fb);
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
