/*
 * enna_config.c
 * Copyright (C) Nicolas Aguirre 2006,2007,2008 <aguirre.nicolas@gmail.com>
 *
 * enna_config.c is free software copyrighted by Nicolas Aguirre.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name ``Nicolas Aguirre'' nor the name of any other
 *    contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * enna_config.c IS PROVIDED BY Nicolas Aguirre ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Nicolas Aguirre OR ANY OTHER CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "enna.h"
#include "enna_config.h"

typedef enum _ENNA_CONFIG_TYPE ENNA_CONFIG_TYPE;

enum _ENNA_CONFIG_TYPE{
  ENNA_CONFIG_STRING,
  ENNA_CONFIG_STRING_LIST,
  ENNA_CONFIG_INT
};




typedef struct _Config_Pair Config_Pair;
typedef struct _Config_Data Config_Data;

struct _Config_Pair
{
   char *key;
   char *value;
};

struct _Config_Data
{
   char *section;
   Evas_List *pair;
};

EAPI const char *
enna_config_theme_get()
{
   return enna_config->theme_file;
}

EAPI const char *
enna_config_theme_file_get(const char *s)
{
   if (!s) return NULL;

   if (s[0] == '/')
     return s;
   else
     {
	char tmp[4096];
	snprintf(tmp, sizeof(tmp), PACKAGE_DATA_DIR "/enna/theme/%s.edj", s);
	if (!ecore_file_exists(tmp))
	  return PACKAGE_DATA_DIR "/enna/theme/default.edj";
	else
	  return strdup(tmp);
     }
   return NULL;
}

static Evas_Hash *
enna_config_load_conf(char *conffile, int size)
{
   char *current_section = NULL;
   char *current_line = conffile;
   Evas_Hash *config = NULL;
   Config_Data *config_data;

   while (current_line < conffile + size)
     {
	char *eol = strchr(current_line, '\n');
	Config_Pair *pair;
	char  *key;
	char  *value;
	if (eol)
	  *eol = 0;
	else			// Maybe the end of file
	  eol = conffile + size;

	// Removing the leading spaces
	while (*current_line && *current_line == ' ')
	  current_line++;

	// Maybe an empty line
	if (!(*current_line))
	  {
	     current_line = eol + 1;
	     continue;
	  }

	// Maybe a comment line
	if (*current_line == '#')
	  {
	     current_line = eol + 1;
	     continue;
	  }

	// We are at a section definition
	if (*current_line == '[')
	  {
	     // ']' must be the last char of this line
	     char  *end_of_section_name = strchr(current_line + 1, ']');

	     if (end_of_section_name[1] != 0)
	       {
		  dbg("malformed section name %s\n", current_line);
		  return NULL;
	       }
	     current_line++;
	     *end_of_section_name = '\0';

	     // Building the section
	     if (current_section)
	       free(current_section);
	     current_section = strdup(current_line);
	     config_data = calloc(1, sizeof(Config_Data));
	     config_data->section = current_section;
	     config_data->pair = NULL;
	     config = evas_hash_add(config, current_section, config_data);
	     printf("Section [\"%s\"]\n", current_section);
	     current_line = eol + 1;
	     continue;

	  }

	// Must be in a section to provide a key/value pair
	if (!current_section)
	  {
	     dbg("No section for this line %s\n", current_line);
	     /* FIXME : free hash and confile*/
	     return NULL;
	  }

	// Building the key/value string pair
	key = current_line;
	value = strchr(current_line, '=');
	if (!value)
	  {
	     dbg("Malformed line %s\n", current_line);
	     /* FIXME : free hash and confile*/
	     return NULL;
	  }
	*value = '\0';
	value++;
	pair = calloc(1, sizeof(Config_Pair));
	pair->key = strdup(key);
	pair->value = strdup(value);
	config_data = evas_hash_find(config, current_section);
	if (config_data)
	  {
	     config_data->pair = evas_list_append(config_data->pair, pair);
	     /* Need this ? */
	     /*evas_hash_modify(hash, current_section, config_data);*/
	  }

       	current_line = eol + 1;
     }
   free(conffile);
   return config;
}


static Evas_Hash *
enna_config_load_conf_file(char *filename)
{
   int                 fd;
   FILE               *f;
   struct stat         st;
   char                tmp[4096];
   char               *conffile;
   int                 ret;

   if (stat(filename, &st))
     {
	dbg("Cannot stat file %s\n", filename);
	sprintf(tmp, "%s/.enna", enna_util_user_home_get());
	if (!ecore_file_is_dir(tmp))
	  ecore_file_mkdir(tmp);

	if (!(f = fopen(filename, "w")))
	  return NULL;
	else
	  {
	     fprintf(f, "[enna]\n"
		     "\n"
		     "fullscreen=0\n\n"
		     "theme=default\n\n"
		     "#x11,xrender,gl,x11_16\n"
		     "engine=x11\n\n"
		     "#libplayer,emotion\n"
		     "backend=libplayer\n\n"
		     "music_ext=ogg,mp3,flac,wav,wma\n"
		     );
	     fclose(f);
	  }

     }

   if (stat(filename, &st))
     {
	dbg("Cannot stat file %s\n", filename);
	return NULL;
     }

   conffile = malloc(st.st_size);

   if (!conffile)
     {
	dbg("Cannot malloc %d bytes\n", (int)st.st_size);
	return NULL;
     }

   if ((fd = open(filename, O_RDONLY)) < 0)
     {
	dbg("Cannot open file\n");
	return NULL;
     }

   ret = read(fd, conffile, st.st_size);

   if (ret != st.st_size)
     {
	dbg("Cannot read conf file entirely, read only %d bytes\n", ret);
	return NULL;
     }

   return enna_config_load_conf(conffile, st.st_size);
}

static void
enna_config_value_store(void *var, char *section, ENNA_CONFIG_TYPE type, Config_Pair *pair)
{
   if (!strcmp(pair->key ,section))
     {
	switch(type)
	  {
	   case ENNA_CONFIG_INT :
	     {
		int *value = var;
		*value = atoi(pair->value);
		break;
	     }
	   case ENNA_CONFIG_STRING :
	     {
		char **value = var;
		*value = strdup(pair->value);
		break;
	     }
	   case ENNA_CONFIG_STRING_LIST :
	     {
		Evas_List *list;
		Evas_List **value = var;
		char **clist;
		char *string;
		int i;

		list = NULL;
		clist = ecore_str_split(pair->value, ",", 0);

		for(i = 0; (string = clist[i]) ; i++ )
		  {
		     if (!string)
		       break;
		     list = evas_list_append(list, string);
		  }
		*value =list;
	     }
	   default:
	      break;
	  }
     }
}

Evas_Bool _hash_foreach (const Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   Config_Data *config_data;
   Evas_List *l;
   printf("Section : %s\n", key);
   if(!strcmp(key, "enna"))
     {
	config_data = data;
	for (l = config_data->pair; l; l = l->next)
	  {
	     Config_Pair *pair = l->data;
	     printf("%s : %s\n", pair->key, pair->value);

	     enna_config_value_store(&enna_config->theme, "theme", ENNA_CONFIG_STRING, pair);
	     enna_config->theme_file = enna_config_theme_file_get(enna_config->theme);
	     enna_config_value_store(&enna_config->fullscreen, "fullscreen", ENNA_CONFIG_INT, pair);
	     enna_config_value_store(&enna_config->engine, "engine", ENNA_CONFIG_STRING, pair);
	     enna_config_value_store(&enna_config->backend, "backend", ENNA_CONFIG_STRING, pair);
	     enna_config_value_store(&enna_config->music_filters, "music_ext", ENNA_CONFIG_STRING_LIST, pair);
	  }
	printf("[Config]\n\ttheme : %s\n\tfullscreen : %d\n\tengine: %s\n\tbackend: %s\n", enna_config->theme,
	       enna_config->fullscreen, enna_config->engine, enna_config->backend);
	printf("\textensions : ");
	for (l = enna_config->music_filters; l; l = l->next)
	  printf("%s ", (char*)l->data);
	printf("\n");
     }


   return 1;
}

EAPI void
enna_config_init()
{
   Evas_List *l;
   Enna_Config_Root_Directories *root;
   char home_dir[FILENAME_MAX];
   Evas_Hash *config;

   enna_config = calloc(1, sizeof(Enna_Config));
   config = enna_config_load_conf_file("/home/nico/.enna/enna.cfg");
   evas_hash_foreach(config, _hash_foreach, NULL);

   /* Theme config */
   //enna_config->theme = evas_stringshare_add(PACKAGE_DATA_DIR"/enna/theme/default.edj");
   /* Module Music config */
   l = NULL;
   root = malloc(sizeof(Enna_Config_Root_Directories));
   snprintf(home_dir, sizeof(home_dir), "file://%s", enna_util_user_home_get());
   root->uri = evas_stringshare_add(home_dir);
   root->label = evas_stringshare_add("Home Directory");
   l = evas_list_append(l, root);
   enna_config->music_local_root_directories = l;

}

EAPI void
enna_config_shutdown()
{

}


