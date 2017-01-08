/****************************************************************************
 * Copyright (C) 2011
 * by Dj_Skual
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 * CFolderList.cpp
 *
 * for SaveGame Manager GX 2011
 ***************************************************************************/
#include "CFolderList.hpp"
#include "DirList.h"

void CFolderList::AddFolder()
{
	FolderStruct * newFolder = new FolderStruct;
	newFolder->name = "";
	newFolder->path = "";
	newFolder->selected = false;
	
	Folders.push_back(newFolder);
}

std::string CFolderList::GetName(int ind)
{
	if(ind < 0 || ind >= (int) Folders.size())
		return "";

	return Folders.at(ind)->name;
}

std::string CFolderList::GetPath(int ind)
{
	if(ind < 0 || ind >= (int) Folders.size())
		return "";

	return Folders.at(ind)->path;
}

bool CFolderList::IsSelected(int ind)
{
	if(ind < 0 || ind >= (int) Folders.size())
		return false;

	return Folders.at(ind)->selected;
}

void CFolderList::Select(int ind)
{
	if(ind < 0 || ind >= (int) Folders.size())
		return;

	Folders.at(ind)->selected = true;
}

void CFolderList::UnSelect(int ind)
{
	if(ind < 0 || ind >= (int) Folders.size())
		return;

	Folders.at(ind)->selected = false;
}

void CFolderList::SelectAll()
{
	if(!Folders.size())
		return;
	
	for(u32 i = 0; i < Folders.size(); i++)
		Folders.at(i)->selected = true;
}

void CFolderList::UnSelectAll()
{
	if(!Folders.size())
		return;
	
	for(u32 i = 0; i < Folders.size(); i++)
		Folders.at(i)->selected = false;
}

int CFolderList::GetFirstSelected()
{
	if(!Folders.size())
		return -1;
	
	int found = -1;
	for(u32 i = 0; i < Folders.size() && found < 0; i++)
	{
		if(Folders.at(i)->selected)
			found = i;
	}
	
	return found;
}

void CFolderList::Click(int ind)
{
	if(ind < 0 || ind >= (int) Folders.size())
		return;

	Folders.at(ind)->selected = !Folders.at(ind)->selected;
}

void CFolderList::Reset()
{
	//for(u32 i = 0; i < Folders.size(); i++)
		//delete Folders.at(i);

	Folders.clear();
}

int CFolderList::GetSelectedCount()
{
	int selectedCount = 0;
	
	for(u32 i = 0; i < Folders.size(); i++)
	{
		if(Folders.at(i)->selected)
			selectedCount++;
	}
	
	return selectedCount;
}

int CFolderList::Get()
{
	Reset();
	
	DirList dir("sd:/install", NULL, DirList::Dirs);
	
	int cnt = dir.GetFilecount();
	if(cnt > 0)
	{
		for(int i = 0; i < cnt; i++)
		{
			AddFolder();
			Folders.at(i)->name = dir.GetFilename(i);
			Folders.at(i)->path = dir.GetFilepath(i);
			Folders.at(i)->selected = false;
		}
	}
	else
	{
		dir.LoadPath("sd:/install", ".tik", DirList::Files);
		
		cnt = dir.GetFilecount();
		if(cnt > 0)
		{
			AddFolder();
			Folders.at(0)->name = "install";
			Folders.at(0)->path = "sd:/install";
			Folders.at(0)->selected = false;
		}
	}
	
	return Folders.size();
}
