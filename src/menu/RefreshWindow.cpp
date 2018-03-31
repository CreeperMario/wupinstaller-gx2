/****************************************************************************
 * Copyright (C) 2011 Dimok
 * Copyright (C) 2012 Cyan
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "Application.h"
#include "RefreshWindow.h"
#include "MainWindow.h"
#include "utils/StringTools.h"
#include "system/power.h"
#include "common/retain_vars.h"
#include "common/common.h"


RefreshWindow::RefreshWindow(CFolderList * list)
	: GuiFrame(0, 0)
	, CThread(CThread::eAttributeAffCore0 | CThread::eAttributePinnedAff)
	, folderList(list)
{   
	mainWindow = Application::instance()->getMainWindow();
	
	folderCount = folderList->GetSelectedCount();
	

	if(folderCount > 0)
	{
		messageBox = new MessageBox(MessageBox::BT_OK, MessageBox::IT_ICONEXCLAMATION, false);
		messageBox->setTitle("Refreshed!");
		messageBox->setMessage1("Return to folder browser.");
		messageBox->messageOkClicked.connect(this, &RefreshWindow::OnCloseWindow);
	}
	else
	{
		messageBox = new MessageBox(MessageBox::BT_OK, MessageBox::IT_ICONEXCLAMATION, false);
		messageBox->setTitle("Refreshed!");
		messageBox->setMessage1("Return to folder browser.");
		messageBox->messageOkClicked.connect(this, &RefreshWindow::OnCloseWindow);
	}
	
	drcFrame = new GuiFrame(0, 0);
	drcFrame->setEffect(EFFECT_FADE, 10, 255);
	drcFrame->setState(GuiElement::STATE_DISABLED);
	drcFrame->effectFinished.connect(this, &RefreshWindow::OnOpenEffectFinish);
	drcFrame->append(messageBox);
	

	tvFrame = new GuiFrame(0, 0);
	tvFrame->setPosition(0, -250);
	
	mainWindow->appendDrc(drcFrame);
	mainWindow->appendTv(tvFrame);
}

RefreshWindow::~RefreshWindow()
{
	drcFrame->remove(messageBox);
	mainWindow->remove(drcFrame);
	mainWindow->remove(tvFrame);
	delete drcFrame;
	delete tvFrame;
	delete messageBox;
}


void RefreshWindow::OnCloseWindow(GuiElement * element, int val)
{
	messageBox->setEffect(EFFECT_FADE, -10, 255);
	messageBox->setState(GuiElement::STATE_DISABLED);
	messageBox->effectFinished.connect(this, &RefreshWindow::OnWindowClosed);
}

void RefreshWindow::OnWindowClosed(GuiElement *element)
{
	messageBox->effectFinished.disconnect(this);
	refreshWindowClosed(this);
	
	AsyncDeleter::pushForDelete(this);
}

void RefreshWindow::OnOpenEffectFinish(GuiElement *element)
{
	element->effectFinished.disconnect(this);
	element->clearState(GuiElement::STATE_DISABLED);
}

void RefreshWindow::OnCloseEffectFinish(GuiElement *element)
{
	remove(element);
	AsyncDeleter::pushForDelete(element);
}
