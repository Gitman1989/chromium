// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/instant/instant_controller.h"

#include "base/command_line.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/instant/instant_delegate.h"
#include "chrome/browser/instant/instant_loader.h"
#include "chrome/browser/instant/instant_loader_manager.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_model.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"

// static
bool InstantController::IsEnabled() {
  static bool enabled = false;
  static bool checked = false;
  if (!checked) {
    checked = true;
    enabled = CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kEnableMatchPreview);
  }
  return enabled;
}

InstantController::InstantController(InstantDelegate* delegate)
    : delegate_(delegate),
      tab_contents_(NULL),
      is_active_(false),
      commit_on_mouse_up_(false),
      last_transition_type_(PageTransition::LINK) {
}

InstantController::~InstantController() {
}

void InstantController::Update(TabContents* tab_contents,
                               const AutocompleteMatch& match,
                               const string16& user_text,
                               string16* suggested_text) {
  if (tab_contents != tab_contents_)
    DestroyPreviewContents();

  const GURL& url = match.destination_url;

  tab_contents_ = tab_contents;
  commit_on_mouse_up_ = false;
  last_transition_type_ = match.transition;

  if (loader_manager_.get() && loader_manager_->active_loader()->url() == url)
    return;

  if (url.is_empty() || !url.is_valid() || !ShouldShowPreviewFor(url)) {
    DestroyPreviewContents();
    return;
  }

  if (!loader_manager_.get())
    loader_manager_.reset(new InstantLoaderManager(this));

  const TemplateURL* template_url = match.template_url;
  if (match.type == AutocompleteMatch::SEARCH_WHAT_YOU_TYPED ||
      match.type == AutocompleteMatch::SEARCH_HISTORY ||
      match.type == AutocompleteMatch::SEARCH_SUGGEST) {
    TemplateURLModel* model = tab_contents->profile()->GetTemplateURLModel();
    template_url = model ? model->GetDefaultSearchProvider() : NULL;
  }
  // TODO(sky): remove the id check. It's only necessary because the search
  // engine saved to prefs doesn't have an id. Jean-luc is fixing separately.
  if (template_url && (!template_url->supports_instant() ||
                       !template_url->id() ||
                       !TemplateURL::SupportsReplacement(template_url))) {
    template_url = NULL;
  }
  TemplateURLID template_url_id = template_url ? template_url->id() : 0;

  InstantLoader* old_loader = loader_manager_->current_loader();
  scoped_ptr<InstantLoader> owned_loader;
  InstantLoader* new_loader =
      loader_manager_->UpdateLoader(template_url_id, &owned_loader);

  new_loader->SetOmniboxBounds(omnibox_bounds_);
  new_loader->Update(tab_contents, match, user_text, template_url,
                     suggested_text);
  if (old_loader != new_loader && new_loader->ready())
    delegate_->ShowInstant(new_loader->preview_contents());
}

void InstantController::SetOmniboxBounds(const gfx::Rect& bounds) {
  if (omnibox_bounds_ == bounds)
    return;

  if (loader_manager_.get()) {
    if (loader_manager_->current_loader())
      loader_manager_->current_loader()->SetOmniboxBounds(bounds);
    if (loader_manager_->pending_loader())
      loader_manager_->pending_loader()->SetOmniboxBounds(bounds);
  }
}

void InstantController::DestroyPreviewContents() {
  if (!loader_manager_.get()) {
    // We're not showing anything, nothing to do.
    return;
  }

  delegate_->HideInstant();
  delete ReleasePreviewContents(INSTANT_COMMIT_DESTROY);
}

void InstantController::CommitCurrentPreview(InstantCommitType type) {
  DCHECK(loader_manager_.get());
  DCHECK(loader_manager_->current_loader());
  delegate_->CommitInstant(ReleasePreviewContents(type));
}

void InstantController::SetCommitOnMouseUp() {
  commit_on_mouse_up_ = true;
}

bool InstantController::IsMouseDownFromActivate() {
  DCHECK(loader_manager_.get());
  DCHECK(loader_manager_->current_loader());
  return loader_manager_->current_loader()->IsMouseDownFromActivate();
}

TabContents* InstantController::ReleasePreviewContents(InstantCommitType type) {
  if (!loader_manager_.get())
    return NULL;

  scoped_ptr<InstantLoader> loader(loader_manager_->ReleaseCurrentLoader());
  TabContents* tab = loader->ReleasePreviewContents(type);

  is_active_ = false;
  omnibox_bounds_ = gfx::Rect();
  commit_on_mouse_up_ = false;
  loader_manager_.reset(NULL);
  return tab;
}

TabContents* InstantController::GetPreviewContents() {
  return loader_manager_.get() ?
      loader_manager_->current_loader()->preview_contents() : NULL;
}

bool InstantController::IsShowingInstant() {
  return loader_manager_.get() &&
      loader_manager_->current_loader()->is_showing_instant();
}

void InstantController::ShowInstantLoader(InstantLoader* loader) {
  DCHECK(loader_manager_.get());
  if (loader_manager_->current_loader() == loader) {
    is_active_ = true;
    delegate_->ShowInstant(loader->preview_contents());
  } else if (loader_manager_->pending_loader() == loader) {
    scoped_ptr<InstantLoader> old_loader;
    loader_manager_->MakePendingCurrent(&old_loader);
    delegate_->ShowInstant(loader->preview_contents());
  } else {
    NOTREACHED();
  }
}

void InstantController::SetSuggestedTextFor(InstantLoader* loader,
                                            const string16& text) {
  if (loader_manager_->current_loader() == loader)
    delegate_->SetSuggestedText(text);
}

gfx::Rect InstantController::GetInstantBounds() {
  return delegate_->GetInstantBounds();
}

bool InstantController::ShouldCommitInstantOnMouseUp() {
  return commit_on_mouse_up_;
}

void InstantController::CommitInstantLoader(InstantLoader* loader) {
  if (loader_manager_.get() && loader_manager_->current_loader() == loader) {
    CommitCurrentPreview(INSTANT_COMMIT_FOCUS_LOST);
  } else {
    // This can happen if the mouse was down, we swapped out the preview and
    // the mouse was released. Generally this shouldn't happen, but if it does
    // revert.
    DestroyPreviewContents();
  }
}

bool InstantController::ShouldShowPreviewFor(const GURL& url) {
  return !url.SchemeIs(chrome::kJavaScriptScheme);
}