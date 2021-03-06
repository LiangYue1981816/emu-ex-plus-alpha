#pragma once

/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/config/defs.hh>
#include <imagine/util/operators.hh>
#include <imagine/time/Time.hh>
#include <jni.h>
#include <utility>

namespace Base
{

enum SurfaceRotation : int;

class AndroidScreen : public NotEquals<AndroidScreen>
{
public:
	constexpr AndroidScreen() {}

	void init(JNIEnv *env, jobject aDisplay, jobject metrics, bool isMain);
	SurfaceRotation rotation(JNIEnv *env) const;
	std::pair<float, float> dpi() const;
	float densityDPI() const;
	jobject displayObject() const;
	int id() const;
	bool operator ==(AndroidScreen const &rhs) const;
	explicit operator bool() const;

protected:
	jobject aDisplay{};
	IG::FloatSeconds frameTime_{};
	float xDPI{}, yDPI{};
	float densityDPI_{};
	float refreshRate_{};
	int width_{}, height_{};
	#ifdef CONFIG_BASE_MULTI_SCREEN
	int id_{};
	#else
	static constexpr int id_ = 0;
	#endif
	bool reliableRefreshRate = true;
};

using ScreenImpl = AndroidScreen;

}
