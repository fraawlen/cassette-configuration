/**
 * Copyright © 2024 Fraawlen <fraawlen@posteo.net>
 *
 * This file is part of the Derelict Resources (DR) library.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <derelict/do.h>
#include <derelict/dr.h>

#include "config.h" /* generated by makefile */

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#define _SAMPLE_CONFIG_PATH "/tmp/dr_sample"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static void _callback        (dr_data_t *cfg, bool load_success, void *ref);
static void _generate_source (void);

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

/* sets of values to mimic a program's configuration */

static long   _a     = 0;
static double _b     = 0.0;
static double _c[3]  = {0.0, 0.0, 0.0};
static bool   _d     = false;
static char   _e[40] = "";
static long   _f     = 0;

static do_color_t _g = {0};

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

/**
 * In this 2nd example, multiple static values (_a, _b, ...) represent the configuration variables or some
 * hypothetical program. Thanks to the callback function, every time the configuration gets reloaded, they
 * first get reset to a built-in default value, then get assigned a new value extracted and converted from
 * the configuration. In case a resource is not found, the value it's assigned to will keep its default
 * value.
 *
 * Value assignment through callbacks is useful when dealing with program extensions. Each extension, when
 * initialized, will add its own callback to the configuration. Then a single call to dr_load() will
 * trigger them, hence reloading the configuration of both the main program and its extensions.
 */

int
main(void)
{
	dr_data_t *cfg;

	/* init */

	cfg = dr_create();

	_generate_source();

	/* operations */

	dr_push_source(cfg, _SAMPLE_CONFIG_PATH);
	dr_push_callback(cfg, _callback, NULL);
	dr_push_parameter_double(cfg, "internal_param", 1337);

	dr_load(cfg); /* load success check is done in callback */

	/* print loaded values */

	printf("a -> %li\n",        _a);
	printf("b -> %f\n",         _b);
	printf("c -> %f, %f, %f\n", _c[0], _c[1], _c[2]);
	printf("d -> %s\n",         _d ? "true" : "false");
	printf("e -> %s\n",         _e);
	printf("f -> %li\n",        _f);
	printf(
		"g -> r = %u, g = %u, b = %u, a = %u\n",
		(unsigned int)(_g.r * 256),
		(unsigned int)(_g.g * 256),
		(unsigned int)(_g.b * 256),
		(unsigned int)(_g.a * 256));

	/* end */

	if (dr_has_failed(cfg))
	{
		printf("configuration has failed during operation.\n");
	}

	dr_destroy(&cfg);

	return 0;
}

/************************************************************************************************************/
/* _ ********************************************************************************************************/
/************************************************************************************************************/

static void
_callback(dr_data_t *cfg, bool load_success, void *ref)
{
	(void)(ref);

	if (!load_success)
	{
		printf("\nconfiguration failed to load\n");
		return;
	}

	/* set defaults */

	_a    = 0;
	_b    = 0.0;
	_c[0] = 0.0;
	_c[1] = 0.0;
	_c[2] = 0.0;
	_d    = false;
	_f    = 0.0;
	_g    = DO_COLOR_BLACK;
	
	snprintf(_e, sizeof(_e), "some-stuff");

	/* apply values from config */

	dr_fetch_resource(cfg, "example-2", "a");
	if (dr_pick_next_resource_value(cfg))
	{
		_a = strtol(dr_get_resource_value(cfg), NULL, 0);
	}

	dr_fetch_resource(cfg, "example-2", "b");
	if (dr_pick_next_resource_value(cfg))
	{
		_b = strtod(dr_get_resource_value(cfg), NULL);
	}

	dr_fetch_resource(cfg, "example-2", "c");
	for (size_t i = 0; i < 3 && dr_pick_next_resource_value(cfg); i++)
	{	
		_c[i] = strtod(dr_get_resource_value(cfg), NULL);
	}

	dr_fetch_resource(cfg, "example-2", "d");
	if (dr_pick_next_resource_value(cfg))
	{
		_d = strtod(dr_get_resource_value(cfg), NULL) != 0.0;
	}

	dr_fetch_resource(cfg, "example-2", "e");
	if (dr_pick_next_resource_value(cfg))
	{
		snprintf(_e, sizeof(_e), "%s", dr_get_resource_value(cfg));
	}

	dr_fetch_resource(cfg, "example-2", "f");
	if (dr_pick_next_resource_value(cfg))
	{
		_f = strtol(dr_get_resource_value(cfg), NULL, 0);
	}

	dr_fetch_resource(cfg, "example-2", "g");
	if (dr_pick_next_resource_value(cfg))
	{
		_g = do_color_convert_str(dr_get_resource_value(cfg), NULL);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
_generate_source(void)
{
	FILE *f;

	if (!(f = fopen(_SAMPLE_CONFIG_PATH, "w")))
	{
		printf("sample configuration in %s could not be generated\n", _SAMPLE_CONFIG_PATH);
		return;
	}

	fprintf(f, "%.*s", examples_config_len, examples_config);

	fclose(f);
}
