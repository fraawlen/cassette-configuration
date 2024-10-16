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
#include <stdbool.h>
#include <stdlib.h>

#include "context.h"
#include "sequence.h"
#include "source.h"
#include "util.h"

/************************************************************************************************************/
/************************************************************************************************************/
/************************************************************************************************************/

/* sequences handlers */

static void combine_var      (struct context *, enum token)   CCFG_NONNULL(1);
static void declare_enum     (struct context *)               CCFG_NONNULL(1);
static void declare_resource (struct context *, const char *) CCFG_NONNULL(1);
static void declare_variable (struct context *)               CCFG_NONNULL(1);
static void include          (struct context *)               CCFG_NONNULL(1);
static void iterate          (struct context *)               CCFG_NONNULL(1);
static void print            (struct context *)               CCFG_NONNULL(1);
static void restrict_mode    (struct context *)               CCFG_NONNULL(1);
static void section_add      (struct context *)               CCFG_NONNULL(1);
static void section_begin    (struct context *)               CCFG_NONNULL(1);
static void section_del      (struct context *)               CCFG_NONNULL(1);
static void seed             (struct context *)               CCFG_NONNULL(1);

/* iteration sequence preprocessing */

static void   preproc_iter_new  (struct context *, bool *)         CCFG_NONNULL(1);
static size_t preproc_iter_nest (struct context *, size_t, bool *) CCFG_NONNULL(1);

/************************************************************************************************************/
/* PRIVATE **************************************************************************************************/
/************************************************************************************************************/

void
sequence_parse(struct context *ctx)
{
	enum token type;
	char token[TOKEN_MAX_LEN];

	if (ctx->depth >= CONTEXT_MAX_DEPTH)
	{
		return;
	}
	
	ctx->depth++;

	if ((type = context_get_token(ctx, token, NULL)) != TOKEN_SECTION_BEGIN && ctx->skip_sequences)
	{
		type = TOKEN_INVALID;
	}
	
	switch (type)
	{
		case TOKEN_VAR_APPEND:
		case TOKEN_VAR_PREPEND:
		case TOKEN_VAR_MERGE:
			combine_var(ctx, type);
			break;

		case TOKEN_VAR_DECLARATION:
			declare_variable(ctx);
			break;

		case TOKEN_ENUM_DECLARATION:
			declare_enum(ctx);
			break;
		
		case TOKEN_SECTION_BEGIN:
			section_begin(ctx);
			break;

		case TOKEN_SECTION_ADD:
			section_add(ctx);
			break;

		case TOKEN_SECTION_DEL:
			section_del(ctx);
			break;

		case TOKEN_INCLUDE:
			include(ctx);
			break;

		case TOKEN_FOR_BEGIN:
			iterate(ctx);
			break;

		case TOKEN_SEED:
			seed(ctx);
			break;

		case TOKEN_PRINT:
			print(ctx);
			break;

		case TOKEN_RESTRICT:
			restrict_mode(ctx);
			break;

		case TOKEN_INVALID:
			break;

		case TOKEN_STRING:
		case TOKEN_NUMBER:
		default:
			declare_resource(ctx, token);
			break;
	}

	context_goto_eol(ctx);

	ctx->depth--;
}

/************************************************************************************************************/
/* STATIC ***************************************************************************************************/
/************************************************************************************************************/

static void
combine_var(struct context *ctx, enum token type)
{
	cstr *val;
	char name[TOKEN_MAX_LEN];
	char token_1[TOKEN_MAX_LEN];
	char token_2[TOKEN_MAX_LEN];
	size_t i;
	size_t j;

	if (ctx->restricted)
	{
		return;
	}

	val = cstr_create();

	/* get params */

	if (context_get_token(ctx, name,    NULL) == TOKEN_INVALID
	 || context_get_token(ctx, token_1, NULL) == TOKEN_INVALID
	 || context_get_token(ctx, token_2, NULL) == TOKEN_INVALID
	 || !cdict_find(ctx->keys_vars, token_1, CONTEXT_DICT_VARIABLE, &i)
	 || (type == TOKEN_VAR_MERGE && !cdict_find(ctx->keys_vars, token_2, CONTEXT_DICT_VARIABLE, &j)))
	{
		return;
	}

	/* generate new values and write them into the variable book */

	cbook_prepare_new_group(ctx->vars);
	for (size_t k = 0; k < cbook_group_length(ctx->vars, i); k++)
	{
		cstr_clear(val);
		cstr_append(val, cbook_word_in_group(ctx->vars, i, k));
		switch (type)
		{
			case TOKEN_VAR_APPEND:
				cstr_append(val, token_2);
				break;

			case TOKEN_VAR_PREPEND:
				cstr_prepend(val, token_2);
				break;

			case TOKEN_VAR_MERGE:
				cstr_append(val, cbook_word_in_group(ctx->vars, j, k));
				break;

			default:
				break;
		}

		cbook_write(ctx->vars, cstr_chars(val));
	}

	/* update variable's reference in the variable dict */

	cdict_write(ctx->keys_vars, name, CONTEXT_DICT_VARIABLE, cbook_groups_number(ctx->vars) - 1);

	cstr_destroy(val);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
declare_enum(struct context *ctx)
{
	char name[TOKEN_MAX_LEN];
	char token[TOKEN_MAX_LEN];
	double min;
	double max;
	double steps;
	double precision;
	double ratio;
	int n = 0;

	if (ctx->restricted)
	{
		return;
	}

	/* get enum name and params, set defaults on missing params */

	n += context_get_token        (ctx, name,  NULL)       != TOKEN_INVALID ? 1 : 0;
	n += context_get_token_numeral(ctx, token, &min)       != TOKEN_INVALID ? 1 : 0;
	n += context_get_token_numeral(ctx, token, &max)       != TOKEN_INVALID ? 1 : 0;
	n += context_get_token_numeral(ctx, token, &steps)     != TOKEN_INVALID ? 1 : 0;
	n += context_get_token_numeral(ctx, token, &precision) != TOKEN_INVALID ? 1 : 0;

	switch (n)
	{
		case 0:
		case 1:
			return;

		case 2:
			max = min;
			min = 0.0;
			/* fallthrough */

		case 3:
			steps = max - min;
			/* fallthrough */

		case 4:
			precision = 0.0;
			/* fallthrough */

		default:
			break;
	}

	if (steps < 1.0 || steps >= SIZE_MAX || precision < 0.0)
	{
		return;
	}

	if (precision > 16.0)
	{
		precision = 16;
	}
	
	/* generate enum values and write them into the variable book */

	cbook_prepare_new_group(ctx->vars);
	for (size_t i = 0; i <= steps; i++)
	{
		ratio = util_interpolate(min, max, i / steps);
		snprintf(token, TOKEN_MAX_LEN, "%.*f", (int)precision, ratio);
		cbook_write(ctx->vars, token);
	}

	/* update variable's reference in the variable dict */

	cdict_write(ctx->keys_vars, name, CONTEXT_DICT_VARIABLE, cbook_groups_number(ctx->vars) - 1);	
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
declare_resource(struct context *ctx, const char *namespace)
{
	char name[TOKEN_MAX_LEN];
	char value[TOKEN_MAX_LEN];
	size_t i;
	size_t n = 0;

	/* get resource's name */

	if (context_get_token(ctx, name, NULL) == TOKEN_INVALID)
	{
		return;
	}

	/* write resource's values into the sequence book */

	cbook_prepare_new_group(ctx->sequences);
	while (context_get_token(ctx, value, NULL) != TOKEN_INVALID)
	{
		cbook_write(ctx->sequences, value);
		n++;
	}

	if (n == 0)
	{
		cbook_undo_new_group(ctx->sequences);
		return;
	}

	/* find namespace reference in sequence dict. if not found, create it */

	if (!cdict_find(ctx->keys_sequences, namespace, 0, &i))
	{
		i = cbook_groups_number(ctx->sequences);
		cdict_write(ctx->keys_sequences, namespace, 0, i);
	}

	/* update sequence's reference in the sequence dict         */
	/* use the namespace's dict value as sequence group (i > 0) */

	cdict_write(ctx->keys_sequences, name, i, cbook_groups_number(ctx->sequences) - 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
declare_variable(struct context *ctx)
{
	char name[TOKEN_MAX_LEN];
	char value[TOKEN_MAX_LEN];
	size_t n = 0;

	if (ctx->restricted)
	{
		return;
	}

	/* get variable's name */

	if (context_get_token(ctx, name, NULL) == TOKEN_INVALID)
	{
		return;
	}

	/* write variable's values into the variable book */

	cbook_prepare_new_group(ctx->vars);
	while (context_get_token(ctx, value, NULL) != TOKEN_INVALID)
	{
		cbook_write(ctx->vars, value);
		n++;
	}

	if (n == 0)
	{
		cbook_undo_new_group(ctx->vars);
		return;
	}

	/* update variable's reference in the variable dict */

	cdict_write(ctx->keys_vars, name, CONTEXT_DICT_VARIABLE, cbook_groups_number(ctx->vars) - 1);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
include(struct context *ctx)
{
	char token[TOKEN_MAX_LEN];
	cstr *filename;

	if (ctx->restricted || ctx->file_inode == 0)
	{
		return;
	}

	filename = cstr_create();

	while (context_get_token(ctx, token, NULL) != TOKEN_INVALID)
	{
		if (token[0] != '/')
		{
			if (ctx->buffer)
			{
				cstr_clear(filename);
				cstr_append(filename, ctx->file_dir);
				cstr_append(filename, "/");
				cstr_append(filename, token);
				source_parse_child(ctx, cstr_chars(filename));
			}
		}
		else
		{	
			source_parse_child(ctx, token);
		}
	}

	cstr_destroy(filename);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
iterate(struct context *ctx)
{
	char name[TOKEN_MAX_LEN];
	char token[TOKEN_MAX_LEN];
	size_t group_start;
	size_t group_end;
	size_t i;
	size_t j;
	bool nested;
	bool fail = false;

	if (ctx->restricted)
	{
		return;
	}

	/* get iteration params and detect if it's nested */

	if (context_get_token(ctx, token,  NULL) == TOKEN_INVALID
	 || !cdict_find(ctx->keys_vars, token, CONTEXT_DICT_VARIABLE, &i))
	{
		return;
	}

	if (context_get_token(ctx, name, NULL) == TOKEN_INVALID)
	{
		snprintf(name, TOKEN_MAX_LEN, "%s", token);
	}
	
	if (cdict_find(ctx->keys_vars, name, CONTEXT_DICT_ITERATION, &j))
	{
		return;
	}

	nested = cbook_length(ctx->iteration);

	/* In the case of a new iteration, read the file and write raw sequences into the iteration book,    */
	/* but do not do that for nested iterations since the data is already written in the iteration book. */
	/* In both cases, find out which saved sequence marks the end of the iteration block                 */

	if (nested)
	{
		group_start = ctx->it_group + 1;
		group_end   = preproc_iter_nest(ctx, group_start, &fail);
	}
	else
	{
		preproc_iter_new(ctx, &fail);
		group_start = 0;
		group_end   = cbook_groups_number(ctx->iteration);
	}

	if (fail)
	{
		goto skip;
	}

	/* run iterated sequences */

	for (size_t k = 0; k < cbook_group_length(ctx->vars, i); k++)
	{
		cdict_write(ctx->keys_vars, name, CONTEXT_DICT_ITERATION, cbook_word_index(ctx->vars, i, k));
		for (ctx->it_group = group_start; ctx->it_group < group_end; ctx->it_group++)
		{
			ctx->it_i = 0;
			sequence_parse(ctx);
		}
	}

	/* restore iterator state */

	cdict_erase(ctx->keys_vars, name, CONTEXT_DICT_ITERATION);

skip:

	if (!nested)
	{
		cbook_clear(ctx->iteration);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static size_t
preproc_iter_nest(struct context *ctx, size_t start_group, bool *fail)
{
	char token[TOKEN_MAX_LEN];
	size_t n = 0;
	size_t i;

	for (i = start_group; i < cbook_groups_number(ctx->iteration); i++)
	{
		ctx->it_group = i;
		ctx->it_i     = 0;
		context_get_token_raw(ctx, token);

		/* look for matching TOKEN_FOR_END */

		switch (token_match(ctx->tokens, token))
		{
			case TOKEN_FOR_BEGIN:
				n++;
				break;

			case TOKEN_FOR_END:
				if (n == 0)
				{
					return i;
				}
				n--;
				break;

			default:
				break;
		}
	}

	*fail = true;

	return i;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
preproc_iter_new(struct context *ctx, bool *fail)
{
	char token[TOKEN_MAX_LEN];
	size_t n = 0;

	context_goto_eol(ctx);

	while (!ctx->eof_reached)
	{
		ctx->eol_reached = false;
		context_get_token_raw(ctx, token);

		/* look for matching TOKEN_FOR_END */

		switch (token_match(ctx->tokens, token))
		{
			case TOKEN_FOR_BEGIN:
				n++;
				break;

			case TOKEN_FOR_END:
				if (n == 0)
				{
					context_goto_eol(ctx);
					return;
				}
				n--;
				break;

			case TOKEN_INVALID:
				context_goto_eol(ctx);
				continue;

			default:
				break;
		}

		/* write down sequences that will be iterated */

		cbook_prepare_new_group(ctx->iteration);
		cbook_write(ctx->iteration, token);
		while (context_get_token_raw(ctx, token) != TOKEN_INVALID)
		{
			cbook_write(ctx->iteration, token);
		}
	}

	*fail = true;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
print(struct context *ctx)
{
	char token[TOKEN_MAX_LEN];
	
	if (ctx->restricted)
	{
		return;
	}

	while (context_get_token(ctx, token, NULL) != TOKEN_INVALID)
	{
		fprintf(stderr, "%s,\t", token);
	}

	fprintf(stderr, "\n");
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
restrict_mode(struct context *ctx)
{
	ctx->restricted = true;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
section_add(struct context *ctx)
{
	char token[TOKEN_MAX_LEN];

	if (ctx->restricted)
	{
		return;
	}

	while (context_get_token(ctx, token, NULL) != TOKEN_INVALID)
	{
		cdict_write(ctx->keys_vars, token, CONTEXT_DICT_SECTION, 0);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
section_begin(struct context *ctx)
{
	char token[TOKEN_MAX_LEN];

	if (ctx->restricted)
	{
		return;
	}

	while (context_get_token(ctx, token, NULL) != TOKEN_INVALID)
	{
		if (!cdict_find(ctx->keys_vars, token, CONTEXT_DICT_SECTION, NULL))
		{
			ctx->skip_sequences = true;
			return;
		}
	}
	
	ctx->skip_sequences = false;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
section_del(struct context *ctx)
{
	char token[TOKEN_MAX_LEN];

	if (ctx->restricted)
	{
		return;
	}

	while (context_get_token(ctx, token, NULL) != TOKEN_INVALID)
	{
		cdict_erase(ctx->keys_vars, token, CONTEXT_DICT_SECTION);
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

static void
seed(struct context *ctx)
{
	char token[TOKEN_MAX_LEN];
	double d;
	
	if (ctx->restricted)
	{
		return;
	}

	if (context_get_token_numeral(ctx, token, &d) != TOKEN_INVALID)
	{
		ctx->rand = crand_seed(d);
	}
}
