
#include "image.h"

#include <png.h>
#include <boost/static_assert.hpp>

BOOST_STATIC_ASSERT(sizeof(unsigned long) == 4);

Image imageFromPng(const string &fname) {
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  CHECK(png_ptr);

  png_infop info_ptr = png_create_info_struct(png_ptr);
  CHECK(info_ptr);
  
  if(setjmp(png_jmpbuf(png_ptr))) {
    CHECK(0);
  }
  
  FILE *fp = fopen(fname.c_str(), "rb");
  CHECK(fp);
  
  png_init_io(png_ptr, fp);
  
  png_read_info(png_ptr, info_ptr);

  if(info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);
  if(info_ptr->color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8)
    png_set_gray_1_2_4_to_8(png_ptr);
  if(info_ptr->bit_depth == 16)
    png_set_strip_16(png_ptr);
  if(info_ptr->bit_depth < 8)
    png_set_packing(png_ptr);
  if(info_ptr->color_type == PNG_COLOR_TYPE_RGB)
    png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  if(info_ptr->color_type == PNG_COLOR_TYPE_GRAY || info_ptr->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  CHECK(info_ptr->bit_depth == 8);
  CHECK(info_ptr->color_type == PNG_COLOR_TYPE_RGBA || info_ptr->color_type == PNG_COLOR_TYPE_RGB);
  
  Image img;
  img.x = info_ptr->width;
  img.y = info_ptr->height;
  
  img.c.resize(img.y, vector<unsigned long>(img.x));

  vector<unsigned long *> ul;
  for(int i = 0; i < img.c.size(); i++)
    ul.push_back(&img.c[i][0]);
  
  png_read_image(png_ptr, (png_byte**)&ul[0]);
  
  fclose(fp);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  
  return img;
};
