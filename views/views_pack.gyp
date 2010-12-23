{
  'variables': {
    'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/chrome',
    'grit_info_cmd': ['python', '../chrome/tools/grit/grit_info.py'],
    'grit_cmd': ['python', '../chrome/tools/grit/grit.py'],
    'repack_locales_cmd': ['python', '../chrome/tools/build/repack_locales.py'],
    # TODO: remove this helper when we have loops in GYP
    'apply_locales_cmd': ['python', '<(DEPTH)/build/apply_locales.py'],
    'directxsdk_exists': '<!(python <(DEPTH)/build/dir_exists.py ../third_party/directxsdk)',
  },
      'targets': [{
        'target_name': 'packed_resources',
        'type': 'none',
        'variables': {
          'repack_path': '../tools/data_pack/repack.py',
        },
        'actions': [
          # TODO(mark): These actions are duplicated for the Mac in the
          # chrome_dll target.  Can they be unified?
          #
          # Mac needs 'process_outputs_as_mac_bundle_resources' to be set,
          # and the option is only effective when the target type is native
          # binary. Hence we cannot build the Mac bundle resources here.
          {
            'action_name': 'repack_chrome',
            'variables': {
              'pak_inputs': [
                #'<(grit_out_dir)/browser_resources.pak',
                #'<(grit_out_dir)/common_resources.pak',
                #'<(grit_out_dir)/default_plugin_resources/default_plugin_resources.pak',
                #'<(grit_out_dir)/renderer_resources.pak',
                #'<(grit_out_dir)/theme_resources.pak',
                #'<(SHARED_INTERMEDIATE_DIR)/app/app_resources/app_resources.pak',
                #'<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
                #'<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_chromium_resources.pak',
                #'<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_resources.pak',
              ],
            },
            'inputs': [
              '<(repack_path)',
              '<@(pak_inputs)',
            ],
            'outputs': [
              '<(INTERMEDIATE_DIR)/repack/chrome.pak',
            ],
            'action': ['python', '<(repack_path)', '<@(_outputs)',
                       '<@(pak_inputs)'],
          },
          {
            'action_name': 'repack_locales',
            'variables': {
              'conditions': [
                ['branding=="Chrome"', {
                  'branding_flag': ['-b', 'google_chrome',],
                }, {  # else: branding!="Chrome"
                  'branding_flag': ['-b', 'chromium',],
                }],
              ],
            },
            'inputs': [
              'tools/build/repack_locales.py',
              # NOTE: Ideally the common command args would be shared amongst
              # inputs/outputs/action, but the args include shell variables
              # which need to be passed intact, and command expansion wants
              # to expand the shell variables. Adding the explicit quoting
              # here was the only way it seemed to work.
              '>!@(<(repack_locales_cmd) -i <(branding_flag) -g \'<(grit_out_dir)\' -s \'<(SHARED_INTERMEDIATE_DIR)\' -x \'<(INTERMEDIATE_DIR)\' <(locales))',
            ],
            'outputs': [
              '>!@(<(repack_locales_cmd) -o -g \'<(grit_out_dir)\' -s \'<(SHARED_INTERMEDIATE_DIR)\' -x \'<(INTERMEDIATE_DIR)\' <(locales))',
            ],
            'action': [
              '<@(repack_locales_cmd)',
              '<@(branding_flag)',
              '-g', '<(grit_out_dir)',
              '-s', '<(SHARED_INTERMEDIATE_DIR)',
              '-x', '<(INTERMEDIATE_DIR)',
              '<@(locales)',
            ],
          }
        ]
      }]
}