# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


from grit.format.policy_templates.writers import template_writer


def GetWriter(config):
  '''Factory method for creating AdmWriter objects.
  See the constructor of TemplateWriter for description of
  arguments.
  '''
  return AdmWriter(['win'], config)


class AdmWriter(template_writer.TemplateWriter):
  '''Class for generating policy templates in Windows ADM format.
  It is used by PolicyTemplateGenerator to write ADM files.
  '''

  TYPE_TO_INPUT = {
    'string': 'EDITTEXT',
    'int': 'NUMERIC',
    'string-enum': 'DROPDOWNLIST',
    'int-enum': 'DROPDOWNLIST',
    'list': 'LISTBOX'}
  NEWLINE = '\r\n'

  def _AddGuiString(self, name, value):
    # Escape newlines in the value.
    value = value.replace('\n', '\\n')
    line = '%s="%s"' % (name, value)
    self.str_list.append(line)

  def _PrintLine(self, string='', indent_diff=0):
    '''Prints a string with indents and a linebreak to the output.

    Args:
      string: The string to print.
      indent_diff: the difference of indentation of the printed line,
        compared to the next/previous printed line. Increment occurs
        after printing the line, while decrement occurs before that.
    '''
    indent_diff *= 2
    if indent_diff < 0:
      self.indent = self.indent[(-indent_diff):]
    if string != '':
      self.policy_list.append(self.indent + string)
    else:
      self.policy_list.append('')
    if indent_diff > 0:
      self.indent += ''.ljust(indent_diff)

  def _WriteSupported(self):
    self._PrintLine('#if version >= 4', 1)
    self._PrintLine('SUPPORTED !!SUPPORTED_WINXPSP2')
    self._PrintLine('#endif', -1)

  def _WritePart(self, policy):
    '''Writes the PART ... END PART section of a policy.

    Args:
      policy: The policy to write to the output.
    '''
    policy_part_name = policy['name'] + '_Part'
    self._AddGuiString(policy_part_name, policy['label'])

    # Print the PART ... END PART section:
    self._PrintLine()
    adm_type = self.TYPE_TO_INPUT[policy['type']]
    self._PrintLine('PART !!%s  %s' % (policy_part_name, adm_type), 1)
    if policy['type'] == 'list':
      # Note that the following line causes FullArmor ADMX Migrator to create
      # corrupt ADMX files. Please use admx_writer to get ADMX files.
      self._PrintLine('KEYNAME "%s\\%s"' %
                      (self.config['win_reg_key_name'], policy['name']))
      self._PrintLine('VALUEPREFIX ""')
    else:
      self._PrintLine('VALUENAME "%s"' % policy['name'])
    if policy['type'] in ('int-enum', 'string-enum'):
      self._PrintLine('ITEMLIST', 1)
      for item in policy['items']:
        if policy['type'] == 'int-enum':
          value_text = 'NUMERIC ' + str(item['value'])
        else:
          value_text = '"' + item['value'] + '"'
        self._PrintLine('NAME !!%s_DropDown VALUE %s' %
            (item['name'], value_text))
        self._AddGuiString(item['name'] + '_DropDown', item['caption'])
      self._PrintLine('END ITEMLIST', -1)
    self._PrintLine('END PART', -1)

  def WritePolicy(self, policy):
    self._AddGuiString(policy['name'] + '_Policy', policy['caption'])
    self._PrintLine('POLICY !!%s_Policy' % policy['name'], 1)
    self._WriteSupported()
    policy_explain_name = policy['name'] + '_Explain'
    self._AddGuiString(policy_explain_name, policy['desc'])
    self._PrintLine('EXPLAIN !!' + policy_explain_name)

    if policy['type'] == 'main':
      self._PrintLine('VALUENAME "%s"' % policy['name'])
      self._PrintLine('VALUEON NUMERIC 1')
      self._PrintLine('VALUEOFF NUMERIC 0')
    else:
      self._WritePart(policy)

    self._PrintLine('END POLICY', -1)
    self._PrintLine()

  def BeginPolicyGroup(self, group):
    self._open_category = len(group['policies']) > 1
    # Open a category for the policies if there is more than one in the
    # group.
    if self._open_category:
      category_name = group['name'] + '_Category'
      self._AddGuiString(category_name, group['caption'])
      self._PrintLine('CATEGORY !!' + category_name, 1)

  def EndPolicyGroup(self):
    if self._open_category:
      self._PrintLine('END CATEGORY', -1)

  def BeginTemplate(self):
    category_path = self.config['win_category_path']
    self._AddGuiString(self.config['win_supported_os'],
                       self.messages['win_supported_winxpsp2']['text'])
    self._PrintLine(
        'CLASS ' + self.config['win_group_policy_class'].upper(),
        1)
    if self.config['build'] == 'chrome':
      self._AddGuiString(category_path[0], 'Google')
      self._AddGuiString(category_path[1], self.config['app_name'])
      self._PrintLine('CATEGORY !!' + category_path[0], 1)
      self._PrintLine('CATEGORY !!' + category_path[1], 1)
    elif self.config['build'] == 'chromium':
      self._AddGuiString(category_path[0], self.config['app_name'])
      self._PrintLine('CATEGORY !!' + category_path[0], 1)
    self._PrintLine('KEYNAME "%s"' % self.config['win_reg_key_name'])
    self._PrintLine()

  def EndTemplate(self):
    if self.config['build'] == 'chrome':
      self._PrintLine('END CATEGORY', -1)
      self._PrintLine('END CATEGORY', -1)
      self._PrintLine('', -1)
    elif self.config['build'] == 'chromium':
      self._PrintLine('END CATEGORY', -1)
      self._PrintLine('', -1)

  def Init(self):
    self.policy_list = []
    self.str_list = ['[Strings]']
    self.indent = ''

  def GetTemplateText(self):
    lines = self.policy_list + self.str_list
    return self.NEWLINE.join(lines)
