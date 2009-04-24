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

#include "sci/engine/state.h"
#include "sci/gfx/operations.h"
#include "sci/gfx/gfx_widgets.h"
#include "sci/gfx/menubar.h"
#include "sci/gfx/gfx_gui.h"
#include "sci/gfx/gfx_state_internal.h"
#include "sci/gfx/font.h"

#include "common/system.h"

namespace Sci {

#define SCI_SPECIAL_CHAR_ARROW_UP 0x18
#define SCI_SPECIAL_CHAR_ARROW_DOWN 0x19

static void clear_titlebar(gfxw_port_t *titlebar) {
	if (titlebar->contents) {
		titlebar->contents->widfree(titlebar->contents);
		titlebar->contents = NULL;
		titlebar->nextpp = &(titlebar->contents);
	}
}

static gfxw_list_t *make_titlebar_list(EngineState *s, rect_t bounds, gfxw_port_t *status_bar) {
	gfx_color_t color = status_bar->_bgcolor;
	gfxw_list_t *list;
	gfxw_box_t *bgbox;


	list = gfxw_new_list(status_bar->_bounds, 0);
	bgbox = gfxw_new_box(s->gfx_state, gfx_rect(0, 0, status_bar->_bounds.width, status_bar->_bounds.height - 1),
	                     color, color, GFX_BOX_SHADE_FLAT);

	list->add((gfxw_container_t *) list, (gfxw_widget_t *) bgbox);

	return list;
}

static gfxw_list_t *finish_titlebar_list(EngineState *s, gfxw_list_t *list, gfxw_port_t *status_bar) {
	gfx_color_t black = s->ega_colors[0];
	gfxw_primitive_t *line;

	line = gfxw_new_line(Common::Point(0, status_bar->_bounds.height - 1), Common::Point(status_bar->_bounds.width, status_bar->_bounds.height - 1),
	                     black, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL);
	list->add((gfxw_container_t *)list, (gfxw_widget_t *)line);

	return list;
}

void sciw_set_status_bar(EngineState *s, gfxw_port_t *status_bar, const Common::String &text, int fgcolor, int bgcolor) {
	gfx_state_t *state;
	gfxw_list_t *list;
	gfx_color_t bg = status_bar->_bgcolor;
	gfx_color_t fg = status_bar->_color;
	gfx_color_t black = s->ega_colors[0];

	if (!status_bar->_visual) {
		GFXERROR("Attempt to change title bar without visual!\n");
		return;
	}

	state = status_bar->_visual->gfx_state;

	if (!state) {
		GFXERROR("Attempt to change title bar with stateless visual!\n");
		return;
	}

	clear_titlebar(status_bar);

	if (!text.empty()) {
		gfxw_text_t *textw = gfxw_new_text(state, gfx_rect(0, 0, status_bar->_bounds.width, status_bar->_bounds.height),
		                                   status_bar->font_nr, text.c_str(), ALIGN_LEFT, ALIGN_CENTER,
		                                   fg, fg, bg, kFontNoNewlines);

		list = make_titlebar_list(s, status_bar->_bounds, status_bar);

		list->add((gfxw_container_t *)list, (gfxw_widget_t *)textw);

	} else {
		gfxw_box_t *bgbox = gfxw_new_box(state, gfx_rect(0, 0, status_bar->_bounds.width, status_bar->_bounds.height - 1),
		                                 black, black, GFX_BOX_SHADE_FLAT);

		list = gfxw_new_list(status_bar->_bounds, 0);

		list->add((gfxw_container_t *)list, (gfxw_widget_t *)bgbox);
	}

	list->add(GFXWC(status_bar), GFXW(list));
	finish_titlebar_list(s, list, status_bar);

	status_bar->draw(GFXW(status_bar), gfxw_point_zero);
	gfxop_update(state);
}

static void sciw_make_window_fit(rect_t *rect, gfxw_port_t *parent) {
	// This window is meant to cover the whole screen, so we allow it to go through.
	if (rect->width == 319 && rect->height == 189) return;

	if (rect->x + rect->width > parent->_bounds.x + parent->_bounds.width)
		rect->x -= (rect->x + rect->width) - (parent->_bounds.x + parent->_bounds.width) + 2;

	if (rect->y + rect->height > parent->_bounds.y + parent->_bounds.height)
		rect->y -= (rect->y + rect->height) - (parent->_bounds.y + parent->_bounds.height) + 2;
}

gfxw_port_t *sciw_new_window(EngineState *s,
		rect_t area, int font,
		gfx_color_t color, gfx_color_t bgcolor,
		int title_font, gfx_color_t title_color, gfx_color_t title_bgcolor,
		const char *title, int flags) {
	gfxw_visual_t *visual = s->visual;
	gfx_state_t *state = s->gfx_state;
	int shadow_offset = 2;
	rect_t frame;
	gfx_color_t black;
	gfxop_set_color(state, &black, 0, 0, 0, 0, 0, 0);
	gfxw_port_t *win;
	gfxw_list_t *decorations;
//	int xextra = !(flags & kWindowNoFrame) ? 1 : 0;
//	int yextra = !(flags & kWindowNoFrame) ? 2 : 0;

	if (area.width == 319 && area.height == 189) {
		flags |= kWindowNoFrame;
		// The below line makes the points bar in QfG2 work, but breaks
		// the one in QfG1. Hm.
		if ((byte)bgcolor.priority == 255) /* Yep, QfG2 */
			area.y += 3;
	}

	/*
	if (area.y + area.height > visual->_bounds.y + visual->_bounds.height) {
		area.y -= (area.y + area.height) - (visual->_bounds.y + visual->_bounds.height) + yextra;
	}

	if (area.x + area.width > visual->_bounds.x + visual->_bounds.width) {
		area.x -= (area.x + area.width) - (visual->_bounds.x + visual->_bounds.width) + xextra;
	}
	*/

	if (flags & kWindowTitle)
		area. y += 10;

	if (!(flags & (kWindowTitle | kWindowNoFrame)))
		area.height -= 1; // Normal windows are drawn one pixel too small.

	sciw_make_window_fit(&area, s->wm_port);
	win = gfxw_new_port(visual, s->wm_port, area, color, bgcolor);

	win->font_nr = font;
	win->title_text = title;
	win->port_flags = flags;

	win->_flags |= GFXW_FLAG_IMMUNE_TO_SNAPSHOTS;

	if (flags & kWindowDontDraw)
		flags = kWindowTransparent | kWindowNoFrame;

	if (flags == (kWindowTransparent | kWindowNoFrame))
		return win; // Fully transparent window

	if (flags & kWindowTitle)
		frame = gfx_rect(area.x - 1, area.y - 10, area.width + 2, area.height + 11);
	else
		frame = gfx_rect(area.x - 1, area.y - 1, area.width + 2, area.height + 2);

	// Set visible window boundaries
	win->_bounds = gfx_rect(frame.x, frame.y, frame.width + shadow_offset, frame.height + shadow_offset);

	decorations = gfxw_new_list(gfx_rect(frame.x, frame.y, frame.width + 1 + shadow_offset, frame.height + 1 + shadow_offset), 0);

	if (!(flags & kWindowTransparent)) {
		// Draw window background
		win->port_bg = (gfxw_widget_t *)gfxw_new_box(state, gfx_rect(1, (flags & kWindowTitle) ? 10 : 1,
		                        area.width, area.height), bgcolor, bgcolor, GFX_BOX_SHADE_FLAT);
		decorations->add((gfxw_container_t *) decorations, win->port_bg);
		win->_flags |= GFXW_FLAG_OPAQUE;
	}

	if (flags & kWindowTitle) {
		// Add window title
		rect_t title_rect = gfx_rect(1, 1, area.width, 8);

		decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)
						gfxw_new_box(state, title_rect, title_bgcolor, title_bgcolor, GFX_BOX_SHADE_FLAT));

		decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)
						gfxw_new_text(state, title_rect, title_font, title, ALIGN_CENTER, ALIGN_CENTER, title_color, title_color,
						title_bgcolor, kFontNoNewlines));
	}

	if (!(flags & kWindowNoFrame)) {
		// Draw backdrop shadow

		if (!(flags & kWindowNoDropShadow)) {
			if (gfxop_set_color(state, &black, 0, 0, 0, 0x80, bgcolor.priority, -1)) {
				GFXERROR("Could not get black/semitrans color entry!\n");
				return NULL;
			}

			decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)
			                 gfxw_new_box(state, gfx_rect(shadow_offset + 1, frame.height - 1,
							 frame.width - 4, shadow_offset), black, black, GFX_BOX_SHADE_FLAT));

			decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)
			                 gfxw_new_box(state, gfx_rect(frame.width - 1, shadow_offset + 1,
							 shadow_offset, frame.height - 2), black, black, GFX_BOX_SHADE_FLAT));
		}

		// Draw frame

		if (gfxop_set_color(state, &black, 0, 0, 0, 0, bgcolor.priority, -1)) {
			GFXERROR("Could not get black color entry!\n");
			return NULL;
		}

		if (!(flags & kWindowNoDropShadow)) {

			decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)
			                 gfxw_new_rect(gfx_rect(0, 0, frame.width - 1, frame.height - 1), black, GFX_LINE_MODE_FINE, GFX_LINE_STYLE_NORMAL));

			if (flags & kWindowTitle)
				decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)gfxw_new_line(Common::Point(1, 9),
									Common::Point(frame.width - 2, 9), black, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL));
		} else {
			decorations->add((gfxw_container_t *)decorations, (gfxw_widget_t *)
			                 gfxw_new_rect(gfx_rect(0, 0, frame.width, frame.height),  black, GFX_LINE_MODE_FINE, GFX_LINE_STYLE_NORMAL));
		}
	}

	win->decorations = decorations;
	decorations->_parent = GFXWC(win);

	return win;
}

//*** Controls ***

static rect_t _move_and_extend_rect(rect_t rect, Common::Point point, int yplus) {
	return gfx_rect(rect.x + point.x, rect.y + point.y, rect.width + 1, rect.height + yplus);
}

gfxw_list_t *_sciw_add_text_to_list(gfxw_list_t *list, gfxw_port_t *port, rect_t zone, char *text,
	int font, gfx_alignment_t align, char framed, char inverse, int flags, char gray_text) {
	gfx_color_t *color1, *color2, *bgcolor;

	if (inverse) {
		color1 = color2 = &(port->_bgcolor);
		bgcolor = &(port->_color);
	} else if (gray_text) {
		bgcolor = color1 = &(port->_bgcolor);
		color2 = &(port->_color);
	} else {
		color1 = color2 = &(port->_color);
		bgcolor = &(port->_bgcolor);
	}

	list->add(GFXWC(list), GFXW(gfxw_new_text(port->_visual->gfx_state, zone, font, text, align, ALIGN_TOP,
	                            *color1, *color2, *bgcolor, flags)));

	zone.width--;
	zone.height -= 2;

	if (framed) {
		list->add(GFXWC(list), GFXW(gfxw_new_rect(zone, *color2, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_STIPPLED)));
	}

	return list;
}

gfxw_list_t *sciw_new_button_control(gfxw_port_t *port, reg_t ID, rect_t zone, char *text, int font, char selected, char inverse, char grayed_out) {
	gfx_color_t *frame_col = (inverse) ? &(port->_bgcolor) : &(port->_color);
	gfxw_list_t *list;

	zone.x--;
	zone.y--;
	zone.width++;
	zone.height++;

	list = gfxw_new_list(_move_and_extend_rect(zone, Common::Point(port->zone.x, port->zone.y), 1), 0);

	gfxw_set_id(GFXW(list), ID.segment, ID.offset);

	zone.x = 0;
	zone.y = 0;

	if (!inverse) {
		list = _sciw_add_text_to_list(list, port, gfx_rect(zone.x + 1, zone.y + 2, zone.width - 1, zone.height),
		                              text, font, ALIGN_CENTER, 0, inverse, kFontIgnoreLF, grayed_out);

		list->add(GFXWC(list),
		          GFXW(gfxw_new_rect(zone, *frame_col, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));
	} else {
		list->add(GFXWC(list), GFXW(gfxw_new_box(NULL, gfx_rect(zone.x, zone.y, zone.width + 1, zone.height + 1),
		                            port->_color, port->_color, GFX_BOX_SHADE_FLAT)));

		list = _sciw_add_text_to_list(list, port, gfx_rect(zone.x + 1, zone.y + 2, zone.width - 1, zone.height),
		                              text, font, ALIGN_CENTER, 0, inverse, kFontIgnoreLF, grayed_out);
	}

	if (selected)
		list->add(GFXWC(list),
		          GFXW(gfxw_new_rect(gfx_rect(zone.x + 1, zone.y + 1, zone.width - 2, zone.height - 2),
		                             *frame_col, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

	return list;
}

gfxw_list_t *sciw_new_text_control(gfxw_port_t *port, reg_t ID, rect_t zone, char *text, int font,
								   gfx_alignment_t align, char framed, char inverse) {
	gfxw_list_t *list = gfxw_new_list(_move_and_extend_rect(zone, Common::Point(port->zone.x, port->zone.y), 2), 0);

	gfxw_set_id(GFXW(list), ID.segment, ID.offset);

	zone.x = 0;
	zone.y = 0;

	return _sciw_add_text_to_list(list, port, zone, text, font, align, framed, inverse, 0, port->gray_text);
}

gfxw_list_t *sciw_new_edit_control(gfxw_port_t *port, reg_t ID, rect_t zone, char *text, int font, unsigned int cursor,
								   char inverse) {
	gfxw_text_t *text_handle;

	gfxw_list_t *list;
	int cursor_height = gfxop_get_font_height(port->_visual->gfx_state, font);

	zone.x--;
	zone.y--;
	zone.width++;
	zone.height++;

	list = gfxw_new_list(_move_and_extend_rect(zone, Common::Point(port->zone.x, port->zone.y), 1), 0);
	gfxw_set_id(GFXW(list), ID.segment, ID.offset);
	zone.x = 1;
	zone.y = 1;

	if ((g_system->getMillis() % 1000) < 500) {
		text_handle = gfxw_new_text(port->_visual->gfx_state, zone, font, text, ALIGN_LEFT, ALIGN_TOP,
		                            port->_color, port->_color, port->_bgcolor, kFontNoNewlines);

		list->add(GFXWC(list), GFXW(text_handle));
	} else {
		char *textdup = (char *)sci_malloc(strlen(text) + 1);

		strncpy(textdup, text, cursor);

		if (cursor <= strlen(text))
			textdup[cursor] = 0; // terminate

		if (cursor > 0) {
			text_handle = gfxw_new_text(port->_visual->gfx_state, zone, font, textdup, ALIGN_LEFT, ALIGN_TOP,
			                            port->_color, port->_color, port->_bgcolor, kFontNoNewlines);

			list->add(GFXWC(list), GFXW(text_handle));
			zone.x += text_handle->width;
		}

		if (cursor < strlen(text)) {
			textdup[0] = text[cursor];
			textdup[1] = 0;
			text_handle =  gfxw_new_text(port->_visual->gfx_state, zone, font, textdup, ALIGN_LEFT, ALIGN_TOP,
			                             port->_bgcolor, port->_bgcolor, port->_color, kFontNoNewlines);
			list->add(GFXWC(list), GFXW(text_handle));
			zone.x += text_handle->width;
		};

		if (cursor + 1 < strlen(text)) {
			text_handle = gfxw_new_text(port->_visual->gfx_state, zone, font, text + cursor + 1, ALIGN_LEFT, ALIGN_TOP,
			                            port->_color, port->_color, port->_bgcolor, kFontNoNewlines);
			list->add(GFXWC(list), GFXW(text_handle));
			zone.x += text_handle->width;
		};

		if (cursor == strlen(text))
			list->add(GFXWC(list), GFXW(gfxw_new_line(Common::Point(zone.x, zone.y), Common::Point(zone.x, zone.y + cursor_height - 1),
			                            port->_color, GFX_LINE_MODE_FAST, GFX_LINE_STYLE_NORMAL)));
		free(textdup);
	}

	zone.x = zone.y = 0;

	list->add(GFXWC(list), GFXW(gfxw_new_rect(zone, port->_color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));

	return list;
}

gfxw_list_t *sciw_new_icon_control(gfxw_port_t *port, reg_t ID, rect_t zone, int view, int loop, int cel,
	char frame, char inverse) {
	gfxw_list_t *list = gfxw_new_list(_move_and_extend_rect(zone, Common::Point(port->zone.x, port->zone.y), 1), 0);
	gfxw_widget_t *icon;
	gfxw_set_id(GFXW(list), ID.segment, ID.offset);

	if (!port->_visual) {
		GFXERROR("Attempting to create icon control for virtual port!\n");
		return NULL;
	}

	zone.x = 0;
	zone.y = 0;

	icon = GFXW(gfxw_new_view(port->_visual->gfx_state, Common::Point(zone.x, zone.y), view, loop, cel, 0, -1, -1,
	                          ALIGN_LEFT, ALIGN_TOP, GFXW_VIEW_FLAG_DONT_MODIFY_OFFSET));

	if (!icon) {
		GFXERROR("Attempt to create icon control with cel %d/%d/%d (invalid)\n", view, loop, cel);
		return NULL;
	}

	list->_flags |= GFXW_FLAG_MULTI_ID;

	list->add(GFXWC(list), icon);

	return list;
}

gfxw_list_t *sciw_new_list_control(gfxw_port_t *port, reg_t ID, rect_t zone, int font_nr, char **entries_list,
	int entries_nr, int list_top, int selection, char inverse) {
	gfxw_list_t *list;

	char arr_up[2], arr_down[2];
	int i;

	int font_height;
	int columns;

	zone.x--;
	zone.y--;
	zone.width++;
	zone.height++;

	list = gfxw_new_list(_move_and_extend_rect(zone, Common::Point(port->zone.x, port->zone.y), 1), 0);

	font_height = gfxop_get_font_height(port->_visual->gfx_state, font_nr);
	columns = (zone.height - 20);

	if (font_height <= 0) {
		GFXERROR("Attempt to create list control with invalid font %d\n", font_nr);
		list->widfree(GFXW(list));
		return NULL;
	}

	columns /= font_height;

	gfxw_set_id(GFXW(list), ID.segment, ID.offset);

	arr_up[0] = SCI_SPECIAL_CHAR_ARROW_UP;
	arr_down[0] = SCI_SPECIAL_CHAR_ARROW_DOWN;
	arr_up[1] = arr_down[1] = 0;

	zone.x = 1;
	zone.y = 11;

	// Draw text

	for (i = list_top; columns-- && i < entries_nr; i++) {
		if (i != selection)
			list->add(GFXWC(list),
			          GFXW(gfxw_new_text(port->_visual->gfx_state, gfx_rect(zone.x, zone.y, zone.width - 2, font_height),
			                             font_nr, entries_list[i], ALIGN_LEFT, ALIGN_TOP,
			                             port->_color, port->_color, port->_bgcolor, kFontNoNewlines)));
		else {
			list->add(GFXWC(list), GFXW(gfxw_new_box(port->_visual->gfx_state, gfx_rect(zone.x, zone.y, zone.width - 1, font_height),
			                            port->_color, port->_color, GFX_BOX_SHADE_FLAT)));
			list->add(GFXWC(list), GFXW(gfxw_new_text(port->_visual->gfx_state, gfx_rect(zone.x, zone.y, zone.width - 2, font_height),
			                             font_nr, entries_list[i], ALIGN_LEFT, ALIGN_TOP,
			                             port->_bgcolor, port->_bgcolor, port->_color, kFontNoNewlines)));
		}

		zone.y += font_height;
	}

	// Draw frames

	zone.x = 0;
	zone.y = 0;

	// Add up arrow
	list->add(GFXWC(list), GFXW(gfxw_new_text(port->_visual->gfx_state, gfx_rect(1, 0, zone.width - 2, 8),
	                             port->font_nr, arr_up, ALIGN_CENTER, ALIGN_CENTER,
	                             port->_color, port->_color, port->_bgcolor, 0)));

	// Add down arrow
	list->add(GFXWC(list), GFXW(gfxw_new_text(port->_visual->gfx_state, gfx_rect(1, zone.height - 9, zone.width - 2, 8),
	                             port->font_nr, arr_down, ALIGN_CENTER, ALIGN_CENTER,
	                             port->_color, port->_color, port->_bgcolor, 0)));

	if (list_top & 1) { // Hack to work around aggressive caching
		list->add(GFXWC(list), GFXW(gfxw_new_rect(zone, port->_color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));
		list->add(GFXWC(list), GFXW(gfxw_new_rect(gfx_rect(zone.x, zone.y + 10, zone.width, zone.height - 20),
		                             port->_color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));
	} else {
		list->add(GFXWC(list),
		          GFXW(gfxw_new_rect(gfx_rect(zone.x, zone.y, zone.width, zone.height - 10),
		                             port->_color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));
		list->add(GFXWC(list),
		          GFXW(gfxw_new_rect(gfx_rect(zone.x, zone.y + 10, zone.width, zone.height - 10),
		                             port->_color, GFX_LINE_MODE_CORRECT, GFX_LINE_STYLE_NORMAL)));
	}

	return list;
}

void sciw_set_menubar(EngineState *s, gfxw_port_t *status_bar, Menubar *menubar, int selection) {
	gfxw_list_t *list = make_titlebar_list(s, status_bar->_bounds, status_bar);
	int offset = MENU_LEFT_BORDER;
	int i;

	clear_titlebar(status_bar);

	for (i = 0; i < (int)menubar->_menus.size(); i++) {
		Menu *menu = &menubar->_menus[i];
		int width = menu->_titleWidth + (MENU_BORDER_SIZE * 2);

		if (i == selection) {
			list->add(GFXWC(list), GFXW(gfxw_new_box(status_bar->_visual->gfx_state, gfx_rect(offset, 0, width, MENU_BAR_HEIGHT),
			                            status_bar->_color, status_bar->_color, GFX_BOX_SHADE_FLAT)));
			list->add(GFXWC(list), GFXW(gfxw_new_text(s->gfx_state, gfx_rect(offset, 0, width, MENU_BAR_HEIGHT),
			                             status_bar->font_nr, menu->_title.c_str(), ALIGN_CENTER, ALIGN_CENTER,
			                             status_bar->_bgcolor, status_bar->_bgcolor, status_bar->_color, kFontNoNewlines)));
		} else
			list->add(GFXWC(list), GFXW(gfxw_new_text(s->gfx_state, gfx_rect(offset, 0, width, MENU_BAR_HEIGHT),
			                             status_bar->font_nr, menu->_title.c_str(), ALIGN_CENTER, ALIGN_CENTER,
			                             status_bar->_color, status_bar->_color, status_bar->_bgcolor, kFontNoNewlines)));
		offset += width;
	}

	status_bar->add(GFXWC(status_bar), GFXW(list));
	finish_titlebar_list(s, list, status_bar);
}

gfxw_port_t *sciw_new_menu(EngineState *s, gfxw_port_t *status_bar, Menubar *menubar, int selection) {
	gfxw_port_t *retval;
	Menu *menu = &menubar->_menus[selection];
	rect_t area = gfx_rect(MENU_LEFT_BORDER, 10, 0, 0);
	int i;

	if (selection < -1)
		return NULL;

	if (selection >= (int)menubar->_menus.size()) {
		GFXERROR("Attempt to make menu #%d of %d\n", selection, menubar->_menus.size());
		return NULL;
	}

	for (i = 0; i < selection; i++)
		area.x += menubar->_menus[i]._titleWidth;

	area.width = menu->_width - 1;
	area.height = menu->_items.size() * 10;

	retval = sciw_new_window(s, area, status_bar->font_nr, status_bar->_color, status_bar->_bgcolor,
	                         0, status_bar->_color, status_bar->_bgcolor, NULL, kWindowNoDropShadow | kWindowTransparent);

	retval->set_visual(GFXW(retval), s->visual);

	for (i = 0; i < (int)menu->_items.size(); i++)
		sciw_toggle_item(retval, menu, i, false);

	return retval;
}

#define MAGIC_ID_OFFSET 0x2000

static gfx_color_t un_prioritize(gfx_color_t col) {
	col.priority = -1;
	col.mask &= ~GFX_MASK_PRIORITY;

	return col;
}

gfxw_widget_t *_make_menu_entry(MenuItem *item, int offset, int width, gfxw_port_t *port, gfx_color_t color, gfx_color_t bgcolor, int ID, int gray) {
	rect_t area = gfx_rect(MENU_BOX_LEFT_PADDING, 0, width - MENU_BOX_LEFT_PADDING, 10);
	rect_t list_area = gfx_rect(port->zone.x, area.y + offset + port->zone.y, width, area.height);
	gfxw_list_t *list = (gfxw_list_t *) gfxw_set_id(GFXW(gfxw_new_list(list_area, 0)), ID, GFXW_NO_ID);
	gfx_color_t xcolor = { PaletteEntry(), 0, 0, 0, 0};

	color = un_prioritize(color);
	bgcolor = un_prioritize(bgcolor);

	xcolor = gray ? color : bgcolor;

	list->add(GFXWC(list), GFXW(gfxw_new_box(port->_visual->gfx_state, area, bgcolor, bgcolor, GFX_BOX_SHADE_FLAT)));
	list->add(GFXWC(list), GFXW(gfxw_new_text(port->_visual->gfx_state, area, port->font_nr, item->_text.c_str(), ALIGN_LEFT, ALIGN_CENTER,
	                            color, xcolor, bgcolor, kFontNoNewlines)));

	if (!item->_keytext.empty()) {
		area.width -= MENU_BOX_RIGHT_PADDING;
		list->add(GFXWC(list), GFXW(gfxw_new_text(port->_visual->gfx_state, area, port->font_nr, item->_keytext.c_str(), ALIGN_RIGHT, ALIGN_CENTER,
		                            color, xcolor, bgcolor, kFontNoNewlines)));
	}

	return GFXW(list);
}

gfxw_widget_t *_make_menu_hbar(int offset, int width, gfxw_port_t *port, gfx_color_t color, gfx_color_t bgcolor, int ID) {
	rect_t area = gfx_rect(0, 0, width, 10);
	rect_t list_area = gfx_rect(area.x + port->zone.x, area.y + offset + port->zone.y, area.width, area.height);
	gfxw_list_t *list = (gfxw_list_t *) gfxw_set_id(GFXW(gfxw_new_list(list_area, 0)), ID, GFXW_NO_ID);

	color = un_prioritize(color);
	bgcolor = un_prioritize(bgcolor);

	list->add(GFXWC(list), GFXW(gfxw_new_box(port->_visual->gfx_state, area, bgcolor, bgcolor, GFX_BOX_SHADE_FLAT)));
	list->add(GFXWC(list), GFXW(gfxw_new_line(Common::Point(0, 5), Common::Point(width, 5), color,
	                            GFX_LINE_MODE_FAST, GFX_LINE_STYLE_STIPPLED)));

	return GFXW(list);
}

gfxw_port_t *sciw_toggle_item(gfxw_port_t *menu_port, Menu *menu, int selection, bool selected) {
	if (selection < 0 || selection >= (int)menu->_items.size())
		return menu_port;

	gfx_color_t fgColor = !selected ? menu_port->_color : menu_port->_bgcolor;
	gfx_color_t bgColor = !selected ? menu_port->_bgcolor : menu_port->_color;

	MenuItem *item = &menu->_items[selection];

	if (item->_type == MENU_TYPE_NORMAL)
		menu_port->add(GFXWC(menu_port), GFXW(_make_menu_entry(item, selection * 10, menu_port->zone.width + 1,
		                                      menu_port, fgColor, bgColor, selection + MAGIC_ID_OFFSET, item->_enabled)));
	else
		menu_port->add(GFXWC(menu_port), GFXW(_make_menu_hbar(selection * 10, menu_port->zone.width + 1,
		                                      menu_port, fgColor, bgColor, selection + MAGIC_ID_OFFSET)));

	return menu_port;
}

} // End of namespace Sci
