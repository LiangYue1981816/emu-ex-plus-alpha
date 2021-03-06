#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/fs/FS.hh>
#include <imagine/util/string.h>
#include <imagine/input/Input.hh>
#include <imagine/util/container/ArrayList.hh>

class TextMenuItem;

struct RecentGameInfo
{
	FS::PathString path{};
	FS::FileString name{};
	static constexpr uint MAX_RECENT = 10;

	constexpr RecentGameInfo() {}

	bool operator ==(RecentGameInfo const& rhs) const
	{
		return string_equal(path.data(), rhs.path.data());
	}

	void handleMenuSelection(TextMenuItem &, Input::Event e);
};

using RecentGameList = StaticArrayList<RecentGameInfo, RecentGameInfo::MAX_RECENT>;
