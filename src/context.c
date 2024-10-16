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

#include <assert.h>
#include <cassette/ccfg.h>
#include <cassette/cobj.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "context.h"
#include "substitution.h"
#include "token.h"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

static char read_char    (struct context *)                              CCFG_NONNULL(1);
static bool read_word    (struct context *, char [static TOKEN_MAX_LEN]) CCFG_NONNULL(1);
static void update_state (struct context *, char)                        CCFG_NONNULL(1);

/************************************************************************************************************/
/* PRIVATE **************************************************************************************************/
/************************************************************************************************************/

enum token
context_get_token(struct context *ctx, char token[static TOKEN_MAX_LEN], double *math_result)
{
	if (context_get_token_raw(ctx, token) == TOKEN_INVALID)
	{
		return TOKEN_INVALID;
	}

	return substitution_apply(ctx, token, math_result);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

enum token
context_get_token_numeral(struct context *ctx, char token[static TOKEN_MAX_LEN], double *math_result)
{
	bool err = false;

	switch (context_get_token(ctx, token, math_result))
	{
		case TOKEN_NUMBER:
			return TOKEN_NUMBER;
		
		case TOKEN_STRING:
			if (token[0] == '#')
			{
				*math_result = ccolor_to_argb_uint(ccolor_from_str(token, &err));
			}
			else
			{
				*math_result = strtod(token, NULL);
			}
			if (!err)
			{
				return TOKEN_NUMBER;
			}
			/* fallthrough */

		default:
			return TOKEN_INVALID;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

enum token
context_get_token_raw(struct context *ctx, char token[static TOKEN_MAX_LEN])
{
	if (ctx->var_i < cbook_group_length(ctx->vars, ctx->var_group))
	{
		snprintf(token, TOKEN_MAX_LEN, "%s", cbook_word_in_group(ctx->vars, ctx->var_group, ctx->var_i++));
	}
	else if (ctx->it_i < cbook_group_length(ctx->iteration, ctx->it_group))
	{
		snprintf(token, TOKEN_MAX_LEN, "%s", cbook_word_in_group(ctx->iteration, ctx->it_group, ctx->it_i++));
	}
	else if (!read_word(ctx, token))
	{
		return TOKEN_INVALID;
	}
	
	return TOKEN_STRING;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

void
context_goto_eol(struct context *ctx)
{
	while (!ctx->eol_reached)
	{
		update_state(ctx, read_char(ctx));
	}

	ctx->var_i = SIZE_MAX;
	ctx->it_i  = SIZE_MAX;
}

/************************************************************************************************************/
/* STATIC ***************************************************************************************************/
/************************************************************************************************************/

static char
read_char(struct context *ctx)
{
	return *ctx->buffer != '\0' ? *(ctx->buffer++) : '\0';
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static bool
read_word(struct context *ctx, char token[static TOKEN_MAX_LEN])
{
	size_t i = 0;
	bool quotes_1 = false;
	bool quotes_2 = false;
	char c;

	if (ctx->eol_reached)
	{
		return false;
	}

	/* skip leading whitespaces */

	for (;;)
	{
		switch ((c = read_char(ctx)))
		{
			case ' ' :
			case '(' :
			case ')' :
			case '\t':
			case '\v':
				break;
			
			default:
				goto exit_lead;
		}
	}

exit_lead:

	/* read word */

	for (;; c = read_char(ctx))
	{
		switch (c)
		{
			case '\0':
				goto exit_word;

			case ' ' :
			case '(' :
			case ')' :
			case '\t':
			case '\v':
			case '\n':
				if (quotes_1 || quotes_2)
				{
					goto char_add;
				}
				goto exit_word;
				
			case '\'':
				if (!quotes_2)
				{
					quotes_1 = !quotes_1;
					break;
				}
				goto char_add;

			case '\"':
				if (!quotes_1)
				{
					quotes_2 = !quotes_2;
					break;
				}
				goto char_add;

			default:
			char_add:
				if (i < TOKEN_MAX_LEN - 1)
				{
					token[i++] = (char)c;
				}
				break;
		}
	}

exit_word:
	
	/* end */

	update_state(ctx, c);

	token[i] = '\0';

	return i;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
update_state(struct context *ctx, char c)
{
	switch (c)
	{
		case '\0':
			ctx->eof_reached = true;
			/* fallthrough */

		case '\n':
			ctx->eol_reached = true;
			/* fallthrough */	

		default:
			break;
	}
}
