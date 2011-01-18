#!/usr/bin/python

# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

import pyauto_functional  # Must be imported before pyauto
import pyauto


class InstantTest(pyauto.PyUITest):
  """TestCase for Omnibox Instant feature."""

  def setUp(self):
    pyauto.PyUITest.setUp(self)
    self.SetPrefs(pyauto.kInstantEnabled, True)

  def _DoneLoading(self):
    info = self.GetInstantInfo()
    return info.get('current') and not info.get('loading')

  def testInstantNavigation(self):
    """Test that instant navigates based on omnibox input."""
    self.SetOmniboxText('google.com')
    self.assertTrue(self.WaitUntil(self._DoneLoading))
    location = self.GetInstantInfo()['location']
    self.assertTrue('google.com' in location,
                    msg='No google.com in %s' % location)

    self.SetOmniboxText('google.es')
    self.assertTrue(self.WaitUntil(self._DoneLoading))
    location = self.GetInstantInfo()['location']
    self.assertTrue('google.es' in location,
                    msg='No google.es in %s' % location)

    # Initiate instant search (at default google.com).
    self.SetOmniboxText('chrome instant')
    self.assertTrue(self.WaitUntil(self._DoneLoading))
    location = self.GetInstantInfo()['location']
    self.assertTrue('google.com' in location,
                    msg='No google.com in %s' % location)

  def testInstantDisabledInIncognito(self):
    """Test that instant is disabled in Incognito mode."""
    self.RunCommand(pyauto.IDC_NEW_INCOGNITO_WINDOW)
    self.SetOmniboxText('google.com', windex=1)
    self.assertFalse(self.GetInstantInfo()['active'],
                     'Instant enabled in Incognito mode.')

  def testInstantOverlayNotStoredInHistory(self):
    """Test that instant overlay page is not stored in history."""
    url = self.GetFileURLForPath(os.path.join(self.DataDir(), 'title2.html'))
    self.SetOmniboxText(url)
    self.assertTrue(self.WaitUntil(self._DoneLoading))
    history = self.GetHistoryInfo().History()
    self.assertEqual(0, len(history))

  def testInstantDisabledForJavaScript(self):
    """Test that instant is disabled for javascript URLs."""
    self.SetOmniboxText('javascript:')
    self.assertFalse(self.GetInstantInfo()['active'],
                     'Instant enabled for javascript URL.')


if __name__ == '__main__':
  pyauto_functional.Main()
