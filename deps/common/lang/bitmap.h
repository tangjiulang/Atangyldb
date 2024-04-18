#ifndef __COMMON_LANG_BITMAP_H__
#define __COMMON_LANG_BITMAP_H__

namespace common
{

class Bitmap {
public:
  Bitmap(char *bitmap, int size);

  bool get_bit(int index);
  void set_bit(int index);
  void clear_bit(int index);

  int  next_unsetted_bit(int start);
  int  next_setted_bit(int start);

private:
  char * bitmap_;
  int    size_;
};

} // namespace common

#endif // __COMMON_LANG_BITMAP_H__