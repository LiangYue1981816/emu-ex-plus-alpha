/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/FilePicker.hh>
#include <imagine/base/Base.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextTableView.hh>
#include "internal.hh"

extern "C"
{
	#include <blueMSX/IoDevice/Disk.h>
}

static std::vector<FS::FileString> machinesNames(const char *basePath)
{
	std::vector<FS::FileString> machineName{};
	auto machinePath = FS::makePathStringPrintf("%s/Machines", basePath);
	for(auto &entry : FS::directory_iterator{machinePath})
	{
		auto configPath = FS::makePathStringPrintf("%s/%s/config.ini", machinePath.data(), entry.name());
		if(!FS::exists(configPath))
		{
			//logMsg("%s doesn't exist", configPath.data());
			continue;
		}
		machineName.emplace_back(FS::makeFileString(entry.name()));
		logMsg("found machine:%s", entry.name());
	}
	std::sort(machineName.begin(), machineName.end(),
		[](FS::FileString n1, FS::FileString n2)
		{
			return FS::fileStringNoCaseLexCompare(n1, n2);
		});
	return machineName;
}

static int machineIndex(std::vector<FS::FileString> &name, FS::FileString searchName)
{
	int currentMachineIdx =
		std::find(name.begin(), name.end(), searchName) - name.begin();
	if(currentMachineIdx != (int)name.size())
	{
		//logMsg("current machine is idx %d", currentMachineIdx);
		return(currentMachineIdx);
	}
	else
	{
		return -1;
	}
}

template <size_t S>
static void printInstallFirmwareFilesStr(char (&str)[S])
{
	FS::PathString basenameTemp;
	string_printf(str, "Install the C-BIOS BlueMSX machine files to: %s", machineBasePath.data());
}

void installFirmwareFiles()
{
	std::error_code ec{};
	FS::create_directory(machineBasePath, ec);
	if(ec && ec.value() != (int)std::errc::file_exists)
	{
		EmuApp::printfMessage(4, 1, "Can't create directory:\n%s", machineBasePath.data());
		return;
	}

	const char *dirsToCreate[] =
	{
		"Machines", "Machines/MSX - C-BIOS",
		"Machines/MSX2 - C-BIOS", "Machines/MSX2+ - C-BIOS"
	};

	for(auto e : dirsToCreate)
	{
		auto pathTemp = FS::makePathStringPrintf("%s/%s", machineBasePath.data(), e);
		std::error_code ec{};
		FS::create_directory(pathTemp, ec);
		if(ec && ec.value() != (int)std::errc::file_exists)
		{
			EmuApp::printfMessage(4, 1, "Can't create directory:\n%s", pathTemp.data());
			return;
		}
	}

	const char *srcPath[] =
	{
		"cbios.txt", "cbios.txt", "cbios.txt",
		"cbios_logo_msx1.rom", "cbios_main_msx1.rom", "config1.ini",
		"cbios_logo_msx2.rom", "cbios_main_msx2.rom", "cbios_sub.rom", "config2.ini",
		"cbios_logo_msx2+.rom", "cbios_main_msx2+.rom", "cbios_sub.rom", "cbios_music.rom", "config3.ini"
	};
	const char *destDir[] =
	{
			"MSX - C-BIOS", "MSX2 - C-BIOS", "MSX2+ - C-BIOS",
			"MSX - C-BIOS", "MSX - C-BIOS", "MSX - C-BIOS",
			"MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS", "MSX2 - C-BIOS",
			"MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS", "MSX2+ - C-BIOS"
	};

	for(auto &e : srcPath)
	{
		auto src = EmuApp::openAppAssetIO(e, IO::AccessHint::ALL);
		if(!src)
		{
			EmuApp::printfMessage(4, 1, "Can't open source file:\n %s", e);
			return;
		}
		auto e_i = &e - srcPath;
		auto pathTemp = FS::makePathStringPrintf("%s/Machines/%s/%s",
				machineBasePath.data(), destDir[e_i], strstr(e, "config") ? "config.ini" : e);
		if(FileUtils::writeToPath(pathTemp.data(), src) == -1)
		{
			EmuApp::printfMessage(4, 1, "Can't write file:\n%s", e);
			return;
		}
	}

	setDefaultMachineName("MSX2 - C-BIOS");
	EmuApp::postMessage("Installation OK");
}

class CustomSystemOptionView : public SystemOptionView
{
private:
	std::vector<FS::FileString> msxMachineName{};
	std::vector<TextMenuItem> msxMachineItem{};

	MultiChoiceMenuItem msxMachine
	{
		"Default Machine Type",
		[](int idx) -> const char*
		{
			if(idx == -1)
				return "None";
			else
				return nullptr;
		},
		0,
		msxMachineItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			if(!msxMachineItem.size())
			{
				EmuApp::printfMessage(4, 1, "Place machine directory in:\n%s", machineBasePath.data());
				return;
			}
			item.defaultOnSelect(view, e);
		}
	};

	void reloadMachineItem()
	{
		msxMachineItem.clear();
		msxMachineName = std::move(machinesNames(machineBasePath.data()));
		for(const auto &name : msxMachineName)
		{
			msxMachineItem.emplace_back(name.data(),
			[](TextMenuItem &item, View &, Input::Event)
			{
				setDefaultMachineName(item.name());
				logMsg("set machine type: %s", item.name());
			});
		}
		msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(optionDefaultMachineName)));
	}

	char installFirmwareFilesStr[512]{};

	TextMenuItem installCBIOS
	{
		"Install MSX C-BIOS",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			printInstallFirmwareFilesStr(installFirmwareFilesStr);
			auto ynAlertView = makeView<YesNoAlertView>(installFirmwareFilesStr);
			ynAlertView->setOnYes(
				[](TextMenuItem &, View &view, Input::Event e)
				{
					view.dismiss();
					installFirmwareFiles();
				});
			EmuApp::pushAndShowModalView(std::move(ynAlertView), e);
		}
	};

	BoolMenuItem skipFdcAccess
	{
		"Fast-forward Disk IO",
		(bool)optionSkipFdcAccess,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			optionSkipFdcAccess = item.flipBoolValue(*this);
		}
	};

	template <size_t S>
	static void printMachinePathMenuEntryStr(char (&str)[S])
	{
		string_printf(str, "System/BIOS Path: %s", strlen(machineCustomPath.data()) ? FS::basename(machineCustomPath).data() : "Default");
	}

	char machineFilePathStr[256]{};

	TextMenuItem machineFilePath
	{
		machineFilePathStr,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			pushAndShowFirmwarePathMenu("System/BIOS Path", e);
			postDraw();
		}
	};

	void onFirmwarePathChange(const char *path, Input::Event e) final
	{
		printMachinePathMenuEntryStr(machineFilePathStr);
		machineFilePath.compile(renderer(), projP);
		machineBasePath = makeMachineBasePath(machineCustomPath);
		reloadMachineItem();
		msxMachine.compile(renderer(), projP);
		if(!strlen(path))
		{
			EmuApp::printfMessage(4, false, "Using default path:\n%s/MSX.emu", (Config::envIsLinux && !Config::MACHINE_IS_PANDORA) ? EmuApp::assetPath().data() : Base::sharedStoragePath().data());
		}
	}

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		reloadMachineItem();
		item.emplace_back(&skipFdcAccess);
		item.emplace_back(&msxMachine);
		printMachinePathMenuEntryStr(machineFilePathStr);
		item.emplace_back(&machineFilePath);
		if(canInstallCBIOS)
		{
			item.emplace_back(&installCBIOS);
		}
	}
};

class MsxMediaFilePicker
{
public:
	enum { ROM, DISK, TAPE };

	static EmuSystem::NameFilterFunc fsFilter(uint type)
	{
		EmuSystem::NameFilterFunc filter = hasMSXROMExtension;
		if(type == DISK)
			filter = hasMSXDiskExtension;
		else if(type == TAPE)
			filter = hasMSXTapeExtension;
		return filter;
	}
};

static const char *insertEjectDiskMenuStr[] {"Insert File", "Eject"};

class MsxIOControlView : public TableView
{
public:
	static const char *hdSlotPrefix[4];
	char hdSlotStr[4][1024]{};

	void updateHDText(int slot)
	{
		string_printf(hdSlotStr[slot], "%s %s", hdSlotPrefix[slot], hdName[slot].data());
	}

	void updateHDStatusFromCartSlot(int cartSlot)
	{
		int hdSlotStart = cartSlot == 0 ? 0 : 2;
		bool isActive = boardGetHdType(cartSlot) == HD_SUNRISEIDE;
		hdSlot[hdSlotStart].setActive(isActive);
		hdSlot[hdSlotStart+1].setActive(isActive);
		updateHDText(hdSlotStart);
		updateHDText(hdSlotStart+1);
	}

	void onHDMediaChange(const char *name, int slot)
	{
		string_copy(hdName[slot], name);
		updateHDText(slot);
		hdSlot[slot].compile(renderer(), projP);
	}

	void addHDFilePickerView(Input::Event e, uint8_t slot)
	{
		auto fPicker = EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				auto id = diskGetHdDriveId(slot / 2, slot % 2);
				logMsg("inserting hard drive id %d", id);
				if(insertDisk(name, id))
				{
					onHDMediaChange(name, slot);
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectHD(TextMenuItem &item, Input::Event e, uint8_t slot)
	{
		if(!item.active())
			return;
		if(strlen(hdName[slot].data()))
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Hard Drive", std::size(insertEjectDiskMenuStr));
			multiChoiceView->appendItem(insertEjectDiskMenuStr[0],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addHDFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView->appendItem(insertEjectDiskMenuStr[1],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					diskChange(diskGetHdDriveId(slot / 2, slot % 2), 0, 0);
					onHDMediaChange("", slot);
					popAndShow();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
		else
		{
			addHDFilePickerView(e, slot);
			window().postDraw();
		}
	}

	TextMenuItem hdSlot[4]
	{
		{hdSlotStr[0], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 0); }},
		{hdSlotStr[1], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 1); }},
		{hdSlotStr[2], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 2); }},
		{hdSlotStr[3], [this](TextMenuItem &item, View &, Input::Event e) { onSelectHD(item, e, 3); }}
	};

	static const char *romSlotPrefix[2];
	char romSlotStr[2][1024]{};

	void updateROMText(int slot)
	{
		string_printf(romSlotStr[slot], "%s %s", romSlotPrefix[slot], cartName[slot].data());
	}

	void onROMMediaChange(const char *name, int slot)
	{
		string_copy(cartName[slot], name);
		updateROMText(slot);
		romSlot[slot].compile(renderer(), projP);
		updateHDStatusFromCartSlot(slot);
	}

	void addROMFilePickerView(Input::Event e, uint8_t slot)
	{
		auto fPicker = EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::ROM),
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				if(insertROM(name, slot))
				{
					onROMMediaChange(name, slot);
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectROM(Input::Event e, uint8_t slot)
	{
		auto multiChoiceView = makeViewWithName<TextTableView>("ROM Cartridge Slot", 5);
		multiChoiceView->appendItem("Insert File",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				addROMFilePickerView(e, slot);
				postDraw();
				popAndShow();
			});
		multiChoiceView->appendItem("Eject",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_UNKNOWN, 0, 0);
				onROMMediaChange("", slot);
				popAndShow();
			});
		multiChoiceView->appendItem("Insert SCC",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCC, "", 0);
				onROMMediaChange("SCC", slot);
				popAndShow();
			});
		multiChoiceView->appendItem("Insert SCC+",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				boardChangeCartridge(slot, ROM_SCCPLUS, "", 0);
				onROMMediaChange("SCC+", slot);
				popAndShow();
			});
		multiChoiceView->appendItem("Insert Sunrise IDE",
			[this, slot](TextMenuItem &, View &, Input::Event e)
			{
				if(!boardChangeCartridge(slot, ROM_SUNRISEIDE, "Sunrise IDE", 0))
				{
					EmuApp::postMessage(true, "Error loading Sunrise IDE device");
				}
				else
					onROMMediaChange("Sunrise IDE", slot);
				popAndShow();
			});
		pushAndShow(std::move(multiChoiceView), e);
	}

	TextMenuItem romSlot[2]
	{
		{romSlotStr[0], [this](TextMenuItem &, View &, Input::Event e) { onSelectROM(e, 0); }},
		{romSlotStr[1], [this](TextMenuItem &, View &, Input::Event e) { onSelectROM(e, 1); }}
	};

	static const char *diskSlotPrefix[2];
	char diskSlotStr[2][1024]{};

	void updateDiskText(int slot)
	{
		string_printf(diskSlotStr[slot], "%s %s", diskSlotPrefix[slot], diskName[slot].data());
	}

	void onDiskMediaChange(const char *name, int slot)
	{
		string_copy(diskName[slot], name);
		updateDiskText(slot);
		diskSlot[slot].compile(renderer(), projP);
	}

	void addDiskFilePickerView(Input::Event e, uint8_t slot)
	{
		auto fPicker = EmuFilePicker::makeForMediaChange(attachParams(), e, EmuSystem::gamePath(),
			MsxMediaFilePicker::fsFilter(MsxMediaFilePicker::DISK),
			[this, slot](FSPicker &picker, const char* name, Input::Event e)
			{
				logMsg("inserting disk in slot %d", slot);
				if(insertDisk(name, slot))
				{
					onDiskMediaChange(name, slot);
				}
				picker.dismiss();
			});
		EmuApp::pushAndShowModalView(std::move(fPicker), e);
	}

	void onSelectDisk(Input::Event e, uint8_t slot)
	{
		if(strlen(diskName[slot].data()))
		{
			auto multiChoiceView = makeViewWithName<TextTableView>("Disk Drive", std::size(insertEjectDiskMenuStr));
			multiChoiceView->appendItem(insertEjectDiskMenuStr[0],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					addDiskFilePickerView(e, slot);
					postDraw();
					popAndShow();
				});
			multiChoiceView->appendItem(insertEjectDiskMenuStr[1],
				[this, slot](TextMenuItem &, View &, Input::Event e)
				{
					diskChange(slot, 0, 0);
					onDiskMediaChange("", slot);
					popAndShow();
				});
			pushAndShow(std::move(multiChoiceView), e);
		}
		else
		{
			addDiskFilePickerView(e, slot);
		}
		window().postDraw();
	}

	TextMenuItem diskSlot[2]
	{
		{diskSlotStr[0], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 0); }},
		{diskSlotStr[1], [this](TextMenuItem &, View &, Input::Event e) { onSelectDisk(e, 1); }}
	};

	StaticArrayList<MenuItem*, 9> item{};

public:
	MsxIOControlView(ViewAttachParams attach):
		TableView
		{
			"IO Control",
			attach,
			[this](const TableView &)
			{
				return item.size();
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				return *item[idx];
			}
		}
	{
		iterateTimes(2, slot)
		{
			updateROMText(slot);
			romSlot[slot].setActive((int)slot < boardInfo.cartridgeCount);
			item.emplace_back(&romSlot[slot]);
		}
		iterateTimes(2, slot)
		{
			updateDiskText(slot);
			diskSlot[slot].setActive((int)slot < boardInfo.diskdriveCount);
			item.emplace_back(&diskSlot[slot]);
		}
		iterateTimes(4, slot)
		{
			updateHDText(slot);
			hdSlot[slot].setActive(boardGetHdType(slot/2) == HD_SUNRISEIDE);
			item.emplace_back(&hdSlot[slot]);
		}
	}
};

const char *MsxIOControlView::romSlotPrefix[2] {"ROM1:", "ROM2:"};
const char *MsxIOControlView::diskSlotPrefix[2] {"Disk1:", "Disk2:"};
const char *MsxIOControlView::hdSlotPrefix[4] {"IDE1-M:", "IDE1-S:", "IDE2-M:", "IDE2-S:"};

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
	TextMenuItem msxIOControl
	{
		"ROM/Disk Control",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active())
			{
				pushAndShow(makeView<MsxIOControlView>(), e);
			}
			else if(EmuSystem::gameIsRunning() && activeBoardType != BOARD_MSX)
			{
				EmuApp::postMessage(2, false, "Only used in MSX mode");
			}
		}
	};

	std::vector<FS::FileString> msxMachineName{};
	std::vector<TextMenuItem> msxMachineItem{};

	MultiChoiceMenuItem msxMachine
	{
		"Machine Type",
		[this](int idx) -> const char*
		{
			if(idx == -1)
				return "None";
			else
				return nullptr;
		},
		0,
		msxMachineItem,
		[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
		{
			if(!msxMachineItem.size())
			{
				return;
			}
			item.defaultOnSelect(view, e);
		}
	};

	void reloadMachineItem()
	{
		msxMachineItem.clear();
		msxMachineName = std::move(machinesNames(machineBasePath.data()));
		for(const auto &name : msxMachineName)
		{
			msxMachineItem.emplace_back(name.data(),
			[this](TextMenuItem &item, View &, Input::Event e)
			{
				auto ynAlertView = makeView<YesNoAlertView>("Change machine type and reset emulation?");
				ynAlertView->setOnYes(
					[this, name = item.name()](TextMenuItem &, View &view, Input::Event)
					{
						if(auto err = setCurrentMachineName(name);
							err)
						{
							EmuApp::printfMessage(3, true, "%s", err->what());
							view.dismiss();
							return;
						}
						auto machineName = currentMachineName();
						strcpy(optionMachineName.val, machineName);
						msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(machineName)));
						EmuSystem::sessionOptionSet();
						view.dismiss();
						popAndShow();
					});
				EmuApp::pushAndShowModalView(std::move(ynAlertView), e);
				return false;
			});
		}
		msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(currentMachineName())));
	}

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&msxIOControl);
		reloadMachineItem();
		item.emplace_back(&msxMachine);
		loadStandardItems();
	}

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		reloadItems();
	}

	void onShow()
	{
		EmuSystemActionsView::onShow();
		msxIOControl.setActive(EmuSystem::gameIsRunning() && activeBoardType == BOARD_MSX);
		msxMachine.setSelected(machineIndex(msxMachineName, FS::makeFileString(currentMachineName())));
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		default: return nullptr;
	}
}
