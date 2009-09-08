/* This file is part of Pazpar2.
   Copyright (C) 2006-2009 Index Data

Pazpar2 is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

Pazpar2 is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/

// This module implements a generic system of settings
// (attribute-value) that can be associated with search targets. The
// system supports both default values, per-target overrides, and
// per-user settings.

#if HAVE_CONFIG_H
#include <config.h>
#endif


#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include "direntz.h"
#include <stdlib.h>
#include <sys/stat.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <yaz/nmem.h>
#include <yaz/log.h>

#include "pazpar2.h"
#include "database.h"
#include "settings.h"

// Used for initializing setting_dictionary with pazpar2-specific settings
static char *hard_settings[] = {
    "pz:piggyback",
    "pz:elements",
    "pz:requestsyntax",
    "pz:cclmap:",
    "pz:xslt",
    "pz:nativesyntax",
    "pz:authentication",
    "pz:allow",
    "pz:maxrecs",
    "pz:id",
    "pz:name",
    "pz:queryencoding",
    "pz:ip",
    "pz:zproxy",
    "pz:apdulog",
    "pz:sru",
    "pz:sru_version",
    "pz:pqf_prefix",
    0
};

struct setting_dictionary
{
    char **dict;
    int size;
    int num;
};

// This establishes the precedence of wildcard expressions
#define SETTING_WILDCARD_NO     0 // No wildcard
#define SETTING_WILDCARD_DB     1 // Database wildcard 'host:port/*'
#define SETTING_WILDCARD_YES    2 // Complete wildcard '*'

// Returns size of settings directory
int settings_num(struct conf_service *service)
{
    return service->dictionary->num;
}

int settings_offset(struct conf_service *service, const char *name)
{
    int i;

    if (!name)
        name = "";
    for (i = 0; i < service->dictionary->num; i++)
        if (!strcmp(name, service->dictionary->dict[i]))
            return i;
    return -1;
}

// Ignores everything after second colon, if present
// A bit of a hack to support the pz:cclmap: scheme (and more to come?)
int settings_offset_cprefix(struct conf_service *service, const char *name)
{
    const char *p;
    int maxlen = 100;
    int i;

    if (!strncmp("pz:", name, 3) && (p = strchr(name + 3, ':')))
        maxlen = (p - name) + 1;
    for (i = 0; i < service->dictionary->num; i++)
        if (!strncmp(name, service->dictionary->dict[i], maxlen))
            return i;
    return -1;
}

char *settings_name(struct conf_service *service, int offset)
{
    return service->dictionary->dict[offset];
}

static int isdir(const char *path)
{
    struct stat st;

    if (stat(path, &st) < 0)
    {
        yaz_log(YLOG_FATAL|YLOG_ERRNO, "stat %s", path);
        exit(1);
    }
    return st.st_mode & S_IFDIR;
}

// Read settings from an XML file, calling handler function for each setting
static void read_settings_file(const char *path,
                               struct conf_service *service,
                               void (*fun)(struct conf_service *service,
                                           struct setting *set))
{
    xmlDoc *doc = xmlParseFile(path);
    xmlNode *n;
    xmlChar *namea, *targeta, *valuea, *usera, *precedencea;

    if (!doc)
    {
        yaz_log(YLOG_FATAL, "Failed to parse %s", path);
        exit(1);
    }
    n = xmlDocGetRootElement(doc);
    namea = xmlGetProp(n, (xmlChar *) "name");
    targeta = xmlGetProp(n, (xmlChar *) "target");
    valuea = xmlGetProp(n, (xmlChar *) "value");
    usera = xmlGetProp(n, (xmlChar *) "user");
    precedencea = xmlGetProp(n, (xmlChar *) "precedence");
    for (n = n->children; n; n = n->next)
    {
        if (n->type != XML_ELEMENT_NODE)
            continue;
        if (!strcmp((const char *) n->name, "set"))
        {
            char *name, *target, *value, *user, *precedence;

            name = (char *) xmlGetProp(n, (xmlChar *) "name");
            target = (char *) xmlGetProp(n, (xmlChar *) "target");
            value = (char *) xmlGetProp(n, (xmlChar *) "value");
            user = (char *) xmlGetProp(n, (xmlChar *) "user");
            precedence = (char *) xmlGetProp(n, (xmlChar *) "precedence");

            if ((!name && !namea) || (!value && !valuea) || (!target && !targeta))
            {
                yaz_log(YLOG_FATAL, "set must specify name, value, and target");
                exit(1);
            }
            else
            {
                struct setting set;
                char nameb[1024];
                char targetb[1024];
                char valueb[1024];

                // Copy everything into a temporary buffer -- we decide
                // later if we are keeping it.
                if (precedence)
                    set.precedence = atoi((char *) precedence);
                else if (precedencea)
                    set.precedence = atoi((char *) precedencea);
                else
                    set.precedence = 0;
                if (target)
                    strcpy(targetb, target);
                else
                    strcpy(targetb, (const char *) targeta);
                set.target = targetb;
                if (name)
                    strcpy(nameb, name);
                else
                    strcpy(nameb, (const char *) namea);
                set.name = nameb;
                if (value)
                    strcpy(valueb, value);
                else
                    strcpy(valueb, (const char *) valuea);
                set.value = valueb;
                set.next = 0;
                (*fun)(service, &set);
            }
            xmlFree(name);
            xmlFree(precedence);
            xmlFree(value);
            xmlFree(user);
            xmlFree(target);
        }
        else
        {
            yaz_log(YLOG_FATAL, "Unknown element %s in settings file", (char*) n->name);
            exit(1);
        }
    }
    xmlFree(namea);
    xmlFree(precedencea);
    xmlFree(valuea);
    xmlFree(usera);
    xmlFree(targeta);

    xmlFreeDoc(doc);
}
 
// Recursively read files or directories, invoking a 
// callback for each one
static void read_settings(const char *path,
                          struct conf_service *service,
                          void (*fun)(struct conf_service *service,
                                      struct setting *set))
{
    DIR *d;
    struct dirent *de;
    char *dot;

    if (isdir(path))
    {
        if (!(d = opendir(path)))
        {
            yaz_log(YLOG_FATAL|YLOG_ERRNO, "%s", path);
            exit(1);
        }
        while ((de = readdir(d)))
        {
            char tmp[1024];
            if (*de->d_name == '.' || !strcmp(de->d_name, "CVS"))
                continue;
            sprintf(tmp, "%s/%s", path, de->d_name);
            read_settings(tmp, service, fun);
        }
        closedir(d);
    }
    else if ((dot = strrchr(path, '.')) && !strcmp(dot + 1, "xml"))
        read_settings_file(path, service, fun);
}

// Determines if a ZURL is a wildcard, and what kind
static int zurl_wildcard(const char *zurl)
{
    if (!zurl)
        return SETTING_WILDCARD_NO;
    if (*zurl == '*')
        return SETTING_WILDCARD_YES;
    else if (*(zurl + strlen(zurl) - 1) == '*')
        return SETTING_WILDCARD_DB;
    else
        return SETTING_WILDCARD_NO;
}

// Callback. Adds a new entry to the dictionary if necessary
// This is used in pass 1 to determine layout of dictionary
// and to load any databases mentioned
static void prepare_dictionary(struct conf_service *service,
                               struct setting *set)
{
    struct setting_dictionary *dictionary = service->dictionary;

    int i;
    char *p;

    // If target address is not wildcard, add the database
    if (*set->target && !zurl_wildcard(set->target))
        find_database(set->target, 0, service);

    // Determine if we already have a dictionary entry
    if (!strncmp(set->name, "pz:", 3) && (p = strchr(set->name + 3, ':')))
        *(p + 1) = '\0';
    for (i = 0; i < dictionary->num; i++)
        if (!strcmp(dictionary->dict[i], set->name))
            return;

    if (!strncmp(set->name, "pz:", 3)) // Probably a typo in config file
        {
            yaz_log(YLOG_FATAL, "Unknown pz: setting '%s'", set->name);
            exit(1);
        }

    // Create a new dictionary entry
    // Grow dictionary if necessary
    if (!dictionary->size)
        dictionary->dict =
            nmem_malloc(service->nmem, (dictionary->size = 50) * sizeof(char*));
    else if (dictionary->num + 1 > dictionary->size)
    {
        char **tmp =
            nmem_malloc(service->nmem, dictionary->size * 2 * sizeof(char*));
        memcpy(tmp, dictionary->dict, dictionary->size * sizeof(char*));
        dictionary->dict = tmp;
        dictionary->size *= 2;
    }
    dictionary->dict[dictionary->num++] = nmem_strdup(service->nmem, set->name);
}


struct update_database_context {
    struct setting *set;
    struct conf_service *service;
};

// This is called from grep_databases -- adds/overrides setting for a target
// This is also where the rules for precedence of settings are implemented
static void update_database(void *context, struct database *db)
{
    struct setting *set = ((struct update_database_context *)
                           context)->set;
    struct conf_service *service = ((struct update_database_context *) 
                                    context)->service;
    struct setting *s, **sp;
    int offset;

    // Is this the right database?
    if (!match_zurl(db->url, set->target))
        return;

    if ((offset = settings_offset_cprefix(service, set->name)) < 0)
        return ;

    // First we determine if this setting is overriding  any existing settings
    // with the same name.
    for (s = db->settings[offset], sp = &db->settings[offset]; s;
            sp = &s->next, s = s->next)
        if (!strcmp(s->name, set->name))
        {
            if (s->precedence < set->precedence)
                // We discard the value (nmem keeps track of the space)
                *sp = (*sp)->next; // unlink value from existing setting
            else if (s->precedence > set->precedence)
                // Db contains a higher-priority setting. Abort search
                break;
            if (zurl_wildcard(s->target) > zurl_wildcard(set->target))
                // target-specific value trumps wildcard. Delete.
                *sp = (*sp)->next; // unlink.....
            else if (!zurl_wildcard(s->target))
                // Db already contains higher-priority setting. Abort search
                break;
        }
    if (!s) // s will be null when there are no higher-priority settings -- we add one
    {
        struct setting *new = nmem_malloc(service->nmem, sizeof(*new));

        memset(new, 0, sizeof(*new));
        new->precedence = set->precedence;
        new->target = nmem_strdup(service->nmem, set->target);
        new->name = nmem_strdup(service->nmem, set->name);
        new->value = nmem_strdup(service->nmem, set->value);
        new->next = db->settings[offset];
        db->settings[offset] = new;
    }
}

// Callback -- updates database records with dictionary entries as appropriate
// This is used in pass 2 to assign name/value pairs to databases
static void update_databases(struct conf_service *service, 
                             struct setting *set)
{
    struct update_database_context context;
    context.set = set;
    context.service = service;
    predef_grep_databases(&context, service, 0, update_database);
}

// This simply copies the 'hard' (application-specific) settings
// to the settings dictionary.
static void initialize_hard_settings(struct conf_service *service)
{
    struct setting_dictionary *dict = service->dictionary;
    dict->dict = nmem_malloc(service->nmem, sizeof(hard_settings) - sizeof(char*));
    dict->size = (sizeof(hard_settings) - sizeof(char*)) / sizeof(char*);
    memcpy(dict->dict, hard_settings, dict->size * sizeof(char*));
    dict->num = dict->size;
}

// Read any settings names introduced in service definition (config) and add to dictionary
// This is done now to avoid errors if user settings are declared in session overrides
static void initialize_soft_settings(struct conf_service *service)
{
    int i;

    for (i = 0; i < service->num_metadata; i++)
    {
        struct setting set;
        struct conf_metadata *md = &service->metadata[i];

        if (md->setting == Metadata_setting_no)
            continue;

        set.precedence = 0;
        set.target = "";
        set.name = md->name;
        set.value = "";
        set.next = 0;
        prepare_dictionary(service, &set);
    }
}

static void prepare_target_dictionary(struct conf_service *service,
                                      struct setting *set)
{
    struct setting_dictionary *dictionary = service->dictionary;

    int i;
    char *p;

    // If target address is not wildcard, add the database
    if (*set->target && !zurl_wildcard(set->target))
        find_database(set->target, 0, service);

    // Determine if we already have a dictionary entry
    if (!strncmp(set->name, "pz:", 3) && (p = strchr(set->name + 3, ':')))
        *(p + 1) = '\0';
    for (i = 0; i < dictionary->num; i++)
        if (!strcmp(dictionary->dict[i], set->name))
            return;
    yaz_log(YLOG_WARN, "Setting '%s' not configured as metadata", set->name);
}

// If we ever decide we need to be able to specify multiple settings directories,
// the two calls to read_settings must be split -- so the dictionary is prepared
// for the contents of every directory before the databases are updated.
void settings_read(struct conf_service *service, const char *path)
{
    read_settings(path, service, prepare_target_dictionary);
    read_settings(path, service, update_databases);
}

void init_settings(struct conf_service *service)
{
    struct setting_dictionary *new;

    assert(service->nmem);

    new = nmem_malloc(service->nmem, sizeof(*new));
    memset(new, 0, sizeof(*new));
    service->dictionary = new;
    initialize_hard_settings(service);
    initialize_soft_settings(service);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * c-file-style: "Stroustrup"
 * indent-tabs-mode: nil
 * End:
 * vim: shiftwidth=4 tabstop=8 expandtab
 */

