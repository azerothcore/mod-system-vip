/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-AGPL3
 */

#include "SystemVip.h"
#include "WorldSessionMgr.h"

#define sV sSystemVip

// Add player scripts
class SystemVipPlayer : public PlayerScript
{
public:
    SystemVipPlayer() : PlayerScript("SystemVipPlayer", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_GIVE_EXP,
        PLAYERHOOK_ON_BEFORE_LOOT_MONEY,
        PLAYERHOOK_ON_PLAYER_RELEASED_GHOST,
        PLAYERHOOK_ON_VICTIM_REWARD_AFTER
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (sConfigMgr->GetOption<bool>("SystemVip.Announce", false))
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00SystemVip |rmodule.");

        if (sV->isVip(player) && sV->loginAnnounce)
            sWorldSessionMgr->SendServerMessage(SERVER_MSG_STRING, sV->getLoginMessage(player));

        if (sV->isVip(player))
            ChatHandler(player->GetSession()).PSendSysMessage("Remaining VIP subscription time: |cff4CFF00{}|r", sV->getFormatedVipTime(player).c_str());

        sV->delExpireVip(player);
        if (sV->saveTeleport && sV->isVip(player))
            sV->loadTeleportVip(player);
    }

    void OnPlayerLogout(Player* player) override
    {
        if (sV->saveTeleport && sV->isVip(player))
            sV->teleportMap.erase(player->GetSession()->GetAccountId());
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (sV->isVip(player) && sV->rateCustom)
            amount *= sV->rateXp;
    }

    void OnPlayerBeforeLootMoney(Player* player, Loot* loot) override
    {
        if (sV->isVip(player) && sV->rateCustom)
            loot->gold *= sV->goldRate;
    }

    void OnPlayerReleasedGhost(Player* player) override
    {
        if (sV->isVip(player) && sV->ghostMount)
        {
            // Cast Mount Speed
            if (!player->HasAura(55164))
                player->AddAura(55164, player);

        }
    }

    void OnPlayerVictimRewardAfter(Player* player, Player* /*victim*/, uint32& /*killer_title*/, int32& /*victim_rank*/, float& honor_f) override
    {
        if (sV->isVip(player) && sV->rateCustom)
        {
            if (sV->honorRate > 1)
                player->ModifyHonorPoints((honor_f * sV->honorRate) - honor_f);
        }
    }
};

class SystemVipVendor : public CreatureScript {
public:
    SystemVipVendor() : CreatureScript("SystemVipVendor") {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/INV_Misc_Coin_02:28:28:-15:0|t VIP subscription.", 0, 1, "Do you want to subscribe to the VIP system for " + to_string(sV->TimeVip / DAY) + " days?\nPrice: " + to_string(sV->TokenAmount) + "\n " + sV->TokenIcon + " " +sV->getItemLink(sV->TokenEntry, player), 0, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/INV_Misc_QuestionMark:28:28:-15:0|t Information.", 0, 2);
        if(sV->isVip(player))
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/ability_hunter_beastcall:28:28:-15:0|t Recover my VIP pet.", 0, 4);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Trade_Engineering:28:28:-15:0|t Close.", 0, 3);
        SendGossipMenuFor(player, 1, creature->GetGUID());
        return true;
    }


    bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
    {
        ClearGossipMenuFor(player);
        switch (action)
        {
            case 1:
                if (player->HasItemCount(sV->TokenEntry, sV->TokenAmount))
                {
                    player->DestroyItemCount(sV->TokenEntry, sV->TokenAmount, true);
                    sV->addRemainingVipTime(player);
                    if (!player->HasItemCount(44824, 1, true))
                        player->AddItem(44824, 1);

                    ChatHandler(player->GetSession()).PSendSysMessage("Thank you for your VIP subscription.");
                    ChatHandler(player->GetSession()).PSendSysMessage("Remaining VIP subscription time: {}", sV->getFormatedVipTime(player).c_str());
                    OnGossipSelect(player, creature, 0, 2);
                }
                else
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("You do not have enough tokens.");
                    CloseGossipMenuFor(player);
                }
                break;
            case 2:
                sV->sendGossipInformation(player, true);
                AddGossipItemFor(player, 0, "|TInterface/ICONS/Trade_Engineering:28:28:-15:0|t Close.", 0, 3);
                SendGossipMenuFor(player, VENDOR_INFO, creature->GetGUID());
                break;
            case 4:
                if (!player->HasItemCount(44824, 1, true))
                {
                    player->AddItem(44824, 1);
                    creature->Whisper("Don't lose it again.", LANG_UNIVERSAL, player, false);
                    CloseGossipMenuFor(player);
                }
                else
                {
                    creature->Whisper("You already have an item to summon your VIP pet.", LANG_UNIVERSAL, player, false);
                    OnGossipHello(player, creature);
                }
                break;
            default:
                CloseGossipMenuFor(player);
                break;
        }
        return true;
    }
};

class SystemVipPocket : ItemScript {
public:
    SystemVipPocket() : ItemScript("SystemVipPocket") {}

    bool OnUse(Player* player, Item* /*item*/, SpellCastTargets const& /*targets*/)
    {
        if (!sV->isVip(player))
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You are not VIP!");
            ChatHandler(player->GetSession()).PSendSysMessage("Please renew your VIP subscription.");
            return false;
        }

        /*if (player->IsInCombat()) {
            ChatHandler(player->GetSession()).PSendSysMessage("You are in combat!");
            return false;
        }*/

        if (player->GetMap()->IsBattleArena())
        {
            ChatHandler(player->GetSession()).PSendSysMessage("You cannot use this in an arena!");
            return false;
        }

        // Sound 3980
        // emote 73213
        player->CastSpell(player, 73213);
        player->PlayDistanceSound(3980, player);

        float distance = 20;
        float angle = player->GetOrientation() * M_PI / 180.0f;

        Creature* pet = player->SummonCreature(100043, player->GetPositionX() + (distance * cos(angle)), player->GetPositionY() + (distance * sin(angle)), player->GetPositionZ(), player->GetOrientation(), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000);
        pet->GetMotionMaster()->MoveFollow(player, PET_FOLLOW_DIST + 2.0, PET_FOLLOW_ANGLE);
        pet->SetFaction(player->GetFaction());
        pet->SetLevel(player->GetLevel());
        pet->SetCreatorGUID(player->GetGUID());
        return false;
    }

};

class SystemVipPet : CreatureScript {
public:
    SystemVipPet() : CreatureScript("SystemVipPet") {}

    bool OnGossipHello(Player* player, Creature* creature)
    {
        ClearGossipMenuFor(player);
        sV->sendGossipInformation(player, false);
        if (creature->GetCreatorGUID() != player->GetGUID())
            return true;

        if (!sV->petEnable)
            return true;

        if(sV->vipZone)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Achievement_Zone_ZulDrak_12:28:28:-15:0|t VIP Zone", 0, 1);
        if(sV->armorRep)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/INV_Hammer_20:28:28:-15:0|t Repair armor.", 0, 2);
        if(sV->bankEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/INV_Ingot_03:28:28:-15:0|t My bank.", 0, 3);
        if(sV->mailEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/inv_letter_15:28:28:-15:0|t My mailbox.", 0, 8);
        if (sV->buffsEnable)
        {
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Spell_Magic_GreaterBlessingofKings:28:28:-15:0|t Buffs", 0, 4);
            AddGossipItemFor(player, 0, "|TInterface/PAPERDOLLINFOFRAME/UI-GearManager-Undo:28:28:-15:0|t Remove buffs", 0, 11);
        }
        if(sV->refreshEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Spell_Holy_LayOnHands:28:28:-15:0|t Restore HP/mana.", 0, 5);
        if(sV->sicknessEnbale)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/spell_shadow_deathscream:28:28:-15:0|t Remove resurrection sickness.", 0, 6);
        if(sV->deserterEnable)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/ability_druid_cower:28:28:-15:0|t Remove deserter.", 0, 7);
        if(sV->resetInstance)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Achievement_Dungeon_Icecrown_IcecrownEntrance:28:28:-15:0|t Reset instances.", 0, 9);
        if(sV->saveTeleport)
            AddGossipItemFor(player, 0, "|TInterface/ICONS/Spell_Holy_LightsGrace:28:28:-15:0|t My teleports.", 0, 10);
        AddGossipItemFor(player, 0, "|TInterface/ICONS/Trade_Engineering:28:28:-15:0|t Close.", 0, 100);

        SendGossipMenuFor(player, PET_INFO, creature->GetGUID());
        return true;
    }
    bool OnGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action)
    {
        ClearGossipMenuFor(player);
        switch (action)
        {
            case 1:
                if (player->IsInCombat())
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("You are in combat!");
                    CloseGossipMenuFor(player);
                }
                else if (!sV->canUseTeleportAt(player->GetMapId(), player->GetZoneId()))
                {
                    ChatHandler(player->GetSession()).PSendSysMessage("You cannot use teleports in dungeons, raids, battlegrounds, arenas or during the Battle for Wintergrasp.");
                    CloseGossipMenuFor(player);
                }
                else
                {
                    player->TeleportTo(sV->vipZoneMapId, sV->vipZonePosX, sV->vipZonePosY, sV->vipZonePosZ, sV->vipZoneO);
                    CloseGossipMenuFor(player);
                }
                break;
            case 2:
                player->DurabilityRepairAll(false, 0, false);
                ChatHandler(player->GetSession()).PSendSysMessage("Your armor has been repaired.");
                OnGossipHello(player, creature);
                break;
            case 3:
                player->GetSession()->SendShowBank(creature->GetGUID());
                break;
            case 4:
                for (size_t i = 0; i < sV->buffIds.size(); i++)
                    player->AddAura(sV->buffIds[i], player);

                player->CastSpell(player, 16609);
                ChatHandler(player->GetSession()).PSendSysMessage("Buffs applied!");
                OnGossipHello(player, creature);
                break;
            case 5:
                if (player->IsInCombat())
                {
                    CloseGossipMenuFor(player);
                    ChatHandler(player->GetSession()).PSendSysMessage("You are in combat!");
                    return false;
                }
                else if (player->getPowerType() == POWER_MANA)
                    player->SetPower(POWER_MANA, player->GetMaxPower(POWER_MANA));

                player->SetHealth(player->GetMaxHealth());
                ChatHandler(player->GetSession()).PSendSysMessage("HP/mana restored!");
                creature->CastSpell(player, 31726, true);
                OnGossipHello(player, creature);
                break;
            case 6:
                if (player->HasAura(15007))
                    player->RemoveAura(15007);
                ChatHandler(player->GetSession()).PSendSysMessage("Your resurrection sickness has been removed.");
                creature->CastSpell(player, 31726, true);
                OnGossipHello(player, creature);
                break;
            case 7:
                // remove deserter
                if(player->HasAura(26013))
                    player->RemoveAura(26013);
                ChatHandler(player->GetSession()).PSendSysMessage("Your deserter debuff has been removed.");
                creature->CastSpell(player, 31726);
                OnGossipHello(player, creature);
                break;
            case 8:
                // show mailbox
                CloseGossipMenuFor(player);
                player->GetSession()->SendShowMailBox(creature->GetGUID());
                break;
            case 9:
                // reset instance lockouts
                for (uint8 i = 0; i < MAX_DIFFICULTY; ++i)
                {
                    BoundInstancesMap const& m_boundInstances = sInstanceSaveMgr->PlayerGetBoundInstances(player->GetGUID(), Difficulty(i));
                    for (BoundInstancesMap::const_iterator itr = m_boundInstances.begin(); itr != m_boundInstances.end();)
                    {
                        if (itr->first != player->GetMapId())
                        {
                            sInstanceSaveMgr->PlayerUnbindInstance(player->GetGUID(), itr->first, Difficulty(i), true, player);
                            itr = m_boundInstances.begin();
                        }
                        else
                            ++itr;
                    }
                }

                ChatHandler(player->GetSession()).PSendSysMessage("Your instances have been reset!");
                creature->CastSpell(player, 59908);
                OnGossipHello(player, creature);
                return true;
                break;
            case 11:
                for (size_t i = 0; i < sV->buffIds.size(); i++)
                {
                    if (player->HasAura(sV->buffIds[i]))
                        player->RemoveAura(sV->buffIds[i]);
                }
                if (player->HasAura(16609))
                    player->RemoveAura(16609);
                creature->CastSpell(player, 31726);
                ChatHandler(player->GetSession()).PSendSysMessage("Buffs removed!");
                OnGossipHello(player, creature);
                break;
            case 10:
                // Teleport system
                AddGossipItemFor(player, 0, "|TInterface/GUILDBANKFRAME/UI-GuildBankFrame-NewTab:28:28:-15:0|t Add new.", 0, 1, "Name to save your current location.", 0, true);
                AddGossipItemFor(player, 0, "|TInterface/PAPERDOLLINFOFRAME/UI-GearManager-Undo:28:28:-15:0|t Delete.", 0, 2, "Name to delete.", 0, true);
                sV->getTeleports(player);
                SendGossipMenuFor(player, 1, creature->GetGUID());
                break;
            case 12:
                sV->teleportPlayer(player, sender);
                CloseGossipMenuFor(player);
                break;
            default:
                CloseGossipMenuFor(player);
                break;
        }
        return true;
    }

    bool OnGossipSelectCode(Player* player, Creature* creature, uint32 /*sender*/, uint32 action, const char* code)
    {
        switch (action)
        {
            case 1:
                sV->addTeleportVip(player, code);
                OnGossipSelect(player, creature, 0, 10);
                break;
            case 2:
                sV->delTeleportVip(player, code);
                OnGossipSelect(player, creature, 0, 10);
                break;
            default:
                CloseGossipMenuFor(player);
                break;
        }
        return true;
    }
};



class SystemVipWorld : WorldScript {
public:
    SystemVipWorld() : WorldScript("SystemVipWorld", {
        WORLDHOOK_ON_AFTER_CONFIG_LOAD
    }) {}

    virtual void OnAfterConfigLoad(bool /*reload*/)
    {
        sV->LoadConfig();
        sV->vipMap.clear();
        QueryResult result = LoginDatabase.Query("SELECT * FROM account_vip;");
        if (result)
        {
            do
            {
                sV->vipMap.emplace((*result)[0].Get<uint32>(), (*result)[1].Get<uint32>());
            } while (result->NextRow());
        }

        LOG_INFO("module", "Loading vip accounts... {} accounts loaded.", sV->vipMap.size());
    }
};
// Add all scripts in one
void Addsystem_vipScripts()
{
    new SystemVipPlayer();
    new SystemVipVendor();
    new SystemVipWorld();
    new SystemVipPocket();
    new SystemVipPet();
}
