// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "app/text_elider.h"
#include "base/file_path.h"
#include "base/i18n/rtl.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "base/sys_string_conversions.h"
#include "base/utf_string_conversions.h"
#include "gfx/font.h"
#include "googleurl/src/gurl.h"
#include "net/base/escape.h"
#include "net/base/net_util.h"
#include "net/base/registry_controlled_domain.h"

namespace {

const wchar_t kEllipsis[] = L"\x2026";

// Cuts |text| to be |length| characters long.  If |cut_in_middle| is true, the
// middle of the string is removed to leave equal-length pieces from the
// beginning and end of the string; otherwise, the end of the string is removed
// and only the beginning remains.  If |insert_ellipsis| is true, then an
// ellipsis character will by inserted at the cut point.
string16 CutString(const string16& text,
                       size_t length,
                       bool cut_in_middle,
                       bool insert_ellipsis) {
  // TODO(tony): This is wrong, it might split the string in the middle of a
  // surrogate pair.
  const string16 kInsert = WideToUTF16(insert_ellipsis ? kEllipsis : L"");
  if (!cut_in_middle)
    return text.substr(0, length) + kInsert;
  // We put the extra character, if any, before the cut.
  const size_t half_length = length / 2;
  return text.substr(0, length - half_length) + kInsert +
      text.substr(text.length() - half_length, half_length);
}

}  // namespace

namespace gfx {

// This function takes a GURL object and elides it. It returns a string
// which composed of parts from subdomain, domain, path, filename and query.
// A "..." is added automatically at the end if the elided string is bigger
// than the available pixel width. For available pixel width = 0, a formatted,
// but un-elided, string is returned.
//
// TODO(pkasting): http://b/119635 This whole function gets
// kerning/ligatures/etc. issues potentially wrong by assuming that the width of
// a rendered string is always the sum of the widths of its substrings.  Also I
// suspect it could be made simpler.
string16 ElideUrl(const GURL& url,
                  const gfx::Font& font,
                  int available_pixel_width,
                  const std::wstring& languages) {
  // Get a formatted string and corresponding parsing of the url.
  url_parse::Parsed parsed;
  string16 url_string = net::FormatUrl(url, WideToUTF8(languages),
      net::kFormatUrlOmitAll, UnescapeRule::SPACES, &parsed, NULL, NULL);
  if (available_pixel_width <= 0)
    return url_string;

  // If non-standard or not file type, return plain eliding.
  if (!(url.SchemeIsFile() || url.IsStandard()))
    return ElideText(url_string, font, available_pixel_width, false);

  // Now start eliding url_string to fit within available pixel width.
  // Fist pass - check to see whether entire url_string fits.
  int pixel_width_url_string = font.GetStringWidth(UTF16ToWideHack(url_string));
  if (available_pixel_width >= pixel_width_url_string)
    return url_string;

  // Get the path substring, including query and reference.
  size_t path_start_index = parsed.path.begin;
  size_t path_len = parsed.path.len;
  string16 url_path_query_etc = url_string.substr(path_start_index);
  string16 url_path = url_string.substr(path_start_index, path_len);

  // Return general elided text if url minus the query fits.
  string16 url_minus_query = url_string.substr(0, path_start_index + path_len);
  if (available_pixel_width >=
      font.GetStringWidth(UTF16ToWideHack(url_minus_query)))
    return ElideText(url_string, font, available_pixel_width, false);

  // Get Host.
  string16 url_host = UTF8ToUTF16(url.host());

  // Get domain and registry information from the URL.
  string16 url_domain = UTF8ToUTF16(
      net::RegistryControlledDomainService::GetDomainAndRegistry(url));
  if (url_domain.empty())
    url_domain = url_host;

  // Add port if required.
  if (!url.port().empty()) {
    url_host += UTF8ToUTF16(":" + url.port());
    url_domain += UTF8ToUTF16(":" + url.port());
  }

  // Get sub domain.
  string16 url_subdomain;
  size_t domain_start_index = url_host.find(url_domain);
  if (domain_start_index > 0)
    url_subdomain = url_host.substr(0, domain_start_index);
  static const string16 kWwwPrefix = UTF8ToUTF16("www.");
  if ((url_subdomain == kWwwPrefix || url_subdomain.empty() ||
      url.SchemeIsFile())) {
    url_subdomain.clear();
  }

  // If this is a file type, the path is now defined as everything after ":".
  // For example, "C:/aa/aa/bb", the path is "/aa/bb/cc". Interesting, the
  // domain is now C: - this is a nice hack for eliding to work pleasantly.
  if (url.SchemeIsFile()) {
    // Split the path string using ":"
    std::vector<string16> file_path_split;
    base::SplitString(url_path, ':', &file_path_split);
    if (file_path_split.size() > 1) {  // File is of type "file:///C:/.."
      url_host.clear();
      url_domain.clear();
      url_subdomain.clear();

      static const string16 kColon = UTF8ToUTF16(":");
      url_host = url_domain = file_path_split.at(0).substr(1) + kColon;
      url_path_query_etc = url_path = file_path_split.at(1);
    }
  }

  // Second Pass - remove scheme - the rest fits.
  int pixel_width_url_host = font.GetStringWidth(UTF16ToWideHack(url_host));
  int pixel_width_url_path = font.GetStringWidth(UTF16ToWideHack(
      url_path_query_etc));
  if (available_pixel_width >=
      pixel_width_url_host + pixel_width_url_path)
    return url_host + url_path_query_etc;

  // Third Pass: Subdomain, domain and entire path fits.
  int pixel_width_url_domain = font.GetStringWidth(UTF16ToWideHack(url_domain));
  int pixel_width_url_subdomain = font.GetStringWidth(UTF16ToWideHack(
      url_subdomain));
  if (available_pixel_width >=
      pixel_width_url_subdomain + pixel_width_url_domain +
      pixel_width_url_path)
    return url_subdomain + url_domain + url_path_query_etc;

  // Query element.
  string16 url_query;
  const int kPixelWidthDotsTrailer = font.GetStringWidth(kEllipsis);
  if (parsed.query.is_nonempty()) {
    url_query = UTF8ToUTF16("?") + url_string.substr(parsed.query.begin);
    if (available_pixel_width >= (pixel_width_url_subdomain +
        pixel_width_url_domain + pixel_width_url_path -
        font.GetStringWidth(UTF16ToWideHack(url_query)))) {
      return ElideText(url_subdomain + url_domain + url_path_query_etc,
                       font, available_pixel_width, false);
    }
  }

  // Parse url_path using '/'.
  static const string16 kForwardSlash = UTF8ToUTF16("/");
  std::vector<string16> url_path_elements;
  base::SplitString(url_path, kForwardSlash[0], &url_path_elements);

  // Get filename - note that for a path ending with /
  // such as www.google.com/intl/ads/, the file name is ads/.
  size_t url_path_number_of_elements = url_path_elements.size();
  DCHECK(url_path_number_of_elements != 0);
  string16 url_filename;
  if ((url_path_elements.at(url_path_number_of_elements - 1)).length() > 0) {
    url_filename = *(url_path_elements.end() - 1);
  } else if (url_path_number_of_elements > 1) {  // Path ends with a '/'.
    url_filename = url_path_elements.at(url_path_number_of_elements - 2) +
        kForwardSlash;
    url_path_number_of_elements--;
  }
  DCHECK(url_path_number_of_elements != 0);

  const size_t kMaxNumberOfUrlPathElementsAllowed = 1024;
  if (url_path_number_of_elements <= 1 ||
      url_path_number_of_elements > kMaxNumberOfUrlPathElementsAllowed) {
    // No path to elide, or too long of a path (could overflow in loop below)
    // Just elide this as a text string.
    return ElideText(url_subdomain + url_domain + url_path_query_etc, font,
                     available_pixel_width, false);
  }

  // Start eliding the path and replacing elements by "../".
  static const string16 kEllipsisAndSlash = WideToUTF16(kEllipsis) +
      kForwardSlash;
  int pixel_width_url_filename = font.GetStringWidth(UTF16ToWideHack(
      url_filename));
  int pixel_width_dot_dot_slash = font.GetStringWidth(UTF16ToWideHack(
      kEllipsisAndSlash));
  int pixel_width_slash = font.GetStringWidth(L"/");
  int pixel_width_url_path_elements[kMaxNumberOfUrlPathElementsAllowed];
  for (size_t i = 0; i < url_path_number_of_elements; ++i) {
    pixel_width_url_path_elements[i] =
       font.GetStringWidth(UTF16ToWideHack(url_path_elements.at(i)));
  }

  // Check with both subdomain and domain.
  string16 elided_path;
  int pixel_width_elided_path;
  for (size_t i = url_path_number_of_elements - 1; i >= 1; --i) {
    // Add the initial elements of the path.
    elided_path.clear();
    pixel_width_elided_path = 0;
    for (size_t j = 0; j < i; ++j) {
      elided_path += url_path_elements.at(j) + kForwardSlash;
      pixel_width_elided_path += pixel_width_url_path_elements[j] +
          pixel_width_slash;
    }

    // Add url_file_name.
    if (i == (url_path_number_of_elements - 1)) {
      elided_path += url_filename;
      pixel_width_elided_path += pixel_width_url_filename;
    } else {
      elided_path += kEllipsisAndSlash + url_filename;
      pixel_width_elided_path += pixel_width_dot_dot_slash +
          pixel_width_url_filename;
    }

    if (available_pixel_width >=
        pixel_width_url_subdomain + pixel_width_url_domain +
        pixel_width_elided_path) {
      return ElideText(url_subdomain + url_domain + elided_path + url_query,
                       font, available_pixel_width, false);
    }
  }

  // Check with only domain.
  // If a subdomain is present, add an ellipsis before domain.
  // This is added only if the subdomain pixel width is larger than
  // the pixel width of kEllipsis. Otherwise, subdomain remains,
  // which means that this case has been resolved earlier.
  string16 url_elided_domain = url_subdomain + url_domain;
  int pixel_width_url_elided_domain = pixel_width_url_domain;
  if (pixel_width_url_subdomain > kPixelWidthDotsTrailer) {
    if (!url_subdomain.empty()) {
      url_elided_domain = kEllipsisAndSlash[0] + url_domain;
      pixel_width_url_elided_domain += kPixelWidthDotsTrailer;
    } else {
      url_elided_domain = url_domain;
    }

    for (size_t i = url_path_number_of_elements - 1; i >= 1; --i) {
      // Add the initial elements of the path.
      elided_path.clear();
      pixel_width_elided_path = 0;
      for (size_t j = 0; j < i; ++j) {
        elided_path += url_path_elements.at(j) + kForwardSlash;
        pixel_width_elided_path += pixel_width_url_path_elements[j] +
            pixel_width_slash;
      }

      // Add url_file_name.
      if (i == (url_path_number_of_elements - 1)) {
        elided_path += url_filename;
        pixel_width_elided_path += pixel_width_url_filename;
      } else {
        elided_path += kEllipsisAndSlash + url_filename;
        pixel_width_elided_path += pixel_width_dot_dot_slash +
            pixel_width_url_filename;
      }

      if (available_pixel_width >=
          pixel_width_url_elided_domain + pixel_width_elided_path) {
        return ElideText(url_elided_domain + elided_path + url_query, font,
                         available_pixel_width, false);
      }
    }
  }

  // Return elided domain/../filename anyway.
  string16 final_elided_url_string(url_elided_domain);
  int url_elided_domain_width = font.GetStringWidth(UTF16ToWideHack(
      url_elided_domain));
  if ((available_pixel_width - url_elided_domain_width) >
      pixel_width_dot_dot_slash + kPixelWidthDotsTrailer +
      font.GetStringWidth(L"UV"))  // A hack to prevent trailing "../...".
    final_elided_url_string += elided_path;
  else
    final_elided_url_string += url_path;

  return ElideText(final_elided_url_string, font, available_pixel_width, false);
}

string16 ElideFilename(const FilePath& filename,
                       const gfx::Font& font,
                       int available_pixel_width) {
#if defined(OS_WIN)
  string16 filename_utf16 = filename.value();
  string16 extension = filename.Extension();
  string16 rootname = filename.BaseName().RemoveExtension().value();
#elif defined(OS_POSIX)
  string16 filename_utf16 = WideToUTF16(base::SysNativeMBToWide(
      filename.value()));
  string16 extension = WideToUTF16(base::SysNativeMBToWide(
      filename.Extension()));
  string16 rootname = WideToUTF16(base::SysNativeMBToWide(
      filename.BaseName().RemoveExtension().value()));
#endif

  int full_width = font.GetStringWidth(UTF16ToWideHack(filename_utf16));
  if (full_width <= available_pixel_width)
    return base::i18n::GetDisplayStringInLTRDirectionality(filename_utf16);

  if (rootname.empty() || extension.empty()) {
    string16 elided_name = ElideText(filename_utf16, font,
                                     available_pixel_width, false);
    return base::i18n::GetDisplayStringInLTRDirectionality(elided_name);
  }

  int ext_width = font.GetStringWidth(UTF16ToWideHack(extension));
  int root_width = font.GetStringWidth(UTF16ToWideHack(rootname));

  // We may have trimmed the path.
  if (root_width + ext_width <= available_pixel_width) {
    string16 elided_name = rootname + extension;
    return base::i18n::GetDisplayStringInLTRDirectionality(elided_name);
  }

  int available_root_width = available_pixel_width - ext_width;
  string16 elided_name =
      ElideText(rootname, font, available_root_width, false);
  elided_name += extension;
  return base::i18n::GetDisplayStringInLTRDirectionality(elided_name);
}

// This function adds an ellipsis at the end of the text if the text
// does not fit the given pixel width.
string16 ElideText(const string16& text,
                   const gfx::Font& font,
                   int available_pixel_width,
                   bool elide_in_middle) {
  return text;

  if (text.empty())
    return text;

  int current_text_pixel_width = font.GetStringWidth(UTF16ToWideHack(text));

  // Pango will return 0 width for absurdly long strings. Cut the string in
  // half and try again.
  // This is caused by an int overflow in Pango (specifically, in
  // pango_glyph_string_extents_range). It's actually more subtle than just
  // returning 0, since on super absurdly long strings, the int can wrap and
  // return positive numbers again. Detecting that is probably not worth it
  // (eliding way too much from a ridiculous string is probably still
  // ridiculous), but we should check other widths for bogus values as well.
  if (current_text_pixel_width <= 0 && !text.empty()) {
    return ElideText(CutString(text, text.length() / 2, elide_in_middle, false),
                     font, available_pixel_width, false);
  }

  if (current_text_pixel_width <= available_pixel_width)
    return text;

  if (font.GetStringWidth(kEllipsis) > available_pixel_width)
    return string16();

  // Use binary search to compute the elided text.
  size_t lo = 0;
  size_t hi = text.length() - 1;
  for (size_t guess = (lo + hi) / 2; guess != lo; guess = (lo + hi) / 2) {
    // We check the length of the whole desired string at once to ensure we
    // handle kerning/ligatures/etc. correctly.
    int guess_length = font.GetStringWidth(UTF16ToWideHack(
        CutString(text, guess, elide_in_middle, true)));
    // Check again that we didn't hit a Pango width overflow. If so, cut the
    // current string in half and start over.
    if (guess_length <= 0) {
      return ElideText(CutString(text, guess / 2, elide_in_middle, false),
                       font, available_pixel_width, elide_in_middle);
    }
    if (guess_length > available_pixel_width)
      hi = guess;
    else
      lo = guess;
  }

  return CutString(text, lo, elide_in_middle, true);
}

// TODO(viettrungluu): convert |languages| to an |std::string|.
SortedDisplayURL::SortedDisplayURL(const GURL& url,
                                   const std::wstring& languages) {
  std::wstring host;
  net::AppendFormattedHost(url, languages, &host, NULL, NULL);
  sort_host_ = WideToUTF16Hack(host);
  string16 host_minus_www = net::StripWWW(WideToUTF16Hack(host));
  url_parse::Parsed parsed;
  display_url_ = net::FormatUrl(url, WideToUTF8(languages),
      net::kFormatUrlOmitAll, UnescapeRule::SPACES, &parsed, &prefix_end_,
      NULL);
  if (sort_host_.length() > host_minus_www.length()) {
    prefix_end_ += sort_host_.length() - host_minus_www.length();
    sort_host_.swap(host_minus_www);
  }
}

SortedDisplayURL::SortedDisplayURL() {
}

SortedDisplayURL::~SortedDisplayURL() {
}

int SortedDisplayURL::Compare(const SortedDisplayURL& other,
                              icu::Collator* collator) const {
  // Compare on hosts first. The host won't contain 'www.'.
  UErrorCode compare_status = U_ZERO_ERROR;
  UCollationResult host_compare_result = collator->compare(
      static_cast<const UChar*>(sort_host_.c_str()),
      static_cast<int>(sort_host_.length()),
      static_cast<const UChar*>(other.sort_host_.c_str()),
      static_cast<int>(other.sort_host_.length()),
      compare_status);
  DCHECK(U_SUCCESS(compare_status));
  if (host_compare_result != 0)
    return host_compare_result;

  // Hosts match, compare on the portion of the url after the host.
  string16 path = this->AfterHost();
  string16 o_path = other.AfterHost();
  compare_status = U_ZERO_ERROR;
  UCollationResult path_compare_result = collator->compare(
      static_cast<const UChar*>(path.c_str()),
      static_cast<int>(path.length()),
      static_cast<const UChar*>(o_path.c_str()),
      static_cast<int>(o_path.length()),
      compare_status);
  DCHECK(U_SUCCESS(compare_status));
  if (path_compare_result != 0)
    return path_compare_result;

  // Hosts and paths match, compare on the complete url. This'll push the www.
  // ones to the end.
  compare_status = U_ZERO_ERROR;
  UCollationResult display_url_compare_result = collator->compare(
      static_cast<const UChar*>(display_url_.c_str()),
      static_cast<int>(display_url_.length()),
      static_cast<const UChar*>(other.display_url_.c_str()),
      static_cast<int>(other.display_url_.length()),
      compare_status);
  DCHECK(U_SUCCESS(compare_status));
  return display_url_compare_result;
}

string16 SortedDisplayURL::AfterHost() const {
  size_t slash_index = display_url_.find(sort_host_, prefix_end_);
  if (slash_index == string16::npos) {
    NOTREACHED();
    return string16();
  }
  return display_url_.substr(slash_index + sort_host_.length());
}

bool ElideString(const std::wstring& input, int max_len, std::wstring* output) {
  DCHECK_GE(max_len, 0);
  if (static_cast<int>(input.length()) <= max_len) {
    output->assign(input);
    return false;
  }

  switch (max_len) {
    case 0:
      output->clear();
      break;
    case 1:
      output->assign(input.substr(0, 1));
      break;
    case 2:
      output->assign(input.substr(0, 2));
      break;
    case 3:
      output->assign(input.substr(0, 1) + L"." +
                     input.substr(input.length() - 1));
      break;
    case 4:
      output->assign(input.substr(0, 1) + L".." +
                     input.substr(input.length() - 1));
      break;
    default: {
      int rstr_len = (max_len - 3) / 2;
      int lstr_len = rstr_len + ((max_len - 3) % 2);
      output->assign(input.substr(0, lstr_len) + L"..." +
                     input.substr(input.length() - rstr_len));
      break;
    }
  }

  return true;
}

}  // namespace gfx
