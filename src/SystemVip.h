#ifndef SYSTEM_VIP_H
#define SYSTEM_VIP_H

#include "Player.h"
#include "Config.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"
#include "DatabaseEnv.h"
#include "Chat.h"

using namespace std;

//#define NPC_TEXT 250000

enum NPCTEXTS {
    VENDOR_INFO = 250000,
    PET_INFO
};

// gossip actions of the VIP pet teleport menu, the teleport id goes in the gossip sender
enum VipPetTeleportActions {
    ACTION_TELEPORT_MENU    = 10,
    ACTION_TELEPORT_USE     = 12,
    ACTION_TELEPORT_SAVE    = 13,
    ACTION_TELEPORT_OPTIONS = 14,
    ACTION_TELEPORT_DELETE  = 15,
    ACTION_TELEPORT_RENAME  = 17,
    ACTION_PET_MAIN_MENU    = 18
};

struct Teleports {
    uint32 id;
    string name;
    uint32 mapId;
    float coord_x;
    float coord_y;
    float coord_z;
    float orientation;
};

class SystemVip {
public:
    static SystemVip* instance();

    unordered_map<uint32, uint32> vipMap;
    unordered_map<uint32, vector<Teleports>> teleportMap;

    uint32 TokenEntry;
    uint32 TokenAmount;
    uint32 TimeVip;
    string TokenIcon;

    bool loginAnnounce;
    string loginMessage;
    bool rateCustom;
    uint32 rateXp;
    uint32 professionRate;
    uint32 goldRate;
    uint32 honorRate;
    bool ghostMount;

    bool petEnable;
    bool vipZone;
    uint32 vipZoneMapId;
    float vipZonePosX;
    float vipZonePosY;
    float vipZonePosZ;
    float vipZoneO;
    bool armorRep;
    bool bankEnable;
    bool mailEnable;
    bool buffsEnable;
    
    vector<uint32> buffIds;
    
    bool refreshEnable;
    bool sicknessEnbale;
    bool deserterEnable;
    bool resetInstance;
    bool saveTeleport;
    uint32 saveTeleportAmount;


    bool isVip(Player* player);
    void addRemainingVipTime(Player* player);
    uint32 getRemainingVipTime(Player* player);
    string getFormatedVipTime(Player* player);
    string getItemLink(uint32 entry, Player* player);
    void delExpireVip(Player* player);
    void LoadConfig();
    void sendGossipInformation(Player* player, bool advantages);
    string getInformationVip(Player* player);
    string getInformationAdavantages();
    string getLoginMessage(Player* player);

    void loadTeleportVip(Player* player);
    void saveTeleportVip(Player* player);
    void renameTeleportVip(Player* player, uint32 id, string newName);
    void delTeleportVip(Player* player, uint32 id);
    void addTeleportsToGossip(Player* player);
    void addTeleportOptionsToGossip(Player* player, uint32 id);
    Teleports* findTeleport(uint32 accountId, uint32 id);
    bool isTeleportNameTaken(uint32 accountId, string const& name, uint32 ignoredId = 0);
    void teleportPlayer(Player* player, uint32 id);
    bool isTeleportMapAllowed(uint32 mapId);
    bool canUseTeleportAt(uint32 mapId, uint32 zoneId);
};

#define sSystemVip SystemVip::instance()

#endif //SYSTEM_VIP_H
