#include <pthread.h>
#include <thread.h>

Thread::Thread()
{
    r = NULL;
}

Thread::Thread(RunnableInterface *r)
{
    this->r = r;
}

Thread::~Thread()
{
    Join();
}

int Thread::Start(void)
{
    return pthread_create(&id, NULL, Instance, this);
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
