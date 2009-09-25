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
};

#endif /* __THREAD_H */
