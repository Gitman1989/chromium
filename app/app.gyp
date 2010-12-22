# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    # TODO: remove this helper when we have loops in GYP
    'apply_locales_cmd': ['python', '<(DEPTH)/build/apply_locales.py',],
    'chromium_code': 1,
    'grit_info_cmd': ['python', '../tools/grit/grit_info.py',],
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/app',
    'grit_cmd': ['python', '../tools/grit/grit.py'],
    'localizable_resources': [
      'resources/app_locale_settings.grd',
      'resources/app_strings.grd',
    ],
  },
  'includes': [
  ],
  'targets': [
    {
      'target_name': 'app_strings',
      'msvs_guid': 'AE9BF4A2-19C5-49D8-BB1A-F28496DD7051',
      'type': 'none',
      'rules': [
        {
          'rule_name': 'grit',
          'extension': 'grd',
          'inputs': [
            '<!@(<(grit_info_cmd) --inputs <(localizable_resources))',
          ],
          'outputs': [
            '<(grit_out_dir)/<(RULE_INPUT_ROOT)/grit/<(RULE_INPUT_ROOT).h',
            # TODO: remove this helper when we have loops in GYP
            '>!@(<(apply_locales_cmd) \'<(grit_out_dir)/<(RULE_INPUT_ROOT)/<(RULE_INPUT_ROOT)_ZZLOCALE.pak\' <(locales))',
          ],
          'action': ['<@(grit_cmd)', '-i', '<(RULE_INPUT_PATH)',
            'build', '-o', '<(grit_out_dir)/<(RULE_INPUT_ROOT)'],
          'message': 'Generating resources from <(RULE_INPUT_PATH)',
          'conditions': [
            ['use_titlecase_in_grd_files==1', {
              'action': ['-D', 'use_titlecase'],
            }],
            ['chromeos==1', {
              'action': ['-D', 'chromeos'],
            }],
          ],
        },
      ],
      'sources': [
        '<@(localizable_resources)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(grit_out_dir)/app_locale_settings',
          '<(grit_out_dir)/app_strings',
        ],
      },
      'conditions': [
        ['OS=="win"', {
          'dependencies': ['../build/win/system.gyp:cygwin'],
        }],
      ],
    },
    {
      'target_name': 'app_resources',
      'type': 'none',
      'msvs_guid': '3FBC4235-3FBD-46DF-AEDC-BADBBA13A095',
      'actions': [
        {
          'action_name': 'app_resources',
          'variables': {
            'input_path': 'resources/app_resources.grd',
          },
          'inputs': [
            '<!@(<(grit_info_cmd) --inputs <(input_path))',
          ],
          'outputs': [
            '<!@(<(grit_info_cmd) --outputs \'<(grit_out_dir)/app_resources\' <(input_path))',
          ],
          'action': ['<@(grit_cmd)',
                     '-i', '<(input_path)', 'build',
                     '-o', '<(grit_out_dir)/app_resources'],
          'conditions': [
            ['toolkit_views==1', {
              'action': ['-D', 'toolkit_views'],
            }],
          ],
          'message': 'Generating resources from <(input_path)',
        },
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(grit_out_dir)/app_resources',
        ],
      },
      'conditions': [
        ['OS=="win"', {
          'dependencies': ['../build/win/system.gyp:cygwin'],
        }],
      ],
    },
  ],
  'conditions': [
    ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
      'targets': [{
        'target_name': 'app_unittest_strings',
        'type': 'none',
        'variables': {
          'repack_path': '<(DEPTH)/tools/data_pack/repack.py',
        },
        'actions': [
          {
            'action_name': 'repack_app_unittest_strings',
            'variables': {
              'pak_inputs': [
                '<(grit_out_dir)/app_strings/app_strings_en-US.pak',
                '<(grit_out_dir)/app_locale_settings/app_locale_settings_en-US.pak',
              ],
            },
            'inputs': [
              '<(repack_path)',
              '<@(pak_inputs)',
            ],
            'outputs': [
              '<(PRODUCT_DIR)/app_unittests_strings/en-US.pak',
            ],
            'action': ['python', '<(repack_path)', '<@(_outputs)',
                       '<@(pak_inputs)'],
          },
        ],
        'copies': [
          {
            'destination': '<(PRODUCT_DIR)/app_unittests_strings',
            'files': [
              '<(grit_out_dir)/app_resources/app_resources.pak',
            ],
          },
        ],
      }],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
