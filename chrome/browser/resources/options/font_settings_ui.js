// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('options', function() {

  /////////////////////////////////////////////////////////////////////////////
  // MinimumFontSizeSelect class:

  // Define a constructor that uses a select element as its underlying element.
  var MinimumFontSizeSelect = cr.ui.define('select');

  MinimumFontSizeSelect.prototype = {
    // Set up the prototype chain
    __proto__: HTMLSelectElement.prototype,

    /**
    * Initialization function for the cr.ui framework.
    */
    decorate: function() {
      var self = this;

      // Listen to pref changes.
      Preferences.getInstance().addEventListener(
          'webkit.webprefs.minimum_font_size', function(event) {
            var value = (event.value && event.value['value'] != undefined)
                ? event.value['value'] : event.value;
            self.managed = (event.value && event.value['managed'] != undefined)
                ? event.value['managed'] : false;
            self.disabled = self.managed;
            for (var i = 0; i < self.options.length; i++) {
              if (self.options[i].value == value) {
                self.selectedIndex = i;
                return;
              }
            }
            // Item not found, select first item.
            self.selectedIndex = 0;
          });

      // Listen to user events.
      this.addEventListener('change',
          function(e) {
            if (self.options[self.selectedIndex].value > 0) {
              Preferences.setIntegerPref(
                  'webkit.webprefs.minimum_font_size',
                   self.options[self.selectedIndex].value,
                   'Options_ChangeMinimumFontSize');
              Preferences.setIntegerPref(
                  'webkit.webprefs.minimum_logical_font_size',
                   self.options[self.selectedIndex].value, '');
            } else {
              Preferences.clearPref(
                  'webkit.webprefs.minimum_font_size',
                   'Options_ChangeMinimumFontSize');
              Preferences.clearPref(
                  'webkit.webprefs.minimum_logical_font_size', '');
            }
          });
    },

    /**
     * Sets up options in select element.
     * @param {Array} options List of option and their display text.
     * Each element in the array is an array of length 2 which contains options
     * value in the first element and display text in the second element.
     *
     * TODO(zelidrag): move this to that i18n template classes.
     */
    initializeValues: function(options) {
      options.forEach(function(values) {
        this.appendChild(new Option(values[1], values[0]));
      }, this);
    }
  };

  // Export
  return {
    MinimumFontSizeSelect: MinimumFontSizeSelect
  };

});

