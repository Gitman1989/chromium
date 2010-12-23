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

int ElideText(std::basic_string<unsigned short, base::string16_char_traits, std::allocator<unsigned short> > const&, Font const&, int, bool)
{
  return 0;
}

}
