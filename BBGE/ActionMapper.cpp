/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.
*/

#include "ActionMapper.h"
#include "Core.h"

#include <algorithm>

ActionMapper::ActionMapper()
{
	cleared = false;
	inputEnabled = true;
	inUpdate = false;
}

ActionMapper::~ActionMapper()
{
	clearCreatedEvents();
}

ActionData *ActionMapper::getActionDataByID(int actionID)
{
	for (auto& data: actionData)
	{
		if (data.id == actionID) return &data;
	}
	return 0;
}

bool ActionMapper::isActing(int actionID)
{
	ActionData *ad = getActionDataByID(actionID);
	if (ad)
	{
		for (auto& button: ad->buttonList)
		{
			if (keyDownMap[button]) return true;
		}
	}
	return false;
}

void ActionMapper::addAction (int actionID, int k)
{
	ActionData *ad = getActionDataByID(actionID);

	if (ad)
	{

	}
	else
	{
		ActionData data;
		data.id = actionID;
		actionData.push_back(data);
		ad = getActionDataByID(actionID);
		if (!ad)
		{
			std::ostringstream os;
			os << "Could not create action for Action ID [" << actionID << "]";
			errorLog(os.str());
			return;
		}
	}

	if (ad)
	{
		if(std::find(ad->buttonList.begin(), ad->buttonList.end(), k) == ad->buttonList.end())
			ad->buttonList.push_back(k);
		keyDownMap[k] = core->getKeyState(k);
	}
}



void ActionMapper::addAction(Event *event, int k, int state)
{
	ActionData data;
	data.event = event;
	data.state = state;
	data.buttonList.push_back(k);
	actionData.push_back(data);

	keyDownMap[k] = core->getKeyState(k);
}

Event* ActionMapper::addCreatedEvent(Event *event)
{
	for (auto& e: createdEvents)
	{
		if (e == event) return event;
	}
	createdEvents.push_back(event);
	return event;
}

void ActionMapper::clearCreatedEvents()
{
	for (auto& e: createdEvents) delete e;
	createdEvents.clear();
}



void ActionMapper::enableInput()
{
	inputEnabled = true;
}

void ActionMapper::disableInput()
{
	inputEnabled = false;
}

void ActionMapper::removeAction(int actionID)
{
	ActionData *ad = getActionDataByID(actionID);
	if (ad)
	{
		for (auto& button: ad->buttonList)
		{
			cleared = true; // it's a hack, but it works
			keyDownMap.erase(button);
		}
		for (ActionDataSet::iterator i = actionData.begin(); i != actionData.end();)
		{
			if (i->id == actionID)
				i = actionData.erase(i);
			else
				i++;
		}
	}
}



bool ActionMapper::pollAction(int actionID)
{
	bool down = false;

	ActionData *ad = getActionDataByID(actionID);
	if (ad)
	{
		ButtonList *blist = &ad->buttonList;
		ButtonList::iterator j;
		j = blist->begin();

		for (; j != blist->end(); j++)
		{
			if (getKeyState((*j)))
			{
				down = true;
				break;
			}
		}
	}

	return down;
}

bool ActionMapper::getKeyState(int k)
{
	bool keyState = false;
	if (k == KEY_ANYKEY)
	{
		keyState = false;
		for (int i = 0; i < KEY_MAXARRAY; i ++)
		{
			if (core->getKeyState(i))
			{
				keyState = true;
				break;
			}
		}
	}
	else if (k >= 0 && k < KEY_MAXARRAY)
	{
		keyState = (core->getKeyState(k));
	}
	else if (k == MOUSE_BUTTON_LEFT)
	{
		keyState = (core->mouse.buttons.left == DOWN);
	}
	else if (k == MOUSE_BUTTON_RIGHT)
	{
		keyState = (core->mouse.buttons.right == DOWN);
	}
	else if (k == MOUSE_BUTTON_MIDDLE)
	{
		keyState = (core->mouse.buttons.middle == DOWN);
	}
	else if (k >= JOY1_BUTTON_0 && k <= JOY1_BUTTON_16)
	{
		int v = k - JOY1_BUTTON_0;

		if (core->joystickEnabled)
			keyState = core->joystick.buttons[v];
	}
	else if (k == JOY1_STICK_LEFT)
	{
		keyState = core->joystick.position.x < -0.6f;
	}
	else if (k == JOY1_STICK_RIGHT)
	{
		keyState = core->joystick.position.x > 0.6f;
	}
	else if (k == JOY1_STICK_UP)
	{
		keyState = core->joystick.position.y < -0.6f;
	}
	else if (k == JOY1_STICK_DOWN)
	{
		keyState = core->joystick.position.y > 0.6f;
	}
	else if (k == X360_BTN_START)
	{
		keyState = core->joystick.btnStart;
	}
	else if (k == X360_BTN_BACK)
	{
		keyState = core->joystick.btnSelect;
	}
	else if (k == JOY1_DPAD_LEFT)
	{
		keyState = core->joystick.dpadLeft;
	}
	else if (k == JOY1_DPAD_RIGHT)
	{
		keyState = core->joystick.dpadRight;
	}
	else if (k == JOY1_DPAD_UP)
	{
		keyState = core->joystick.dpadUp;
	}
	else if (k == JOY1_DPAD_DOWN)
	{
		keyState = core->joystick.dpadDown;
	}

	return keyState;
}

void ActionMapper::onUpdate (float dt)
{
	if (inUpdate) return;
	inUpdate = true;

	if (cleared) cleared = false;
	ActionDataSet::iterator i;
	KeyDownMap oldKeyDownMap = keyDownMap;
	for (i = actionData.begin(); i != actionData.end(); ++i)
	{
		ButtonList::iterator j;
		j = i->buttonList.begin();
		for (; j != i->buttonList.end(); j++)
		{
			int k = (*j);
			int keyState=false;
			//joystick

			keyState = getKeyState(k);

			if (keyState != oldKeyDownMap[k])
			{
				keyDownMap[k] = keyState;
				if (inputEnabled)
				{
					ActionData *ad = &(*i);
					if (ad->event)
					{
						if (ad->state==-1 || keyState == ad->state)
						{
							ad->event->act();
						}
					}
					else
					{
						action(ad->id, keyState);
					}
					if (core->loopDone) goto out;
				}
				if (cleared) { cleared = false; goto out; } // actionData has been cleared, stop iteration
			}
		}
	}

out:
	inUpdate = false;

}

void ActionMapper::clearActions()
{
	cleared = true;
	keyDownMap.clear();
	actionData.clear();
}

void ActionMapper::removeAllActions()
{
	std::vector <int> deleteList;
	ActionDataSet::iterator i;
	for (i = actionData.begin(); i != actionData.end(); i++)
	{
		deleteList.push_back(i->id);
	}
	for (int c = 0; c < deleteList.size(); c++)
	{
		removeAction (deleteList[c]);
	}
	actionData.clear();
}
