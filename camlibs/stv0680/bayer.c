/*
 * STV0680 Vision Camera Chipset Driver
 * Copyright (C) 2000 Adam Harrison <adam@antispin.org> 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   
 */

#include <stdio.h>
#include <math.h>
#include "bayer.h"

/* Enhanced by Kurt Garloff to do scaling and debayering at the same time. */
void bayer_unshuffle_preview(int w, int h, int scale, unsigned char *raw, unsigned char *output)
{

    int x, y, nx, ny;
    int colour; int rgb[3];
    int nw = w >> scale;
    int nh = h >> scale;
    int incr = 1<<scale;

    for (ny = 0; ny < nh; ++ny, raw += w<<scale) {
	for (nx = 0; nx < nw; ++nx, output += 3) {
	    rgb[0] = 0; rgb[1] = 0; rgb[2] = 0;
	    for (y = 0; y < incr; ++y) {
		for (x = 0; x < incr; ++x) {
		    colour = 1 - (x&1) + (y&1);
		    rgb[colour] += raw[y*w + (nx<<(scale-1))+(x>>1) + ((x&1)? 0: (w>>1))];
		}
	    }
	    output[0] = rgb[0]>>(2*scale-2);
	    output[1] = rgb[1]>>(2*scale-1);
	    output[2] = rgb[2]>>(2*scale-2);
	}
    }
}

/****** gamma correction from trans[], plus hardcoded white balance */
/* Thanks to Alexander Schwartx <alexander.schwartx@gmx.net> for this code.
   Gamma correction (trans[] values generated by (pow((i-17)/256, GAMMA)*255)
   where GAMMA=0.5, 1<i<255.  */

#define GAMMA 0.450
#define ZERO 17.0

void light_enhance(int vw, int vh, int coarse, int fine, unsigned char *output)
{
    unsigned long int i;
    int lt=3; /* 3 is auto */
    float wb[3][3];
    double x, y;
    unsigned int trans[258], v;    
    int tmp1, tmp2, tmp3, whitex=20, whitey=20, j, k;

    double brightness = 1.0; /* FIXME: configurable? */
    
    /*fprintf(stderr, "(FineExp=%i CoarseExp=%i => filter=", fine, coarse); */

    if (fine >= (coarse<<1)) {
	lt = 0;
	/*fprintf(stderr, "natural)\n");*/
    } else if (((fine<<1) < coarse) && (coarse < 400)) {
	lt = 2;
	/*fprintf(stderr, "incandescent)\n");*/
    } else {
	lt = 1;
	/*fprintf(stderr, "fluorescent)\n");*/
    }
    x = brightness;
    wb[0][0] = 1.08 * x;  wb[0][1] = 1.00 * x;  wb[0][2] = 0.95 * x;
    wb[1][0] = 1.05 * x;  wb[1][1] = 1.00 * x;  wb[1][2] = 1.00 * x;
    wb[2][0] = 0.90 * x;  wb[2][1] = 1.00 * x;  wb[2][2] = 1.10 * x;
	    
    /* find white pixel */
    for (j=0;j<vh;j++)
    {
	for (k=0; k<vw; k++)
	{
	    i = (j*vw + k)*3;
	    tmp1 = abs(*(output+i) - *(output+i+1)); 
	    tmp2 = abs(*(output+i) - *(output+i+2)); 
	    tmp3 = abs(*(output+i+1) - *(output+i+2));
	    if ((tmp1<20) && (tmp2<20) && (tmp3<20) && (*(output+i)>100)) {
		whitex = k;  whitey = j;
		break;
	    }
	}
    }
    
    for(i=1; i<256; ++i)
    {
	x=i; x -= ZERO;
	if (x<1.0)
	    x = 1.0;
	y = pow((x/256.0),GAMMA)*255.0; 	
	trans[i] = (unsigned int) y;
    }
    trans[0] = 0;

    for (i=0;i<(vw*vh*3);i+=3)
    {   /* this (adjusting white) isn't quite right yet, so I turned it off */
	if ( (abs(*(output+i)-*(output+i+1))<1) &&  
	     (abs(*(output+i)-*(output+i+2))<1) &&
	     (abs(*(output+i+1)-*(output+i+2))<1)) {
    		y = trans[*(output+i)]*wb[lt][1]; v = (unsigned int)y;
		v = (v > 255) ? 255 : v;
		*(output+i) = (unsigned char) (v);
		*(output+i+1) = (unsigned char) (v);
		*(output+i+2) = (unsigned char) (v);
//		fprintf(stdout,"Adjusting white\n");
	} else {          /*  this is OK  */
		y = trans[*(output+i)]*wb[lt][0]; v = (unsigned int)y;
		*(output+i) = (unsigned char) (v > 255) ? 255 : v;
		y = trans[*(output+i+1)]*wb[lt][1]; v = (unsigned int)y;
		*(output+i+1) = (unsigned char) (v > 255) ? 255 : v;
		y = trans[*(output+i+2)]*wb[lt][2]; v = (unsigned int)y;
		*(output+i+2) = (unsigned char) (v > 255) ? 255 : v;
	}
    }  /* for */

}  /* light_enhance  */


