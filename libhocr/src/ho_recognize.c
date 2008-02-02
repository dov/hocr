
/***************************************************************************
 *            ho_recognize.c
 *
 *  Fri Aug 12 20:13:33 2005
 *  Copyright  2005-2007  Yaacov Zamir
 *  <kzamir@walla.co.il>
 ****************************************************************************/

/*  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#ifndef TRUE
#define TRUE -1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif

#include "ho_bitmap.h"
#include "ho_objmap.h"
#include "ho_segment.h"
#include "ho_font.h"
#include "ho_recognize_font_david.h"

#include "ho_recognize.h"

int
ho_recognize_array_in_size ()
{
  return HO_ARRAY_IN_SIZE;
}

int
ho_recognize_array_out_size ()
{
  return HO_ARRAY_OUT_SIZE;
}

int
ho_recognize_dimentions (const ho_bitmap * m_text,
  const ho_bitmap * m_mask, double *height,
  double *width, double *top, double *bottom,
  double *top_left, double *top_mid, double *top_right,
  double *mid_left, double *mid_right,
  double *bottom_left, double *bottom_mid,
  double *bottom_right,
  double *has_two_hlines_up,
  double *has_two_hlines_down,
  double *has_three_hlines_up, double *has_three_hlines_down)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_clean = NULL;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  *height = (double) font_height / (double) line_height;
  *width = (double) (m_text->width) / (double) line_height;

  *top = (double) (line_start - font_start) / (double) line_height;
  *bottom = (double) (line_end - font_end) / (double) line_height;

  /* check if font is not just a little off line */
  if (*top < 0.025 && *top > -0.025)
  {
    /* regular size */
    if (*height < 1.025 && *height > 0.975)
    {
      *top = *bottom = 0.0;
    }
    /* short and high font */
    else
    {
      *bottom -= *top;
      *top = 0.0;
    }
  }
  else if (*bottom < 0.025 && *bottom > -0.025)
  {
    *top -= *bottom;
    *bottom = 0.0;
  }

  /* all values are 0..1 */
  *height = *height / 2.0;
  if (*height > 1.0)
    *height = 1.0;

  *width = *width / 2.0;
  if (*width > 1.0)
    *width = 1.0;

  *top = (*top + 1.0) / 2.0;
  if (*top > 1.0)
    *top = 1.0;
  if (*top < 0.0)
    *top = 0.0;

  *bottom = (*bottom + 1.0) / 2.0;
  if (*bottom > 1.0)
    *bottom = 1.0;
  if (*bottom < 0.0)
    *bottom = 0.0;

  /* get font egdes */
  for (y = font_start, x = 0;
    x < m_mask->width / 2 && y < (font_start + font_height / 2)
    && !ho_bitmap_get (m_text, x, y); x++, y++) ;
  *top_left = (double) x / (double) (m_mask->width / 2);

  if (*top_left > 1.0)
    *top_left = 1.0;

  for (y = font_end, x = 0;
    x < m_mask->width / 2 && y > (font_start + font_height / 2)
    && !ho_bitmap_get (m_text, x, y); x++, y--) ;
  *bottom_left = (double) x / (double) (m_mask->width / 2);

  if (*bottom_left > 1.0)
    *bottom_left = 1.0;

  for (y = font_start, x = m_text->width - 1;
    x > m_mask->width / 2 && y < (font_start + font_height / 2)
    && !ho_bitmap_get (m_text, x, y); x--, y++) ;
  *top_right = (double) (m_mask->width - x) / (double) (m_mask->width / 2);

  if (*top_right > 1.0)
    *top_right = 1.0;

  for (y = font_end, x = m_text->width - 1;
    x > m_mask->width / 2 && y > (font_start + font_height / 2)
    && !ho_bitmap_get (m_text, x, y); x--, y--) ;
  *bottom_right = (double) (m_mask->width - x) / (double) (m_mask->width / 2);

  if (*bottom_right > 1.0)
    *bottom_right = 1.0;

  for (y = font_start, x = m_text->width / 2;
    y < (font_start + font_height / 2) && !ho_bitmap_get (m_text, x, y); y++) ;
  *top_mid = (double) (y - font_start) / (double) (font_height / 2);

  if (*top_mid > 1.0)
    *top_mid = 1.0;

  for (y = font_end, x = m_text->width / 2;
    y > (font_start + font_height / 2) && !ho_bitmap_get (m_text, x, y); y--) ;
  *bottom_mid = (double) (font_end - y) / (double) (font_height / 2);

  if (*bottom_mid > 1.0)
    *bottom_mid = 1.0;

  for (y = font_start + font_height / 2, x = 0;
    x < (m_text->width / 2) && !ho_bitmap_get (m_text, x, y); x++) ;
  *mid_left = (double) (x) / (double) (m_text->width / 2);

  if (*mid_left > 1.0)
    *mid_left = 1.0;

  for (y = font_start + font_height / 2, x = m_text->width - 1;
    x > (m_text->width / 2) && !ho_bitmap_get (m_text, x, y); x--) ;
  *mid_right = (double) (m_text->width - x) / (double) (m_text->width / 2);

  if (*mid_right > 1.0)
    *mid_right = 1.0;

  m_clean = ho_bitmap_dilation_n (m_text, 6);

  *has_two_hlines_up = 0.0;
  *has_three_hlines_up = 0.0;
  sum = 0;
  y = font_start + font_height / 3;
  x = 0;
  while (x < m_text->width)
  {
    for (; x < m_text->width, !ho_bitmap_get (m_clean, x, y); x++) ;
    if (ho_bitmap_get (m_clean, x, y))
      sum++;
    for (; x < m_text->width, ho_bitmap_get (m_clean, x, y); x++) ;
  }

  if (sum == 2)
    *has_two_hlines_up = 1.0;
  else if (sum == 3)
    *has_three_hlines_up = 1.0;

  *has_two_hlines_down = 0.0;
  *has_three_hlines_down = 0.0;
  sum = 0;
  y = font_start + 2 * font_height / 3;
  x = 0;
  while (x < m_text->width)
  {
    for (; x < m_text->width, !ho_bitmap_get (m_clean, x, y); x++) ;
    if (ho_bitmap_get (m_text, x, y))
      sum++;
    for (; x < m_text->width, ho_bitmap_get (m_clean, x, y); x++) ;
  }

  if (sum == 2)
    *has_two_hlines_down = 1.0;
  else if (sum == 3)
    *has_three_hlines_down = 1.0;

  ho_bitmap_free (m_clean);

  return FALSE;
}

int
ho_recognize_bars (const ho_bitmap * m_text,
  const ho_bitmap * m_mask, double *has_top_bar,
  double *has_mid_hbar, double *has_bottom_bar, double *has_left_bar,
  double *has_mid_vbar, double *has_right_bar, double *has_diagonal_bar,
  double *has_diagonal_left_bar)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_bars = NULL;

  *has_top_bar = 0.0;
  *has_mid_hbar = 0.0;
  *has_bottom_bar = 0.0;
  *has_left_bar = 0.0;
  *has_mid_vbar = 0.0;
  *has_right_bar = 0.0;
  *has_diagonal_bar = 0.0;
  *has_diagonal_left_bar = 0.0;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  /* get horizontal bars */
  m_bars = ho_font_hbars (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_bar = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_hbar = 1.0;
      else
        *has_bottom_bar = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  /* get vertical bars */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_vbars (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_bar = 1.0;
      else if (x < 2 * m_bars->width / 3)
        *has_mid_vbar = 1.0;
      else
        *has_right_bar = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  /* get diagonals */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_diagonal (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a bar in left 1/4 of font */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width && !ho_bitmap_get (m_bars, x, y); x++) ;
  if (ho_bitmap_get (m_bars, x, y) && x < m_bars->width)
    *has_diagonal_bar = 1.0;

  /* get diagonals */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_diagonal_left (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a bar in left 1/4 of font */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width && !ho_bitmap_get (m_bars, x, y); x++) ;
  if (ho_bitmap_get (m_bars, x, y) && x < m_bars->width)
    *has_diagonal_left_bar = 1.0;

  ho_bitmap_free (m_bars);

  return FALSE;
}

int
ho_recognize_edges (const ho_bitmap * m_text,
  const ho_bitmap * m_mask,
  double *has_top_left_edge,
  double *has_mid_left_edge,
  double *has_bottom_left_edge,
  double *has_top_right_edge,
  double *has_mid_right_edge,
  double *has_bottom_right_edge,
  double *has_left_top_edge,
  double *has_mid_top_edge,
  double *has_right_top_edge,
  double *has_left_bottom_edge,
  double *has_mid_bottom_edge, double *has_right_bottom_edge)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_bars = NULL;

  *has_top_left_edge = 0.0;
  *has_mid_left_edge = 0.0;
  *has_bottom_left_edge = 0.0;
  *has_top_right_edge = 0.0;
  *has_mid_right_edge = 0.0;
  *has_bottom_right_edge = 0.0;
  *has_left_top_edge = 0.0;
  *has_mid_top_edge = 0.0;
  *has_right_top_edge = 0.0;
  *has_left_bottom_edge = 0.0;
  *has_mid_bottom_edge = 0.0;
  *has_right_bottom_edge = 0.0;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  /* get horizontal left egdes */
  m_bars = ho_font_edges_left (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_left_edge = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_left_edge = 1.0;
      else
        *has_bottom_left_edge = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_right (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_right_edge = 1.0;
      else if ((y - font_start) < (2 * font_height / 3))
        *has_mid_right_edge = 1.0;
      else
        *has_bottom_right_edge = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_top (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_top_edge = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_top_edge = 1.0;
      else
        *has_right_top_edge = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_bottom (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_bottom_edge = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_bottom_edge = 1.0;
      else
        *has_right_bottom_edge = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  ho_bitmap_free (m_bars);

  return FALSE;
}

int
ho_recognize_edges_big (const ho_bitmap * m_text,
  const ho_bitmap * m_mask,
  double *has_top_left_edge,
  double *has_mid_left_edge,
  double *has_bottom_left_edge,
  double *has_top_right_edge,
  double *has_mid_right_edge,
  double *has_bottom_right_edge,
  double *has_left_top_edge,
  double *has_mid_top_edge,
  double *has_right_top_edge,
  double *has_left_bottom_edge,
  double *has_mid_bottom_edge, double *has_right_bottom_edge)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_bars = NULL;

  *has_top_left_edge = 0.0;
  *has_mid_left_edge = 0.0;
  *has_bottom_left_edge = 0.0;
  *has_top_right_edge = 0.0;
  *has_mid_right_edge = 0.0;
  *has_bottom_right_edge = 0.0;
  *has_left_top_edge = 0.0;
  *has_mid_top_edge = 0.0;
  *has_right_top_edge = 0.0;
  *has_left_bottom_edge = 0.0;
  *has_mid_bottom_edge = 0.0;
  *has_right_bottom_edge = 0.0;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  /* get horizontal left egdes */
  m_bars = ho_font_edges_left_big (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_left_edge = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_left_edge = 1.0;
      else
        *has_bottom_left_edge = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_right_big (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_right_edge = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_right_edge = 1.0;
      else
        *has_bottom_right_edge = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_top_big (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_top_edge = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_top_edge = 1.0;
      else
        *has_right_top_edge = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_bottom_big (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_bottom_edge = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_bottom_edge = 1.0;
      else
        *has_right_bottom_edge = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  ho_bitmap_free (m_bars);

  return FALSE;
}

int
ho_recognize_notches (const ho_bitmap * m_text,
  const ho_bitmap * m_mask,
  double *has_top_left_notch,
  double *has_mid_left_notch,
  double *has_bottom_left_notch,
  double *has_top_right_notch,
  double *has_mid_right_notch,
  double *has_bottom_right_notch,
  double *has_left_top_notch,
  double *has_mid_top_notch,
  double *has_right_top_notch,
  double *has_left_bottom_notch,
  double *has_mid_bottom_notch, double *has_right_bottom_notch)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_bars = NULL;

  *has_top_left_notch = 0.0;
  *has_mid_left_notch = 0.0;
  *has_bottom_left_notch = 0.0;
  *has_top_right_notch = 0.0;
  *has_mid_right_notch = 0.0;
  *has_bottom_right_notch = 0.0;
  *has_left_top_notch = 0.0;
  *has_mid_top_notch = 0.0;
  *has_right_top_notch = 0.0;
  *has_left_bottom_notch = 0.0;
  *has_mid_bottom_notch = 0.0;
  *has_right_bottom_notch = 0.0;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  /* get horizontal left notches */
  m_bars = ho_font_notch_left (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_left_notch = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_left_notch = 1.0;
      else
        *has_bottom_left_notch = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_notch_right (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_right_notch = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_right_notch = 1.0;
      else
        *has_bottom_right_notch = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  ho_bitmap_free (m_bars);
  m_bars = ho_font_notch_top (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_top_notch = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_top_notch = 1.0;
      else
        *has_right_top_notch = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_notch_bottom (m_text, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_bottom_notch = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_bottom_notch = 1.0;
      else
        *has_right_bottom_notch = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  ho_bitmap_free (m_bars);

  return FALSE;
}

int
ho_recognize_parts (const ho_bitmap * m_text,
  const ho_bitmap * m_mask, double *has_one_hole,
  double *has_two_holes, double *has_hey_part,
  double *has_dot_part, double *has_comma_part)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_parts = NULL;

  *has_one_hole = 0.0;
  *has_two_holes = 0.0;
  *has_hey_part = 0.0;
  *has_dot_part = 0.0;
  *has_comma_part = 0.0;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  /* holes */
  m_parts = ho_font_holes (m_text, m_mask);
  if (m_parts)
  {
    sum = ho_bitmap_filter_count_objects (m_parts);
    if (sum == 1)
      *has_one_hole = 1.0;
    else if (sum == 2)
      *has_two_holes = 1.0;
  }

  /* hey and kuf part */

  if (m_parts)
    ho_bitmap_free (m_parts);
  m_parts = ho_font_second_object (m_text, m_mask);
  if (m_parts)
  {
    /* hey part */
    y = font_end - font_height / 5;

    for (x = 0; x < m_parts->width / 2 && !ho_bitmap_get (m_parts, x, y); x++) ;
    /* part is in left side of font */
    if (x < m_parts->width / 2)
    {
      for (; x < m_parts->width / 2 && ho_bitmap_get (m_parts, x, y); x++) ;
      /* part ends in left side of font */
      if (x < m_parts->width / 2)
        *has_hey_part = 1.0;
    }

    /* dot part */
    for (y = font_start; y < font_end && !ho_bitmap_get (m_parts, x, y); y++)
      for (x = 0; x < m_parts->width && !ho_bitmap_get (m_parts, x, y); x++) ;
    /* part is in middle bottom of font */
    if (y > font_start + 2 * font_height / 3 && x > m_parts->width / 4)
      *has_dot_part = 1.0;

    /* comma part */
    for (y = font_start; y < font_end && !ho_bitmap_get (m_parts, x, y); y++)
      for (x = 0; x < m_parts->width && !ho_bitmap_get (m_parts, x, y); x++) ;
    /* part is in middle bottom of font */
    if (y > font_start + font_height / 2 && x > m_parts->width / 2)
      *has_comma_part = 1.0;
  }

  if (m_parts)
    ho_bitmap_free (m_parts);

  return FALSE;
}

int
ho_recognize_ends (const ho_bitmap * m_text,
  const ho_bitmap * m_mask,
  double *has_top_left_end,
  double *has_top_mid_end,
  double *has_top_right_end,
  double *has_mid_left_end,
  double *has_mid_mid_end,
  double *has_mid_right_end,
  double *has_bottom_left_end,
  double *has_bottom_mid_end,
  double *has_bottom_right_end,
  double *has_top_left_cross,
  double *has_top_mid_cross,
  double *has_top_right_cross,
  double *has_mid_left_cross,
  double *has_mid_mid_cross,
  double *has_mid_right_cross,
  double *has_bottom_left_cross,
  double *has_bottom_mid_cross, double *has_bottom_right_cross)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_parts = NULL;

  /* init values */
  *has_top_left_end = 0.0;
  *has_top_mid_end = 0.0;
  *has_top_right_end = 0.0;

  *has_mid_left_end = 0.0;
  *has_mid_mid_end = 0.0;
  *has_mid_right_end = 0.0;

  *has_bottom_left_end = 0.0;
  *has_bottom_mid_end = 0.0;
  *has_bottom_right_end = 0.0;

  *has_top_left_cross = 0.0;
  *has_top_mid_cross = 0.0;
  *has_top_right_cross = 0.0;

  *has_mid_left_cross = 0.0;
  *has_mid_mid_cross = 0.0;
  *has_mid_right_cross = 0.0;

  *has_bottom_left_cross = 0.0;
  *has_bottom_mid_cross = 0.0;
  *has_bottom_right_cross = 0.0;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_text, x, y);
  font_end = y + 1;
  if (font_start < 1)
    font_start = 1;

  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  /* ends */
  m_parts = ho_font_ends (m_text, m_mask);
  if (!m_parts)
    return TRUE;

  /* top left */
  sum = 0;
  for (x = 0; x < m_parts->width / 3; x++)
    for (y = font_start - 1;
      y < font_start + font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_top_left_end = 1.0;

  /* top mid */
  sum = 0;
  for (x = m_parts->width / 3; x < 2 * m_parts->width / 3; x++)
    for (y = font_start - 1;
      y < font_start + font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_top_mid_end = 1.0;

  /* top right */
  sum = 0;
  for (x = 2 * m_parts->width / 3; x < m_parts->width; x++)
    for (y = font_start - 1;
      y < font_start + font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_top_right_end = 1.0;

  /* mid left */
  sum = 0;
  for (x = 0; x < m_parts->width / 3; x++)
    for (y = font_start + font_height / 3;
      y < font_start + 2 * font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_mid_left_end = 1.0;

  /* mid mid */
  sum = 0;
  for (x = m_parts->width / 3; x < 2 * m_parts->width / 3; x++)
    for (y = font_start + font_height / 3;
      y < font_start + 2 * font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_mid_mid_end = 1.0;

  /* mid right */
  sum = 0;
  for (x = 2 * m_parts->width / 3; x < m_parts->width; x++)
    for (y = font_start + font_height / 3;
      y < font_start + 2 * font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_mid_right_end = 1.0;

  /* bottom left */
  sum = 0;
  for (x = 0; x < m_parts->width / 3; x++)
    for (y = font_start + 2 * font_height / 3;
      y < font_start + font_height; y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_bottom_left_end = 1.0;

  /* bottom mid */
  sum = 0;
  for (x = m_parts->width / 3; x < 2 * m_parts->width / 3; x++)
    for (y = font_start + 2 * font_height / 3;
      y < font_start + font_height; y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_bottom_mid_end = 1.0;

  /* bottom right */
  sum = 0;
  for (x = 2 * m_parts->width / 3; x < m_parts->width; x++)
    for (y = font_start + 2 * font_height / 3;
      y < font_start + font_height; y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_bottom_right_end = 1.0;

  /* crosses */
  ho_bitmap_free (m_parts);
  m_parts = ho_font_cross (m_text, m_mask);
  if (!m_parts)
    return TRUE;

  /* top left */
  sum = 0;
  for (x = 0; x < m_parts->width / 3; x++)
    for (y = font_start - 1;
      y < font_start + font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_top_left_cross = 1.0;

  /* top mid */
  sum = 0;
  for (x = m_parts->width / 3; x < 2 * m_parts->width / 3; x++)
    for (y = font_start - 1;
      y < font_start + font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_top_mid_cross = 1.0;

  /* top right */
  sum = 0;
  for (x = 2 * m_parts->width / 3; x < m_parts->width; x++)
    for (y = font_start;
      y < font_start + font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_top_right_cross = 1.0;

  /* mid left */
  sum = 0;
  for (x = 0; x < m_parts->width / 3; x++)
    for (y = font_start + font_height / 3;
      y < font_start + 2 * font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_mid_left_cross = 1.0;

  /* mid mid */
  sum = 0;
  for (x = m_parts->width / 3; x < 2 * m_parts->width / 3; x++)
    for (y = font_start + font_height / 3;
      y < font_start + 2 * font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_mid_mid_cross = 1.0;

  /* mid right */
  sum = 0;
  for (x = 2 * m_parts->width / 3; x < m_parts->width; x++)
    for (y = font_start + font_height / 3;
      y < font_start + 2 * font_height / 3;
      y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_mid_right_cross = 1.0;

  /* bottom left */
  sum = 0;
  for (x = 0; x < m_parts->width / 3; x++)
    for (y = font_start + 2 * font_height / 3;
      y < font_start + font_height; y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_bottom_left_cross = 1.0;

  /* bottom mid */
  sum = 0;
  for (x = m_parts->width / 3; x < 2 * m_parts->width / 3; x++)
    for (y = font_start + 2 * font_height / 3;
      y < font_start + font_height; y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_bottom_mid_cross = 1.0;

  /* bottom right */
  sum = 0;
  for (x = 2 * m_parts->width / 3; x < m_parts->width; x++)
    for (y = font_start + 2 * font_height / 3;
      y < font_start + font_height; y++, sum += ho_bitmap_get (m_parts, x, y)) ;
  if (sum)
    *has_bottom_right_cross = 1.0;

  ho_bitmap_free (m_parts);

  return FALSE;
}

int
ho_recognize_holes_dimentions (const ho_bitmap * m_text,
  const ho_bitmap * m_mask, double *height,
  double *width, double *top, double *bottom,
  double *top_left, double *top_right,
  double *bottom_left, double *bottom_right)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int font_start_x;
  int font_end_x;
  int font_width;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_holes = NULL;

  *height = 0.0;
  *width = 0.0;
  *top = 0.0;
  *bottom = 0.0;
  *top_left = 0.0;
  *top_right = 0.0;
  *bottom_left = 0.0;
  *bottom_right = 0.0;

  m_holes = ho_font_holes (m_text, m_mask);
  if (!m_holes)
    return TRUE;

  sum = ho_bitmap_filter_count_objects (m_holes);
  if (sum > 2 || sum < 1)
    return TRUE;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  sum = 0;
  for (x = 0; x < m_mask->width && sum == 0; x++)
    for (sum = 0, y = font_start; y < font_end; y++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_start_x = x - 1;
  sum = 0;
  for (x = m_mask->width - 1; x > font_start_x && sum == 0; x--)
    for (sum = 0, y = font_start; y < font_end; y++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_end_x = x + 1;
  font_width = font_end_x - font_start_x;

  if (!font_width)
    return TRUE;

  *height = (double) font_height / (double) line_height;
  *width = (double) font_width / (double) line_height;

  *top = (double) (line_start - font_start) / (double) line_height;
  *bottom = (double) (line_end - font_end) / (double) line_height;

  /* check if font is not just a little off line */
  if (*top < 0.1 && *top > -0.1)
  {
    /* regular size */
    if (*height < 1.1 && *height > 0.9)
    {
      *top = *bottom = 0.0;
    }
    /* short and high font */
    else
    {
      *bottom -= *top;
      *top = 0.0;
    }
  }
  else if (*bottom < 0.1 && *bottom > -0.1)
  {
    *top -= *bottom;
    *bottom = 0.0;
  }

  /* all values are 0..1 */
  *height = *height / 2.0;
  if (*height > 1.0)
    *height = 1.0;

  *width = *width / 2.0;
  if (*width > 1.0)
    *width = 1.0;

  *top = (*top + 1.0) / 2.0;
  if (*top > 1.0)
    *top = 1.0;
  if (*top < 0.0)
    *top = 0.0;

  *bottom = (*bottom + 1.0) / 2.0;
  if (*bottom > 1.0)
    *bottom = 1.0;
  if (*bottom < 0.0)
    *bottom = 0.0;

  /* get font egdes */
  for (y = font_start, x = font_start_x;
    x < (font_start_x + font_width / 2)
    && y < (font_start + font_height / 2)
    && !ho_bitmap_get (m_holes, x, y); x++, y++) ;
  *top_left = (double) (x - font_start_x) / (double) (font_width / 2);

  if (*top_left > 1.0)
    *top_left = 1.0;

  for (y = font_end, x = font_start_x;
    x < (font_start_x + font_width / 2)
    && y > (font_start + font_height / 2)
    && !ho_bitmap_get (m_holes, x, y); x++, y--) ;
  *bottom_left = (double) (x - font_start_x) / (double) (font_width / 2);

  if (*bottom_left > 1.0)
    *bottom_left = 1.0;

  for (y = font_start, x = font_end_x;
    x > (font_start_x + font_width / 2)
    && y < (font_start + font_height / 2)
    && !ho_bitmap_get (m_holes, x, y); x--, y++) ;
  *top_right = (double) (font_end_x - x) / (double) (font_width / 2);

  if (*top_right > 1.0)
    *top_right = 1.0;

  for (y = font_end, x = font_end_x;
    x > (font_start_x + font_width / 2)
    && y > (font_start + font_height / 2)
    && !ho_bitmap_get (m_holes, x, y); x--, y--) ;
  *bottom_right = (double) (font_end_x - x) / (double) (font_width / 2);

  if (*bottom_right > 1.0)
    *bottom_right = 1.0;

  return FALSE;
}

int
ho_recognize_holes_edges (const ho_bitmap * m_text,
  const ho_bitmap * m_mask,
  double *has_top_left_edge,
  double *has_mid_left_edge,
  double *has_bottom_left_edge,
  double *has_top_right_edge,
  double *has_mid_right_edge,
  double *has_bottom_right_edge,
  double *has_left_top_edge,
  double *has_mid_top_edge,
  double *has_right_top_edge,
  double *has_left_bottom_edge,
  double *has_mid_bottom_edge, double *has_right_bottom_edge)
{
  int sum, x, y;
  int font_start;
  int font_end;
  int font_height;
  int font_start_x;
  int font_end_x;
  int font_width;
  int line_start;
  int line_end;
  int line_height;

  ho_bitmap *m_bars = NULL;
  ho_bitmap *m_holes = NULL;

  *has_top_left_edge = 0.0;
  *has_mid_left_edge = 0.0;
  *has_bottom_left_edge = 0.0;
  *has_top_right_edge = 0.0;
  *has_mid_right_edge = 0.0;
  *has_bottom_right_edge = 0.0;
  *has_left_top_edge = 0.0;
  *has_mid_top_edge = 0.0;
  *has_right_top_edge = 0.0;
  *has_left_bottom_edge = 0.0;
  *has_mid_bottom_edge = 0.0;
  *has_right_bottom_edge = 0.0;

  m_holes = ho_font_holes (m_text, m_mask);
  if (!m_holes)
    return TRUE;

  sum = ho_bitmap_filter_count_objects (m_holes);
  if (sum > 2 || sum < 1)
    return TRUE;

  /* get line start and end */
  x = m_mask->width / 2;
  for (y = 0; y < m_mask->height && !ho_bitmap_get (m_mask, x, y); y++) ;
  line_start = y - 1;
  for (; y < m_mask->height && ho_bitmap_get (m_mask, x, y); y++) ;
  line_end = y;
  line_height = line_end - line_start;

  /* get font start and end */
  sum = 0;
  for (y = 0; y < m_mask->height && sum == 0; y++)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_start = y - 1;
  sum = 0;
  for (y = m_mask->height - 1; y > font_start && sum == 0; y--)
    for (sum = 0, x = 0; x < m_text->width; x++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_end = y + 1;
  font_height = font_end - font_start;

  if (!font_height || !line_height)
    return TRUE;

  sum = 0;
  for (x = 0; x < m_mask->width && sum == 0; x++)
    for (sum = 0, y = font_start; y < font_end; y++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_start_x = x - 1;
  sum = 0;
  for (x = m_mask->width - 1; x > font_start_x && sum == 0; x--)
    for (sum = 0, y = font_start; y < font_end; y++)
      sum += ho_bitmap_get (m_holes, x, y);
  font_end_x = x + 1;
  font_width = font_end_x - font_start_x;

  if (!font_width)
    return TRUE;

  /* get horizontal left egdes */
  m_bars = ho_font_edges_left (m_holes, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_left_edge = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_left_edge = 1.0;
      else
        *has_bottom_left_edge = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_right (m_holes, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a horizontal line in bitmap */
  x = m_bars->width / 2;
  for (y = font_start; y < font_end; y++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if ((y - font_start) < font_height / 3)
        *has_top_right_edge = 1.0;
      else if ((y - font_start) < 2 * font_height / 3)
        *has_mid_right_edge = 1.0;
      else
        *has_bottom_right_edge = 1.0;

      /* get out of the line */
      y++;
      for (; y < font_end && ho_bitmap_get (m_bars, x, y); y++) ;
    }
  }

  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_top (m_holes, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_top_edge = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_top_edge = 1.0;
      else
        *has_right_top_edge = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  /* get horizontal right edges */
  ho_bitmap_free (m_bars);
  m_bars = ho_font_edges_bottom (m_holes, m_mask);
  if (!m_bars)
    return TRUE;

  /* look for a vertical line in bitmap */
  y = m_bars->height / 2;
  for (x = 0; x < m_bars->width; x++)
  {
    /* find a line */
    if (ho_bitmap_get (m_bars, x, y))
    {
      /* where are we in fonts */
      if (x < m_bars->width / 3)
        *has_left_bottom_edge = 1.0;
      else if (x < (2 * m_bars->width / 3))
        *has_mid_bottom_edge = 1.0;
      else
        *has_right_bottom_edge = 1.0;

      /* get out of the line */
      x++;
      for (; x < m_bars->width && ho_bitmap_get (m_bars, x, y); x++) ;
    }
  }

  ho_bitmap_free (m_bars);

  return FALSE;
}

int
ho_recognize_create_array_in (const ho_bitmap * m_text,
  const ho_bitmap * m_mask, double *array_in)
{
  int i;

  double height;
  double width;
  double top;
  double bottom;
  double top_left;
  double top_mid;
  double top_right;
  double bottom_left;
  double bottom_mid;
  double bottom_right;
  double mid_left;
  double mid_right;
  double has_two_hlines_up;
  double has_two_hlines_down;
  double has_three_hlines_up;
  double has_three_hlines_down;

  double has_top_bar;
  double has_mid_hbar;
  double has_bottom_bar;
  double has_left_bar;
  double has_mid_vbar;
  double has_right_bar;
  double has_diagonal_bar;
  double has_diagonal_left_bar;

  double has_top_left_edge;
  double has_mid_left_edge;
  double has_bottom_left_edge;
  double has_top_right_edge;
  double has_mid_right_edge;
  double has_bottom_right_edge;
  double has_left_top_edge;
  double has_mid_top_edge;
  double has_right_top_edge;
  double has_left_bottom_edge;
  double has_mid_bottom_edge;
  double has_right_bottom_edge;

  double has_top_left_notch;
  double has_mid_left_notch;
  double has_bottom_left_notch;
  double has_top_right_notch;
  double has_mid_right_notch;
  double has_bottom_right_notch;
  double has_left_top_notch;
  double has_mid_top_notch;
  double has_right_top_notch;
  double has_left_bottom_notch;
  double has_mid_bottom_notch;
  double has_right_bottom_notch;

  double has_one_hole;
  double has_two_holes;
  double has_hey_part;
  double has_dot_part;
  double has_comma_part;

  double has_top_left_end;
  double has_top_mid_end;
  double has_top_right_end;
  double has_mid_left_end;
  double has_mid_mid_end;
  double has_mid_right_end;
  double has_bottom_left_end;
  double has_bottom_mid_end;
  double has_bottom_right_end;
  double has_top_left_cross;
  double has_top_mid_cross;
  double has_top_right_cross;
  double has_mid_left_cross;
  double has_mid_mid_cross;
  double has_mid_right_cross;
  double has_bottom_left_cross;
  double has_bottom_mid_cross;
  double has_bottom_right_cross;

  /* init values to zero, if some function fails */
  for (i = 0; i < HO_ARRAY_IN_SIZE; i++)
    array_in[i] = 0.0;

  /* fill array with values */
  
  ho_recognize_dimentions (m_text, m_mask,
    &height, &width, &top, &bottom,
    &top_left, &top_mid, &top_right,
    &mid_left, &mid_right,
    &bottom_left, &bottom_mid, &bottom_right,
    &has_two_hlines_up, &has_two_hlines_down,
    &has_three_hlines_up, &has_three_hlines_down);

  array_in[0] = height;
  array_in[1] = width;
  array_in[2] = width / height;
  if (array_in[2] > 1.0)
    array_in[2] = 1.0;
  array_in[3] = top;
  array_in[4] = bottom;
  array_in[5] = top_left;
  array_in[6] = top_mid;
  array_in[7] = top_right;
  array_in[8] = mid_left;
  array_in[9] = mid_right;
  array_in[10] = bottom_left;
  array_in[11] = bottom_mid;
  array_in[12] = bottom_right;

  array_in[13] = has_two_hlines_up;
  array_in[14] = has_two_hlines_down;
  array_in[15] = has_three_hlines_up;
  array_in[16] = has_three_hlines_down;

  ho_recognize_bars (m_text, m_mask,
    &has_top_bar, &has_mid_hbar,
    &has_bottom_bar,
    &has_left_bar, &has_mid_vbar, &has_right_bar, &has_diagonal_bar,
    &has_diagonal_left_bar);

  array_in[17] = has_top_bar;
  array_in[18] = has_mid_hbar;
  array_in[19] = has_bottom_bar;

  array_in[20] = has_left_bar;
  array_in[21] = has_mid_vbar;
  array_in[22] = has_right_bar;

  array_in[23] = has_diagonal_bar;
  array_in[24] = has_diagonal_left_bar;

  ho_recognize_edges (m_text, m_mask,
    &has_top_left_edge,
    &has_mid_left_edge,
    &has_bottom_left_edge,
    &has_top_right_edge,
    &has_mid_right_edge,
    &has_bottom_right_edge,
    &has_left_top_edge,
    &has_mid_top_edge,
    &has_right_top_edge,
    &has_left_bottom_edge, &has_mid_bottom_edge, &has_right_bottom_edge);

  array_in[25] = has_top_left_edge;
  array_in[26] = has_mid_left_edge;
  array_in[27] = has_bottom_left_edge;

  array_in[28] = has_top_right_edge;
  array_in[29] = has_mid_right_edge;
  array_in[30] = has_bottom_right_edge;

  array_in[31] = has_left_top_edge;
  array_in[32] = has_mid_top_edge;
  array_in[33] = has_right_top_edge;

  array_in[34] = has_left_bottom_edge;
  array_in[35] = has_mid_bottom_edge;
  array_in[36] = has_right_bottom_edge;

  ho_recognize_notches (m_text,
    m_mask,
    &has_top_left_notch,
    &has_mid_left_notch,
    &has_bottom_left_notch,
    &has_top_right_notch,
    &has_mid_right_notch,
    &has_bottom_right_notch,
    &has_left_top_notch,
    &has_mid_top_notch,
    &has_right_top_notch,
    &has_left_bottom_notch, &has_mid_bottom_notch, &has_right_bottom_notch);

  array_in[37] = has_top_left_notch;
  array_in[38] = has_mid_left_notch;
  array_in[39] = has_bottom_left_notch;

  array_in[40] = has_top_right_notch;
  array_in[41] = has_mid_right_notch;
  array_in[42] = has_bottom_right_notch;

  array_in[43] = has_left_top_notch;
  array_in[44] = has_mid_top_notch;
  array_in[45] = has_right_top_notch;

  array_in[46] = has_left_bottom_notch;
  array_in[47] = has_mid_bottom_notch;
  array_in[48] = has_right_bottom_notch;

  ho_recognize_ends (m_text,
    m_mask,
    &has_top_left_end,
    &has_top_mid_end,
    &has_top_right_end,
    &has_mid_left_end,
    &has_mid_mid_end,
    &has_mid_right_end,
    &has_bottom_left_end,
    &has_bottom_mid_end,
    &has_bottom_right_end,
    &has_top_left_cross,
    &has_top_mid_cross,
    &has_top_right_cross,
    &has_mid_left_cross,
    &has_mid_mid_cross,
    &has_mid_right_cross,
    &has_bottom_left_cross, &has_bottom_mid_cross, &has_bottom_right_cross);

  array_in[49] = has_top_left_end;
  array_in[50] = has_top_mid_end;
  array_in[51] = has_top_right_end;

  array_in[52] = has_mid_left_end;
  array_in[53] = has_mid_mid_end;
  array_in[54] = has_mid_right_end;

  array_in[55] = has_bottom_left_end;
  array_in[56] = has_bottom_mid_end;
  array_in[57] = has_bottom_right_end;

  array_in[58] = has_top_left_cross;
  array_in[59] = has_top_mid_cross;
  array_in[60] = has_top_right_cross;

  array_in[61] = has_mid_left_cross;
  array_in[62] = has_mid_mid_cross;
  array_in[63] = has_mid_right_cross;

  array_in[64] = has_bottom_left_cross;
  array_in[65] = has_bottom_mid_cross;
  array_in[66] = has_bottom_right_cross;

  ho_recognize_parts (m_text,
    m_mask, &has_one_hole, &has_two_holes, &has_hey_part,
    &has_dot_part, &has_comma_part);

  array_in[67] = has_one_hole;
  array_in[68] = has_two_holes;
  array_in[69] = has_hey_part;
  array_in[70] = has_dot_part;
  array_in[71] = has_comma_part;

  ho_recognize_holes_edges (m_text, m_mask,
    &has_top_left_edge,
    &has_mid_left_edge,
    &has_bottom_left_edge,
    &has_top_right_edge,
    &has_mid_right_edge,
    &has_bottom_right_edge,
    &has_left_top_edge,
    &has_mid_top_edge,
    &has_right_top_edge,
    &has_left_bottom_edge, &has_mid_bottom_edge, &has_right_bottom_edge);

  array_in[72] = has_top_left_edge;
  array_in[73] = has_mid_left_edge;
  array_in[74] = has_bottom_left_edge;

  array_in[75] = has_top_right_edge;
  array_in[76] = has_mid_right_edge;
  array_in[77] = has_bottom_right_edge;

  array_in[78] = has_left_top_edge;
  array_in[79] = has_mid_top_edge;
  array_in[80] = has_right_top_edge;

  array_in[81] = has_left_bottom_edge;
  array_in[82] = has_mid_bottom_edge;
  array_in[83] = has_right_bottom_edge;

  ho_recognize_holes_dimentions (m_text, m_mask,
    &height, &width, &top, &bottom,
    &top_left, &top_right, &bottom_left, &bottom_right);

  array_in[84] = height;
  array_in[85] = width;
  array_in[86] = top;
  array_in[87] = bottom;
  array_in[88] = top_left;
  array_in[89] = top_right;
  array_in[90] = bottom_left;
  array_in[91] = bottom_right;

  ho_recognize_edges_big (m_text, m_mask,
    &has_top_left_edge,
    &has_mid_left_edge,
    &has_bottom_left_edge,
    &has_top_right_edge,
    &has_mid_right_edge,
    &has_bottom_right_edge,
    &has_left_top_edge,
    &has_mid_top_edge,
    &has_right_top_edge,
    &has_left_bottom_edge, &has_mid_bottom_edge, &has_right_bottom_edge);

  array_in[92] = has_top_left_edge;
  array_in[93] = has_mid_left_edge;
  array_in[94] = has_bottom_left_edge;

  array_in[95] = has_top_right_edge;
  array_in[96] = has_mid_right_edge;
  array_in[97] = has_bottom_right_edge;

  array_in[98] = has_left_top_edge;
  array_in[99] = has_mid_top_edge;
  array_in[100] = has_right_top_edge;

  array_in[101] = has_left_bottom_edge;
  array_in[102] = has_mid_bottom_edge;
  array_in[103] = has_right_bottom_edge;

  return 0;
}

double
ho_recognize_array (const double *array_in, const int sign_index)
{
  double return_value;

  switch (sign_index)
  {
  case 1:                      /* alef */
    return_value = ho_recognize_font_david_alef (array_in);
    break;
  case 2:                      /* bet */
    return_value = ho_recognize_font_david_bet (array_in);
    break;
  case 3:                      /* gimal */
    return_value = ho_recognize_font_david_gimal (array_in);
    break;
  case 4:                      /* dalet */
    return_value = ho_recognize_font_david_dalet (array_in);
    break;
  case 5:                      /* hey */
    return_value = ho_recognize_font_david_hey (array_in);
    break;
  case 6:                      /* vav */
    return_value = ho_recognize_font_david_vav (array_in);
    break;
  case 7:                      /* zayin */
    return_value = ho_recognize_font_david_zayin (array_in);
    break;
  case 8:                      /* het */
    return_value = ho_recognize_font_david_het (array_in);
    break;
  case 9:                      /* tet */
    return_value = ho_recognize_font_david_tet (array_in);
    break;
  case 10:                     /* yud */
    return_value = ho_recognize_font_david_yud (array_in);
    break;
  case 11:                     /* caf */
    return_value = ho_recognize_font_david_caf (array_in);
    break;
  case 12:                     /* caf sofit */
    return_value = ho_recognize_font_david_caf_sofit (array_in);
    break;
  case 13:                     /* lamed */
    return_value = ho_recognize_font_david_lamed (array_in);
    break;
  case 14:                     /* mem */
    return_value = ho_recognize_font_david_mem (array_in);
    break;
  case 15:                     /* mem sofit */
    return_value = ho_recognize_font_david_mem_sofit (array_in);
    break;
  case 16:                     /* nun */
    return_value = ho_recognize_font_david_nun (array_in);
    break;
  case 17:                     /* nun sofit */
    return_value = ho_recognize_font_david_nun_sofit (array_in);
    break;
  case 18:                     /* samech */
    return_value = ho_recognize_font_david_samech (array_in);
    break;
  case 19:                     /* ayin */
    return_value = ho_recognize_font_david_ayin (array_in);
    break;
  case 20:                     /* pey */
    return_value = ho_recognize_font_david_pey (array_in);
    break;
  case 21:                     /* pey sofit */
    return_value = ho_recognize_font_david_pey_sofit (array_in);
    break;
  case 22:                     /* tzadi */
    return_value = ho_recognize_font_david_tzadi (array_in);
    break;
  case 23:                     /* tzadi sofit */
    return_value = ho_recognize_font_david_tzadi_sofit (array_in);
    break;
  case 24:                     /* kuf */
    return_value = ho_recognize_font_david_kuf (array_in);
    break;
  case 25:                     /* resh */
    return_value = ho_recognize_font_david_resh (array_in);
    break;
  case 26:                     /* shin */
    return_value = ho_recognize_font_david_shin (array_in);
    break;
  case 27:                     /* tav */
    return_value = ho_recognize_font_david_tav (array_in);
    break;
  case 28:                     /* dot */
    return_value = ho_recognize_font_david_dot (array_in);
    break;
  case 29:                     /* comma */
    return_value = ho_recognize_font_david_comma (array_in);
    break;
  case 30:                     /* tag */
    return_value = ho_recognize_font_david_tag (array_in);
    break;
  case 31:                     /* ? */
    return_value = ho_recognize_font_david_question (array_in);
    break;
  case 32:                     /* ! */
    return_value = ho_recognize_font_david_exclem (array_in);
    break;
  case 33:                     /* : */
    return_value = ho_recognize_font_david_dot_dot (array_in);
    break;
  case 34:                     /* ; */
    return_value = ho_recognize_font_david_dot_comma (array_in);
    break;
  case 37:                     /* minus */
    return_value = ho_recognize_font_david_minus (array_in);
  case 38:                     /* 0 */
  case 39:                     /* 1 */
  case 40:                     /* 2 */
  case 41:                     /* 3 */
  case 42:                     /* 4 */
  case 43:                     /* 5 */
  case 44:                     /* 6 */
  case 45:                     /* 7 */
  case 46:                     /* 8 */
  case 47:                     /* 9 */
  default:
    return_value = 0.0;
    break;
  }

  return return_value;
}

int
ho_recognize_create_array_out (const double *array_in, double *array_out)
{
  int i;

  /* set array out */
  array_out[0] = 0.2;
  for (i = 1; i < HO_ARRAY_OUT_SIZE; i++)
    array_out[i] = ho_recognize_array (array_in, i);
  return FALSE;
}

const char *
ho_recognize_array_out_to_font (const double *array_out)
{
  int i = 0;
  int max_i = 0;

  /* find the font with hiegher score */
  for (i = 1; i < HO_ARRAY_OUT_SIZE; i++)
    if (array_out[i] > array_out[max_i])
      max_i = i;
  return ho_sign_array[max_i];
}

const char *
ho_recognize_font (const ho_bitmap * m_text, const ho_bitmap * m_mask)
{
  double array_in[HO_ARRAY_IN_SIZE];
  double array_out[HO_ARRAY_OUT_SIZE];
  const char *font;

  ho_recognize_create_array_in (m_text, m_mask, array_in);
  ho_recognize_create_array_out (array_in, array_out);
  font = ho_recognize_array_out_to_font (array_out);
  
  return font;
}
