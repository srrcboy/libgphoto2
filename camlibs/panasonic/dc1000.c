/*
	$Id$

	Copyright (c) 2000 Mariusz Zynel <mariusz@mizar.org> (gPhoto port)
	Copyright (C) 2000 Fredrik Roubert <roubert@df.lth.se> (idea)
	Copyright (C) 1999 Galen Brooks <galen@nine.com> (DC1580 code)

	This file is part of the gPhoto project and may only be used,  modified,
	and distributed under the terms of the gPhoto project license,  COPYING.
	By continuing to use, modify, or distribute  this file you indicate that
	you have read the license, understand and accept it fully.
	
	THIS  SOFTWARE IS PROVIDED AS IS AND COME WITH NO WARRANTY  OF ANY KIND,
	EITHER  EXPRESSED OR IMPLIED.  IN NO EVENT WILL THE COPYRIGHT  HOLDER BE
	LIABLE FOR ANY DAMAGES RESULTING FROM THE USE OF THIS SOFTWARE.

	Note:
	
	This is a Panasonic PV/NV-DC1000 camera gPhoto library source code.
	
*/	
 
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define _(String) (String)
#    define N_(String) (String)
#  endif
#else
#  define _(String) (String)
#  define N_(String) (String)
#endif

#include "dc.h"
#include "dc1000.h"

#ifndef __FILE__
#  define __FILE__ "dc1000.c"
#endif

/******************************************************************************/

/* Internal utility functions */


/* dsc1_connect - try hand shake with camera and establish connection */

int dsc1_connect(dsc_t *dsc, int speed) {
	
	u_int8_t	data = 0;
	
	DEBUG_PRINT(("Connecting a camera."));
		
	if (dsc1_setbaudrate(dsc, speed) != GP_OK)
		return GP_ERROR;
	
	if (dsc1_getmodel(dsc) != DSC1)
		RETURN_ERROR(EDSCBADDSC, dsc1_connect);
		/* bad camera model */
	
	dsc1_sendcmd(dsc, DSC1_CMD_CONNECT, &data, 1);
	
	if (dsc1_retrcmd(dsc) != DSC1_RSP_OK) 
		RETURN_ERROR(EDSCBADRSP, dsc1_connect);
		/* bad response */

	DEBUG_PRINT(("Camera connected successfully."));
	
	return GP_OK;	
}

/* dsc1_disconnect - reset camera, free buffers and close files */

int dsc1_disconnect(dsc_t *dsc) {
	
	DEBUG_PRINT(("Disconnecting the camera."));
	
	if (dsc1_sendcmd(dsc, DSC1_CMD_RESET, NULL, 0) != GP_OK)
		return GP_ERROR;
	
	if (dsc1_retrcmd(dsc) != DSC1_RSP_OK) 
		RETURN_ERROR(EDSCBADRSP, dsc1_disconnect)
		/* bad response */	
	else 
		sleep(DSC_PAUSE); /* let camera to redraw its screen */
	
	DEBUG_PRINT(("Camera disconnected."));
	
	return GP_OK;
}

/* dsc1_getindex - retrieve the number of images stored in camera memory */

int dsc1_getindex(dsc_t *dsc) {
	
	int	result = GP_ERROR;

	DEBUG_PRINT(("Retrieving the number of images."));
		
	if (dsc1_sendcmd(dsc, DSC1_CMD_GET_INDEX, NULL, 0) != GP_OK)
		return GP_ERROR;
	
	if (dsc1_retrcmd(dsc) == DSC1_RSP_INDEX)
		result = dsc->size / 2;
	else
		RETURN_ERROR(EDSCBADRSP, dsc1_getindex);
		/* bad response */
	
	DEBUG_PRINT(("Number of images: %i", result));
	
	return result;
}

/* dsc1_delete - delete image #index from camera memory */

int dsc1_delete(dsc_t *dsc, u_int8_t index) {
	
	DEBUG_PRINT(("Deleting image: %i.", index));
		
	if (index < 1)
		RETURN_ERROR(EDSCBADNUM, dsc1_delete);
		/* bad image number */

	if (dsc1_sendcmd(dsc, DSC1_CMD_DELETE, &index, 1) != GP_OK)
		return GP_ERROR;

	if (dsc1_retrcmd(dsc) != DSC1_RSP_OK)
		RETURN_ERROR(EDSCBADRSP, dsc1_delete);
		/* bad response */

	DEBUG_PRINT(("Image: %i deleted.", index));

	return GP_OK;
}	
	
/* dsc1_selectimage - select image to download, return its size */

int dsc1_selectimage(dsc_t *dsc, u_int8_t index)
{
	int	size = 0;
	
	DEBUG_PRINT(("Selecting image: %i.", index));	
	
	if (index < 1)
		RETURN_ERROR(EDSCBADNUM, dsc1_selectimage);
		/* bad image number */

	if (dsc1_sendcmd(dsc, DSC1_CMD_SELECT, &index, 1) != GP_OK)
		return GP_ERROR;

	if (dsc1_retrcmd(dsc) != DSC1_RSP_IMGSIZE)
		RETURN_ERROR(EDSCBADRSP, dsc1_selectimage);
		/* bad response */
	
	if (dsc->size != 4)
		RETURN_ERROR(EDSCBADRSP, dsc1_selectimage);
		/* bad response */

	size =	(u_int32_t)dsc->buf[3] |
		((u_int8_t)dsc->buf[2] << 8) |
		((u_int8_t)dsc->buf[1] << 16) |
		((u_int8_t)dsc->buf[0] << 24);		
	
	DEBUG_PRINT(("Selected image: %i, size: %i.", index, size));

	return size;
}

/* gp_port_readimageblock - read #block block (1024 bytes) of an image into buf */

int dsc1_readimageblock(dsc_t *dsc, int block, char *buffer) {
	
	char	buf[2];
	
	DEBUG_PRINT(("Reading image block: %i.", block));

	buf[0] = (u_int8_t)(block >> 8);
	buf[1] = (u_int8_t)block;
	
	if (dsc1_sendcmd(dsc, DSC1_CMD_GET_DATA, buf, 2) != GP_OK)
		return GP_ERROR;
	
	if (dsc1_retrcmd(dsc) != DSC1_RSP_DATA)
		RETURN_ERROR(EDSCBADRSP, dsc1_readimageblock);
		/* bad response */

	if (buffer)
		memcpy(buffer, dsc->buf, dsc->size);
	
	DEBUG_PRINT(("Block: %i read in.", block));
	
	return dsc->size;
}

/* dsc1_readimage - read #index image or thumbnail and return its contents */

char *dsc1_readimage(dsc_t *dsc, int index, int *size) {

	int	rsize, i, s;
	char	*buffer = NULL;

	DEBUG_PRINT(("Reading image: %i.", index));
		
	if ((*size = dsc1_selectimage(dsc, index)) < 0)
		return NULL;
	
	if (!(buffer = (char*)malloc(*size))) {
		DEBUG_PRINT(("Failed to allocate memory for image data."));
		return NULL;
	}
	
	for (i = 0, s = 0; s < *size; i++) {
		if ((rsize = dsc1_readimageblock(dsc, i, &buffer[s])) == GP_ERROR) {
			DEBUG_PRINT(("Error during image transfer."));
			free(buffer);
			return NULL;
		}
		s += rsize;
	}

	DEBUG_PRINT(("Image: %i read in.", index));
	
	return buffer;
}

/* dsc1_setimageres - set image resolution based on its size, used on upload to camera */

int dsc1_setimageres(dsc_t *dsc, int size) {
	
	dsc_quality_t	res;
	
	DEBUG_PRINT(("Setting image resolution, image size: %i.", size));	
	
	if (size < 65536)
		res = normal;
	else if (size < 196608)
		res = fine;
	else
		res = superfine;

	if (dsc1_sendcmd(dsc, DSC1_CMD_SET_RES, &res, 1) != GP_OK)
		return GP_ERROR;

	if (dsc1_retrcmd(dsc) != DSC1_RSP_OK)
		RETURN_ERROR(EDSCBADRSP, dsc1_setimageres);
		/* bad response */
		
	DEBUG_PRINT(("Image resolution set to: %i", res));

	return GP_OK;
}

/* gp_port_writeimageblock - write size bytes from buffer rounded to 1024 bytes to camera */

int dsc1_writeimageblock(dsc_t *dsc, int block, char *buffer, int size) {

	DEBUG_PRINT(("Writing image block: %i", block));

	dsc1_sendcmd(dsc, DSC1_CMD_SEND_DATA, buffer, size);

	if (dsc1_retrcmd(dsc) != DSC1_RSP_OK)
		RETURN_ERROR(EDSCBADRSP, dsc1_writeimageblock);
		/* bad response */
		
	DEBUG_PRINT(("Block: %i of size: %i written.", block, size));
	
	return GP_OK;
}

/* gp_port_writeimage - write an image to camera memory, size bytes at buffer */

int dsc1_writeimage(dsc_t *dsc, char *buffer, int size)
{
	int	blocks, blocksize, i;

	DEBUG_PRINT(("Writing an image of size: %i.", size));
		
	if ((dsc1_setimageres(dsc, size)) != GP_OK)
		return GP_ERROR;
	
	blocks = (size - 1)/DSC_BLOCKSIZE + 1;
			
	for (i = 0; i < blocks; i++) {
		blocksize = size - i*DSC_BLOCKSIZE;
		if (DSC_BLOCKSIZE < blocksize) 
			blocksize = DSC_BLOCKSIZE;
		if (dsc1_writeimageblock(dsc, i, &buffer[i*DSC_BLOCKSIZE], blocksize) != GP_OK) {
			DEBUG_PRINT(("Error during image transfer."));
			return GP_ERROR;
		}
	}
	
	DEBUG_PRINT(("Image written successfully."));
	
	return GP_OK;
}

/* dsc1_preview - show selected image on camera's LCD - is it supported? */

int dsc1_preview(dsc_t *dsc, int index)
{
	if (index < 1)
		RETURN_ERROR(EDSCBADNUM, dsc1_preview);
		/* bad image number */

	if (dsc1_sendcmd(dsc, DSC1_CMD_PREVIEW, &index, 1) != GP_OK)
		return GP_ERROR;
	
	if (dsc1_retrcmd(dsc) != DSC1_RSP_OK)
		RETURN_ERROR(EDSCBADRSP, dsc1_preview);
		/* bad response */
	
	return GP_OK;
}

/******************************************************************************/

/* Library interface functions */

int camera_id (CameraText *id) {

	strcpy(id->text, "panasonic-dc1000");

	return (GP_OK);
}

int camera_abilities (CameraAbilitiesList *list) {

	int ret;
	CameraAbilities *a;

	ret = gp_abilities_new (&a);
	if (ret != GP_OK)
		return (ret);

	strcpy(a->model, "Panasonic DC1000");
	a->port		= GP_PORT_SERIAL;
	a->speed[0] 	= 9600;
	a->speed[1] 	= 19200;
	a->speed[2] 	= 38400;
	a->speed[3] 	= 57600;			
	a->speed[4] 	= 115200;	
	a->speed[5] 	= 0;	
	a->operations        = GP_OPERATION_NONE;
	a->file_operations   = GP_FILE_OPERATION_DELETE;
	a->folder_operations = GP_FOLDER_OPERATION_PUT_FILE;

	if (gp_abilities_list_append(list, a) == GP_ERROR)
		return GP_ERROR;

	return GP_OK;
}

int camera_exit (Camera *camera) {
	
	dsc_t	*dsc = (dsc_t *)camera->camlib_data;
	
	dsc_print_status(camera, _("Disconnecting camera."));
	
	dsc1_disconnect(dsc);
	
	if (dsc->dev) {
		gp_port_close(dsc->dev);
		gp_port_free(dsc->dev);
	}
	if (dsc->fs)
		gp_filesystem_free(dsc->fs);
	
	free(dsc);

	return (GP_OK);
}

int camera_folder_list_folders (Camera *camera, const char *folder, 
				CameraList *list) 
{
	return GP_OK; 	/* folders are unsupported but it is OK */
}

int camera_folder_list_files (Camera *camera, const char *folder, 
			      CameraList *list) 
{
	dsc_t	*dsc = (dsc_t *)camera->camlib_data;
	int 	count, i;
	const char *name;
	
	if ((count = dsc1_getindex (dsc)) == GP_ERROR)
		return GP_ERROR;
	
	gp_filesystem_populate (dsc->fs, "/", DSC_FILENAMEFMT, count);

	for (i = 0; i < count; i++) {
		gp_filesystem_name (dsc->fs, "/", i, &name);
		gp_list_append (list, name, NULL);
	}

	return GP_OK;
}

int camera_file_get (Camera *camera, const char *folder, const char *filename, 
		     CameraFileType type, CameraFile *file) 
{
	dsc_t	*dsc = (dsc_t *)camera->camlib_data;
	int	index, size, rsize, i, s;

	if (type != GP_FILE_TYPE_NORMAL)
		return (GP_ERROR_NOT_SUPPORTED);

	dsc_print_status(camera, _("Downloading image %s."), filename);

	/* index is the 0-based image number on the camera */
	index = gp_filesystem_number (dsc->fs, folder, filename);
	if (index < 0) 
		return (index);

	if ((size = dsc1_selectimage(dsc, index + 1)) < 0)
		return (GP_ERROR);
	
	gp_file_set_name (file, filename);
	gp_file_set_mime_type (file, "image/jpeg");

	gp_frontend_progress (camera, file, 0.00);

	for (i = 0, s = 0; s < size; i++) {
		if ((rsize = dsc1_readimageblock(dsc, i, NULL)) == GP_ERROR) 
			return GP_ERROR;
		s += rsize;
		gp_file_append (file, dsc->buf, dsc->size);
		gp_frontend_progress (camera, file, 
				      (float)(s)/(float)size*100.0);
	}
	
	return (GP_OK);
}

int camera_folder_put_file (Camera *camera, const char *folder, 
			    CameraFile *file) 
{
	dsc_t	*dsc = (dsc_t *)camera->camlib_data;
	int	blocks, blocksize, i;
	int	result;
	const char *name;
	const char *data;
	long int size;
	
	gp_file_get_name (file, &name);
	dsc_print_status(camera, _("Uploading image: %s."), name);	
	
/*	We can not figure out file type, at least by now.

	if (strcmp(file->type, "image/jpg") != 0) {
		dsc_print_message(camera, "JPEG image format allowed only.");
		return GP_ERROR;		
	}
*/	
	gp_file_get_data_and_size (file, &data, &size);
	if (size > DSC_MAXIMAGESIZE) {
		dsc_print_message (camera, _("File size is %i bytes. "
				   "The size of the largest file possible to "
				   "upload is: %i bytes."), size, 
				   DSC_MAXIMAGESIZE);
		return GP_ERROR;		
	}

	result = dsc1_setimageres (dsc, size);
	if (result != GP_OK)
		return (result);
	
	gp_frontend_progress(camera, file, 0.00);
	
	blocks = (size - 1)/DSC_BLOCKSIZE + 1;
			
	for (i = 0; i < blocks; i++) {
		blocksize = size - i*DSC_BLOCKSIZE;
		if (DSC_BLOCKSIZE < blocksize) 
			blocksize = DSC_BLOCKSIZE;
		result = dsc1_writeimageblock (dsc, i, 
					       (char*)&data[i*DSC_BLOCKSIZE], 
					       blocksize);
		if (result != GP_OK)
			return (result);

		gp_frontend_progress (camera, file, 
				      (float)(i+1)/(float)blocks*100.0);
	}

	return GP_OK;
}

int camera_file_delete (Camera *camera, const char *folder, 
			const char *filename) 
{
	dsc_t	*dsc = (dsc_t *)camera->camlib_data;
	int	index;

	dsc_print_status(camera, _("Deleting image %s."), filename);

	/* index is the 0-based image number on the camera */
	index = gp_filesystem_number (dsc->fs, folder, filename);
	if (index < 0)
		return (index);

	return dsc1_delete (dsc, index + 1);
}

int camera_summary (Camera *camera, CameraText *summary) 
{
	strcpy(summary->text, _("Summary not available."));

	return (GP_OK);
}

int camera_manual (Camera *camera, CameraText *manual) 
{
	strcpy (manual->text, _("Manual not available."));

	return (GP_OK);
}

int camera_about (Camera *camera, CameraText *about) 
{
	strcpy(about->text,
			_("Panasonic DC1000 gPhoto library\n"
			"Mariusz Zynel <mariusz@mizar.org>\n\n"
			"Based on dc1000 program written by\n"
			"Fredrik Roubert <roubert@df.lth.se> and\n"
			"Galen Brooks <galen@nine.com>."));
	return (GP_OK);
}

int camera_init (Camera *camera) {

        int ret;
        dsc_t           *dsc = NULL;
        
        dsc_print_status(camera, _("Initializing camera %s"), camera->model);   
        /* First, set up all the function pointers */
        camera->functions->exit                 = camera_exit;
        camera->functions->folder_list_folders  = camera_folder_list_folders;
        camera->functions->folder_list_files    = camera_folder_list_files;
        camera->functions->file_get             = camera_file_get;
        camera->functions->folder_put_file      = camera_folder_put_file;
        camera->functions->file_delete          = camera_file_delete;
        camera->functions->summary              = camera_summary;
        camera->functions->manual               = camera_manual;
        camera->functions->about                = camera_about;

        if (dsc && dsc->dev) {
                gp_port_close(dsc->dev);
                gp_port_free(dsc->dev);
        }
        free(dsc);

        /* first of all allocate memory for a dsc struct */
        if ((dsc = (dsc_t*)malloc(sizeof(dsc_t))) == NULL) { 
                dsc_errorprint(EDSCSERRNO, __FILE__, "camera_init", __LINE__);
                return GP_ERROR; 
        } 
        
        camera->camlib_data = dsc;
        
        if ((ret = gp_port_new(&(dsc->dev), GP_PORT_SERIAL)) < 0) {
            free(dsc);
            return (ret);
        }
        
        gp_port_timeout_set(dsc->dev, 5000);
        strcpy(dsc->settings.serial.port, camera->port_info->path);
        dsc->settings.serial.speed      = 9600; /* hand shake speed */
        dsc->settings.serial.bits       = 8;
        dsc->settings.serial.parity     = 0;
        dsc->settings.serial.stopbits   = 1;

        gp_port_settings_set(dsc->dev, dsc->settings);

        gp_port_open(dsc->dev);
        
        /* allocate memory for a dsc read/write buffer */
        if ((dsc->buf = (char *)malloc(sizeof(char)*(DSC_BUFSIZE))) == NULL) {
                dsc_errorprint(EDSCSERRNO, __FILE__, "camera_init", __LINE__);
                free(dsc);
                return GP_ERROR;
        }
        
        /* allocate memory for camera filesystem struct */
        if ((ret = gp_filesystem_new(&dsc->fs)) != GP_OK) {
                dsc_errorprint(EDSCSERRNO, __FILE__, "camera_init", __LINE__);
                free(dsc);
                return ret;
        }
        
        return dsc1_connect(dsc, camera->port_info->speed); 
                /* connect with selected speed */
}


/* End of dc1000.c */
