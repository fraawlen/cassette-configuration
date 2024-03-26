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

#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#include <derelict/do.h>

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

struct _config_t
{
	do_book_t *sequences;        /* parsed sequences of values                        */
	do_tracker_t *sources;       /* source files to read from                         */
	do_tracker_t *callbacks;     /* callback list to call after load                  */
	do_dictionary_t *references; /* reference sequences matching resources            */
	do_dictionary_t *tokens;     /* token references kept here to reuse between loads */
	bool failed;
};

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

#endif /* CONFIG_H */
