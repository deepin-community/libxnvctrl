/*
 * nvidia-settings: A tool for configuring the NVIDIA X driver on Unix
 * and Linux systems.
 *
 * Copyright (C) 2004 NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "NvCtrlAttributes.h"
#include "query-assign.h"
#include "msg.h"
#include "glxinfo.h"

#include <GL/glx.h> /* GLX #defines */
#include <EGL/egl.h>


/*
 * print_extension_list() - Formats OpenGL/GLX extension strings
 * to contain commas and returns a pointer to the formatted string.
 * The user is responsible for freeing this buffer.
 *
 * If there is an error or there is not enough memory to create
 * the buffer, NULL is returned.
 *
 */

static char *
format_extension_list(const char *ext)
{
   int    i;
   char * extTmp = NULL; /* Actual string to print */
   const char *extCountTmp;


   if ( !ext || !ext[0] )
      return NULL;

   /* Count number of extensions (to get number of commas needed) */
   i = 0;
   extCountTmp = ext;
   while ( *extCountTmp != '\0' ) {
       if ( *extCountTmp == ' ' ) {
           i++;
       }
       extCountTmp++;
   }

   /*
    * Allocate buffer that will hold the extension string with
    * commas in it 
    */
   extTmp = nvalloc( (strlen(ext) +i +1) *sizeof(char) );

   /* Copy extension string to buffer, adding commas */
   i = 0;
   while ( *ext != '\0' && *ext != '\n' ) {
       if ( *ext == ' ' ) {
           extTmp[i++] = ',';
       }
       extTmp[i++] = *ext;
       ext++;
   }
   extTmp[i] = '\0';

   /* Remove any trailing whitespace and commas */
   while ( extTmp[ strlen(extTmp)-1 ] == ' ' || 
           extTmp[ strlen(extTmp)-1 ] == ',' ) {
       extTmp[ strlen(extTmp)-1 ] = '\0';
   }

   return extTmp;
} /* format_extension_list() */


/*
 * print_fbconfig_attribs() & helper functions -
 *   Prints a table of fbconfig attributes
 *
 * NOTE: Only support FBconfig for GLX v1.3+
 *
 */

#ifdef GLX_VERSION_1_3

const char *
render_type_abbrev(int rend_type)
{
    switch (rend_type) {
       case GLX_RGBA_BIT:
           return "rgb";
       case GLX_COLOR_INDEX_BIT:
	   return "ci";
       case (GLX_RGBA_BIT | GLX_COLOR_INDEX_BIT):
	   return "any";
       default:
	   return ".";
    }
}


const char *
transparent_type_abbrev(int trans_type)
{
    switch (trans_type) {
       case GLX_NONE:
	   return ".";
       case GLX_TRANSPARENT_RGB:
	   return "rg";
       case GLX_TRANSPARENT_INDEX:
	   return "ci";
       default:
	   return ".";
    }
}


const char *
x_visual_type_abbrev(int x_visual_type)
{
    switch (x_visual_type) {
       case GLX_TRUE_COLOR:
           return "tc";
       case GLX_DIRECT_COLOR:
           return "dc";
       case GLX_PSEUDO_COLOR:
           return "pc";
       case GLX_STATIC_COLOR:
           return "sc";
       case GLX_GRAY_SCALE:
           return "gs";
       case GLX_STATIC_GRAY:
           return "sg";
       default:
           return ".";
    }
}


const char *
caveat_abbrev(int caveat)
{
   if (caveat == GLX_NONE_EXT || caveat == 0)
      return(".");
   else if (caveat == GLX_SLOW_VISUAL_EXT)
      return("slo");
   else if (caveat == GLX_NON_CONFORMANT_VISUAL_EXT)
      return("NoC");
   else
       return(".");
}


static void
print_fbconfig_attribs(const GLXFBConfigAttr *fbca)
{
    int i; /* Iterator */


    if ( fbca == NULL ) {
        return;
    }

    printf("--fc- --vi- vt buf lv rgb d s colorbuffer ax dp st "
           "accumbuffer ---ms---- cav -----pbuffer----- ---transparent----\n");
    printf("  id    id     siz l  ci  b t  r  g  b  a bf th en "
           " r  g  b  a mvs mcs b eat widt hght max-pxs typ  r  g  b  a  i\n");
    printf("---------------------------------------------------"
           "--------------------------------------------------------------\n");

    i = 0;
    while ( fbca[i].fbconfig_id != 0 ) {
        
        printf("0x%03x ", fbca[i].fbconfig_id);
        if ( fbca[i].visual_id ) {
            printf("0x%03x ", fbca[i].visual_id);
        } else {
            printf("   .  ");
        }
        printf("%2.2s %3d %2d %3.3s %1c %1c ",
               x_visual_type_abbrev(fbca[i].x_visual_type),
               fbca[i].buffer_size,
               fbca[i].level,
               render_type_abbrev(fbca[i].render_type),
               fbca[i].doublebuffer ? 'y' : '.',
               fbca[i].stereo ? 'y' : '.'
               );
        printf("%2d %2d %2d %2d %2d %2d %2d ",
               fbca[i].red_size,
               fbca[i].green_size,
               fbca[i].blue_size,
               fbca[i].alpha_size,
               fbca[i].aux_buffers,
               fbca[i].depth_size,
               fbca[i].stencil_size
               );
        printf("%2d %2d %2d %2d ",
               fbca[i].accum_red_size,
               fbca[i].accum_green_size,
               fbca[i].accum_blue_size,
               fbca[i].accum_alpha_size
               );
        if ( fbca[i].multi_sample_valid == 1 ) {
            printf("%3d ",
                   fbca[i].multi_samples
                   );

            if ( fbca[i].multi_sample_coverage_valid == 1 ) {
                printf("%3d ",
                       fbca[i].multi_samples_color
                       );
            } else {
                printf("%3d ",
                       fbca[i].multi_samples
                       );
            }
            printf("%1d ",
                   fbca[i].multi_sample_buffers
                   );

        } else {
            printf("  .   . . ");
        }
        printf("%3.3s %4x %4x %7x %3.3s %2d %2d %2d %2d %2d\n",
               caveat_abbrev(fbca[i].config_caveat),
               fbca[i].pbuffer_width,
               fbca[i].pbuffer_height,
               fbca[i].pbuffer_max,
               transparent_type_abbrev(fbca[i].transparent_type),
               fbca[i].transparent_red_value,
               fbca[i].transparent_green_value,
               fbca[i].transparent_blue_value,
               fbca[i].transparent_alpha_value,
               fbca[i].transparent_index_value
               );
        
        i++;
    } /* Done printing FBConfig attributes for FBConfig */

} /* print_fbconfig_attribs() */

#endif /* GLX_VERSION_1_3 */


/*
 * print_glxinfo() - prints information about glx
 *
 */

#define TAB "  "

#define SAFE_FREE(m) \
if ( (m) != NULL ) { \
    free( m ); \
    m = NULL; \
}

#define NULL_TO_EMPTY(s) \
((s)!=NULL)?(s):""

void print_glxinfo(const char *display_name, CtrlSystemList *systems)
{
    CtrlSystem       *system;
    CtrlTargetNode  *node;
    ReturnStatus     status = NvCtrlSuccess;

    char            *direct_rendering  = NULL;
    char            *glx_extensions    = NULL;
    char            *server_vendor     = NULL;
    char            *server_version    = NULL;
    char            *server_extensions = NULL;
    char            *client_vendor     = NULL;
    char            *client_version    = NULL;
    char            *client_extensions = NULL;
    char            *opengl_vendor     = NULL;
    char            *opengl_renderer   = NULL;
    char            *opengl_version    = NULL;
    char            *opengl_extensions = NULL;

    GLXFBConfigAttr *fbconfig_attribs  = NULL;

    char            *formatted_ext_str = NULL;

    system = NvCtrlConnectToSystem(display_name, systems);
    if (system == NULL) {
        return;
    }

    /* Print information for each screen */
    for (node = system->targets[X_SCREEN_TARGET]; node; node = node->next) {

        CtrlTarget *t = node->t;

        /* No screen, move on */
        if ( !t->h ) continue;

        nv_msg(NULL, "GLX Information for %s:", t->name);

        /* Get GLX information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_DIRECT_RENDERING,
                                          &direct_rendering);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_GLX_EXTENSIONS,
                                          &glx_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( glx_extensions != NULL ) {
            formatted_ext_str = format_extension_list(glx_extensions);
            if ( formatted_ext_str != NULL ) {
                free(glx_extensions);
                glx_extensions = formatted_ext_str;
            }
        }
        /* Get server GLX information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_SERVER_VENDOR,
                                          &server_vendor);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_SERVER_VERSION,
                                          &server_version);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_SERVER_EXTENSIONS,
                                          &server_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( server_extensions != NULL ) {
            formatted_ext_str = format_extension_list(server_extensions);
            if ( formatted_ext_str != NULL ) {
                free(server_extensions);
                server_extensions = formatted_ext_str;
            }
        }
        /* Get client GLX information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_CLIENT_VENDOR,
                                          &client_vendor);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_CLIENT_VERSION,
                                          &client_version);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_CLIENT_EXTENSIONS,
                                          &client_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( client_extensions != NULL ) {
            formatted_ext_str = format_extension_list(client_extensions);
            if ( formatted_ext_str != NULL ) {
                free(client_extensions);
                client_extensions = formatted_ext_str;
            }
        }
        /* Get OpenGL information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_VENDOR,
                                          &opengl_vendor);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_RENDERER,
                                          &opengl_renderer);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_VERSION,
                                          &opengl_version);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_GLX_OPENGL_EXTENSIONS,
                                          &opengl_extensions);
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }
        if ( opengl_extensions != NULL ) {
            formatted_ext_str = format_extension_list(opengl_extensions);
            if ( formatted_ext_str != NULL ) {
                free(opengl_extensions);
                opengl_extensions = formatted_ext_str;
            }
        }

        /* Get FBConfig information */
        status = NvCtrlGetVoidAttribute(t,
                                        NV_CTRL_ATTR_GLX_FBCONFIG_ATTRIBS,
                                        (void *)(&fbconfig_attribs));
        if ( status != NvCtrlSuccess &&
             status != NvCtrlNoAttribute ) { goto finish; }

        /* Print results */
        nv_msg(TAB, "direct rendering: %s", NULL_TO_EMPTY(direct_rendering));
        nv_msg(TAB, "GLX extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(glx_extensions));
        nv_msg(" ", "\n");
        nv_msg(TAB, "server glx vendor string: %s",
               NULL_TO_EMPTY(server_vendor));
        nv_msg(TAB, "server glx version string: %s",
               NULL_TO_EMPTY(server_version));
        nv_msg(TAB, "server glx extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(server_extensions));
        nv_msg(" ", "\n");
        nv_msg(TAB, "client glx vendor string: %s",
               NULL_TO_EMPTY(client_vendor));
        nv_msg(TAB, "client glx version string: %s",
               NULL_TO_EMPTY(client_version));
        nv_msg(TAB, "client glx extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(client_extensions));
        nv_msg(" ", "\n");
        nv_msg(TAB, "OpenGL vendor string: %s",
               NULL_TO_EMPTY(opengl_vendor));
        nv_msg(TAB, "OpenGL renderer string: %s",
               NULL_TO_EMPTY(opengl_renderer));
        nv_msg(TAB, "OpenGL version string: %s",
               NULL_TO_EMPTY(opengl_version));
        nv_msg(TAB, "OpenGL extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(opengl_extensions));
#ifdef GLX_VERSION_1_3        
        if ( fbconfig_attribs != NULL ) {
            nv_msg(" ", "\n");
            print_fbconfig_attribs(fbconfig_attribs);
        }
#endif
        fflush(stdout);

        /* Free memory used */
        SAFE_FREE(server_vendor);
        SAFE_FREE(server_version);
        SAFE_FREE(server_extensions);
        SAFE_FREE(client_vendor);
        SAFE_FREE(client_version);
        SAFE_FREE(client_extensions);
        SAFE_FREE(direct_rendering);
        SAFE_FREE(glx_extensions);
        SAFE_FREE(opengl_vendor);
        SAFE_FREE(opengl_renderer);
        SAFE_FREE(opengl_version);
        SAFE_FREE(opengl_extensions);
        SAFE_FREE(fbconfig_attribs);

    } /* Done looking at all screens */


    /* Fall through */
 finish:
    if ( status == NvCtrlError ) {
        nv_error_msg("Error fetching GLX Information: %s",
                     NvCtrlAttributesStrError(status) );
    }

    /* Free any leftover memory used */
    SAFE_FREE(server_vendor);
    SAFE_FREE(server_version);
    SAFE_FREE(server_extensions);
    SAFE_FREE(client_vendor);
    SAFE_FREE(client_version);
    SAFE_FREE(client_extensions);
    SAFE_FREE(direct_rendering);
    SAFE_FREE(glx_extensions);
    SAFE_FREE(opengl_vendor);
    SAFE_FREE(opengl_renderer);
    SAFE_FREE(opengl_version);
    SAFE_FREE(opengl_extensions);
    SAFE_FREE(fbconfig_attribs);

    NvCtrlFreeAllSystems(systems);

} /* print_glxinfo() */



/*
 * EGL information
 *
 */

const char *egl_color_buffer_type_abbrev(int type)
{
    switch (type) {
    case EGL_RGB_BUFFER:
        return "rgb";
    case EGL_LUMINANCE_BUFFER:
        return "lum";
    default:
        return ".";
    }
}

const char *egl_config_caveat_abbrev(int type)
{
    switch (type) {
    case EGL_SLOW_CONFIG:
        return "slo";
    case EGL_NON_CONFORMANT_CONFIG:
        return "NoC";
    case EGL_NONE:
    default:
        return ".";
    }
}

const char *egl_transparent_type_abbrev(int type)
{
    switch (type) {
    case EGL_TRANSPARENT_RGB:
        return "rgb";
    case EGL_NONE:
    default:
        return ".";
    }
}

static void print_egl_config_attribs(const EGLConfigAttr *fbca)
{
    int i;

    if (fbca == NULL) {
        return;
    }

    printf("--fc- --vi- --vt-- buf lv rgb colorbuffer am lm dp st "
           "-bind cfrm sb sm cav -----pbuffer----- swapin nv   rn   su "
           "-transparent--\n");
    printf("  id    id         siz l  lum  r  g  b  a sz sz th en "
           " -  a            eat widt hght max-pxs  mx mn rd   ty   ty "
           "typ  r  g  b  \n");
    printf("------------------------------------------------------"
           "-----------------------------------------------------------"
           "--------------\n");

    for (i = 0; fbca[i].config_id != 0; i++) {

        printf("0x%03x ", fbca[i].config_id);
        if (fbca[i].native_visual_id) {
            printf("0x%03x ", fbca[i].native_visual_id);
        } else {
            printf("   .  ");
        }
        printf("0x%X %3d %2d %3s ",
               fbca[i].native_visual_type,
               fbca[i].buffer_size,
               fbca[i].level,
               egl_color_buffer_type_abbrev(fbca[i].color_buffer_type));
        printf("%2d %2d %2d %2d %2d %2d %2d %2d ",
               fbca[i].red_size,
               fbca[i].green_size,
               fbca[i].blue_size,
               fbca[i].alpha_size,
               fbca[i].alpha_mask_size,
               fbca[i].luminance_size,
               fbca[i].depth_size,
               fbca[i].stencil_size);
        printf("%2c %2c ",
               fbca[i].bind_to_texture_rgb ? 'y' : '.',
               fbca[i].bind_to_texture_rgba ? 'y' : '.');
        printf("0x%02X %2d %2d ",
               fbca[i].conformant,
               fbca[i].sample_buffers,
               fbca[i].samples);
        printf("%3.3s %4x %4x %7x %2d %2d ",
               egl_config_caveat_abbrev(fbca[i].config_caveat),
               fbca[i].max_pbuffer_width,
               fbca[i].max_pbuffer_height,
               fbca[i].max_pbuffer_pixels,
               fbca[i].max_swap_interval,
               fbca[i].min_swap_interval);
        printf("%2c %4x %4x ",
               fbca[i].native_renderable ? 'y' : '.',
               fbca[i].renderable_type,
               fbca[i].surface_type);
        printf("%3s %2d %2d %2d\n",
               egl_transparent_type_abbrev(fbca[i].transparent_type),
               fbca[i].transparent_red_value,
               fbca[i].transparent_green_value,
               fbca[i].transparent_blue_value);
    }

} /* print_egl_config_attribs() */


/*
 * print_eglinfo() - prints information about egl
 *
 */

void print_eglinfo(const char *display_name, CtrlSystemList *systems)
{
    CtrlSystem     *system;
    CtrlTargetNode *node;
    CtrlTargetType target;
    ReturnStatus   status = NvCtrlSuccess;

    char *egl_vendor        = NULL;
    char *egl_version       = NULL;
    char *egl_extensions    = NULL;
    char *formatted_ext_str = NULL;

    EGLConfigAttr *egl_config_attribs = NULL;


    system = NvCtrlConnectToSystem(display_name, systems);
    if (system == NULL) {
        return;
    }

    if (system->has_nv_control) {
        target = X_SCREEN_TARGET;
    } else {
        target = GPU_TARGET;
    }

    /* Print information for each target */
    for (node = system->targets[target]; node; node = node->next) {

        CtrlTarget *t = node->t;

        /* No target, move on */
        if (!t->h) continue;

        nv_msg(NULL, "EGL Information for %s:", t->name);

        /* Get EGL information */
        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_EGL_VENDOR,
                                          &egl_vendor);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_EGL_VERSION,
                                          &egl_version);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        status = NvCtrlGetStringAttribute(t,
                                          NV_CTRL_STRING_EGL_EXTENSIONS,
                                          &egl_extensions);
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }

        if (egl_extensions != NULL) {
            formatted_ext_str = format_extension_list(egl_extensions);
            if (formatted_ext_str != NULL) {
                free(egl_extensions);
                egl_extensions = formatted_ext_str;
            }
        }

        /* Get FBConfig information */
        status = NvCtrlGetVoidAttribute(t,
                                        NV_CTRL_ATTR_EGL_CONFIG_ATTRIBS,
                                        (void *)(&egl_config_attribs));
        if (status != NvCtrlSuccess && status != NvCtrlNoAttribute) {
            goto finish;
        }


        /* Print results */
        nv_msg(TAB, "EGL vendor string: %s", NULL_TO_EMPTY(egl_vendor));
        nv_msg(TAB, "EGL version string: %s", NULL_TO_EMPTY(egl_version));
        nv_msg(TAB, "EGL extensions:");
        nv_msg("    ", "%s", NULL_TO_EMPTY(egl_extensions));
        nv_msg(" ", "\n");

        if (egl_config_attribs != NULL) {
            nv_msg(" ", "\n");
            print_egl_config_attribs(egl_config_attribs);
        }

        fflush(stdout);

        /* Free memory used */
        SAFE_FREE(egl_vendor);
        SAFE_FREE(egl_version);
        SAFE_FREE(egl_extensions);
        SAFE_FREE(egl_config_attribs);

        /* If using a gpu, only process the first target */
        if (target == GPU_TARGET) {
            break;
        }
    } /* Done looking at all targets */


    /* Fall through */
 finish:
    if (status == NvCtrlError) {
        nv_error_msg("Error fetching EGL Information: %s",
                     NvCtrlAttributesStrError(status) );
    }

    /* Free any leftover memory used */
    SAFE_FREE(egl_vendor);
    SAFE_FREE(egl_version);
    SAFE_FREE(egl_extensions);
    SAFE_FREE(egl_config_attribs);

    NvCtrlFreeAllSystems(systems);

} /* print_eglinfo() */


