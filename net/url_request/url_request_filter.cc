// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/url_request_filter.h"

#include <set>

#include "base/logging.h"

URLRequestFilter* URLRequestFilter::shared_instance_ = NULL;

/* static */
URLRequestFilter* URLRequestFilter::GetInstance() {
  if (!shared_instance_)
    shared_instance_ = new URLRequestFilter;
  return shared_instance_;
}

/* static */
net::URLRequestJob* URLRequestFilter::Factory(net::URLRequest* request,
                                              const std::string& scheme) {
  // Returning null here just means that the built-in handler will be used.
  return GetInstance()->FindRequestHandler(request, scheme);
}

URLRequestFilter::~URLRequestFilter() {}

void URLRequestFilter::AddHostnameHandler(const std::string& scheme,
    const std::string& hostname, net::URLRequest::ProtocolFactory* factory) {
  hostname_handler_map_[make_pair(scheme, hostname)] = factory;

  // Register with the ProtocolFactory.
  net::URLRequest::RegisterProtocolFactory(scheme,
                                           &URLRequestFilter::Factory);

#ifndef NDEBUG
  // Check to see if we're masking URLs in the url_handler_map_.
  for (UrlHandlerMap::const_iterator i = url_handler_map_.begin();
       i != url_handler_map_.end(); ++i) {
    const GURL& url = GURL(i->first);
    HostnameHandlerMap::iterator host_it =
        hostname_handler_map_.find(make_pair(url.scheme(), url.host()));
    if (host_it != hostname_handler_map_.end())
      NOTREACHED();
  }
#endif  // !NDEBUG
}

void URLRequestFilter::RemoveHostnameHandler(const std::string& scheme,
                                             const std::string& hostname) {
  HostnameHandlerMap::iterator iter =
      hostname_handler_map_.find(make_pair(scheme, hostname));
  DCHECK(iter != hostname_handler_map_.end());

  hostname_handler_map_.erase(iter);
  // Note that we don't unregister from the net::URLRequest ProtocolFactory as
  // this would left no protocol factory for the scheme.
  // URLRequestFilter::Factory will keep forwarding the requests to the
  // URLRequestInetJob.
}

bool URLRequestFilter::AddUrlHandler(
    const GURL& url,
    net::URLRequest::ProtocolFactory* factory) {
  if (!url.is_valid())
    return false;
  url_handler_map_[url.spec()] = factory;

  // Register with the ProtocolFactory.
  net::URLRequest::RegisterProtocolFactory(url.scheme(),
                                           &URLRequestFilter::Factory);
#ifndef NDEBUG
  // Check to see if this URL is masked by a hostname handler.
  HostnameHandlerMap::iterator host_it =
      hostname_handler_map_.find(make_pair(url.scheme(), url.host()));
  if (host_it != hostname_handler_map_.end())
    NOTREACHED();
#endif  // !NDEBUG

  return true;
}

void URLRequestFilter::RemoveUrlHandler(const GURL& url) {
  UrlHandlerMap::iterator iter = url_handler_map_.find(url.spec());
  DCHECK(iter != url_handler_map_.end());

  url_handler_map_.erase(iter);
  // Note that we don't unregister from the net::URLRequest ProtocolFactory as
  // this would left no protocol factory for the scheme.
  // URLRequestFilter::Factory will keep forwarding the requests to the
  // URLRequestInetJob.
}

void URLRequestFilter::ClearHandlers() {
  // Unregister with the ProtocolFactory.
  std::set<std::string> schemes;
  for (UrlHandlerMap::const_iterator i = url_handler_map_.begin();
       i != url_handler_map_.end(); ++i) {
    schemes.insert(GURL(i->first).scheme());
  }
  for (HostnameHandlerMap::const_iterator i = hostname_handler_map_.begin();
       i != hostname_handler_map_.end(); ++i) {
    schemes.insert(i->first.first);
  }
  for (std::set<std::string>::const_iterator scheme = schemes.begin();
       scheme != schemes.end(); ++scheme) {
    net::URLRequest::RegisterProtocolFactory(*scheme, NULL);
  }

  url_handler_map_.clear();
  hostname_handler_map_.clear();
  hit_count_ = 0;
}

URLRequestFilter::URLRequestFilter() : hit_count_(0) { }

net::URLRequestJob* URLRequestFilter::FindRequestHandler(
    net::URLRequest* request,
    const std::string& scheme) {
  net::URLRequestJob* job = NULL;
  if (request->url().is_valid()) {
    // Check the hostname map first.
    const std::string& hostname = request->url().host();

    HostnameHandlerMap::iterator i =
        hostname_handler_map_.find(make_pair(scheme, hostname));
    if (i != hostname_handler_map_.end())
      job = i->second(request, scheme);

    if (!job) {
      // Not in the hostname map, check the url map.
      const std::string& url = request->url().spec();
      UrlHandlerMap::iterator i = url_handler_map_.find(url);
      if (i != url_handler_map_.end())
        job = i->second(request, scheme);
    }
  }
  if (job) {
    DVLOG(1) << "URLRequestFilter hit for " << request->url().spec();
    hit_count_++;
  }
  return job;
}
