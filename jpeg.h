/* jpeg.h
 * This code was written by Nathan Stenzel for gphoto
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GPHOTO2_JPEG_H__
#define __GPHOTO2_JPEG_H__
typedef enum {
    JPEG_START=0xD8,        JPEG_COMMENT=0xFE,      JPEG_APPO=0xE0,
    JPEG_QUANTIZATION=0xDB, JPEG_HUFFMAN=0xC4,      JPEG_SOFC0=0xC0,
    JPEG_SSSEAHAL=0xDA,     JPEG_EOI=0xD9
} jpegmarker;

const jpegmarker JPEG_MARKERS[] = {
    JPEG_START,             JPEG_COMMENT,           JPEG_APPO,
    JPEG_QUANTIZATION,      JPEG_HUFFMAN,           JPEG_SOFC0,
    JPEG_SSSEAHAL,          JPEG_EOI
};

const char *JPEG_MARKERNAMES[] = {
    "Start",                "Comment",              "APPO",
    "Quantization table",   "Huffman table",        "SOFC0",
    "SsSeAhAl",             "End of image"
};

typedef struct chunk{
    int size;
    unsigned char *data;
} chunk;

typedef char jpeg_quantization_table[64];

typedef struct jpeg {
    int count;
    struct chunk *marker[20]; /* I think this should be big enough */
}jpeg;

chunk *chunk_new(int length);
chunk *chunk_new_filled(int length, const char *data);
void chunk_destroy(chunk *mychunk);
void chunk_print(chunk *mychunk);

char  gp_jpeg_findff(int *location, chunk *picture);
char  gp_jpeg_findactivemarker(char *id, int *location, chunk *picture);
char *gp_jpeg_markername(int c);

void  gp_jpeg_init       (jpeg *myjpeg);
void  gp_jpeg_destroy    (jpeg *myjpeg);
void  gp_jpeg_add_marker (jpeg *myjpeg, chunk *picture, int start, int end);
void  gp_jpeg_add_chunk  (jpeg *myjpeg, chunk *source);
void  gp_jpeg_parse      (jpeg *myjpeg, chunk *picture);
void  gp_jpeg_print      (jpeg *myjpeg);

chunk *gp_jpeg_make_start   (void);
chunk *gp_jpeg_make_SOFC    (int width, int height,
			     char vh1, char vh2, char vh3,
			     char q1, char q2, char q3);
chunk *gp_jpeg_makeSsSeAhAl (int huffset1, int huffset2, int huffset3);

void gp_jpeg_print_quantization_table(jpeg_quantization_table *table);
chunk *gp_jpeg_make_quantization(jpeg_quantization_table *table, int number);
jpeg_quantization_table *gp_jpeg_quantization2table(chunk *qmarker);

jpeg *gp_jpeg_header(int width, int height,
    char vh1, char vh2, char vh3,
    char q1, char q2, char q3,
    jpeg_quantization_table *quant1, jpeg_quantization_table *quant2,
    char huffset1, char huffset2, char huffset3,
    chunk *huff1, chunk *huff2, chunk *huff3, chunk *huff4);

void gp_jpeg_write(CameraFile *file, const char *name, jpeg *myjpeg);
#endif
