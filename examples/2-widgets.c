/**
 * Copyright © 2024 Fraawlen <fraawlen@posteo.net>
 *
 * This file is part of the Cassette Configuration (CCFG) library.
 *
 * This library is free software; you can redistribute it and/or modify it either under the terms of the GNU
 * Lesser General Public License as published by the Free Software Foundation; either version 2.1 of the
 * License or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
 * See the LGPL for the specific language governing rights and limitations.
 *
 * You should have received a copy of the GNU Lesser General Public License along with this program. If not,
 * see <http://www.gnu.org/licenses/>.
 */

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#include <cassette/ccfg.h>
#include <cassette/cobj.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h" /* generated by makefile */

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#define SAMPLE_CONFIG_PATH "/tmp/ccfg_sample"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

struct widget
{
	const char *name;
	unsigned long border_width;
	struct ccolor border_color;
	struct ccolor background_color;
};

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static void _generate_source (void);
static void _widget_config   (struct widget *w);
static void _widget_print    (const struct widget *w);

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static ccfg *_cfg = CCFG_PLACEHOLDER;

struct widget _label  = {.name = "label",  .border_width = 0, .border_color = {0} , .background_color = {0}};
struct widget _button = {.name = "button", .border_width = 0, .border_color = {0} , .background_color = {0}};
struct widget _switch = {.name = "switch", .border_width = 0, .border_color = {0} , .background_color = {0}};
struct widget _gauge  = {.name = "gauge",  .border_width = 0, .border_color = {0} , .background_color = {0}};

/************************************************************************************************************/
/* MAIN *****************************************************************************************************/
/************************************************************************************************************/

/**
 * In this 2nd example, we query three resources (background colour, border colour, and border width) for
 * four widget types: label, button, switch, and gauge. Unlike the 1st example, the queried resources must be
 * converted into the correct data type. If a resource is not found, hardcoded default values will be used.
 *
 * This example demonstrates how to use the FOR_EACH iteration sequences within the configuration file. This
 * iteration sets the same border width and colour for all widgets.
 */

int
main(void)
{
	/* Setup */

	_cfg = ccfg_create();

	_generate_source();

	ccfg_push_source(_cfg, SAMPLE_CONFIG_PATH);

	/* Operations */

	ccfg_load(_cfg);

	_widget_config(&_label);
	_widget_config(&_button);
	_widget_config(&_switch);
	_widget_config(&_gauge);

	_widget_print(&_label);
	_widget_print(&_button);
	_widget_print(&_switch);
	_widget_print(&_gauge);

	/* End */

	if (ccfg_error(_cfg))
	{
		printf("Configuration parser failed during operation.\n");
	}

	return 0;
}

/************************************************************************************************************/
/* _ ********************************************************************************************************/
/************************************************************************************************************/

static void
_generate_source(void)
{
	FILE *f;

	if (!(f = fopen(SAMPLE_CONFIG_PATH, "w")))
	{
		printf("Sample configuration in %s could not be generated\n", SAMPLE_CONFIG_PATH);
		return;
	}

	fprintf(f, "%.*s", examples_config_len, examples_config);
	fclose(f);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
_widget_config(struct widget *w)
{
	/* background color */

	ccfg_fetch(_cfg, w->name, "background_color");
	if (ccfg_iterate(_cfg))
	{
		w->background_color = ccolor_from_str(ccfg_resource(_cfg), NULL);
	}

	/* border color */

	ccfg_fetch(_cfg, w->name, "border_color");
	if (ccfg_iterate(_cfg))
	{
		w->border_color = ccolor_from_str(ccfg_resource(_cfg), NULL);
	}

	/* border width */
	
	ccfg_fetch(_cfg, w->name, "border_width");
	if (ccfg_iterate(_cfg))
	{
		w->border_width = strtoul(ccfg_resource(_cfg), NULL, 0);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
_widget_print(const struct widget *w)
{
	printf("%s:\n", w->name);

	printf(
		"\tbackground_color (r,g,b) : %u, %u, %u\n",
		(unsigned int)(w->background_color.r * 255),
		(unsigned int)(w->background_color.g * 255),
		(unsigned int)(w->background_color.b * 255));

	printf(
		"\tbackground_color (r,g,b) : %u, %u, %u\n",
		(unsigned int)(w->border_color.r * 255),
		(unsigned int)(w->border_color.g * 255),
		(unsigned int)(w->border_color.b * 255));

	printf("\tborder_width : %lu\n\n", w->border_width);
}

