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
#include "InstallWindow.h"
#include "MainWindow.h"
#include "utils/StringTools.h"
//#include "fs/install.h"
//#include "dynamic_libs/os_functions.h"
//#include "utils/logger.h"

#define MCP_COMMAND_INSTALL_ASYNC   0x81
#define MAX_INSTALL_PATH_LENGTH     0x27F

static int installCompleted = 0;
static u32 installError = 0;

static int IosInstallCallback(unsigned int errorCode, unsigned int * priv_data)
{
    installError = errorCode;
    installCompleted = 1;
    return 0;
}

InstallWindow::InstallWindow(CFolderList * list)
	: GuiFrame(0, 0)
	, CThread(CThread::eAttributeAffCore0 | CThread::eAttributePinnedAff)
	, folderList(list)
{   
	mainWindow = Application::instance()->getMainWindow();
	
	folderCount = folderList->GetSelectedCount();
	
	if(folderCount > 0)
	{
		std::string message = fmt("%d application(s)", folderCount);
		messageBox = new MessageBox(MessageBox::BT_YESNO, MessageBox::IT_ICONQUESTION, false);
		messageBox->setTitle("Are you sure you want to install:");
		messageBox->setMessage1(message);
		messageBox->messageYesClicked.connect(this, &InstallWindow::OnValidInstallClick);
		messageBox->messageNoClicked.connect(this, &InstallWindow::OnCloseWindow);
	}
	else
	{
		messageBox = new MessageBox(MessageBox::BT_OK, MessageBox::IT_ICONEXCLAMATION, false);
		messageBox->setTitle("No content selected.");
		messageBox->setMessage1("Return to folder browser.");
		messageBox->messageOkClicked.connect(this, &InstallWindow::OnCloseWindow);
	}
	
	drcFrame = new GuiFrame(0, 0);
	drcFrame->setEffect(EFFECT_FADE, 10, 255);
	drcFrame->setState(GuiElement::STATE_DISABLED);
	drcFrame->effectFinished.connect(this, &InstallWindow::OnOpenEffectFinish);
	drcFrame->append(messageBox);
	
	//progressWindow = new ProgressWindow("title");
	//tvFrame = new GuiFrame(0, 0);
	//tvFrame->setPosition(0, -250);
	
	mainWindow->appendDrc(drcFrame);
	//mainWindow->appendTv(tvFrame);
}

InstallWindow::~InstallWindow()
{
	drcFrame->remove(messageBox);
	mainWindow->remove(drcFrame);
	//mainWindow->remove(tvFrame);
	delete drcFrame;
	//delete tvFrame;
	delete messageBox;
	//delete progressWindow;
}

void InstallWindow::OnValidInstallClick(GuiElement * element, int val)
{
	messageBox->messageYesClicked.disconnect(this);
	messageBox->messageNoClicked.disconnect(this);
	messageBox->reload("Where do you want to install?", "", "", MessageBox::BT_DEST, MessageBox::IT_ICONQUESTION);
	messageBox->messageYesClicked.connect(this, &InstallWindow::OnDestinationChoice);
	messageBox->messageNoClicked.connect(this, &InstallWindow::OnDestinationChoice);
}

void InstallWindow::OnDestinationChoice(GuiElement * element, int choice)
{
	if(choice == MessageBox::MR_YES)
		target = NAND;
	else
		target = USB;
	
	Application::instance()->exitDisable();
	
	messageBox->messageYesClicked.disconnect(this);
	messageBox->messageNoClicked.disconnect(this);
	
	startInstalling();
}

/*void InstallWindow::showTvProgress()
{
	progressWindow->setEffect(EFFECT_FADE, 10, 255);
	progressWindow->effectFinished.connect(this, &InstallWindow::OnOpenEffectFinish);
	
	tvFrame->append(progressWindow);
}*/

/*void InstallWindow::hideTvProgress()
{
	progressWindow->setEffect(EFFECT_FADE, -10, 255);
	progressWindow->effectFinished.connect(this, &InstallWindow::OnCloseTvProgressEffectFinish);
}*/

/*void InstallWindow::setTvProgressTitle(std::string title)
{
	progressWindow->setTitle(title);
}*/

/*void InstallWindow::OnCloseTvProgressEffectFinish(GuiElement * element)
{
	progressWindow->effectFinished.disconnect(this);
    tvFrame->remove(progressWindow);
}*/

void InstallWindow::executeThread()
{
	canceled = false;
	
	int total = folderList->GetSelectedCount();
	int pos = 1;
	
	while(pos <= total && !canceled)
	{
		InstallProcess(pos, total);
		
		if(pos < total)
		{
			int time = 6;
			u64 startTime = OSGetTime();
			u32 passedMs = 0;
			
			while(time && !canceled)
			{
				passedMs = (OSGetTime() - startTime) * 4000ULL / BUS_SPEED;
				
				if(passedMs >= 1000)
				{
					time--;
					startTime = OSGetTime();
					messageBox->setMessage2(fmt("Starting next installation in %d second(s)", time));
				}
			}
		}
		
		pos++;
	}
	
	Application::instance()->exitEnable();
}

void InstallWindow::InstallProcess(int pos, int total)
{
	int index = folderList->GetFirstSelected();
	
	std::string title = fmt("Installing... (%d/%d)", pos, total);
	std::string gameName = folderList->GetName(index);
	
	messageBox->reload(title, gameName, "", MessageBox::BT_NOBUTTON, MessageBox::IT_ICONINFORMATION, true, "0.0 %");
	
	//setTvProgressTitle(fmt("Installing %s", gameName.c_str()));
	//showTvProgress();
	
	/////////////////////////////
	// install process
	/////////////////////////////
	
	int result = 0;
	installCompleted = 1;
	installError = 0;

	//!---------------------------------------------------
    //! This part of code originates from Crediars MCP patcher assembly code
    //! it is just translated to C
    //!---------------------------------------------------
    unsigned int mcpHandle = MCP_Open();
    if(mcpHandle == 0)
    {
        messageBox->reload("Install failed", gameName, "Failed to open MCP.", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
		
        result = -1;
    }
	else
	{
		unsigned int * mcpInstallInfo = (unsigned int *)OSAllocFromSystem(0x24, 0x40);
		char * mcpInstallPath = (char *)OSAllocFromSystem(MAX_INSTALL_PATH_LENGTH, 0x40);
		unsigned int * mcpPathInfoVector = (unsigned int *)OSAllocFromSystem(0x0C, 0x40);
		
		do
		{
			if(!mcpInstallInfo || !mcpInstallPath || !mcpPathInfoVector)
			{
				messageBox->reload("Install failed", gameName, "Could not allocate memory.", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
				result = -2;
				break;
			}
			
			std::string installFolder = folderList->GetPath(index);
			installFolder.erase(0, 4);
			installFolder.insert(0, "vol/app_sd/");
			
			log_printf("installFolder = %s\n", installFolder.c_str());
			
			int res = MCP_InstallGetInfo(mcpHandle, installFolder.c_str(), mcpInstallInfo);
			if(res != 0)
			{
				//__os_snprintf(errorText1, sizeof(errorText1), "Error: MCP_InstallGetInfo 0x%08X", MCP_GetLastRawError());
				//__os_snprintf(errorText2, sizeof(errorText2), "Confirm complete WUP files are in the folder. Try power down.");
				messageBox->reload("Install failed", gameName, "Confirm complete WUP files are in the folder.", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
				result = -3;
				break;
			}
			
			u32 titleIdHigh = mcpInstallInfo[0];
			u32 titleIdLow = mcpInstallInfo[1];
			bool spoofFiles = false;
			if ((titleIdHigh == 00050010)
				&&(	   (titleIdLow == 0x10041000)     // JAP Version.bin
					|| (titleIdLow == 0x10041100)     // USA Version.bin
					|| (titleIdLow == 0x10041200)))   // EUR Version.bin
			{
				spoofFiles = true;
				target = NAND;
			}
			
			if (spoofFiles
			   || (titleIdHigh == 0x0005000E)     // game update
			   || (titleIdHigh == 0x00050000)     // game
			   || (titleIdHigh == 0x0005000C)     // DLC
			   || (titleIdHigh == 0x00050002))    // Demo
			{
				//installedTitle = ((u64)titleIdHigh << 32ULL) | titleIdLow;
				
				res = MCP_InstallSetTargetDevice(mcpHandle, target);
				if(res != 0)
				{
					//__os_snprintf(errorText1, sizeof(errorText1), "Error: MCP_InstallSetTargetDevice 0x%08X", MCP_GetLastRawError());
					//if (installToUsb)
					//	__os_snprintf(errorText2, sizeof(errorText2), "Possible USB HDD disconnected or failure");
					messageBox->reload("Install failed", gameName, fmt("MCP_InstallSetTargetDevice 0x%08X", MCP_GetLastRawError()), MessageBox::BT_OK, MessageBox::IT_ICONERROR);
					result = -5;
					break;
				}
				res = MCP_InstallSetTargetUsb(mcpHandle, target);
				if(res != 0)
				{
					//__os_snprintf(errorText1, sizeof(errorText1), "Error: MCP_InstallSetTargetUsb 0x%08X", MCP_GetLastRawError());
					//if (installToUsb)
					//	__os_snprintf(errorText2, sizeof(errorText2), "Possible USB HDD disconnected or failure");
					messageBox->reload("Install failed", gameName, fmt("MCP_InstallSetTargetUsb 0x%08X", MCP_GetLastRawError()), MessageBox::BT_OK, MessageBox::IT_ICONERROR);
					result = -6;
					break;
				}
				
				mcpInstallInfo[2] = (unsigned int)MCP_COMMAND_INSTALL_ASYNC;
				mcpInstallInfo[3] = (unsigned int)mcpPathInfoVector;
				mcpInstallInfo[4] = (unsigned int)1;
				mcpInstallInfo[5] = (unsigned int)0;
				
				memset(mcpInstallPath, 0, MAX_INSTALL_PATH_LENGTH);
				__os_snprintf(mcpInstallPath, MAX_INSTALL_PATH_LENGTH, installFolder.c_str());
				memset(mcpPathInfoVector, 0, 0x0C);
				
				mcpPathInfoVector[0] = (unsigned int)mcpInstallPath;
				mcpPathInfoVector[1] = (unsigned int)MAX_INSTALL_PATH_LENGTH;
				
				installCompleted = 0;
				res = IOS_IoctlvAsync(mcpHandle, MCP_COMMAND_INSTALL_ASYNC, 1, 0, mcpPathInfoVector, (void*)IosInstallCallback, mcpInstallInfo);
				if(res != 0)
				{
					messageBox->reload("Install failed", gameName, fmt("MCP_InstallTitleAsync 0x%08X", MCP_GetLastRawError()), MessageBox::BT_OK, MessageBox::IT_ICONERROR);
					result = -7;
					break;
				}
				
				while(!installCompleted)
				{
					memset(mcpInstallInfo, 0, 0x24);
					
					/*res = */MCP_InstallGetProgress(mcpHandle, mcpInstallInfo);
					
					if(mcpInstallInfo[0] == 1)
					{
						u64 totalSize = ((u64)mcpInstallInfo[3] << 32ULL) | mcpInstallInfo[4];
						u64 installedSize = ((u64)mcpInstallInfo[5] << 32ULL) | mcpInstallInfo[6];
						int percent = (totalSize != 0) ? ((installedSize * 100.0f) / totalSize) : 0;
						
						messageBox->setProgress(percent);
						messageBox->setProgressBarInfo(fmt("%0.1f / %0.1f MB (%i%%)", installedSize / (1024.0f * 1024.0f), totalSize / (1024.0f * 1024.0f), percent));
					}
					
					usleep(50000);
				}
				
				if(installError != 0)
				{
					if ((installError == 0xFFFCFFE9) && (target == USB))
					{
						messageBox->reload("Install failed", gameName, fmt("0x%08X access failed (no USB storage attached?)", installError), MessageBox::BT_OK, MessageBox::IT_ICONERROR);
						result = -8;
					}
					else
					{
						//__os_snprintf(errorText1, sizeof(errorText1), "Error: install error code 0x%08X", installError);
						if (installError == 0xFFFBF446 || installError == 0xFFFBF43F)
							messageBox->reload("Install failed", gameName, "Possible missing or bad title.tik file", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
						else if (installError == 0xFFFBF441)
							messageBox->reload("Install failed", gameName, "Possible incorrect console for DLC title.tik file", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
						else if (installError == 0xFFFCFFE4)
							messageBox->reload("Install failed", gameName, "Possible not enough memory on target device", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
						else if (installError == 0xFFFFF825)
							messageBox->reload("Install failed", gameName, "Possible bad SD card.  Reformat (32k blocks) or replace", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
						else if ((installError & 0xFFFF0000) == 0xFFFB0000)
							messageBox->reload("Install failed", gameName, "Verify WUP files are correct & complete. DLC/E-shop require Sig Patch", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
						
						result = -9;
					}
				}
			}
			else
			{
				messageBox->reload("Install failed", gameName, "Not a game, game update, DLC, demo or version title", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
				result = -4;
			}
		}
		while(0);

		MCP_Close(mcpHandle);
		if(mcpPathInfoVector)
			OSFreeToSystem(mcpPathInfoVector);
		if(mcpInstallPath)
			OSFreeToSystem(mcpInstallPath);
		if(mcpInstallInfo)
			OSFreeToSystem(mcpInstallInfo);
	}
	/////////////////////////////
	
	//hideTvProgress();
	
	if(result >= 0)
	{
		if(pos == total)
		{
			messageBox->reload("Succesfully installed", gameName, "", MessageBox::BT_OK, MessageBox::IT_ICONTRUE);
			messageBox->messageOkClicked.connect(this, &InstallWindow::OnCloseWindow);
		}
		else
		{
			messageBox->reload("Succesfully installed", gameName, "Starting next installation in 6 second(s)", MessageBox::BT_CANCEL, MessageBox::IT_ICONTRUE);
			messageBox->messageCancelClicked.connect(this, &InstallWindow::OnInstallProcessCancel);
		}
		
		folderList->UnSelect(index);
	}
	else
	{
		//messageBox->reload("Install failed", gameName, "", MessageBox::BT_OK, MessageBox::IT_ICONERROR);
		messageBox->messageOkClicked.connect(this, &InstallWindow::OnCloseWindow);
		
		canceled = true;
		folderList->UnSelectAll();
	}
}

void InstallWindow::OnInstallProcessCancel(GuiElement *element, int val)
{
	canceled = true;
	folderList->UnSelectAll();
	OnCloseWindow(this, 0);
}

void InstallWindow::OnCloseWindow(GuiElement * element, int val)
{
	messageBox->setEffect(EFFECT_FADE, -10, 255);
    messageBox->setState(GuiElement::STATE_DISABLED);
    messageBox->effectFinished.connect(this, &InstallWindow::OnWindowClosed);
}

void InstallWindow::OnWindowClosed(GuiElement *element)
{
	messageBox->effectFinished.disconnect(this);
	installWindowClosed(this);
	AsyncDeleter::pushForDelete(this);
}

void InstallWindow::OnOpenEffectFinish(GuiElement *element)
{
	element->effectFinished.disconnect(this);
    element->clearState(GuiElement::STATE_DISABLED);
}

void InstallWindow::OnCloseEffectFinish(GuiElement *element)
{
	remove(element);
    AsyncDeleter::pushForDelete(element);
}
