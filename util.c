#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gphoto2.h>

#include "util.h"

int gp_abilities_dump (CameraAbilities *abilities) {

	int x=0;

	gp_debug_printf(GP_DEBUG_LOW, "core", "Abilities for camera                  : %s", 
		abilities->model);
	gp_debug_printf(GP_DEBUG_LOW, "core", "Serial port support                   : %s", 
		SERIAL_SUPPORTED(abilities->port)? "yes":"no");
	gp_debug_printf(GP_DEBUG_LOW, "core", "Parallel port support                 : %s", 
		PARALLEL_SUPPORTED(abilities->port)? "yes":"no");
	gp_debug_printf(GP_DEBUG_LOW, "core", "USB support                           : %s", 
		USB_SUPPORTED(abilities->port)? "yes":"no");
	gp_debug_printf(GP_DEBUG_LOW, "core", "IEEE1394 support                      : %s", 
		IEEE1394_SUPPORTED(abilities->port)? "yes":"no");
	gp_debug_printf(GP_DEBUG_LOW, "core", "Network support                       : %s", 
		NETWORK_SUPPORTED(abilities->port)? "yes":"no");
	if (abilities->speed[0] != 0) {
	gp_debug_printf(GP_DEBUG_LOW, "core", "Transfer speeds supported             :");
        x=0;
        do {
	gp_debug_printf(GP_DEBUG_LOW, "core", "                                      : %i", abilities->speed[x]);
			x++;
		} while (abilities->speed[x]!=0);
	}

        gp_debug_printf(GP_DEBUG_LOW, "core", "Capture choices                       :");
        x=0;
        while (abilities->capture[x].type != GP_CAPTURE_NONE) {
        gp_debug_printf(GP_DEBUG_LOW, "core", "                                      : %s",
                        abilities->capture[x].name);
                x++;
        }
        gp_debug_printf(GP_DEBUG_LOW, "core", "Configuration  support                : %s",
                abilities->config == 0? "no":"yes");
	gp_debug_printf(GP_DEBUG_LOW, "core", "Delete files on camera support        : %s", 
		abilities->file_operations & GP_FILE_OPERATION_DELETE? "no":"yes");
	gp_debug_printf(GP_DEBUG_LOW, "core", "File preview (thumbnail) support      : %s", 
		abilities->file_operations & GP_FILE_OPERATION_PREVIEW? "no":"yes");
	gp_debug_printf(GP_DEBUG_LOW, "core", "File upload support                   : %s", 
		abilities->folder_operations & GP_FOLDER_OPERATION_PUT? "no":"yes");

	return (GP_OK);
}

int gp_abilities_clear (CameraAbilities *abilities) {

	strcpy(abilities->model, "");
	abilities->port = 0;
	abilities->speed[0] = 0;
	abilities->capture[0].type = GP_CAPTURE_NONE;
	abilities->config = 0;
	abilities->file_operations = GP_FILE_OPERATION_NONE;
	abilities->folder_operations = GP_FOLDER_OPERATION_NONE;

	return (GP_OK);
}
