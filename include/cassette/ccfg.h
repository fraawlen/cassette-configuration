/**
 * Copyright © 2024 Fraawlen <fraawlen@posteo.net>
 *
 * This file is part of the Cassette Config (CCFG) library.
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

#pragma once

#include <cassette/cobj.h>
#include <stdbool.h>
#include <stdlib.h>

#if __GNUC__ > 4
	#define CCFG_NONNULL_RETURN __attribute__((returns_nonnull))
	#define CCFG_NONNULL(...)   __attribute__((nonnull (__VA_ARGS__)))
	#define CCFG_HIDDEN         __attribute__((visibility ("hidden")))
	#define CCFG_PURE           __attribute__((pure))
#else
	#define CCFG_NONNULL_RETURN
	#define CCFG_NONNULL(...)
	#define CCFG_HIDDEN
	#define CCFG_PURE
#endif

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************************************************/
/* TYPES ****************************************************************************************************/
/************************************************************************************************************/

/**
 * Opaque config object that holds all settings like sources and parameters as well as resolved parsed
 * resources. A decision was made to use a parser that saves all resources instead of setting target values
 * as the resources get read and resolved so that on a source file is read, a configuration object can be
 * shared and re-used in software plugins.
 *
 * Some methods, upon failure, will set an error that can be checked with ccfg_error(). If any error is set
 * all config methods will exit early with default return values and no side-effects. It's possible to clear
 * errors with ccfg_repair().
 */
typedef struct ccfg ccfg;

/************************************************************************************************************/
/* GLOBALS **************************************************************************************************/
/************************************************************************************************************/

/**
 * A macro that gives uninitialized config objects a non-NULL value that is safe to use with the config's
 * related functions. However, any function called with a handle set to this value will return early without
 * any side effects.
 */
#define CCFG_PLACEHOLDER (&ccfg_placeholder_instance)

/**
 * Global string object instance with the error state set to CERR_INVALID. This instance is only made
 * available to allow the static initialization of string object pointers with the macro CCFG_PLACEHOLDER.
 */
extern ccfg ccfg_placeholder_instance;

/************************************************************************************************************/
/* CONSTRUCTORS / DESTRUCTORS *******************************************************************************/
/************************************************************************************************************/

/**
 * Creates a config instance and deep copy the contents of another config instance into it.
 *
 * @return     : Created config instance
 * @return_err : CCFG_PLACEHOLDER
 */
ccfg *
ccfg_clone(ccfg *cfg)
CCFG_NONNULL_RETURN
CCFG_NONNULL(1);

/**
 * Creates an empty config instance.
 *
 * @return     : Created config instance
 * @return_err : CCFG_PLACEHOLDER
 */
ccfg *
ccfg_create(void)
CCFG_NONNULL_RETURN;

/**
 * Destroys the given config and frees memory.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_destroy(ccfg *cfg)
CCFG_NONNULL(1);

/************************************************************************************************************/
/* IMPURE METHODS *******************************************************************************************/
/************************************************************************************************************/

/**
 * Convenience generic wrapper for parameter types.
 */
#define ccfg_push_param(CFG, NAME, VAL) \
	_Generic (VAL, \
		char *       : ccfg_push_param_str,    \
		const char * : ccfg_push_param_str,    \
		float        : ccfg_push_param_double, \
		double       : ccfg_push_param_double, \
		default      : ccfg_push_param_long    \
	)(CFG, NAME, VAL)

/**
 * Removes all added parameters.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_clear_params(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Removes all parsed resources.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_clear_resources(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Removes all added sources.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_clear_sources(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Looks-up a resource by its namespace and property name. If found, its reference is kept around and the
 * resource values will become accessible through ccfg_iterate() and ccfg_resource(). To get the number of
 * values a resource has, use ccfg_resouce_length().
 *
 * Usage example :
 *
 *	ccfg_fetch(cfg, "something", "something");
 *	while (ccfg_iterate(cfg))
 *	{
 *		printf("%s\n", ccfg_resource(cfg));
 *	}
 *
 * @param cfg       : Config instance to interact with
 * @param namespace : Resource namespace
 * @param property  : Resource property name
 */
void
ccfg_fetch(ccfg *cfg, const char *namespace, const char *property)
CCFG_NONNULL(1, 2, 3);

/**
 * Increments an internal iterator offset and makes available the next value associated to a resource fetched
 * with ccfg_fetch(). Said value can be accessed with ccfg_resource(). This function exits early and returns
 * false if the iterator cannot be incremented because it has already reached the last resource value or
 * because the config has an error.
 *
 * @param cfg : Config instance to interact with
 *
 * @return     : True is the next value could be picked, false otherwhise.
 * @return_err : False
 */
bool
ccfg_iterate(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Reads the first source file that can be opened, parses it, and stores the resolved resources. Every time
 * this function is called the previously parsed resources will be cleared first before reading the source.
 * This function has no effects if no source file can be read. It should be noted that not being able to
 * open any source files is not considered to be an error by default. If such a check is needed, use
 * ccfg_can_open_sources().
 *
 * @param cfg : Config instance to interact with
 *
 * @error CERR_OVERFLOW : The size of an internal components was about to overflow
 * @error CERR_MEMORY   : Failed memory allocation during parsing
 */
void
ccfg_load(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Similar to ccfg_load() except that no source file is opened. Instead, the resources will be parsed from
 * the given buffer. The only different behavior from standard parsing is the interpretation of relative 
 * paths when an INCLUDE sequence is processed as these will be ignored if they get declared in the given
 * buffer.
 *
 * @param cfg    : Config instance to interact with
 * @param buffer : NUL terminated C-string to parse
 *
 * @error CERR_OVERFLOW : The size of an internal components was about to overflow
 * @error CERR_MEMORY   : Failed memory allocation during parsing
 */
void
ccfg_load_internal(ccfg *cfg, const char *buffer)
CCFG_NONNULL(1, 2);

/**
 * Adds a double as a config parameter. This parameter's value can then be accessed from a config source
 * file. Unlike user-defined variables, only one value per parameter can be defined.
 *
 * @param cfg  : Config instance to interact with
 * @param name : Name of the parameter to use in the source config
 * @param d    : Value
 *
 * @error CERR_OVERFLOW : The size of an internal components was about to overflow
 * @error CERR_MEMORY   : Failed memory allocation during parsing
 */
void
ccfg_push_param_double(ccfg *cfg, const char *name, double d)
CCFG_NONNULL(1, 2);

/**
 * Adds a long as a config parameter. This parameter's value can then be accessed from a config source file.
 * Unlike user-defined variables, only one value per parameter can be defined.
 *
 * @param cfg  : Config instance to interact with
 * @param name : Name of the parameter to use in the source config
 * @param l    : Value
 *
 * @error CERR_OVERFLOW : The size of an internal components was about to overflow
 * @error CERR_MEMORY   : Failed memory allocation
 */
void
ccfg_push_param_long(ccfg *cfg, const char *name, long long l)
CCFG_NONNULL(1, 2);

/**
 * Adds a C string as a config parameter. This parameter's value can then be accessed from a config source
 * file. Unlike user-defined variables, only one value per parameter can be defined.
 *
 * @param cfg  : Config instance to interact with
 * @param name : Name of the parameter to use in the source config
 * @param str  : Value
 *
 * @error CERR_OVERFLOW : The size of an internal components was about to overflow
 * @error CERR_MEMORY   : Failed memory allocation
 */
void
ccfg_push_param_str(ccfg *cfg, const char *name, const char *str)
CCFG_NONNULL(1, 2, 3);

/**
 * Adds a file as a config source. Only the first source that can be opened will be parsed. The remaining
 * sources act as fallback.
 *
 * @param cfg      : Config instance to interact with
 * @param filename : Full path to the source file
 *
 * @error CERR_OVERFLOW : The size of an internal components was about to overflow
 * @error CERR_MEMORY   : Failed memory allocation
 */
void
ccfg_push_source(ccfg *cfg, const char *filename)
CCFG_NONNULL(1, 2);

/**
 * Clears errors and puts the config back into an usable state. The only unrecoverable error is CCFG_INVALID.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_repair(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Enables the restricted parsing mode.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_restrict(ccfg *cfg)
CCFG_NONNULL(1);

/**
 * Disables the restricted parsing mode.
 *
 * @param cfg : Config instance to interact with
 */
void
ccfg_unrestrict(ccfg *cfg)
CCFG_NONNULL(1);

/************************************************************************************************************/
/* PURE METHODS *********************************************************************************************/
/************************************************************************************************************/

/**
 * Returns true if any added source file can be opened up and read. Moreover, if the index parameter is
 * provided, this function will write into it the rank of the source file that was opened.
 *
 * @param cfg      : Config instance to interact with
 * @param index    : Optional, source rank
 *
 * @return     : Source availability
 * @return_err : False
 */
bool
ccfg_can_open_sources(const ccfg *cfg, size_t *index)
CCFG_NONNULL(1);

/**
 * Gets the error state.
 *
 * @param cfg : Config instance to interact with
 *
 * @return : Error value
 */
enum cerr
ccfg_error(const ccfg *cfg)
CCFG_NONNULL(1)
CCFG_PURE;

/**
 * Gets the resource value an internal iterator is pointing at. The value is returned as a C string. It's the
 * responsibility of the caller to convert it into the required datatype. If no resource was pre-fetched
 * or the iterator hasn't been incremented once before this function gets called, return_err will be returned.
 *
 * @param cfg : Config instance to interact with
 *
 * @return     : Resource value
 * @return_err : "\0";
 */
const char *
ccfg_resource(const ccfg *cfg)
CCFG_NONNULL_RETURN
CCFG_NONNULL(1)
CCFG_PURE;

/**
 * Gets the number of values a pre-fetched resource has.
 *
 * @param cfg : Config instance to interact with
 *
 * @return     : Number of values
 * @return_err : 0
 */
size_t
ccfg_resource_length(const ccfg *cfg)
CCFG_NONNULL(1)
CCFG_PURE;

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#ifdef __cplusplus
}
#endif
