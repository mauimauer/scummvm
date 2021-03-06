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
 */

#include "gob/surface.h"
#include "gob/anifile.h"
#include "gob/cmpfile.h"
#include "gob/aniobject.h"

namespace Gob {

ANIObject::ANIObject(const ANIFile &ani) : _ani(&ani), _cmp(0),
	_visible(false), _paused(false), _mode(kModeContinuous),
	_x(0), _y(0), _background(0), _drawn(false) {

	setAnimation(0);
	setPosition();
}

ANIObject::ANIObject(const CMPFile &cmp) : _ani(0), _cmp(&cmp),
	_visible(false), _paused(false), _mode(kModeContinuous),
	_x(0), _y(0), _background(0), _drawn(false) {

	setAnimation(0);
	setPosition();
}

ANIObject::~ANIObject() {
	delete _background;
}

void ANIObject::setVisible(bool visible) {
	_visible = visible;
}

bool ANIObject::isVisible() const {
	return _visible;
}

void ANIObject::setPause(bool pause) {
	_paused = pause;
}

bool ANIObject::isPaused() const {
	return _paused;
}

void ANIObject::setMode(Mode mode) {
	_mode = mode;
}

void ANIObject::setAnimation(uint16 animation) {
	_animation = animation;
	_frame     = 0;
}

void ANIObject::rewind() {
	_frame = 0;
}

void ANIObject::setPosition() {
	// CMP "animations" have no default position
	if (_cmp)
		return;

	if (_animation >= _ani->getAnimationCount())
		return;

	const ANIFile::Animation &animation = _ani->getAnimationInfo(_animation);

	_x = animation.x;
	_y = animation.y;
}

void ANIObject::setPosition(int16 x, int16 y) {
	_x = x;
	_y = y;
}

void ANIObject::getPosition(int16 &x, int16 &y) const {
	x = _x;
	y = _y;
}

void ANIObject::getFramePosition(int16 &x, int16 &y) const {
	// CMP "animations" have no specific frame positions
	if (_cmp) {
		getPosition(x, y);
		return;
	}

	if (_animation >= _ani->getAnimationCount())
		return;

	const ANIFile::Animation &animation = _ani->getAnimationInfo(_animation);
	if (_frame >= animation.frameCount)
		return;

	x = _x + animation.frameAreas[_frame].left;
	y = _y + animation.frameAreas[_frame].top;
}

void ANIObject::getFrameSize(int16 &width, int16 &height) const {
	if (_cmp) {
		width  = _cmp->getWidth (_animation);
		height = _cmp->getHeight(_animation);

		return;
	}

	if (_animation >= _ani->getAnimationCount())
		return;

	const ANIFile::Animation &animation = _ani->getAnimationInfo(_animation);
	if (_frame >= animation.frameCount)
		return;

	width  = animation.frameAreas[_frame].right  - animation.frameAreas[_frame].left + 1;
	height = animation.frameAreas[_frame].bottom - animation.frameAreas[_frame].top  + 1;
}

bool ANIObject::isIn(int16 x, int16 y) const {
	if (!isVisible())
		return false;

	int16 frameX, frameY, frameWidth, frameHeight;
	getFramePosition(frameX, frameY);
	getFrameSize(frameWidth, frameHeight);

	if ((x < frameX) || (y < frameY))
		return false;
	if ((x > (frameX + frameWidth)) || (y > (frameY + frameHeight)))
		return false;

	return true;
}

bool ANIObject::isIn(const ANIObject &obj) const {
	if (!isVisible() || !obj.isVisible())
		return false;

	int16 frameX, frameY, frameWidth, frameHeight;
	getFramePosition(frameX, frameY);
	getFrameSize(frameWidth, frameHeight);

	return obj.isIn(frameX                 , frameY                  ) ||
	       obj.isIn(frameX + frameWidth - 1, frameY                  ) ||
	       obj.isIn(frameX                 , frameY + frameHeight - 1) ||
	       obj.isIn(frameX + frameWidth - 1, frameY + frameHeight - 1);
}

bool ANIObject::draw(Surface &dest, int16 &left, int16 &top,
                                    int16 &right, int16 &bottom) {

	if (!_visible)
		return false;

	if      (_cmp)
		return drawCMP(dest, left, top, right, bottom);
	else if (_ani)
		return drawANI(dest, left, top, right, bottom);

	return false;
}

bool ANIObject::drawCMP(Surface &dest, int16 &left, int16 &top,
                                       int16 &right, int16 &bottom) {

	if (!_background) {
		uint16 width, height;

		_cmp->getMaxSize(width, height);

		_background = new Surface(width, height, dest.getBPP());
	}

	const uint16 cR = _cmp->getWidth (_animation) - 1;
	const uint16 cB = _cmp->getHeight(_animation) - 1;

	_backgroundLeft   = CLIP<int16>(   + _x, 0, dest.getWidth () - 1);
	_backgroundTop    = CLIP<int16>(   + _y, 0, dest.getHeight() - 1);
	_backgroundRight  = CLIP<int16>(cR + _x, 0, dest.getWidth () - 1);
	_backgroundBottom = CLIP<int16>(cB + _y, 0, dest.getHeight() - 1);

	_background->blit(dest, _backgroundLeft , _backgroundTop,
	                        _backgroundRight, _backgroundBottom, 0, 0);

	_cmp->draw(dest, _animation, _x, _y, 0);

	_drawn = true;

	left   = _backgroundLeft;
	top    = _backgroundTop;
	right  = _backgroundRight;
	bottom = _backgroundBottom;

	return true;
}

bool ANIObject::drawANI(Surface &dest, int16 &left, int16 &top,
                                       int16 &right, int16 &bottom) {

	if (!_background) {
		uint16 width, height;

		_ani->getMaxSize(width, height);

		_background = new Surface(width, height, dest.getBPP());
	}

	const ANIFile::Animation &animation = _ani->getAnimationInfo(_animation);
	if (_frame >= animation.frameCount)
		return false;

	const ANIFile::FrameArea &area = animation.frameAreas[_frame];

	_backgroundLeft   = CLIP<int16>(area.left   + _x, 0, dest.getWidth () - 1);
	_backgroundTop    = CLIP<int16>(area.top    + _y, 0, dest.getHeight() - 1);
	_backgroundRight  = CLIP<int16>(area.right  + _x, 0, dest.getWidth () - 1);
	_backgroundBottom = CLIP<int16>(area.bottom + _y, 0, dest.getHeight() - 1);

	_background->blit(dest, _backgroundLeft , _backgroundTop,
	                        _backgroundRight, _backgroundBottom, 0, 0);

	_ani->draw(dest, _animation, _frame, _x, _y);

	_drawn = true;

	left   = _backgroundLeft;
	top    = _backgroundTop;
	right  = _backgroundRight;
	bottom = _backgroundBottom;

	return true;
}

bool ANIObject::clear(Surface &dest, int16 &left, int16 &top,
                                     int16 &right, int16 &bottom) {

	if (!_drawn)
		return false;

	const int16 bgRight  = _backgroundRight  - _backgroundLeft;
	const int16 bgBottom = _backgroundBottom - _backgroundTop;

	dest.blit(*_background, 0, 0, bgRight, bgBottom, _backgroundLeft, _backgroundTop);

	_drawn = false;

	left   = _backgroundLeft;
	top    = _backgroundTop;
	right  = _backgroundRight;
	bottom = _backgroundBottom;

	return true;
}

void ANIObject::advance() {
	if (_paused)
		return;

	// CMP "animations" have only one frame
	if (_cmp)
		return;

	if (_animation >= _ani->getAnimationCount())
		return;

	const ANIFile::Animation &animation = _ani->getAnimationInfo(_animation);

	_frame = (_frame + 1) % animation.frameCount;

	if (_frame == 0) {
		_x += animation.deltaX;
		_y += animation.deltaY;

		if (_mode == kModeOnce) {
			_paused  = true;
			_visible = false;
		}
	}
}

uint16 ANIObject::getAnimation() const {
	return _animation;
}

uint16 ANIObject::getFrame() const {
	return _frame;
}

bool ANIObject::lastFrame() const {
	// CMP "animations" have only one frame
	if (_cmp)
		return true;

	if (_animation >= _ani->getAnimationCount())
		return true;

	const ANIFile::Animation &animation = _ani->getAnimationInfo(_animation);

	return (_frame + 1) >= animation.frameCount;
}

} // End of namespace Gob
