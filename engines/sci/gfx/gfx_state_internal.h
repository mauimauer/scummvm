/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#ifndef SCI_GFX_GFX_STATE_INTERNAL_H
#define SCI_GFX_GFX_STATE_INTERNAL_H

#include "sci/gfx/gfx_tools.h"
#include "sci/gfx/gfx_options.h"
#include "sci/gfx/operations.h"
#include "sci/gfx/gfx_resmgr.h"
#include "sci/gfx/gfx_system.h"

namespace Sci {

#define GFXW_FLAG_VISIBLE (1<<0)
#define GFXW_FLAG_OPAQUE (1<<1)
#define GFXW_FLAG_CONTAINER (1<<2)
#define GFXW_FLAG_DIRTY (1<<3)
#define GFXW_FLAG_TAGGED (1<<4)
#define GFXW_FLAG_MULTI_ID (1<<5) /* Means that the ID used herein may be used more than once, i.e. is not unique */
#define GFXW_FLAG_IMMUNE_TO_SNAPSHOTS (1<<6) /* Snapshot restoring doesn't kill this widget, and +5 bonus to saving throws vs. Death Magic */
#define GFXW_FLAG_NO_IMPLICIT_SWITCH (1<<7) /* Ports: Don't implicitly switch to this port when disposing windows */

struct gfxw_snapshot_t {
	int serial; /* The first serial number to kill */
	rect_t area;
};

enum gfxw_widget_type_t {
	GFXW_, /* Base widget */

	GFXW_BOX,
	GFXW_RECT,
	GFXW_LINE, /* For lines, the bounding rectangle's xl, yl determine the line's expansion:
		   ** (x2, y2) = (x+xl, y+yl) */
	GFXW_VIEW,
	GFXW_STATIC_VIEW,
	GFXW_DYN_VIEW,
	GFXW_PIC_VIEW,
	GFXW_TEXT,

	GFXW_CONTAINER,

	GFXW_LIST,
	GFXW_SORTED_LIST,
	GFXW_VISUAL,
	GFXW_PORT
};


#define GFXW_MAGIC_VALID 0xC001
#define GFXW_MAGIC_INVALID 0xbad

#define GFXW_NO_ID -1

struct gfxw_widget_t;
struct gfxw_container_t;
struct gfxw_visual_t;
struct gfxw_port_t;

typedef int gfxw_point_op(gfxw_widget_t *, Common::Point);
typedef int gfxw_visual_op(gfxw_widget_t *, gfxw_visual_t *);
typedef int gfxw_op(gfxw_widget_t *);
typedef int gfxw_op_int(gfxw_widget_t *, int);
typedef int gfxw_bin_op(gfxw_widget_t *, gfxw_widget_t *);

struct gfxw_widget_t {
	int _magic; /* Extra check after typecasting */
	int _serial; /* Serial number */
	int _flags; /* Widget flags */
	gfxw_widget_type_t _type;
	rect_t _bounds; /* Boundaries */
	gfxw_widget_t *_next; /* Next widget in widget list */
	int _ID; /* Unique ID or GFXW_NO_ID */
	int _subID; /* A 'sub-ID', or GFXW_NO_ID */
	gfxw_container_t *_parent; /* The parent widget, or NULL if not owned */
	gfxw_visual_t *_visual; /* The owner visual */
	int _widgetPriority; /* Drawing priority, or -1 */

public:
	// TODO: Replace the following with virtual methods
	gfxw_point_op *draw; /* Draw widget (if dirty) and anything else required for the display to be consistent */
	gfxw_op *widfree; /* Remove widget (and any sub-widgets it may contain) */
	gfxw_op *tag; /* Tag the specified widget */
	gfxw_op_int *print; /* Prints the widget's contents, using sciprintf. Second parameter is indentation. */
	gfxw_bin_op *compare_to; /* a.compare_to(a, b) returns <0 if a<b, =0 if a=b and >0 if a>b */
	gfxw_bin_op *equals; /* a equals b if both cause the same data to be displayed */
	gfxw_bin_op *should_replace; /* (only if a equals b) Whether b should replace a even though they are equivalent */
	gfxw_bin_op *superarea_of; /* a superarea_of b <=> for each pixel of b there exists an opaque pixel in a at the same location */
	gfxw_visual_op *set_visual; /* Sets the visual the widget belongs to */

	gfxw_widget_t(gfxw_widget_type_t type);
};


#define GFXW_IS_BOX(widget) ((widget)->_type == GFXW_BOX)
struct gfxw_box_t : public gfxw_widget_t {
	gfx_color_t _color1, _color2;
	gfx_box_shade_t _shadeType;

	gfxw_box_t(gfx_state_t *state, rect_t area, gfx_color_t color1, gfx_color_t color2, gfx_box_shade_t shade_type);
};


#define GFXW_IS_PRIMITIVE(widget) ((widget)->_type == GFXW_RECT || (widget)->_type == GFXW_LINE)
struct gfxw_primitive_t : public gfxw_widget_t {
	gfx_color_t _color;
	gfx_line_mode_t _lineMode;
	gfx_line_style_t _lineStyle;

	gfxw_primitive_t(rect_t area, gfx_color_t color, gfx_line_mode_t mode,
						gfx_line_style_t style, gfxw_widget_type_t type);
};



#define GFXW_IS_VIEW(widget) ((widget)->_type == GFXW_VIEW || (widget)->_type == GFXW_STATIC_VIEW \
			      || (widget)->_type == GFXW_DYN_VIEW || (widget)->_type == GFXW_PIC_VIEW)
struct gfxw_view_t  : public gfxw_widget_t {
	Common::Point _pos; /* Implies the value of 'bounds' in gfxw_widget_t */
	gfx_color_t _color;
	int _view, _loop, _cel;
	int _palette;

	gfxw_view_t(gfx_state_t *state, Common::Point pos, int view_nr, int loop, int cel, int palette, int priority, int control,
		gfx_alignment_t halign, gfx_alignment_t valign, int flags);
};

#define GFXW_IS_DYN_VIEW(widget) ((widget)->_type == GFXW_DYN_VIEW || (widget)->_type == GFXW_PIC_VIEW)
struct gfxw_dyn_view_t : public gfxw_view_t {
	/* FIXME: This code is specific to SCI */
	rect_t draw_bounds; /* The correct position to draw to */
	void *under_bitsp, *signalp;
	int under_bits, signal;
	int _z; /* The z coordinate: Added to y, but used for sorting */
	int sequence; /* Sequence number: For sorting */
	int force_precedence; /* Precedence enforcement variable for sorting- defaults to 0 */

	gfxw_dyn_view_t(gfx_state_t *state, Common::Point pos, int z, int view, int loop, int cel, int palette, int priority, int control,
		gfx_alignment_t halign, gfx_alignment_t valign, int sequence);
};



#define GFXW_IS_TEXT(widget) ((widget)->_type == GFXW_TEXT)
struct gfxw_text_t : public gfxw_widget_t {
	int font_nr;
	int lines_nr, lineheight, lastline_width;
	char *text;
	gfx_alignment_t halign, valign;
	gfx_color_t _color1, _color2, _bgcolor;
	int text_flags;
	int width, height; /* Real text width and height */
	gfx_text_handle_t *text_handle;

	gfxw_text_t(gfx_state_t *state, rect_t area, int font, const char *text, gfx_alignment_t halign,
		gfx_alignment_t valign, gfx_color_t color1, gfx_color_t color2, gfx_color_t bgcolor, int text_flags);
};


/* Container widgets */

typedef int gfxw_unary_container_op(gfxw_container_t *);
typedef int gfxw_container_op(gfxw_container_t *, gfxw_widget_t *);
typedef int gfxw_rect_op(gfxw_container_t *, rect_t, int);


struct gfxw_container_t : public gfxw_widget_t {
	rect_t zone; /* The writeable zone (absolute) for contained objects */
	gfx_dirty_rect_t *dirty; /* List of dirty rectangles */
	gfxw_widget_t *contents;
	gfxw_widget_t **nextpp; /* Pointer to the 'next' pointer in the last entry in contents */

public:
	// TODO: Replace the following with virtual methods
	gfxw_unary_container_op *free_tagged; /* Free all tagged contained widgets */
	gfxw_unary_container_op *free_contents; /* Free all contained widgets */
	gfxw_rect_op *add_dirty_abs; /* Add an absolute dirty rectangle */
	gfxw_rect_op *add_dirty_rel; /* Add a relative dirty rectangle */
	gfxw_container_op *add;  /* Append widget to an appropriate position (for view and control lists) */

	gfxw_container_t(rect_t area, gfxw_widget_type_t type);
};


#define GFXW_IS_CONTAINER(widget) ((widget)->_type == GFXW_PORT || (widget)->_type == GFXW_VISUAL || \
				   (widget)->_type == GFXW_SORTED_LIST || (widget)->_type == GFXW_LIST)

#define GFXW_IS_LIST(widget) ((widget)->_type == GFXW_LIST || (widget)->_type == GFXW_SORTED_LIST)
#define GFXW_IS_SORTED_LIST(widget) ((widget)->_type == GFXW_SORTED_LIST)

struct gfxw_list_t : public gfxw_container_t {
	gfxw_list_t(rect_t area, bool sorted);
};

#define GFXW_IS_VISUAL(widget) ((widget)->_type == GFXW_VISUAL)
struct gfxw_visual_t : public gfxw_container_t {
	gfxw_port_t **port_refs; /* References to ports */
	int port_refs_nr;
	int font_nr; /* Default font */
	gfx_state_t *gfx_state;

	gfxw_visual_t(gfx_state_t *state, int font);
};

#define GFXW_IS_PORT(widget) ((widget)->_type == GFXW_PORT)
struct gfxw_port_t : public gfxw_container_t {
	gfxw_list_t *decorations; /* optional window decorations- drawn before the contents */
	gfxw_widget_t *port_bg; /* Port background widget or NULL */
	gfx_color_t _color, _bgcolor;
	int font_nr;
	Common::Point draw_pos; /* Drawing position */
	gfxw_snapshot_t *restore_snap; /* Snapshot to be restored automagically,
					  experimental feature used in the PQ3 interpreter */
	int port_flags; /* interpreter-dependant flags */
	const char *title_text;
	byte gray_text; /* Whether text is 'grayed out' (dithered) */

	gfxw_port_t(gfxw_visual_t *visual, rect_t area, gfx_color_t fgcolor, gfx_color_t bgcolor);
};

} // End of namespace Sci

#endif // SCI_GFX_GFX_STATE_INTERNAL_H
