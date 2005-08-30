/***************************************************************************
 *            hocr.c
 *
 *  Fri Aug 12 20:18:56 2005
 *  Copyright  2005  Yaacov Zamir
 *  <kzamir@walla.co.il>
 ****************************************************************************/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "consts.h"
#include "page_layout.h"
#include "font_layout.h"
#include "font_markers.h"
#include "hocr.h"

/* 
 internal line_eq stractures 
 */

int
hocr_line_eq_get_y (hocr_line_eq line, int x)
{
	return (int) (line.a * (double) x + line.b);
}

/* 
 internal text_buffer stractures 
 */

hocr_text_buffer *
hocr_text_buffer_new ()
{
	hocr_text_buffer *new_text_buffer;

	/* allocate memory for pixbuf */
	new_text_buffer =
		(hocr_text_buffer *) malloc (sizeof (hocr_text_buffer));

	if (!new_text_buffer)
		return 0;

	new_text_buffer->size = 0;
	new_text_buffer->allocated_size = MEMORY_CHANK_FOR_TEXT_BUFFER;

	new_text_buffer->text =
		(char *) malloc (sizeof (char) *
				 new_text_buffer->allocated_size);

	if (!new_text_buffer->text)
	{
		free (new_text_buffer);
		return 0;
	}

	(new_text_buffer->text)[0] = '\0';

	return new_text_buffer;
}

int
hocr_text_buffer_unref (hocr_text_buffer * text_buffer)
{
	if (text_buffer)
		free (text_buffer);

	return 1;
}

int
hocr_text_buffer_add_string (hocr_text_buffer * text_buffer,
			     const char *new_text)
{
	int len;
	char *new_allocated_text;

	len = strlen (new_text);

	/* check for allocated space and try to get more memory */
	if ((text_buffer->size + len) >= text_buffer->allocated_size)
	{
		new_allocated_text =
			realloc (text_buffer->text, sizeof (char) *
				 (text_buffer->allocated_size +
				  MEMORY_CHANK_FOR_TEXT_BUFFER));

		if (new_allocated_text)
		{
			text_buffer->text = new_allocated_text;
			text_buffer->allocated_size +=
				MEMORY_CHANK_FOR_TEXT_BUFFER;
		}
		else
		{
			return text_buffer->size;
		}
	}

	strcat (text_buffer->text, new_text);
	text_buffer->size = strlen (text_buffer->text);

	return text_buffer->size;
}

/* 
 internal pixbuf stractures 
 */

int
hocr_pixbuf_get_n_channels (hocr_pixbuf * pix)
{
	return pix->n_channels;
}

int
hocr_pixbuf_get_height (hocr_pixbuf * pix)
{
	return pix->height;
}

int
hocr_pixbuf_get_width (hocr_pixbuf * pix)
{
	return pix->width;
}

int
hocr_pixbuf_get_rowstride (hocr_pixbuf * pix)
{
	return pix->rowstride;
}

int
hocr_pixbuf_get_brightness (hocr_pixbuf * pix)
{
	return pix->brightness;
}

unsigned char *
hocr_pixbuf_get_pixels (hocr_pixbuf * pix)
{
	return pix->pixels;
}

int
hocr_pixbuf_get_pixel (hocr_pixbuf * pix, int x, int y)
{
	unsigned char *pixel;

	if (x < 0 || x > pix->width || y < 0 || y > pix->height)
		return 0;

	pixel = pix->pixels + x * pix->n_channels + y * pix->rowstride;

	return (pixel[0] < pix->brightness) ? 1 : 0;
}

int
hocr_pixbuf_set_pixel (hocr_pixbuf * pix, int x, int y, int channel,
		       int value)
{
	unsigned char *pixel;

	if (x < 0 || x > pix->width || y < 0 || y > pix->height)
		return 0;

	pixel = pix->pixels + x * pix->n_channels + y * pix->rowstride;
	pixel[channel] = value;

	return (pixel[channel] < pix->brightness) ? 1 : 0;
}

/* hocr_pixbuf file utils */

unsigned char
hocr_pbm_getc (FILE * file)
{
	unsigned char ch;
	int comment = FALSE;

	do
	{
		ch = getc (file);
		if (ch == '\n')
			comment = FALSE;
		else if (ch == '#')
			comment = TRUE;
	}
	while (comment);

	return ch;
}

int
hocr_pbm_getint (FILE * file)
{
	unsigned char ch;
	int i = 0;

	do
	{
		ch = hocr_pbm_getc (file);
	}
	while (ch == ' ' || ch == '\n' || ch == '\t');

	do
	{
		i = (i * 10) + (ch - '0');
		ch = hocr_pbm_getc (file);
	}
	while (ch >= '0' && ch <= '9');

	return i;
}

int
hocr_pbm_getbit (FILE * file, int gray_scale)
{

	static unsigned char byte = 0;
	static unsigned char mask = 0;
	int return_bit;

	if (gray_scale)
	{
		return_bit = getc (file);
	}
	else
	{
		if (mask == 0)
		{
			mask = 0x80;
			byte = getc (file);
		}

		return_bit = (byte & mask) ? 0 : 255;

		mask >>= 1;
	}

	return return_bit;
}

hocr_pixbuf *
hocr_pixbuf_new_from_file (const char *filename)
{
	int x, y;
	unsigned char *pixel;
	int bit_read;
	char char_read;
	hocr_pixbuf *new_pixbuf;
	FILE *file = NULL;
	int gray_scale = FALSE;
	int dippnes = 1;

	/* open file */
	file = fopen (filename, "r");
	if (!file)
		return NULL;

	/* allocate memory for pixbuf */
	new_pixbuf = (hocr_pixbuf *) malloc (sizeof (hocr_pixbuf));
	if (!new_pixbuf)
		return NULL;

	/* read magic number "P4" for pbm file */
	char_read = hocr_pbm_getc (file);
	if (char_read != 'P')
		return NULL;
	char_read = hocr_pbm_getc (file);
	if (char_read == '4')
		gray_scale = FALSE;
	else if (char_read == '5')
		gray_scale = TRUE;
	else
		return NULL;

	/* read header */
	new_pixbuf->n_channels = 3;
	new_pixbuf->brightness = 100;
	new_pixbuf->pixels = NULL;

	/* read width and height */
	new_pixbuf->width = hocr_pbm_getint (file);
	new_pixbuf->height = hocr_pbm_getint (file);
	new_pixbuf->rowstride = new_pixbuf->width * 3;

	/* read gray_scale dippnes */
	if (gray_scale)
		dippnes = hocr_pbm_getint (file);
	if (dippnes > 255)
		return NULL;

	/* allocate memory for data */
	new_pixbuf->pixels =
		malloc (new_pixbuf->height * new_pixbuf->rowstride);
	if (!(new_pixbuf->pixels))
		return NULL;

	/* read data */
	for (y = 0; y < new_pixbuf->height; y++)
	{
		for (x = 0; x < new_pixbuf->width; x++)
		{
			bit_read = hocr_pbm_getbit (file, gray_scale);
			pixel = new_pixbuf->pixels +
				x * new_pixbuf->n_channels +
				y * new_pixbuf->rowstride;
			pixel[0] = bit_read;
			pixel[1] = bit_read;
			pixel[2] = bit_read;
		}
	}

	/* return the new pixbuf to user */
	return new_pixbuf;
}

int
hocr_pixbuf_unref (hocr_pixbuf * pix)
{
	if (pix->pixels)
		free (pix->pixels);

	if (pix)
		free (pix);

	return 1;
}

/* 
 visualization helper finction
 */

int
print_font (hocr_pixbuf * pix, hocr_box font)
{
	int x, y;
	int new_color;

	for (y = font.y1; y < (font.y2 + 0); y++)
	{
		for (x = font.x1; x < (font.x2 + 1); x++)
		{
			new_color = hocr_pixbuf_get_pixel (pix, x, y);
			printf ("%d", new_color);
		}
		printf ("\n");
	}

	printf ("\n");

	return 0;
}

int
color_hocr_box_full (hocr_pixbuf * pix, hocr_box rect, int chanell, int value)
{
	int x, y;

	for (y = rect.y1; y < rect.y2; y++)
		for (x = rect.x1; x < rect.x2; x++)
		{
			hocr_pixbuf_set_pixel (pix, x, y, chanell, value);
		}

	return 0;
}

int
color_hocr_box (hocr_pixbuf * pix, hocr_box rect, int chanell, int value)
{
	int x, y;

	for (y = rect.y1; y < rect.y2; y++)
	{
		hocr_pixbuf_set_pixel (pix, rect.x1, y, chanell, value);
		hocr_pixbuf_set_pixel (pix, rect.x2, y, chanell, value);
	}

	for (x = rect.x1; x < rect.x2; x++)
	{
		hocr_pixbuf_set_pixel (pix, x, rect.y1, chanell, value);
		hocr_pixbuf_set_pixel (pix, x, rect.y2, chanell, value);
	}

	return 0;
}

int
color_hocr_line_eq (hocr_pixbuf * pix, hocr_line_eq * line, int x1, int x2,
		    int chanell, int value)
{
	int x, y;

	for (x = x1; x < x2; x++)
	{
		y = hocr_line_eq_get_y (*line, x);
		hocr_pixbuf_set_pixel (pix, x, y, chanell, value);
	}

	return 0;
}

/*
 */

int
hocr_do_ocr (hocr_pixbuf * pix, hocr_text_buffer * text_buffer,
	     hocr_error * error)
{
	hocr_box column;
	/* hocr_box column; is a place holder to a time when we add column support */
	hocr_box lines[MAX_LINES];
	hocr_line_eq line_eqs[MAX_LINES][2];
	hocr_box fonts[MAX_LINES][MAX_FONTS_IN_LINE];

	int num_of_fonts[MAX_LINES];
	int num_of_lines;
	int num_of_fonts_in_page;

	int avg_font_hight_in_page;
	int avg_font_width_in_page;

	int i, j, k;
	int y1, y2;

	/* font position classes */
	int base_class;
	int top_class;
	int hight_class;
	int width_class;
	int end_of_line;
	int end_of_word;
	int end_of_paragraph;

	/* font shape markers */

	/* an array of font marks */
	int font_mark[MAX_FONTS_IN_FONT_LIB];
	int number_of_fonts_in_font_lib;
	char font_strings[MAX_FONTS_IN_FONT_LIB][MAX_NUM_OF_CHARS_IN_FONT];

	/* an array of function for detecting font marks */
	has_font_mark_function has_font_mark[MAX_FONTS_IN_FONT_LIB];

	/* need this to put in the text_buffer */
	int last_was_quot = 0;
	/* FIXME: what size is the new string to add ? */
	char chars[MAX_NUM_OF_CHARS_IN_FONT];

	/* create an array of all has_font_mark_functions */
	init_has_font_mark_functions_hebrew_alfabet (has_font_mark,
						     &number_of_fonts_in_font_lib,
						     font_strings);

	/* get all lines in this column */
	fill_lines_array (pix, column, lines, &num_of_lines, MAX_LINES);

	/* get all fonts for all the lines */
	for (i = 0; i < num_of_lines; i++)
	{
		/* visual aids to see line hocr_box on screen */
		/* color_hocr_box (pix, lines[i], 1, 0); */
		fill_fonts_array (pix, lines[i],
				  fonts[i],
				  &(num_of_fonts[i]), MAX_FONTS_IN_LINE);
	}

	/* get size statistics for all fonts for all the lines */
	num_of_fonts_in_page = 0;
	avg_font_hight_in_page = 0;
	avg_font_width_in_page = 0;
	for (i = 0; i < num_of_lines; i++)
	{
		for (j = 0; j < num_of_fonts[i]; j++)
		{
			/* visual aids to see line hocr_box on screen */
			/* color_hocr_box (pix, fonts[i][j], 2, 0); */

			num_of_fonts_in_page++;
			avg_font_width_in_page += fonts[i][j].width;
			avg_font_hight_in_page += fonts[i][j].hight;
		}
	}

	if (num_of_fonts_in_page != 0)
	{
		avg_font_width_in_page /= num_of_fonts_in_page;
		avg_font_hight_in_page /= num_of_fonts_in_page;
	}

	/* get lines equations for non horizontal lines */
	for (i = 0; i < num_of_lines; i++)
	{
		find_font_baseline_eq (lines[i], fonts[i], &(line_eqs[i][0]), &(line_eqs[i][1]),
				       avg_font_hight_in_page, num_of_fonts[i]);
		
		color_hocr_line_eq (pix, &(line_eqs[i][0]), lines[i].x1, lines[i].x2,
				    2, 0);
		color_hocr_line_eq (pix, &(line_eqs[i][1]), lines[i].x1, lines[i].x2,
				    2, 0);
	}

	/* get all fonts for all the lines */
	for (i = 0; i < num_of_lines; i++)
	{
		for (j = 0; j < num_of_fonts[i]; j++)
		{

			y1 = find_font_topline (fonts[i],
						avg_font_hight_in_page,
						j, num_of_fonts[i]);
			y2 = find_font_baseline (fonts[i],
						 avg_font_hight_in_page,
						 j, num_of_fonts[i]);

			/* font position and size markers 
			 */
			base_class =
				get_font_base_class (fonts[i][j].y2,
						     y2,
						     avg_font_hight_in_page);
			top_class =
				get_font_top_class (fonts[i][j].y1,
						    y1,
						    avg_font_hight_in_page);
			hight_class =
				get_font_hight_class (fonts[i][j].
						      hight,
						      avg_font_hight_in_page);
			width_class =
				get_font_width_class (fonts[i][j].
						      width,
						      avg_font_width_in_page);

			/* line markers */
			end_of_line = (j == (num_of_fonts[i] - 1)) ? 1 : 0;
			end_of_paragraph = 0;
			if (end_of_line == 0)
			{
				end_of_word =
					(fonts[i][j].x1 -
					 fonts[i][j + 1].x2) >
					MIN_DISTANCE_BETWEEN_WORDS;
			}
			else
			{
				if ((i < num_of_lines - 1)
				    && ((lines[i + 1].y1 - lines[i].y2) >
					(int) (1.5 *
					       (double)
					       avg_font_hight_in_page)))
					end_of_paragraph = 1;

				end_of_word = 1;
			}

			/* font shape markers */

			/* TODO: this shuld be moved to the right place 
			 * and not doen unnesseraly for all fonts */

			for (k = 1; k <= number_of_fonts_in_font_lib; k++)
			{
				font_mark[k] =
					(has_font_mark[k]) (pix, fonts[i][j],
							    end_of_word,
							    lines[i].y1,
							    lines[i].y2,
							    avg_font_width_in_page);
			}

			font_mark[0] = 0;

			/* if wide then arteffact */

			if (width_class == 1)
			{
				/* arteffact */
				sprintf (chars, "__");
				font_mark[0] = 1;
			}

			/* small fonts */

			else if (hight_class == -1
				 && top_class == 1 && base_class == 1)
			{
				/* '-' */
				sprintf (chars, "-");
			}
			else if (hight_class == -1 && top_class == 0)
			{
				if (font_mark[34] == 1)
				{
					/* '-' */
					sprintf (chars, "-");
				}
				else if (font_mark[10] == 1)
				{
					/* yud */
					sprintf (chars, "י");
				}
				else if (font_mark[31] == 1)
				{
					sprintf (chars, "\"");
				}
				else if (font_mark[30] == 1)
				{
					sprintf (chars, "\'");
				}
				else
				{
					sprintf (chars, "_");
					font_mark[0] = 1;
				}
			}
			else if ((hight_class == -1) && (top_class == 1)
				 && (base_class == 0))
			{
				/* period */
				sprintf (chars, ".");
			}
			else if ((hight_class == -1)
				 && (top_class == 1) && (base_class == -1))
			{
				/* we assume comma */
				sprintf (chars, ",");
			}

			/* high fonts desending from top */

			else if (hight_class == 1 && top_class == -1)
			{
				/* lamed */
				sprintf (chars, "ל");
			}

			/* high and thin fonts desending from buttom */

			else if (hight_class == 1 && width_class == -1
				 && top_class == 0)
			{
				/* nun sofit */
				sprintf (chars, "ן");
			}

			/* high fonts desending from buttom */

			else if (hight_class == 1 && width_class == 0
				 && top_class == 0)
			{
				if (font_mark[12] == 1)
				{
					/* kaf sofit */
					sprintf (chars, "ך");
				}
				else if (font_mark[19] == 1)
				{
					/*  ayin */
					sprintf (chars, "ע");
				}
				else if (font_mark[21] == 1)
				{
					/*  pe sofit */
					sprintf (chars, "ף");
				}
				else if (font_mark[23] == 1)
				{
					/* tzadi */

					sprintf (chars, "ץ");
				}
				else if (font_mark[24] == 1)
				{
					/* kuf */
					sprintf (chars, "ק");
				}
				else
				{
					sprintf (chars, "_");
					font_mark[0] = 1;
				}
			}

			/* thin fonts */

			else if (width_class == -1)
			{
				/* gimel 2, vav 5, zayin 6, tet 8, nun 15 */

				if (font_mark[32] == 1)
				{
					/* tet */
					sprintf (chars, "!");
				}
				else if (font_mark[33] == 1)
				{
					/* gimel */
					sprintf (chars, "?");
				}
				else if (font_mark[3] == 1)
				{
					/* gimel */
					sprintf (chars, "ג");
				}
				else if (font_mark[9] == 1)
				{
					/* tet */
					sprintf (chars, "ט");
				}
				else if (font_mark[19] == 1)
				{
					/* ayin */
					sprintf (chars, "ע");
				}
				else if (font_mark[16] == 1)
				{
					/* nun */
					sprintf (chars, "נ");
				}
				else if (font_mark[6] == 1)
				{
					/* vav */
					sprintf (chars, "ו");
				}
				else if (font_mark[25] == 1)
				{
					/* resh */
					sprintf (chars, "ר");
				}
				else if (font_mark[7] == 1)
				{
					/* zayin */
					sprintf (chars, "ז");
				}
				else
				{
					sprintf (chars, "_");
					font_mark[0] = 1;
				}
			}
			/* regular fonts */
			/* TODO: sort fonts by statistical number of appearences in text */
			else
			{

				if (font_mark[1] == 1)
				{
					sprintf (chars, "א");
				}
				else if (font_mark[2] == 1)
				{
					sprintf (chars, "ב");
				}
				else if (font_mark[3] == 1)
				{
					sprintf (chars, "ג");
				}
				else if (font_mark[4] == 1)
				{
					sprintf (chars, "ד");
				}
				else if (font_mark[5] == 1)
				{
					sprintf (chars, "ה");
				}
				else if (font_mark[8] == 1)
				{
					sprintf (chars, "ח");
				}
				else if (font_mark[9] == 1)
				{
					sprintf (chars, "ט");
				}
				else if (font_mark[11] == 1)
				{
					sprintf (chars, "כ");
				}
				else if (font_mark[14] == 1)
				{
					sprintf (chars, "מ");
				}
				else if (font_mark[15] == 1)
				{
					sprintf (chars, "ם");
				}
				//else if (nun_mark == 1)
				//{
				//      sprintf (chars, "נ");
				//}
				else if (font_mark[18] == 1)
				{
					sprintf (chars, "ס");
				}
				else if (font_mark[19])
				{
					sprintf (chars, "ע");
				}
				else if (font_mark[20] == 1)
				{
					sprintf (chars, "פ");
				}
				else if (font_mark[22])
				{
					sprintf (chars, "צ");
				}
				else if (font_mark[25] == 1)
				{
					sprintf (chars, "ר");
				}
				else if (font_mark[26] == 1)
				{
					sprintf (chars, "ש");
				}
				else if (font_mark[27] == 1)
				{
					sprintf (chars, "ת");
				}
				else
				{
					sprintf (chars, "_");
					font_mark[0] = 1;
				}
			}

			if (chars[0] == '\'' && chars[1] == '\0'
			    && last_was_quot == 0)
			{
				last_was_quot = 1;
				if (fonts[i][j].width > 2
				    && fonts[i][j].hight > 2)
					hocr_text_buffer_add_string
						(text_buffer, chars);
			}
			else if (chars[0] == '\'' && chars[1] == '\0'
				 && last_was_quot == 1)
			{
				last_was_quot = 0;
				text_buffer->text[text_buffer->size - 1] =
					'\"';
			}
			else
			{
				last_was_quot = 0;
				if (fonts[i][j].width > 2
				    && fonts[i][j].hight > 2)
					hocr_text_buffer_add_string
						(text_buffer, chars);
			}

			/* color unknown fonts in the pixbuf */
			if (font_mark[0] == 1)
				color_hocr_box_full (pix, fonts[i][j], 1,
						     255);

			/* check for end of word and end of line */
			if (end_of_word == 1)
			{
				hocr_text_buffer_add_string (text_buffer,
							     " ");
			}
			if (end_of_line == 1)
			{
				hocr_text_buffer_add_string (text_buffer,
							     "\n");
			}
			if (end_of_paragraph == 1)
			{
				hocr_text_buffer_add_string (text_buffer,
							     "\n");
			}

			/* visual aids to see font hocr_box on screen */
			color_hocr_box (pix, fonts[i][j], 1, 0);
			/* print_font (pix, fonts[i][j]); */
		}

	}

	if (error)
		error = HOCR_ERROR_OK;

	return 0;
}
