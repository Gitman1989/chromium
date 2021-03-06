// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/live_sync/live_sessions_sync_test.h"

// @TODO(zea): Test each individual session command we care about separately.
// (as well as multi-window). We're currently only checking basic single-window/
// single-tab functionality.

IN_PROC_BROWSER_TEST_F(TwoClientLiveSessionsSyncTest, SingleClientChanged) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_TRUE(CheckInitialState(0));
  ASSERT_TRUE(CheckInitialState(1));

  std::vector<SessionWindow*>* client0_windows =
      InitializeNewWindowWithTab(0, GURL("about:bubba"));
  ASSERT_TRUE(client0_windows);

  GetClient(0)->AwaitMutualSyncCycleCompletion(GetClient(1));

  // Get foreign session data from client 1.
  ScopedVector<ForeignSession> sessions1;
  ASSERT_TRUE(GetSessionData(1, &sessions1.get()));

  // Verify client 1's foreign session matches client 0 current window.
  ASSERT_EQ(1U, sessions1.size());
  ASSERT_TRUE(WindowsMatch(sessions1[0]->windows, *client0_windows));
}

IN_PROC_BROWSER_TEST_F(TwoClientLiveSessionsSyncTest, BothChanged) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";

  ASSERT_TRUE(CheckInitialState(0));
  ASSERT_TRUE(CheckInitialState(1));

  // Open tabs on both clients and retain window information.
  std::vector<SessionWindow*>* client0_windows =
      InitializeNewWindowWithTab(0, GURL("about:bubba0"));
  ASSERT_TRUE(client0_windows);
  std::vector<SessionWindow*>* client1_windows =
      InitializeNewWindowWithTab(1, GURL("about:bubba1"));
  ASSERT_TRUE(client1_windows);

  // Wait for sync.
  ASSERT_TRUE(AwaitQuiescence());

  // Get foreign session data from client 0 and 1.
  ScopedVector<ForeignSession> sessions0;
  ScopedVector<ForeignSession> sessions1;
  ASSERT_TRUE(GetSessionData(0, &sessions0.get()));
  ASSERT_TRUE(GetSessionData(1, &sessions1.get()));

  // Verify client 1's foreign session matches client 0's current window and
  // vice versa.
  ASSERT_EQ(1U, sessions0.size());
  ASSERT_EQ(1U, sessions1.size());
  ASSERT_TRUE(WindowsMatch(sessions1[0]->windows, *client0_windows));
  ASSERT_TRUE(WindowsMatch(sessions0[0]->windows, *client1_windows));
}

