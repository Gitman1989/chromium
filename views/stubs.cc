#include "base/file_path.h"
#include "googleurl/src/gurl.h"

namespace net {

GURL FilePathToFileURL(const FilePath& path) {
  return GURL("");
}

}

#include "base/string16.h"

namespace gfx {

class Font;  

std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> >
ElideText(std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > const& s, Font const&, int, bool)
{
  return s;
}

}
