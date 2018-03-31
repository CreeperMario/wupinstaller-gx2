#ifndef REFRESH_WINDOW_H_
#define REFRESH_WINDOW_H_

#include "fs/CFolderList.hpp"
#include "gui/MessageBox.h"
#include "ProgressWindow.h"

class MainWindow;

class RefreshWindow : public GuiFrame, public CThread, public sigslot::has_slots<>
{
public:
	RefreshWindow(CFolderList * list);
	~RefreshWindow();
	
	sigslot::signal1<GuiElement *> refreshWindowClosed;
	
private:

	void OnCloseWindow(GuiElement * element, int val);
	void OnWindowClosed(GuiElement * element);
	void OnOpenEffectFinish(GuiElement *element);
	void OnCloseEffectFinish(GuiElement *element);

	GuiFrame * drcFrame;
	GuiFrame * tvFrame;
	
	CFolderList * folderList;
	
	MessageBox * messageBox;

	
	MainWindow * mainWindow;
	
	int folderCount;
	bool canceled;
	int target;	
};

#endif
