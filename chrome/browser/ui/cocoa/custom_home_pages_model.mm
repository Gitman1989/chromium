// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/custom_home_pages_model.h"

#include "base/sys_string_conversions.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/browser/prefs/session_startup_pref.h"

NSString* const kHomepageEntryChangedNotification =
    @"kHomepageEntryChangedNotification";

@interface CustomHomePagesModel (Private)
- (void)setURLsInternal:(const std::vector<GURL>&)urls;
@end

@implementation CustomHomePagesModel

- (id)initWithProfile:(Profile*)profile {
  if ((self = [super init])) {
    profile_ = profile;
    entries_.reset([[NSMutableArray alloc] init]);
  }
  return self;
}

- (NSUInteger)countOfCustomHomePages {
  return [entries_ count];
}

- (id)objectInCustomHomePagesAtIndex:(NSUInteger)index {
  return [entries_ objectAtIndex:index];
}

- (void)insertObject:(id)object inCustomHomePagesAtIndex:(NSUInteger)index {
  [entries_ insertObject:object atIndex:index];
}

- (void)removeObjectFromCustomHomePagesAtIndex:(NSUInteger)index {
  [entries_ removeObjectAtIndex:index];
  // Force a save.
  [self validateURLs];
}

// Get/set the urls the model currently contains as a group. These will weed
// out any URLs that are empty and not add them to the model. As a result,
// the next time they're persisted to the prefs backend, they'll disappear.
- (std::vector<GURL>)URLs {
  std::vector<GURL> urls;
  for (CustomHomePageEntry* entry in entries_.get()) {
    const char* urlString = [[entry URL] UTF8String];
    if (urlString && std::strlen(urlString)) {
      urls.push_back(GURL(std::string(urlString)));
    }
  }
  return urls;
}

- (void)setURLs:(const std::vector<GURL>&)urls {
  [self willChangeValueForKey:@"customHomePages"];
  [self setURLsInternal:urls];
  SessionStartupPref pref(SessionStartupPref::GetStartupPref(profile_));
  pref.urls = urls;
  SessionStartupPref::SetStartupPref(profile_, pref);
  [self didChangeValueForKey:@"customHomePages"];
}

// Converts the C++ URLs to Cocoa objects without notifying KVO.
- (void)setURLsInternal:(const std::vector<GURL>&)urls {
  [entries_ removeAllObjects];
  for (size_t i = 0; i < urls.size(); ++i) {
    scoped_nsobject<CustomHomePageEntry> entry(
        [[CustomHomePageEntry alloc] init]);
    const char* urlString = urls[i].spec().c_str();
    if (urlString && std::strlen(urlString)) {
      [entry setURL:[NSString stringWithCString:urlString
                                       encoding:NSUTF8StringEncoding]];
      [entries_ addObject:entry];
    }
  }
}

- (void)reloadURLs {
  [self willChangeValueForKey:@"customHomePages"];
  SessionStartupPref pref(SessionStartupPref::GetStartupPref(profile_));
  [self setURLsInternal:pref.urls];
  [self didChangeValueForKey:@"customHomePages"];
}

- (void)validateURLs {
  [self setURLs:[self URLs]];
}

- (void)setURLStringEmptyAt:(NSUInteger)index {
  // This replaces the data at |index| with an empty (invalid) URL string.
  CustomHomePageEntry* entry = [entries_ objectAtIndex:index];
  [entry setURL:[NSString stringWithString:@""]];
}

@end

//---------------------------------------------------------------------------

@implementation CustomHomePageEntry

- (void)setURL:(NSString*)url {
  // |url| can be nil if the user cleared the text from the edit field.
  if (!url)
    url = [NSString stringWithString:@""];

  // Make sure the url is valid before setting it by fixing it up.
  std::string fixedUrl(URLFixerUpper::FixupURL(
      base::SysNSStringToUTF8(url), std::string()).possibly_invalid_spec());
  url_.reset([base::SysUTF8ToNSString(fixedUrl) retain]);

  // Broadcast that an individual item has changed.
  [[NSNotificationCenter defaultCenter]
      postNotificationName:kHomepageEntryChangedNotification object:nil];

  // TODO(pinkerton): fetch favicon, convert to NSImage http://crbug.com/34642
}

- (NSString*)URL {
  return url_.get();
}

- (void)setImage:(NSImage*)image {
  icon_.reset(image);
}

- (NSImage*)image {
  return icon_.get();
}

- (NSString*)description {
  return url_.get();
}

@end
