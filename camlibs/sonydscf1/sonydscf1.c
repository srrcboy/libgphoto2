#include <string.h>
#include <gphoto2.h>

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

#include "sonydscf1.h"

gp_port *dev;

extern int glob_debug;
char glob_camera_model[64];
static int all_pic_num = -1;

int camera_id (CameraText *id) {

        strcpy(id->text, "sonydscf1-bvl");

        return (GP_OK);
}

int camera_debug_set (int onoff) {

        if(onoff)
          printf("Setting debugging to on\n");
        glob_debug=onoff;
        return (GP_OK);
}

int camera_abilities (CameraAbilitiesList *list) {

        //*count = 1;
        CameraAbilities *a;

        gp_abilities_new(&a);
        /* Fill in each camera model's abilities */
        /* Make separate entries for each conneciton type (usb, serial, etc...)
           if a camera supported multiple ways. */
        strcpy(a->model, "Sony DSC-F1");
        a->port=GP_PORT_SERIAL;
        a->speed[0] = 9600;
        a->speed[1] = 19200;
        a->speed[2] = 38400;
        a->operations        =  GP_OPERATION_NONE;
        a->file_operations   =  GP_FILE_OPERATION_DELETE |
                                GP_FILE_OPERATION_PREVIEW;
        a->folder_operations =  GP_FOLDER_OPERATION_NONE;
        gp_abilities_list_append(list, a);

        return (GP_OK);
}

int camera_exit (Camera *camera) {
        if(F1ok())
           return(GP_ERROR);
        return (F1fclose());
}

int camera_folder_list_folders (Camera *camera, const char *folder,
                                CameraList *list)
{
       /* printf("sony dscf1: folderlist\n");*/
        return (GP_OK);
}

int camera_folder_set(Camera *camera, const char *folder_name)
{
/*                printf("sony dscf1: folder set\n");*/
        printf("folder: %s\n",folder_name);
        return (GP_OK);
}

int camera_file_count (Camera *camera)
{
        if(!F1ok())
           return (GP_ERROR);

        return (F1howmany());
}

int camera_file_get (Camera *camera, const char *folder, const char *filename,
                     CameraFileType type, CameraFile *file)
{
        int num;
	long int size;
	const char *data = NULL;
        SonyStruct *b = (SonyStruct*)camera->camlib_data;
        printf("folder: %s, file: %s\n", folder, filename);
        /*gp_frontend_progress(camera, NULL, 0.00);*/
        if(!F1ok())
           return (GP_ERROR);

	gp_file_set_name (file, filename);
	gp_file_set_mime_type (file, "image/jpeg");

        /* Retrieve the number of the photo on the camera */
        num = gp_filesystem_number(b->fs, "/", filename);

	switch (type) {
	case GP_FILE_TYPE_NORMAL:
		size = get_picture (num, &data, JPEG, 0, 
				    camera_file_count (camera));
		break;
	case GP_FILE_TYPE_PREVIEW:
		size = get_picture (num, &data, JPEG_T, TRUE,
				    camera_file_count (camera));
		break;
	default:
		return (GP_ERROR_NOT_SUPPORTED);
	}

        if (!data)
                return GP_ERROR;

	gp_file_set_data_and_size (file, data, size);

        return GP_OK;
}

int camera_file_delete (Camera *camera, const char *folder ,
                        const char *filename)
{
        int max, num;
        SonyStruct *b = (SonyStruct*)camera->camlib_data;
        num = gp_filesystem_number(b->fs, "/", filename);
        max = gp_filesystem_count(b->fs,folder)  ;
        printf("sony dscf1: file delete: %d\n",num);
        if(!F1ok())
           return (GP_ERROR);
        delete_picture(num,max);
        return(GP_OK);
        /*return (F1deletepicture(file_number));*/
}

int camera_summary (Camera *camera, CameraText *summary)
{
        /*printf("->camera summary");*/
        int i;
        if(!F1ok())
           return (GP_ERROR);
        get_picture_information(&i,2);
        return (F1newstatus(1, summary->text));
}

int camera_manual (Camera *camera, CameraText *manual)
{
                /*printf("sony dscf1: manual\n");*/
        strcpy(manual->text, _("Manual Not Available"));

        return (GP_OK);
}

int camera_about (Camera *camera, CameraText *about)
{
        strcpy(about->text,
_("Sony DSC-F1 Digital Camera Support\nM. Adam Kendall <joker@penguinpub.com>\nBased on the chotplay CLI interface from\nKen-ichi Hayashi\nGphoto2 port by Bart van Leeuwen <bart@netage.nl>"));

        return (GP_OK);
}

int camera_folder_list_files (Camera *camera, const char *folder,
                              CameraList *list)
{
        int count, x;
        SonyStruct *b = (SonyStruct*)camera->camlib_data;
	const char *name;
        F1ok();
        /*if(F1ok())
           return(GP_ERROR);*/
        count = F1howmany();
        /* Populate the filesystem */
        gp_filesystem_populate(b->fs, "/", "PSN%05i.jpg", count);

        for (x=0; x<gp_filesystem_count(b->fs, folder); x++) {
		gp_filesystem_name(b->fs, folder, x, &name);
                gp_list_append (list, name, NULL);
	}

        return GP_OK;
}

int camera_init (Camera *camera) {
        gp_port_settings settings;
        SonyStruct *b;
        int ret;

        if(glob_debug)
        {
         printf("sony dscf1: Initializing the camera\n");
         printf("port: %s\n",camera->port_info->path);
        }

        camera->functions->exit         = camera_exit;
        camera->functions->folder_list_folders  = camera_folder_list_folders;
        camera->functions->folder_list_files    = camera_folder_list_files;
        //camera->functions->folder_set   = camera_folder_set;
        //camera->functions->file_count   = camera_file_count;
        camera->functions->file_get     = camera_file_get;
        camera->functions->file_delete  = camera_file_delete;
//        camera->functions->capture      = camera_capture;
        camera->functions->summary      = camera_summary;
        camera->functions->manual       = camera_manual;
        camera->functions->about        = camera_about;

        b = (SonyStruct*)malloc(sizeof(SonyStruct));
        camera->camlib_data = b;

        if ((ret = gp_port_new(&(b->dev), GP_PORT_SERIAL)) < 0) {
            return (ret);
        }


        gp_port_timeout_set(b->dev, 5000);
        strcpy(settings.serial.port, camera->port_info->path);

        settings.serial.speed   = camera->port_info->speed;
        settings.serial.bits    = 8;
        settings.serial.parity  = 0;
        settings.serial.stopbits= 1;

        gp_port_settings_set(b->dev, settings);
        gp_port_open(b->dev);

        /* Create the filesystem */
        gp_filesystem_new(&b->fs);
        dev = b->dev;
        return (GP_OK);
}

