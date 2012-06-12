/*
 * InspIRCd -- Internet Relay Chat Daemon
 *
 *   Copyright (C) 2007 Dennis Friis <peavey@inspircd.org>
 *   Copyright (C) 2007 Robin Burchell <robin+git@viroteck.net>
 *   Copyright (C) 2006 Craig Edwards <craigedwards@brainbox.cc>
 *
 * This file is part of InspIRCd.  InspIRCd is free software: you can
 * redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "inspircd.h"

/* $ModDesc: Provides support for hiding oper status with user mode +H */

/** Handles user mode +H
 */
class HideOper : public ModeHandler
{
 public:
	HideOper(InspIRCd* Instance) : ModeHandler(Instance, 'H', 0, 0, false, MODETYPE_USER, true) { }

	ModeAction OnModeChange(User* source, User* dest, Channel* channel, std::string &parameter, bool adding, bool)
	{
		if (adding != dest->IsModeSet('H'))
		{
			dest->SetMode('H', adding);
			return MODEACTION_ALLOW;
		}

		return MODEACTION_DENY;
	}
};

class ModuleHideOper : public Module
{

	HideOper* hm;
 public:
	ModuleHideOper(InspIRCd* Me)
		: Module(Me)
	{

		hm = new HideOper(ServerInstance);
		if (!ServerInstance->Modes->AddMode(hm))
			throw ModuleException("Could not add new modes!");
		Implementation eventlist[] = { I_OnWhoisLine, I_OnStats };
		ServerInstance->Modules->Attach(eventlist, this, 2);
	}


	virtual ~ModuleHideOper()
	{
		ServerInstance->Modes->DelMode(hm);
		delete hm;
	}

	virtual Version GetVersion()
	{
		return Version("$Id$", VF_COMMON | VF_VENDOR, API_VERSION);
	}

	int OnStats(char symbol, User* user, string_list &results)
	{
		if ((symbol != 'P') || user->HasPrivPermission("users/auspex"))
			return 0;

		/* Duplicates code from cmd_stats.cpp */
		std::string sn(ServerInstance->Config->ServerName);
		int idx = 0;
		for (user_hash::iterator i = ServerInstance->Users->clientlist->begin(); i != ServerInstance->Users->clientlist->end(); i++)
		{
			if (IS_OPER(i->second) && !ServerInstance->ULine(i->second->server) && !i->second->IsModeSet('H'))
			{
				results.push_back(sn+" 249 "+user->nick+" :"+i->second->nick+" ("+i->second->ident+"@"+i->second->dhost+") Idle: "+
					(IS_LOCAL(i->second) ? ConvToStr(ServerInstance->Time() - i->second->idle_lastmsg) + " secs" : "unavailable"));
				idx++;
			}
		}
		results.push_back(sn+" 249 "+user->nick+" :"+ConvToStr(idx)+" OPER(s)");

		return 1;
	}

	int OnWhoisLine(User* user, User* dest, int &numeric, std::string &text)
	{
		/* Dont display numeric 313 (RPL_WHOISOPER) if they have +H set and the
		 * person doing the WHOIS is not an oper
		 */
		if (numeric != 313)
			return 0;

		if (!dest->IsModeSet('H'))
			return 0;

		if (!user->HasPrivPermission("users/auspex"))
			return 1;

		return 0;
	}
};


MODULE_INIT(ModuleHideOper)
