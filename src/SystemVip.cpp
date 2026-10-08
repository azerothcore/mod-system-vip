#include "SystemVip.h"
#include "AreaDefines.h"
#include "BattlefieldMgr.h"
#include "DBCStores.h"
#include "MapMgr.h"
#include "Util.h"
#include "World.h"

SystemVip* SystemVip::instance()
{
    static SystemVip instance;
    return &instance;
}

void SystemVip::LoadConfig() {
    TimeVip = sConfigMgr->GetOption<uint32>("SystemVip.TimeVip", 7) * 86400;
    TokenEntry = sConfigMgr->GetOption<uint32>("SystemVip.Token", 123);
    TokenAmount = sConfigMgr->GetOption<uint32>("SystemVip.TokenAmount", 10);
    TokenIcon = sConfigMgr->GetOption<string>("SystemVip.TokenIcon", "|TInterface/ICONS/inv_misc_rune_05:15:15:-15:0|t ");

    loginAnnounce = sConfigMgr->GetOption<bool>("SystemVip.LoginAnnounce", false);
    loginMessage = sConfigMgr->GetOption<string>("SystemVip.LoginAnnounceMessage", "VIP player %s has logged in.");
    rateCustom = sConfigMgr->GetOption<bool>("SystemVip.EnableRateCustom", false);
    rateXp = sConfigMgr->GetOption<uint32>("SystemVip.RateXP", 1);
    professionRate = sConfigMgr->GetOption<uint32>("SystemVip.ProfessionRate", 1);
    goldRate = sConfigMgr->GetOption<uint32>("SystemVip.GoldRate", 1);
    honorRate = sConfigMgr->GetOption<uint32>("SystemVip.HonorRate", 1);
    ghostMount = sConfigMgr->GetOption<bool>("SystemVip.GhostMount", false);

    petEnable = sConfigMgr->GetOption<bool>("SystemVip.Pet", false);
    vipZone = sConfigMgr->GetOption<bool>("SystemVip.VipZone", false);
    vipZoneMapId = sConfigMgr->GetOption<uint32>("SystemVip.VipZoneMapId", 571);
    vipZonePosX = sConfigMgr->GetOption<float>("SystemVip.VipZoneCoorX", 5804.15);
    vipZonePosY = sConfigMgr->GetOption<float>("SystemVip.VipZoneCoorY", 624.771);
    vipZonePosZ = sConfigMgr->GetOption<float>("SystemVip.VipZoneCoorZ", 647.767);
    vipZoneO = sConfigMgr->GetOption<float>("SystemVip.VipZoneOrien", 1.64);
    armorRep = sConfigMgr->GetOption<bool>("SystemVip.ArmorRep", false);
    bankEnable = sConfigMgr->GetOption<bool>("SystemVip.Bank", false);
    mailEnable = sConfigMgr->GetOption<bool>("SystemVip.Mail", false);
    buffsEnable = sConfigMgr->GetOption<bool>("SystemVip.Buffs", false);

    string buffString = sConfigMgr->GetOption<string>("SystemVip.BuffIds", "25898,48469,42995,48169,48073,48161,53307,23735,23736,23737,23738,23766,23767,23768,23769");
    stringstream ss(buffString);
    buffIds.clear();
    while (ss.good()) {
        std::string substr;
        std::getline(ss, substr, ',');
        int num = std::stoi(substr);
        buffIds.push_back(num);
    }

    refreshEnable = sConfigMgr->GetOption<bool>("SystemVip.Refresh", false);
    sicknessEnbale = sConfigMgr->GetOption<bool>("SystemVip.Sickness", false);
    deserterEnable = sConfigMgr->GetOption<bool>("SystemVip.Deserter", false);
    resetInstance = sConfigMgr->GetOption<bool>("SystemVip.ResetInstance", false);
    saveTeleport = sConfigMgr->GetOption<bool>("SystemVip.SaveTeleport", false);
    saveTeleportAmount = sConfigMgr->GetOption<uint32>("SystemVip.SaveTeleportAmount", 5);
}

bool SystemVip::isVip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();

    if (vipMap.count(accountId) == 0)
        return false;

    return time(nullptr) < vipMap[accountId];
}

void SystemVip::addRemainingVipTime(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    if (isVip(player)) {
        vipMap[accountId] += TimeVip;
        LoginDatabase.Execute("UPDATE account_vip SET subscription_date = {} WHERE id = {};", vipMap[accountId], accountId);
    }
    else {
        vipMap.erase(accountId);
        vipMap.emplace(accountId, time(nullptr) + TimeVip);
        LoginDatabase.Execute("REPLACE INTO account_vip (id, subscription_date) VALUES ({}, {});", accountId, vipMap[accountId]);
    }
}

uint32 SystemVip::getRemainingVipTime(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    return time(nullptr) >= vipMap[accountId] ? 0 : vipMap[accountId] - time(nullptr);
}

string SystemVip::getFormatedVipTime(Player* player) {
    uint32 time = getRemainingVipTime(player);
    int minutes = time / 60;
    int hours = minutes / 60;
    int days = hours / 24;

    hours = hours % 24;
    minutes = minutes % 60;
    // int seconds = time % 60;

    string result = to_string(days) + " days, " + to_string(hours) + " hours, " + to_string(minutes) + " minutes.";
    return result;
}

string SystemVip::getItemLink(uint32 entry, Player* player) {
    const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
    int loc_idx = player->GetSession()->GetSessionDbLocaleIndex();
    std::string name = temp->Name1;
    if (ItemLocale const* il = sObjectMgr->GetItemLocale(entry))
        ObjectMgr::GetLocaleString(il->Name, loc_idx, name);

    std::ostringstream oss;
    oss << "|c" << std::hex << ItemQualityColors[temp->Quality] << std::dec <<
        "|Hitem:" << entry << ":0:0:0:0:0:0:0:0:0|h[" << name << "]|h|r";

    return oss.str();
}

string SystemVip::getInformationVip(Player* player) {
    std::ostringstream text;
    std::string accName;
    if (AccountMgr::GetName(player->GetSession()->GetAccountId(), accName))
        text << "Remaining time: |CFF0DD617" << getFormatedVipTime(player) << "|r\n";

    return text.str();
}

string SystemVip::getInformationAdavantages() {
    std::ostringstream text;
    text << "Benefits:\n";
    text << "----------------------------------\n";
    if(loginAnnounce)
        text << "|TInterface/ICONS/Spell_unused2:15:15:-10:-5|t Login announcement." << "\n";
    if (rateCustom) {
        text << "|TInterface/ICONS/Achievement_BG_KillXEnemies_GeneralsRoom:15:15:-10:-8|t XP rate           x " << rateXp << "\n";
        text << "|TInterface/ICONS/Achievement_BG_overcome500disadvantage:15:15:-10:-8|t Profession rate   x " << professionRate << "\n";
        text << "|TInterface/ICONS/Achievement_BG_ABshutout:15:15:-10:-8|t Gold rate         x " << goldRate << "\n";
        text << "|TInterface/ICONS/Achievement_BG_kill_carrier_opposing_flagroom:15:15:-10::-8|t Honor rate        x " << honorRate << "\n";
    }
    if(ghostMount)
        text << "|TInterface/ICONS/ability_vanish:15:15:-10::-8|t Ghost speed boost." << "\n";
    if (petEnable) {
        text << "|TInterface/ICONS/ability_hunter_beastcall:15:15:-10::-8|t VIP pet." << "\n";
        if(vipZone)
            text << "|TInterface/ICONS/Achievement_Zone_ZulDrak_12:15:15:-10::-8|t VIP zone." << "\n";
        if(armorRep)
            text << "|TInterface/ICONS/INV_Hammer_20:15:15:-10::-8|t Armor repair." << "\n";
        if(bankEnable)
            text << "|TInterface/ICONS/INV_Ingot_03:15:15:-10::-8|t Personal bank." << "\n";
        if(mailEnable)
            text << "|TInterface/ICONS/inv_letter_15:15:15:-10::-8|t Open mailbox." << "\n";
        if(buffsEnable)
            text << "|TInterface/ICONS/Spell_Magic_GreaterBlessingofKings:15:15:-10::-8|t VIP buffs." << "\n";
        if(refreshEnable)
            text << "|TInterface/ICONS/Spell_Holy_LayOnHands:15:15:-10::-8|t Restore HP/mana." << "\n";
        if(sicknessEnbale)
            text << "|TInterface/ICONS/spell_shadow_deathscream:15:15:-10::-8|t Remove resurrection sickness." << "\n";
        if(deserterEnable)
            text << "|TInterface/ICONS/ability_druid_cower:15:15:-10::-8|t Remove deserter." << "\n";
        if(resetInstance)
            text << "|TInterface/ICONS/Achievement_Dungeon_Icecrown_IcecrownEntrance:15:15:-10::-8|t Reset instances." << "\n";
        if(saveTeleport)
            text << "|TInterface/ICONS/Spell_Holy_LightsGrace:15:15:-10::-8|t Save teleport locations." << "\n";
    }
    return text.str();
}

void SystemVip::sendGossipInformation(Player* player, bool advantages) {
    std::ostringstream text;
    std::string accName;
    if (AccountMgr::GetName(player->GetSession()->GetAccountId(), accName))
        text << "Account: |CFF0E3CE6" << accName << "|r\n";

    if (isVip(player)) {
        text << "Remaining time: |CFF0DD617" << getFormatedVipTime(player) << "|r\n\n";
        text << "Thank you for purchasing a VIP subscription.\n\n";
    }
    else {
        text << "You do not have an active VIP subscription.\n";
        text << "Purchase a subscription and enjoy all the benefits of being VIP!\n";
    }

    if (advantages) {
        text << "Remember that VIP benefits apply to every character on your account." << "\n";
        text << getInformationAdavantages();
    }

    WorldPacket data(384, 100);
    if(advantages)
        data << uint32_t(VENDOR_INFO); // id npc_text
    else
        data << uint32_t(PET_INFO); // id npc_text
    for (int i = 0; i < 10; ++i) {
        data << float(0.0f);
        data << std::string(text.str());
        data << std::string(text.str());
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
        data << uint32_t(0);
    }
    player->GetSession()->SendPacket(&data);
}

void SystemVip::delExpireVip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    if (vipMap.count(accountId)) {
        if (getRemainingVipTime(player) == 0) {
            vipMap.erase(accountId);
        }
    }
}


string SystemVip::getLoginMessage(Player* player) {
    string welcomeMessage = loginMessage;
    string name = player->GetName();
    size_t pos = welcomeMessage.find("%s");

    if (pos != std::string::npos) {
        welcomeMessage.replace(pos, 2, name);
    }
    return welcomeMessage;
}

void SystemVip::loadTeleportVip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    QueryResult result = LoginDatabase.Query("SELECT * FROM account_vip_teleport WHERE id = {};", accountId);
    if (result) {
        uint32 i = 1;
        do
        {
            Teleports teleport = { i, (*result)[1].Get<string>(), (*result)[2].Get<uint32>(), (*result)[3].Get<float>(), (*result)[4].Get<float>(), (*result)[5].Get<float>(), (*result)[6].Get<float>() };
            teleportMap[accountId].push_back(teleport);
            i++;
        } while (result->NextRow());
    }
}

Teleports* SystemVip::findTeleport(uint32 accountId, uint32 id) {
    for (Teleports& teleport : teleportMap[accountId])
        if (teleport.id == id)
            return &teleport;

    return nullptr;
}

bool SystemVip::isTeleportNameTaken(uint32 accountId, string const& name, uint32 ignoredId) {
    // case-insensitive like the DB primary key, otherwise the query fails as a duplicate
    for (Teleports const& teleport : teleportMap[accountId])
        if (teleport.id != ignoredId && StringEqualI(teleport.name, name))
            return true;

    return false;
}

void SystemVip::saveTeleportVip(Player* player) {
    if (!canUseTeleportAt(player->GetMapId(), player->GetZoneId())) {
        ChatHandler(player->GetSession()).PSendSysMessage("You cannot save teleports in dungeons, raids, "
            "battlegrounds, arenas, or in Wintergrasp while the battle is active.");
        return;
    }

    uint32 accountId = player->GetSession()->GetAccountId();
    vector<Teleports>& teleports = teleportMap[accountId];
    if (teleports.size() >= saveTeleportAmount) {
        ChatHandler(player->GetSession()).PSendSysMessage("You cannot save any more teleports!");
        return;
    }

    // named after the area, the player can rename it later
    string baseName = "Teleport";
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(player->GetAreaId())) {
        string areaName = area->area_name[player->GetSession()->GetSessionDbcLocale()];
        if (areaName.empty())
            areaName = area->area_name[sWorld->GetDefaultDbcLocale()];
        if (!areaName.empty())
            baseName = areaName;
    }

    string name = baseName;
    for (uint32 suffix = 2; isTeleportNameTaken(accountId, name); ++suffix)
        name = baseName + " " + to_string(suffix);

    // ids only live in memory, they are renumbered on login
    uint32 id = teleports.empty() ? 1 : teleports.back().id + 1;
    Teleports teleport = { id, name, player->GetMapId(), player->GetPositionX(), player->GetPositionY(),
        player->GetPositionZ(), player->GetOrientation() };
    teleports.push_back(teleport);

    string escapedName = name;
    LoginDatabase.EscapeString(escapedName);
    LoginDatabase.Execute("INSERT INTO account_vip_teleport VALUES ( {} , '{}', {}, {}, {}, {}, {} );", accountId, escapedName, teleport.mapId, teleport.coord_x, teleport.coord_y, teleport.coord_z, teleport.orientation);
    ChatHandler(player->GetSession()).PSendSysMessage("Location saved as \"{}\".", name);
}

void SystemVip::renameTeleportVip(Player* player, uint32 id, string newName) {
    uint32 accountId = player->GetSession()->GetAccountId();
    Teleports* teleport = findTeleport(accountId, id);
    if (!teleport)
        return;

    if (newName.empty() || newName.size() > 50) {
        ChatHandler(player->GetSession()).PSendSysMessage("The name must have between 1 and 50 characters.");
        return;
    }

    if (isTeleportNameTaken(accountId, newName, id)) {
        ChatHandler(player->GetSession()).PSendSysMessage("A teleport with that name already exists!");
        return;
    }

    // name is typed by the player, escape before building the query
    string escapedOldName = teleport->name;
    string escapedNewName = newName;
    LoginDatabase.EscapeString(escapedOldName);
    LoginDatabase.EscapeString(escapedNewName);
    LoginDatabase.Execute("UPDATE account_vip_teleport SET name = '{}' WHERE id = {} AND name = '{}';",
        escapedNewName, accountId, escapedOldName);

    teleport->name = newName;
    ChatHandler(player->GetSession()).PSendSysMessage("Teleport renamed to \"{}\".", newName);
}

void SystemVip::delTeleportVip(Player* player, uint32 id) {
    uint32 accountId = player->GetSession()->GetAccountId();
    vector<Teleports>& teleports = teleportMap[accountId];
    for (size_t i = 0; i < teleports.size(); i++) {
        if (teleports[i].id != id)
            continue;

        string escapedName = teleports[i].name;
        LoginDatabase.EscapeString(escapedName);
        LoginDatabase.Execute("DELETE FROM account_vip_teleport WHERE id = {} AND name = '{}';",
            accountId, escapedName);
        ChatHandler(player->GetSession()).PSendSysMessage("Teleport \"{}\" deleted.", teleports[i].name);
        teleports.erase(teleports.begin() + i);
        return;
    }
}

void SystemVip::addTeleportsToGossip(Player* player) {
    uint32 accountId = player->GetSession()->GetAccountId();
    for (Teleports const& teleport : teleportMap[accountId])
        AddGossipItemFor(player, 0, "|TInterface/CURSOR/Taxi:28:28:-15:0|t " + teleport.name,
            teleport.id, ACTION_TELEPORT_OPTIONS);
}

void SystemVip::addTeleportOptionsToGossip(Player* player, uint32 id) {
    Teleports* teleport = findTeleport(player->GetSession()->GetAccountId(), id);
    if (!teleport)
        return;

    AddGossipItemFor(player, 0, "|TInterface/CURSOR/Taxi:28:28:-15:0|t Teleport.", id, ACTION_TELEPORT_USE,
        "Teleport to \"" + teleport->name + "\"?", 0, false);
    // no popup text: with one the 3.3.5 client shows a confirm box first and the text box only after it
    AddGossipItemFor(player, 0, "|TInterface/ICONS/INV_Misc_Note_01:28:28:-15:0|t Rename.", id, ACTION_TELEPORT_RENAME,
        "", 0, true);
    AddGossipItemFor(player, 0, "|TInterface/PAPERDOLLINFOFRAME/UI-GearManager-Undo:28:28:-15:0|t Delete.", id,
        ACTION_TELEPORT_DELETE, "Delete \"" + teleport->name + "\"?", 0, false);
}

void SystemVip::teleportPlayer(Player* player, uint32 id) {
    if (!canUseTeleportAt(player->GetMapId(), player->GetZoneId())) {
        ChatHandler(player->GetSession()).PSendSysMessage("You cannot use teleports in dungeons, raids, "
            "battlegrounds, arenas, or in Wintergrasp while the battle is active.");
        return;
    }

    uint32 accountId = player->GetSession()->GetAccountId();
    for (Teleports const& teleport : teleportMap[accountId]) {
        if (teleport.id != id)
            continue;

        // also checks the destination, locations saved before this check existed may be inside an instance.
        // map and coords first: GetZoneId asserts on a map id missing from Map.dbc
        if (!MapMgr::IsValidMapCoord(teleport.mapId, teleport.coord_x, teleport.coord_y, teleport.coord_z,
                teleport.orientation)
            || !isTeleportMapAllowed(teleport.mapId)
            || !canUseTeleportAt(teleport.mapId, sMapMgr->GetZoneId(player->GetPhaseMask(), teleport.mapId,
                teleport.coord_x, teleport.coord_y, teleport.coord_z))) {
            ChatHandler(player->GetSession()).PSendSysMessage("You cannot teleport to that location right now.");
            return;
        }

        player->TeleportTo(teleport.mapId, teleport.coord_x, teleport.coord_y, teleport.coord_z, teleport.orientation);
        return;
    }
}

bool SystemVip::isTeleportMapAllowed(uint32 mapId) {
    MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
    return mapEntry && !mapEntry->IsDungeon() && !mapEntry->IsBattlegroundOrArena();
}

bool SystemVip::canUseTeleportAt(uint32 mapId, uint32 zoneId) {
    if (!isTeleportMapAllowed(mapId))
        return false;

    if (zoneId == AREA_WINTERGRASP)
        if (Battlefield* wintergrasp = sBattlefieldMgr->GetBattlefieldToZoneId(AREA_WINTERGRASP))
            return !wintergrasp->IsWarTime();

    return true;
}
