// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/autofill_model_associator2.h"

#include <vector>

#include "base/task.h"
#include "base/time.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autofill/autofill_profile.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/glue/autofill_change_processor.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/protocol/autofill_specifics.pb.h"
#include "chrome/browser/webdata/web_database.h"
#include "net/base/escape.h"

using base::TimeTicks;

namespace browser_sync {

extern const char kAutofillTag[];
extern const char kAutofillEntryNamespaceTag[];
const char kAutofillProfileNamespaceTag[] = "autofill_profile|";

static const int kMaxNumAttemptsToFindUniqueLabel = 100;

struct AutofillModelAssociator2::DataBundle {
  std::set<AutofillKey> current_entries;
  std::vector<AutofillEntry> new_entries;
  std::set<string16> current_profiles;
  std::vector<AutoFillProfile*> updated_profiles;
  std::vector<AutoFillProfile*> new_profiles;  // We own these pointers.
  ~DataBundle() { STLDeleteElements(&new_profiles); }
};

AutofillModelAssociator2::DoOptimisticRefreshTask::DoOptimisticRefreshTask(
    PersonalDataManager* pdm) : pdm_(pdm) {}

AutofillModelAssociator2::DoOptimisticRefreshTask::~DoOptimisticRefreshTask() {}

void AutofillModelAssociator2::DoOptimisticRefreshTask::Run() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  pdm_->Refresh();
}

AutofillModelAssociator2::AutofillModelAssociator2(
    ProfileSyncService* sync_service,
    WebDatabase* web_database,
    PersonalDataManager* personal_data)
    : sync_service_(sync_service),
      web_database_(web_database),
      personal_data_(personal_data),
      autofill_node_id_(sync_api::kInvalidId),
      abort_association_pending_(false) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  DCHECK(sync_service_);
  DCHECK(web_database_);
  DCHECK(personal_data_);
}

AutofillModelAssociator2::~AutofillModelAssociator2() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
}

bool AutofillModelAssociator2::TraverseAndAssociateChromeAutofillEntries(
    sync_api::WriteTransaction* write_trans,
    const sync_api::ReadNode& autofill_root,
    const std::vector<AutofillEntry>& all_entries_from_db,
    std::set<AutofillKey>* current_entries,
    std::vector<AutofillEntry>* new_entries) {

  const std::vector<AutofillEntry>& entries = all_entries_from_db;
  for (std::vector<AutofillEntry>::const_iterator ix = entries.begin();
       ix != entries.end(); ++ix) {
    std::string tag = KeyToTag(ix->key().name(), ix->key().value());
    if (id_map_.find(tag) != id_map_.end()) {
      // It seems that name/value pairs are not unique in the web database.
      // As a result, we have to filter out duplicates here.  This is probably
      // a bug in the database.
      continue;
    }

    sync_api::ReadNode node(write_trans);
    if (node.InitByClientTagLookup(syncable::AUTOFILL, tag)) {
      const sync_pb::AutofillSpecifics& autofill(node.GetAutofillSpecifics());
      DCHECK_EQ(tag, KeyToTag(UTF8ToUTF16(autofill.name()),
                              UTF8ToUTF16(autofill.value())));

      std::vector<base::Time> timestamps;
      if (MergeTimestamps(autofill, ix->timestamps(), &timestamps)) {
        AutofillEntry new_entry(ix->key(), timestamps);
        new_entries->push_back(new_entry);

        sync_api::WriteNode write_node(write_trans);
        if (!write_node.InitByClientTagLookup(syncable::AUTOFILL, tag)) {
          LOG(ERROR) << "Failed to write autofill sync node.";
          return false;
        }
        AutofillChangeProcessor2::WriteAutofillEntry(new_entry, &write_node);
      }

      Associate(&tag, node.GetId());
    } else {
      sync_api::WriteNode node(write_trans);
      if (!node.InitUniqueByCreation(syncable::AUTOFILL,
                                     autofill_root, tag)) {
        LOG(ERROR) << "Failed to create autofill sync node.";
        return false;
      }
      node.SetTitle(UTF8ToWide(tag));
      AutofillChangeProcessor2::WriteAutofillEntry(*ix, &node);
      Associate(&tag, node.GetId());
    }

    current_entries->insert(ix->key());
  }
  return true;
}

bool AutofillModelAssociator2::TraverseAndAssociateChromeAutoFillProfiles(
    sync_api::WriteTransaction* write_trans,
    const sync_api::ReadNode& autofill_root,
    const std::vector<AutoFillProfile*>& all_profiles_from_db,
    std::set<string16>* current_profiles,
    std::vector<AutoFillProfile*>* updated_profiles) {
  const std::vector<AutoFillProfile*>& profiles = all_profiles_from_db;
  for (std::vector<AutoFillProfile*>::const_iterator ix = profiles.begin();
       ix != profiles.end(); ++ix) {
    string16 label((*ix)->Label());
    std::string tag(ProfileLabelToTag(label));

    sync_api::ReadNode node(write_trans);
    if (node.InitByClientTagLookup(syncable::AUTOFILL, tag)) {
      const sync_pb::AutofillSpecifics& autofill(node.GetAutofillSpecifics());
      DCHECK(autofill.has_profile());
      DCHECK_EQ(ProfileLabelToTag(UTF8ToUTF16(autofill.profile().label())),
                tag);
      int64 sync_id = node.GetId();
      if (id_map_.find(tag) != id_map_.end()) {
        // We just looked up something we already associated.  Move aside.
        label = MakeUniqueLabel(label, string16(), write_trans);
        if (label.empty()) {
          return false;
        }
        tag = ProfileLabelToTag(label);
        // TODO(dhollowa): Replace with |AutoFillProfile::set_guid|.
        // http://crbug.com/58813
        (*ix)->set_label(label);
        if (!MakeNewAutofillProfileSyncNode(write_trans, autofill_root,
                                            tag, **ix, &sync_id)) {
          return false;
        }
        updated_profiles->push_back(*ix);
      } else {
        // Overwrite local with cloud state.
        if (OverwriteProfileWithServerData(*ix, autofill.profile()))
          updated_profiles->push_back(*ix);
        sync_id = node.GetId();
      }

      Associate(&tag, sync_id);
    } else {
      int64 id;
      if (!MakeNewAutofillProfileSyncNode(write_trans, autofill_root,
                                          tag, **ix, &id)) {
        return false;
      }
      Associate(&tag, id);
    }
    current_profiles->insert(label);
  }
  return true;
}

// static
string16 AutofillModelAssociator2::MakeUniqueLabel(
    const string16& non_unique_label,
    const string16& existing_unique_label,
    sync_api::BaseTransaction* trans) {
  if (!non_unique_label.empty() && non_unique_label == existing_unique_label) {
    return existing_unique_label;
  }
  int unique_id = 1;  // Priming so we start by appending "2".
  while (unique_id++ < kMaxNumAttemptsToFindUniqueLabel) {
    string16 suffix(base::IntToString16(unique_id));
    string16 unique_label = non_unique_label + suffix;
    if (unique_label == existing_unique_label)
      return unique_label;  // We'll use the one we already have.
    sync_api::ReadNode node(trans);
    if (node.InitByClientTagLookup(syncable::AUTOFILL,
                                   ProfileLabelToTag(unique_label))) {
      continue;
    }
    return unique_label;
  }

  LOG(ERROR) << "Couldn't create unique tag for autofill node. Srsly?!";
  return string16();
}

bool AutofillModelAssociator2::MakeNewAutofillProfileSyncNode(
    sync_api::WriteTransaction* trans, const sync_api::BaseNode& autofill_root,
    const std::string& tag, const AutoFillProfile& profile, int64* sync_id) {
  sync_api::WriteNode node(trans);
  if (!node.InitUniqueByCreation(syncable::AUTOFILL, autofill_root, tag)) {
    LOG(ERROR) << "Failed to create autofill sync node.";
    return false;
  }
  node.SetTitle(UTF8ToWide(tag));
  AutofillChangeProcessor2::WriteAutofillProfile(profile, &node);
  *sync_id = node.GetId();
  return true;
}


bool AutofillModelAssociator2::LoadAutofillData(
    std::vector<AutofillEntry>* entries,
    std::vector<AutoFillProfile*>* profiles) {
  if (IsAbortPending())
    return false;
  if (!web_database_->GetAllAutofillEntries(entries))
    return false;

  if (IsAbortPending())
    return false;
  if (!web_database_->GetAutoFillProfiles(profiles))
    return false;

  return true;
}

bool AutofillModelAssociator2::AssociateModels() {
  VLOG(1) << "Associating Autofill Models";
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  {
    AutoLock lock(abort_association_pending_lock_);
    abort_association_pending_ = false;
  }

  // TODO(zork): Attempt to load the model association from storage.
  std::vector<AutofillEntry> entries;
  ScopedVector<AutoFillProfile> profiles;

  if (!LoadAutofillData(&entries, &profiles.get())) {
    LOG(ERROR) << "Could not get the autofill data from WebDatabase.";
    return false;
  }

  DataBundle bundle;
  {
    sync_api::WriteTransaction trans(
        sync_service_->backend()->GetUserShareHandle());

    sync_api::ReadNode autofill_root(&trans);
    if (!autofill_root.InitByTagLookup(kAutofillTag)) {
      LOG(ERROR) << "Server did not create the top-level autofill node. We "
                 << "might be running against an out-of-date server.";
      return false;
    }

    if (!TraverseAndAssociateChromeAutofillEntries(&trans, autofill_root,
            entries, &bundle.current_entries, &bundle.new_entries) ||
        !TraverseAndAssociateChromeAutoFillProfiles(&trans, autofill_root,
            profiles.get(), &bundle.current_profiles,
            &bundle.updated_profiles) ||
        !TraverseAndAssociateAllSyncNodes(&trans, autofill_root, &bundle)) {
      return false;
    }
  }

  // Since we're on the DB thread, we don't have to worry about updating
  // the autofill database after closing the write transaction, since
  // this is the only thread that writes to the database.  We also don't have
  // to worry about the sync model getting out of sync, because changes are
  // propogated to the ChangeProcessor on this thread.
  if (!SaveChangesToWebData(bundle)) {
    LOG(ERROR) << "Failed to update autofill entries.";
    return false;
  }

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      new DoOptimisticRefreshTask(personal_data_));
  return true;
}

bool AutofillModelAssociator2::SaveChangesToWebData(const DataBundle& bundle) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));

  if (IsAbortPending())
    return false;

  if (bundle.new_entries.size() &&
      !web_database_->UpdateAutofillEntries(bundle.new_entries)) {
    return false;
  }

  for (size_t i = 0; i < bundle.new_profiles.size(); i++) {
    if (IsAbortPending())
      return false;
    if (!web_database_->AddAutoFillProfile(*bundle.new_profiles[i]))
      return false;
  }

  for (size_t i = 0; i < bundle.updated_profiles.size(); i++) {
    if (IsAbortPending())
      return false;
    if (!web_database_->UpdateAutoFillProfile(*bundle.updated_profiles[i]))
      return false;
  }
  return true;
}

bool AutofillModelAssociator2::TraverseAndAssociateAllSyncNodes(
    sync_api::WriteTransaction* write_trans,
    const sync_api::ReadNode& autofill_root,
    DataBundle* bundle) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));

  int64 sync_child_id = autofill_root.GetFirstChildId();
  while (sync_child_id != sync_api::kInvalidId) {
    sync_api::ReadNode sync_child(write_trans);
    if (!sync_child.InitByIdLookup(sync_child_id)) {
      LOG(ERROR) << "Failed to fetch child node.";
      return false;
    }
    const sync_pb::AutofillSpecifics& autofill(
        sync_child.GetAutofillSpecifics());

    if (autofill.has_value())
      AddNativeEntryIfNeeded(autofill, bundle, sync_child);
    else if (autofill.has_profile())
      AddNativeProfileIfNeeded(autofill.profile(), bundle, sync_child);
    else
      NOTREACHED() << "AutofillSpecifics has no autofill data!";

    sync_child_id = sync_child.GetSuccessorId();
  }
  return true;
}

void AutofillModelAssociator2::AddNativeEntryIfNeeded(
    const sync_pb::AutofillSpecifics& autofill, DataBundle* bundle,
    const sync_api::ReadNode& node) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  AutofillKey key(UTF8ToUTF16(autofill.name()), UTF8ToUTF16(autofill.value()));

  if (bundle->current_entries.find(key) == bundle->current_entries.end()) {
    std::vector<base::Time> timestamps;
    int timestamps_count = autofill.usage_timestamp_size();
    for (int c = 0; c < timestamps_count; ++c) {
      timestamps.push_back(base::Time::FromInternalValue(
          autofill.usage_timestamp(c)));
    }
    std::string tag(KeyToTag(key.name(), key.value()));
    Associate(&tag, node.GetId());
    bundle->new_entries.push_back(AutofillEntry(key, timestamps));
  }
}

void AutofillModelAssociator2::AddNativeProfileIfNeeded(
    const sync_pb::AutofillProfileSpecifics& profile, DataBundle* bundle,
    const sync_api::ReadNode& node) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  if (bundle->current_profiles.find(UTF8ToUTF16(profile.label())) ==
      bundle->current_profiles.end()) {
    std::string tag(ProfileLabelToTag(UTF8ToUTF16(profile.label())));
    Associate(&tag, node.GetId());
    AutoFillProfile* p = personal_data_->
        CreateNewEmptyAutoFillProfileForDBThread(UTF8ToUTF16(profile.label()));
    OverwriteProfileWithServerData(p, profile);
    bundle->new_profiles.push_back(p);
  }
}

bool AutofillModelAssociator2::DisassociateModels() {
  id_map_.clear();
  id_map_inverse_.clear();
  return true;
}

bool AutofillModelAssociator2::SyncModelHasUserCreatedNodes(bool* has_nodes) {
  DCHECK(has_nodes);
  *has_nodes = false;
  int64 autofill_sync_id;
  if (!GetSyncIdForTaggedNode(kAutofillTag, &autofill_sync_id)) {
    LOG(ERROR) << "Server did not create the top-level autofill node. We "
               << "might be running against an out-of-date server.";
    return false;
  }
  sync_api::ReadTransaction trans(
      sync_service_->backend()->GetUserShareHandle());

  sync_api::ReadNode autofill_node(&trans);
  if (!autofill_node.InitByIdLookup(autofill_sync_id)) {
    LOG(ERROR) << "Server did not create the top-level autofill node. We "
               << "might be running against an out-of-date server.";
    return false;
  }

  // The sync model has user created nodes if the autofill folder has any
  // children.
  *has_nodes = sync_api::kInvalidId != autofill_node.GetFirstChildId();
  return true;
}

void AutofillModelAssociator2::AbortAssociation() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  AutoLock lock(abort_association_pending_lock_);
  abort_association_pending_ = true;
}

const std::string*
AutofillModelAssociator2::GetChromeNodeFromSyncId(int64 sync_id) {
  return NULL;
}

bool AutofillModelAssociator2::InitSyncNodeFromChromeId(
    const std::string& node_id,
    sync_api::BaseNode* sync_node) {
  return false;
}

int64 AutofillModelAssociator2::GetSyncIdFromChromeId(
    const std::string& autofill) {
  AutofillToSyncIdMap::const_iterator iter = id_map_.find(autofill);
  return iter == id_map_.end() ? sync_api::kInvalidId : iter->second;
}

void AutofillModelAssociator2::Associate(
    const std::string* autofill, int64 sync_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  DCHECK_NE(sync_api::kInvalidId, sync_id);
  DCHECK(id_map_.find(*autofill) == id_map_.end());
  DCHECK(id_map_inverse_.find(sync_id) == id_map_inverse_.end());
  id_map_[*autofill] = sync_id;
  id_map_inverse_[sync_id] = *autofill;
}

void AutofillModelAssociator2::Disassociate(int64 sync_id) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  SyncIdToAutofillMap::iterator iter = id_map_inverse_.find(sync_id);
  if (iter == id_map_inverse_.end())
    return;
  CHECK(id_map_.erase(iter->second));
  id_map_inverse_.erase(iter);
}

bool AutofillModelAssociator2::GetSyncIdForTaggedNode(const std::string& tag,
                                                     int64* sync_id) {
  sync_api::ReadTransaction trans(
      sync_service_->backend()->GetUserShareHandle());
  sync_api::ReadNode sync_node(&trans);
  if (!sync_node.InitByTagLookup(tag.c_str()))
    return false;
  *sync_id = sync_node.GetId();
  return true;
}

bool AutofillModelAssociator2::IsAbortPending() {
  AutoLock lock(abort_association_pending_lock_);
  return abort_association_pending_;
}

// static
std::string AutofillModelAssociator2::KeyToTag(const string16& name,
                                              const string16& value) {
  std::string ns(kAutofillEntryNamespaceTag);
  return ns + EscapePath(UTF16ToUTF8(name)) + "|" +
         EscapePath(UTF16ToUTF8(value));
}

// static
std::string AutofillModelAssociator2::ProfileLabelToTag(const string16& label) {
  std::string ns(kAutofillProfileNamespaceTag);
  return ns + EscapePath(UTF16ToUTF8(label));
}

// static
bool AutofillModelAssociator2::MergeTimestamps(
    const sync_pb::AutofillSpecifics& autofill,
    const std::vector<base::Time>& timestamps,
    std::vector<base::Time>* new_timestamps) {
  DCHECK(new_timestamps);
  std::set<base::Time> timestamp_union(timestamps.begin(),
                                       timestamps.end());

  size_t timestamps_count = autofill.usage_timestamp_size();

  bool different = timestamps.size() != timestamps_count;
  for (size_t c = 0; c < timestamps_count; ++c) {
    if (timestamp_union.insert(base::Time::FromInternalValue(
            autofill.usage_timestamp(c))).second) {
      different = true;
    }
  }

  if (different) {
    new_timestamps->insert(new_timestamps->begin(),
                           timestamp_union.begin(),
                           timestamp_union.end());
  }
  return different;
}

// Helper to compare the local value and cloud value of a field, merge into
// the local value if they differ, and return whether the merge happened.
bool MergeField2(FormGroup* f, AutoFillFieldType t,
                const std::string& specifics_field) {
  if (UTF16ToUTF8(f->GetFieldText(AutoFillType(t))) == specifics_field)
    return false;
  f->SetInfo(AutoFillType(t), UTF8ToUTF16(specifics_field));
  return true;
}

// static
bool AutofillModelAssociator2::OverwriteProfileWithServerData(
    AutoFillProfile* merge_into,
    const sync_pb::AutofillProfileSpecifics& specifics) {
  bool diff = false;
  AutoFillProfile* p = merge_into;
  const sync_pb::AutofillProfileSpecifics& s(specifics);
  diff = MergeField2(p, NAME_FIRST, s.name_first()) || diff;
  diff = MergeField2(p, NAME_LAST, s.name_last()) || diff;
  diff = MergeField2(p, NAME_MIDDLE, s.name_middle()) || diff;
  diff = MergeField2(p, ADDRESS_HOME_LINE1, s.address_home_line1()) || diff;
  diff = MergeField2(p, ADDRESS_HOME_LINE2, s.address_home_line2()) || diff;
  diff = MergeField2(p, ADDRESS_HOME_CITY, s.address_home_city()) || diff;
  diff = MergeField2(p, ADDRESS_HOME_STATE, s.address_home_state()) || diff;
  diff = MergeField2(p, ADDRESS_HOME_COUNTRY, s.address_home_country()) || diff;
  diff = MergeField2(p, ADDRESS_HOME_ZIP, s.address_home_zip()) || diff;
  diff = MergeField2(p, EMAIL_ADDRESS, s.email_address()) || diff;
  diff = MergeField2(p, COMPANY_NAME, s.company_name()) || diff;
  diff = MergeField2(p, PHONE_FAX_WHOLE_NUMBER, s.phone_fax_whole_number())
      || diff;
  diff = MergeField2(p, PHONE_HOME_WHOLE_NUMBER, s.phone_home_whole_number())
      || diff;
  return diff;
}

}  // namespace browser_sync

