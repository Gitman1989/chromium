# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.cros import chrome_test

class desktopui_SyncIntegrationTests(chrome_test.ChromeTestBase):
    version = 1

    def run_once(self):
        password_file = '%s/sync_password.txt' % self.bindir

        self.run_chrome_test('sync_integration_tests',
            ('--password-file-for-test=%s ' +
             '--test-terminate-timeout=300000') % password_file)
