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

#include <imagine/gui/AlertView.hh>
#include "Recent.hh"
#include "RecentGameView.hh"

RecentGameView::RecentGameView(ViewAttachParams attach, RecentGameList &list):
	TableView
	{
		"Recent Games",
		attach,
		[this](const TableView &)
		{
			return 1 + recentGame.size();
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			return idx < recentGame.size() ? recentGame[idx] : clear;
		}
	},
	clear
	{
		"Clear List",
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Really clear the list?");
			ynAlertView->setOnYes(
				[this](TextMenuItem &, View &view, Input::Event)
				{
					view.dismiss();
					this->list.clear();
					popAndShow();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	list{list}
{
	recentGame.reserve(list.size());
	for(auto &e : list)
	{
		recentGame.emplace_back(e.name.data(),
			[&e](TextMenuItem &item, View &view, Input::Event ev)
			{
				e.handleMenuSelection(item, ev);
			});
		recentGame.back().setActive(FS::exists(e.path.data()));
	}
	clear.setActive(list.size());
}
