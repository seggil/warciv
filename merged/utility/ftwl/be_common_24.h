/**********************************************************************
 Freeciv - Copyright (C) 2004 - The Freeciv Project
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifndef FC__BE_24_H
#define FC__BE_24_H

#define ENABLE_IMAGE_ACCESS_CHECK	0

/* Internal 24 bit format, 8 bit are for masking. */
struct image {
  int width, height;
  int pitch;
  unsigned char *data;
  struct ct_rect full_rect;
};

#define MASK_ALPHA  0x01
#define MASK_OPAQUE 0x02

struct Sprite {
  struct image *image;
};

struct osda {
  int magic;
  struct image *image;
  bool has_transparent_pixels;
};

struct image *image_create(int width, int height);
void image_destroy(struct image *image);
struct image *image_clone_sub(struct image *src, const struct ct_point *pos, 
			      const struct ct_size *size);
void image_copy_full(struct image *src, struct image *dest,
		     struct ct_rect *region);
void image_set_mask(const struct image *image, const struct ct_rect *rect,
		    unsigned char mask);

#define IMAGE_GET_ADDRESS(image, x, y) ((image)->data + (image)->pitch * (y) + 4 * (x))

#if ENABLE_IMAGE_ACCESS_CHECK
#define IMAGE_CHECK(image, x, y) \
if((image)->data==NULL || (x)<0 || (x)>=(image)->width || (y)<0 || (y)>=(image)->height) { \
    printf("ERROR: image data=%p size=(%dx%d) pos=(%d,%d)\n",\
	   (image)->data, (image)->width,(image)->height,(x),(y));\
    assert(0);\
}
#else
#define IMAGE_CHECK(image, x, y)
#endif

#endif
