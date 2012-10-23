/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PageLoadClientEfl.h"

#include "WKFrame.h"
#include "WKPage.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_error_private.h"
#include "ewk_intent_private.h"
#include "ewk_intent_service_private.h"
#include "ewk_view.h"

namespace WebKit {

static inline PageLoadClientEfl* toPageLoadClientEfl(const void* clientInfo)
{
    return static_cast<PageLoadClientEfl*>(const_cast<void*>(clientInfo));
}

void PageLoadClientEfl::didReceiveTitleForFrame(WKPageRef, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ewk_view_title_changed(ewkView, toImpl(title)->string().utf8().data());
}

#if ENABLE(WEB_INTENTS)
void PageLoadClientEfl::didReceiveIntentForFrame(WKPageRef, WKFrameRef, WKIntentDataRef intent, WKTypeRef, const void* clientInfo)
{
    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    RefPtr<Ewk_Intent> ewkIntent = Ewk_Intent::create(intent);
    ewk_view_intent_request_new(ewkView, ewkIntent.get());
}
#endif

#if ENABLE(WEB_INTENTS_TAG)
void PageLoadClientEfl::registerIntentServiceForFrame(WKPageRef, WKFrameRef, WKIntentServiceInfoRef serviceInfo, WKTypeRef, const void* clientInfo)
{
    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    RefPtr<Ewk_Intent_Service> ewkIntentService = Ewk_Intent_Service::create(serviceInfo);
    ewk_view_intent_service_register(ewkView, ewkIntentService.get());
}
#endif

void PageLoadClientEfl::didChangeProgress(WKPageRef page, const void* clientInfo)
{
    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ewk_view_load_progress_changed(ewkView, WKPageGetEstimatedProgress(page));
}

void PageLoadClientEfl::didFinishLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ewk_view_load_finished(ewkView);
}

void PageLoadClientEfl::didFailLoadWithErrorForFrame(WKPageRef, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    ewk_view_load_error(ewkView, ewkError.get());
    ewk_view_load_finished(ewkView);
}

void PageLoadClientEfl::didStartProvisionalLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ewk_view_load_provisional_started(ewkView);
}

void PageLoadClientEfl::didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ewk_view_load_provisional_redirect(ewkView);
}

void PageLoadClientEfl::didFailProvisionalLoadWithErrorForFrame(WKPageRef, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    ewk_view_load_provisional_failed(ewkView, ewkError.get());
}

void PageLoadClientEfl::didChangeBackForwardList(WKPageRef, WKBackForwardListItemRef addedItem, WKArrayRef removedItems, const void* clientInfo)
{
    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ASSERT(ewkView);

    Ewk_Back_Forward_List* list = ewk_view_back_forward_list_get(ewkView);
    ASSERT(list);
    list->update(addedItem, removedItems);

    ewk_view_back_forward_list_changed(ewkView);
}

void PageLoadClientEfl::didSameDocumentNavigationForFrame(WKPageRef, WKFrameRef frame, WKSameDocumentNavigationType, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->view();
    ewk_view_url_update(ewkView);
}

PageLoadClientEfl::PageLoadClientEfl(Evas_Object* view)
    : m_view(view)
{
    WKPageRef pageRef = ewk_view_wkpage_get(m_view);
    ASSERT(pageRef);

    WKPageLoaderClient loadClient;
    memset(&loadClient, 0, sizeof(WKPageLoaderClient));
    loadClient.version = kWKPageLoaderClientCurrentVersion;
    loadClient.clientInfo = this;
    loadClient.didReceiveTitleForFrame = didReceiveTitleForFrame;
#if ENABLE(WEB_INTENTS)
    loadClient.didReceiveIntentForFrame = didReceiveIntentForFrame;
#endif
#if ENABLE(WEB_INTENTS_TAG)
    loadClient.registerIntentServiceForFrame = registerIntentServiceForFrame;
#endif
    loadClient.didStartProgress = didChangeProgress;
    loadClient.didChangeProgress = didChangeProgress;
    loadClient.didFinishProgress = didChangeProgress;
    loadClient.didFinishLoadForFrame = didFinishLoadForFrame;
    loadClient.didFailLoadWithErrorForFrame = didFailLoadWithErrorForFrame;
    loadClient.didStartProvisionalLoadForFrame = didStartProvisionalLoadForFrame;
    loadClient.didReceiveServerRedirectForProvisionalLoadForFrame = didReceiveServerRedirectForProvisionalLoadForFrame;
    loadClient.didFailProvisionalLoadWithErrorForFrame = didFailProvisionalLoadWithErrorForFrame;
    loadClient.didChangeBackForwardList = didChangeBackForwardList;
    loadClient.didSameDocumentNavigationForFrame = didSameDocumentNavigationForFrame;
    WKPageSetPageLoaderClient(pageRef, &loadClient);
}

} // namespace WebKit