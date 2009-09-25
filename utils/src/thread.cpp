#include <pthread.h>
#include <thread.h>

Thread::Thread()
{
    r = NULL;
    id = -1;
}

Thread::Thread(RunnableInterface *r)
{
    this->r = r;
    id = -1;
}

Thread::~Thread()
{
    if (id > 0)
        Join();
}

int Thread::Start(void)
{
    int ret;

    ret = pthread_create(&id, NULL, Instance, this);
    if (ret)
        id = -1;

    return ret;
}

int Thread::Join(void)
{
    return pthread_join(id, NULL);
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
