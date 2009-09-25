/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __PORTBASE_H
#define __PORTBASE_H

#include <OMX_Core.h>
#include <OMX_Component.h>

class PortBase
{
public:
    /*
     * constructor & destructor
     */
    PortBase();
    PortBase(const OMX_PARAM_PORTDEFINITIONTYPE *param);
    virtual ~PortBase();

    /* end of constructor & destructor */

private:
    /* common routines for constructor */
    void __PortBase(void);
};

#endif /* __PORTBASE_H */
