#ifndef __SQ905_H__
#define __SQ905_H__

#include <libgphoto2_port/gphoto2-port.h>

typedef unsigned char SQData;

typedef enum {
	SQ_MODEL_ARGUS,
	SQ_MODEL_POCK_CAM,
	SQ_MODEL_PRECISION,
	SQ_MODEL_UNKNOWN
} SQModel;

int sq_reset             (GPPort *);
int sq_init              (GPPort *, SQModel *, SQData *);

/* Those functions don't need data transfer with the camera */
int sq_get_num_pics      (SQData *); 
int sq_get_comp_ratio    (SQData *, int n);
int sq_get_picture_width (SQData *, int n);

unsigned char *sq_read_data         (GPPort *, char *data, int size);
unsigned char *sq_read_picture_data (GPPort *, char *data, int size);

#endif

