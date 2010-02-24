/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portvideo.h>

#define LOG_TAG "portvideo"
#include <log.h>

PortVideo::PortVideo()
{
    memset(&videoparam, 0, sizeof(videoparam));
    ComponentBase::SetTypeHeader(&videoparam, sizeof(videoparam));

    videoparam.nIndex = 0;
    videoparam.eCompressionFormat = OMX_VIDEO_CodingUnused;
    videoparam.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    videoparam.xFramerate = 15 << 16;
}

OMX_ERRORTYPE PortVideo::SetPortVideoParam(
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (videoparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        videoparam.nPortIndex = p->nPortIndex;

    videoparam.nIndex = p->nIndex;
    videoparam.eCompressionFormat = p->eCompressionFormat;
    videoparam.eColorFormat = p->eColorFormat;
    videoparam.xFramerate = p->xFramerate;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_PORTFORMATTYPE *PortVideo::GetPortVideoParam(void)
{
    return &videoparam;
}

/* end of PortVideo */

PortAvc::PortAvc()
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;

    memcpy(&videoparam, GetPortVideoParam(), sizeof(videoparam));
    videoparam.eCompressionFormat = OMX_VIDEO_CodingAVC;
    videoparam.eColorFormat = OMX_COLOR_FormatUnused;
    videoparam.xFramerate = 15 << 16;
    SetPortVideoParam(&videoparam, false);

    memset(&avcparam, 0, sizeof(avcparam));
    ComponentBase::SetTypeHeader(&avcparam, sizeof(avcparam));
}

OMX_ERRORTYPE PortAvc::SetPortAvcParam(
    const OMX_VIDEO_PARAM_AVCTYPE *p, bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (avcparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;

        memcpy(&videoparam, GetPortVideoParam(), sizeof(videoparam));
        videoparam.nPortIndex = p->nPortIndex;
        SetPortVideoParam(&videoparam, true);

        avcparam.nPortIndex = p->nPortIndex;
    }

    avcparam.nSliceHeaderSpacing = p->nSliceHeaderSpacing;
    avcparam.nPFrames = p->nPFrames;
    avcparam.nBFrames = p->nBFrames;
    avcparam.bUseHadamard = p->bUseHadamard;
    avcparam.nRefFrames = p->nRefFrames;
    avcparam.nRefIdx10ActiveMinus1 = p->nRefIdx10ActiveMinus1;
    avcparam.nRefIdx11ActiveMinus1 = p->nRefIdx11ActiveMinus1;
    avcparam.bEnableUEP = p->bEnableUEP;
    avcparam.bEnableFMO = p->bEnableFMO;
    avcparam.bEnableASO = p->bEnableASO;
    avcparam.bEnableRS = p->bEnableRS;
    avcparam.nAllowedPictureTypes = p->nAllowedPictureTypes;
    avcparam.bFrameMBsOnly = p->bFrameMBsOnly;
    avcparam.bMBAFF = p->bMBAFF;
    avcparam.bEntropyCodingCABAC = p->bEntropyCodingCABAC;
    avcparam.bWeightedPPrediction = p->bWeightedPPrediction;
    avcparam.nWeightedBipredicitonMode = p->nWeightedBipredicitonMode;
    avcparam.bconstIpred = p->bconstIpred;
    avcparam.bDirect8x8Inference = p->bDirect8x8Inference;
    avcparam.bDirectSpatialTemporal = p->bDirectSpatialTemporal;
    avcparam.nCabacInitIdc = p->nCabacInitIdc;
    avcparam.eLoopFilterMode = p->eLoopFilterMode;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_AVCTYPE *PortAvc::GetPortAvcParam(void)
{
    return &avcparam;
}

/* end of PortAvc */

PortMpeg4::PortMpeg4()
{
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;

    memcpy(&videoparam, GetPortVideoParam(), sizeof(videoparam));
    videoparam.eCompressionFormat = OMX_VIDEO_CodingMPEG4;
    videoparam.eColorFormat = OMX_COLOR_FormatUnused;
    videoparam.xFramerate = 15 << 16;
    SetPortVideoParam(&videoparam, false);

    memset(&mpeg4param, 0, sizeof(mpeg4param));
    ComponentBase::SetTypeHeader(&mpeg4param, sizeof(mpeg4param));
}

OMX_ERRORTYPE PortMpeg4::SetPortMpeg4Param(
    const OMX_VIDEO_PARAM_MPEG4TYPE *p, bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (mpeg4param.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;

        memcpy(&videoparam, GetPortVideoParam(), sizeof(videoparam));
        videoparam.nPortIndex = p->nPortIndex;
        SetPortVideoParam(&videoparam, true);

        mpeg4param.nPortIndex = p->nPortIndex;
    }

    mpeg4param.nSliceHeaderSpacing = p->nSliceHeaderSpacing;
    mpeg4param.bSVH = p->bSVH;
    mpeg4param.bGov = p->bGov;
    mpeg4param.nPFrames = p->nPFrames;
    mpeg4param.nBFrames = p->nBFrames;
    mpeg4param.nIDCVLCThreshold = p->nIDCVLCThreshold;
    mpeg4param.bACPred = p->bACPred;
    mpeg4param.nMaxPacketSize = p->nMaxPacketSize;
    mpeg4param.nTimeIncRes = p->nTimeIncRes;
    mpeg4param.eProfile = p->eProfile;
    mpeg4param.eLevel = p->eLevel;
    mpeg4param.nAllowedPictureTypes = p->nAllowedPictureTypes;
    mpeg4param.nHeaderExtension = p->nHeaderExtension;
    mpeg4param.bReversibleVLC = p->bReversibleVLC;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_MPEG4TYPE *PortMpeg4::GetPortMpeg4Param(void)
{
    return &mpeg4param;
}

/* end of PortMpeg4 */

