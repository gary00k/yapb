//
// YaPB, based on PODBot by Markus Klinge ("CountFloyd").
// Copyright © YaPB Project Developers <yapb@jeefo.net>.
//
// SPDX-License-Identifier: MIT
//

#include <yapb.h>

ConVar cv_display_welcome_text ("display_welcome_text", "1", "Enables or disables showing welcome message to host entity on game start.");
ConVar cv_enable_query_hook ("enable_query_hook", "0", "Enables or disables fake server queries response, that shows bots as real players in server browser.");
ConVar cv_breakable_health_limit ("breakable_health_limit", "500.0", "Specifies the maximum health of breakable object, that bot will consider to destroy.", true, 1.0f, 3000.0);

BotSupport::BotSupport () {
   m_needToSendWelcome = false;
   m_welcomeReceiveTime = 0.0f;

   // add default messages
   m_sentences.push ("hello user,communication is acquired");
   m_sentences.push ("your presence is acknowledged");
   m_sentences.push ("high man, your in command now");
   m_sentences.push ("blast your hostile for good");
   m_sentences.push ("high man, kill some idiot here");
   m_sentences.push ("is there a doctor in the area");
   m_sentences.push ("warning, experimental materials detected");
   m_sentences.push ("high amigo, shoot some but");
   m_sentences.push ("time for some bad ass explosion");
   m_sentences.push ("bad ass son of a breach device activated");
   m_sentences.push ("high, do not question this great service");
   m_sentences.push ("engine is operative, hello and goodbye");
   m_sentences.push ("high amigo, your administration has been great last day");
   m_sentences.push ("attention, expect experimental armed hostile presence");
   m_sentences.push ("warning, medical attention required");

   m_tags.emplace ("[[", "]]");
   m_tags.emplace ("-=", "=-");
   m_tags.emplace ("-[", "]-");
   m_tags.emplace ("-]", "[-");
   m_tags.emplace ("-}", "{-");
   m_tags.emplace ("-{", "}-");
   m_tags.emplace ("<[", "]>");
   m_tags.emplace ("<]", "[>");
   m_tags.emplace ("[-", "-]");
   m_tags.emplace ("]-", "-[");
   m_tags.emplace ("{-", "-}");
   m_tags.emplace ("}-", "-{");
   m_tags.emplace ("[", "]");
   m_tags.emplace ("{", "}");
   m_tags.emplace ("<", "[");
   m_tags.emplace (">", "<");
   m_tags.emplace ("-", "-");
   m_tags.emplace ("|", "|");
   m_tags.emplace ("=", "=");
   m_tags.emplace ("+", "+");
   m_tags.emplace ("(", ")");
   m_tags.emplace (")", "(");

   // register weapon aliases
   m_weaponAlias[Weapon::USP] = "usp"; // HK USP .45 Tactical
   m_weaponAlias[Weapon::Glock18] = "glock"; // Glock18 Select Fire
   m_weaponAlias[Weapon::Deagle] = "deagle"; // Desert Eagle .50AE
   m_weaponAlias[Weapon::P228] = "p228"; // SIG P228
   m_weaponAlias[Weapon::Elite] = "elite"; // Dual Beretta 96G Elite
   m_weaponAlias[Weapon::FiveSeven] = "fn57"; // FN Five-Seven
   m_weaponAlias[Weapon::M3] = "m3"; // Benelli M3 Super90
   m_weaponAlias[Weapon::XM1014] = "xm1014"; // Benelli XM1014
   m_weaponAlias[Weapon::MP5] = "mp5"; // HK MP5-Navy
   m_weaponAlias[Weapon::TMP] = "tmp"; // Steyr Tactical Machine Pistol
   m_weaponAlias[Weapon::P90] = "p90"; // FN P90
   m_weaponAlias[Weapon::MAC10] = "mac10"; // Ingram MAC-10
   m_weaponAlias[Weapon::UMP45] = "ump45"; // HK UMP45
   m_weaponAlias[Weapon::AK47] = "ak47"; // Automat Kalashnikov AK-47
   m_weaponAlias[Weapon::Galil] = "galil"; // IMI Galil
   m_weaponAlias[Weapon::Famas] = "famas"; // GIAT FAMAS
   m_weaponAlias[Weapon::SG552] = "sg552"; // Sig SG-552 Commando
   m_weaponAlias[Weapon::M4A1] = "m4a1"; // Colt M4A1 Carbine
   m_weaponAlias[Weapon::AUG] = "aug"; // Steyr Aug
   m_weaponAlias[Weapon::Scout] = "scout"; // Steyr Scout
   m_weaponAlias[Weapon::AWP] = "awp"; // AI Arctic Warfare/Magnum
   m_weaponAlias[Weapon::G3SG1] = "g3sg1"; // HK G3/SG-1 Sniper Rifle
   m_weaponAlias[Weapon::SG550] = "sg550"; // Sig SG-550 Sniper
   m_weaponAlias[Weapon::M249] = "m249"; // FN M249 Para
   m_weaponAlias[Weapon::Flashbang] = "flash"; // Concussion Grenade
   m_weaponAlias[Weapon::Explosive] = "hegren"; // High-Explosive Grenade
   m_weaponAlias[Weapon::Smoke] = "sgren"; // Smoke Grenade
   m_weaponAlias[Weapon::Armor] = "vest"; // Kevlar Vest
   m_weaponAlias[Weapon::ArmorHelm] = "vesthelm"; // Kevlar Vest and Helmet
   m_weaponAlias[Weapon::Defuser] = "defuser"; // Defuser Kit
   m_weaponAlias[Weapon::Shield] = "shield"; // Tactical Shield
   m_weaponAlias[Weapon::Knife] = "knife"; // Knife

   m_clients.resize (kGameMaxPlayers + 1);
}

bool BotSupport::isAlive (edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }
   return ent->v.deadflag == DEAD_NO && ent->v.health > 0 && ent->v.movetype != MOVETYPE_NOCLIP;
}

bool BotSupport::isVisible (const Vector &origin, edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }
   TraceResult tr {};
   game.testLine (ent->v.origin + ent->v.view_ofs, origin, TraceIgnore::Everything, ent, &tr);

   if (!cr::fequal (tr.flFraction, 1.0f)) {
      return false;
   }
   return true;
}

void BotSupport::traceDecals (entvars_t *pev, TraceResult *trace, int logotypeIndex) {
   // this function draw spraypaint depending on the tracing results.

   auto logo = conf.getRandomLogoName (logotypeIndex);

   int entityIndex = -1, message = TE_DECAL;
   int decalIndex = engfuncs.pfnDecalIndex (logo.chars ());

   if (decalIndex < 0) {
      decalIndex = engfuncs.pfnDecalIndex ("{lambda06");
   }

   if (cr::fequal (trace->flFraction, 1.0f)) {
      return;
   }
   if (!game.isNullEntity (trace->pHit)) {
      if (trace->pHit->v.solid == SOLID_BSP || trace->pHit->v.movetype == MOVETYPE_PUSHSTEP) {
         entityIndex = game.indexOfEntity (trace->pHit);
      }
      else {
         return;
      }
   }
   else {
      entityIndex = 0;
   }

   if (entityIndex != 0) {
      if (decalIndex > 255) {
         message = TE_DECALHIGH;
         decalIndex -= 256;
      }
   }
   else {
      message = TE_WORLDDECAL;

      if (decalIndex > 255) {
         message = TE_WORLDDECALHIGH;
         decalIndex -= 256;
      }
   }

   if (logo.startsWith ("{")) {
      MessageWriter (MSG_BROADCAST, SVC_TEMPENTITY)
         .writeByte (TE_PLAYERDECAL)
         .writeByte (game.indexOfEntity (pev->pContainingEntity))
         .writeCoord (trace->vecEndPos.x)
         .writeCoord (trace->vecEndPos.y)
         .writeCoord (trace->vecEndPos.z)
         .writeShort (static_cast <short> (game.indexOfEntity (trace->pHit)))
         .writeByte (decalIndex);
   }
   else {
      MessageWriter msg;

      msg.start (MSG_BROADCAST, SVC_TEMPENTITY)
         .writeByte (message)
         .writeCoord (trace->vecEndPos.x)
         .writeCoord (trace->vecEndPos.y)
         .writeCoord (trace->vecEndPos.z)
         .writeByte (decalIndex);

      if (entityIndex) {
         msg.writeShort (entityIndex);
      }
      msg.end ();
   }
}

bool BotSupport::isPlayer (edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }

   if (ent->v.flags & FL_PROXY) {
      return false;
   }

   if ((ent->v.flags & (FL_CLIENT | FL_FAKECLIENT)) || bots[ent] != nullptr) {
      return !strings.isEmpty (ent->v.netname.chars ());
   }
   return false;
}

bool BotSupport::isMonster (edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }

   if (~ent->v.flags & FL_MONSTER) {
      return false;
   }

   if (isHostageEntity (ent)) {
      return false;
   }

   return true;
}

bool BotSupport::isItem (edict_t *ent) {
   return ent && ent->v.classname.str ().contains ("item_");
}

bool BotSupport::isPlayerVIP (edict_t *ent) {
   if (!game.mapIs (MapFlags::Assassination)) {
      return false;
   }

   if (!isPlayer (ent)) {
      return false;
   }
   return *(engfuncs.pfnInfoKeyValue (engfuncs.pfnGetInfoKeyBuffer (ent), "model")) == 'v';
}

bool BotSupport::isDoorEntity (edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }
   return ent->v.classname.str ().startsWith ("func_door");
}

bool BotSupport::isHostageEntity (edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }
   const auto classHash = ent->v.classname.str ().hash ();

   constexpr auto kHostageEntity = StringRef::fnv1a32 ("hostage_entity");
   constexpr auto kMonsterScientist = StringRef::fnv1a32 ("monster_scientist");

   return classHash == kHostageEntity || classHash == kMonsterScientist;
}

bool BotSupport::isShootableBreakable (edict_t *ent) {
   if (game.isNullEntity (ent)) {
      return false;
   }
   const auto limit = cv_breakable_health_limit.float_ ();

   // not shootable
   if (ent->v.health >= limit) {
      return false;
   }
   constexpr auto kFuncBreakable = StringRef::fnv1a32 ("func_breakable");
   constexpr auto kFuncPushable = StringRef::fnv1a32 ("func_pushable");
   constexpr auto kFuncWall = StringRef::fnv1a32 ("func_wall");

   if (ent->v.takedamage > 0.0f && ent->v.impulse <= 0 && !(ent->v.flags & FL_WORLDBRUSH) && !(ent->v.spawnflags & SF_BREAK_TRIGGER_ONLY)) {
      auto classHash = ent->v.classname.str ().hash ();

      if (classHash == kFuncBreakable || (classHash == kFuncPushable && (ent->v.spawnflags & SF_PUSH_BREAKABLE)) || classHash == kFuncWall) {
         return ent->v.movetype == MOVETYPE_PUSH || ent->v.movetype == MOVETYPE_PUSHSTEP;
      }
   }
   return false;
}

bool BotSupport::isFakeClient (edict_t *ent) {
   if (bots[ent] != nullptr || (!game.isNullEntity (ent) && (ent->v.flags & FL_FAKECLIENT))) {
      return true;
   }
   return false;
}

void BotSupport::checkWelcome () {
   // the purpose of this function, is  to send quick welcome message, to the listenserver entity.

   if (game.isDedicated () || !cv_display_welcome_text.bool_ () || !m_needToSendWelcome) {
      return;
   }
   m_welcomeReceiveTime = 0.0f;


   const bool needToSendMsg = (graph.length () > 0 ? m_needToSendWelcome : true);
   auto receiveEntity = game.getLocalEntity ();

   if (isAlive (receiveEntity) && m_welcomeReceiveTime < 1.0f && needToSendMsg) {
      m_welcomeReceiveTime = game.time () + 4.0f; // receive welcome message in four seconds after game has commencing
   }

   if (m_welcomeReceiveTime > 0.0f && needToSendMsg) {
      if (!game.is (GameFlags::Mobility | GameFlags::Xash3D)) {
         game.serverCommand ("speak \"%s\"", m_sentences.random ());
      }
      String authorStr = "Official Navigation Graph";

      StringRef graphAuthor = graph.getAuthor ();
      StringRef graphModified = graph.getModifiedBy ();

      if (!graphAuthor.startsWith (product.name)) {
         authorStr.assignf ("Navigation Graph by: %s", graphAuthor);

         if (!graphModified.empty ()) {
            authorStr.appendf (" (Modified by: %s)", graphModified);
         }
      }

      MessageWriter (MSG_ONE, msgs.id (NetMsg::TextMsg), nullptr, receiveEntity)
         .writeByte (HUD_PRINTTALK)
         .writeString (strings.format ("----- %s v%s {%s}, (c) %s, by %s (%s)-----", product.name, product.version, product.date, product.year, product.author, product.url));

      MessageWriter (MSG_ONE, SVC_TEMPENTITY, nullptr, receiveEntity)
         .writeByte (TE_TEXTMESSAGE)
         .writeByte (1)
         .writeShort (MessageWriter::fs16 (-1.0f, 13.0f))
         .writeShort (MessageWriter::fs16 (-1.0f, 13.0f))
         .writeByte (2)
         .writeByte (rg.get (33, 255))
         .writeByte (rg.get (33, 255))
         .writeByte (rg.get (33, 255))
         .writeByte (0)
         .writeByte (rg.get (230, 255))
         .writeByte (rg.get (230, 255))
         .writeByte (rg.get (230, 255))
         .writeByte (200)
         .writeShort (MessageWriter::fu16 (0.0078125f, 8.0f))
         .writeShort (MessageWriter::fu16 (2.0f, 8.0f))
         .writeShort (MessageWriter::fu16 (6.0f, 8.0f))
         .writeShort (MessageWriter::fu16 (0.1f, 8.0f))
         .writeString (strings.format ("\nHello! You are playing with %s v%s\nDevised by %s\n\n%s", product.name, product.version, product.author, authorStr));

      m_welcomeReceiveTime = 0.0f;
      m_needToSendWelcome = false;
   }
}

bool BotSupport::findNearestPlayer (void **pvHolder, edict_t *to, float searchDistance, bool sameTeam, bool needBot, bool needAlive, bool needDrawn, bool needBotWithC4) {
   // this function finds nearest to to, player with set of parameters, like his
   // team, live status, search distance etc. if needBot is true, then pvHolder, will
   // be filled with bot pointer, else with edict pointer(!).

   searchDistance = cr::sqrf (searchDistance);

   edict_t *survive = nullptr; // pointer to temporally & survive entity
   float nearestPlayer = 4096.0f; // nearest player

   const int toTeam = game.getTeam (to);

   for (const auto &client : m_clients) {
      if (!(client.flags & ClientFlags::Used) || client.ent == to) {
         continue;
      }

      if ((sameTeam && client.team != toTeam) || (needAlive && !(client.flags & ClientFlags::Alive)) || (needBot && !bots[client.ent]) || (needDrawn && (client.ent->v.effects & EF_NODRAW)) || (needBotWithC4 && (client.ent->v.weapons & Weapon::C4))) {
         continue; // filter players with parameters
      }
      const float distanceSq = client.ent->v.origin.distanceSq (to->v.origin);

      if (distanceSq < nearestPlayer && distanceSq < searchDistance) {
         nearestPlayer = distanceSq;
         survive = client.ent;
      }
   }

   if (game.isNullEntity (survive)) {
      return false; // nothing found
   }

   // fill the holder
   if (needBot) {
      *pvHolder = reinterpret_cast <void *> (bots[survive]);
   }
   else {
      *pvHolder = reinterpret_cast <void *> (survive);
   }
   return true;
}

void BotSupport::updateClients () {

   // record some stats of all players on the server
   for (int i = 0; i < game.maxClients (); ++i) {
      edict_t *player = game.playerOfIndex (i);
      Client &client = m_clients[i];

      if (!game.isNullEntity (player) && (player->v.flags & FL_CLIENT)) {
         client.ent = player;
         client.flags |= ClientFlags::Used;

         if (util.isAlive (player)) {
            client.flags |= ClientFlags::Alive;
         }
         else {
            client.flags &= ~ClientFlags::Alive;
         }

         if (client.flags & ClientFlags::Alive) {
            client.origin = player->v.origin;
            sounds.simulateNoise (i);
         }
      }
      else {
         client.flags &= ~(ClientFlags::Used | ClientFlags::Alive);
         client.ent = nullptr;
      }
   }
}

int BotSupport::getPingBitmask (edict_t *ent, int loss, int ping) {
   // this function generates bitmask for SVC_PINGS engine message
   // see:
   //    https://github.com/dreamstalker/rehlds/blob/a680f18ee1e7eb8c39fbdc45682163ca9477d783/rehlds/engine/sv_main.cpp#L4590

   const auto emit = [] (int s0, int s1, int s2) {
      return (s0 & (cr::bit (s1) - 1)) << s2;
   };
   return emit (loss, 7, 18) | emit (ping, 12, 6) | emit (game.indexOfPlayer (ent), 5, 1) | 1;
}

void BotSupport::calculatePings () {
   worker.enqueue ([this] () {
      syncCalculatePings ();
   });
}

void BotSupport::syncCalculatePings () {
   if (!game.is (GameFlags::HasFakePings) || cv_show_latency.int_ () != 2) {
      return;
   }
   MutexScopedLock lock (m_cs);

   Twin <int, int> average { 0, 0 };
   int numHumans = 0;

   // first get average ping on server, and store real client pings
   for (auto &client : m_clients) {
      if (!(client.flags & ClientFlags::Used) || isFakeClient (client.ent)) {
         continue;
      }
      int ping, loss;
      engfuncs.pfnGetPlayerStats (client.ent, &ping, &loss);

      // @note: for those who asking on a email, we CAN call pfnGetPlayerStats hl-engine function in a separate thread
      // since the function doesn't modify anything inside engine, so race-condition and crash isn't viable situation
      // it's just fills ping and loss from engine structures, the only way to cause crash in separate thread
      // is to call it with a invalid ``client`` pointer (on goldsrc), thus causing Con_Printf which is not compatible with
      // multi-threaded environment
      //
      // see:
      //    https://github.com/dreamstalker/rehlds/blob/a680f18ee1e7eb8c39fbdc45682163ca9477d783/rehlds/engine/pr_cmds.cpp#L2735C15-L2735C32
      //    https://github.com/fwgs/xash3d-fwgs/blob/f5b9826fd9bbbdc5293c1ff522de11ce28d3c9f2/engine/server/sv_game.c#L4443

      // store normal client ping
      client.ping = getPingBitmask (client.ent, loss, ping > 0 ? ping : rg.get (8, 16)); // getting player ping sometimes fails
      ++numHumans;

      average.first += ping;
      average.second += loss;
   }

   if (numHumans > 0) {
      average.first /= numHumans;
      average.second /= numHumans;
   }
   else {
      average.first = rg.get (30, 40);
      average.second = rg.get (5, 10);
   }

   // now calculate bot ping based on average from players
   for (auto &client : m_clients) {
      if (!(client.flags & ClientFlags::Used)) {
         continue;
      }
      auto bot = bots[client.ent];

      // we're only interested in bots here
      if (!bot) {
         continue;
      }
      const int part = static_cast <int> (static_cast <float> (average.first) * 0.2f);

      int botPing = bot->m_basePing + rg.get (average.first - part, average.first + part) + rg.get (bot->m_difficulty / 2, bot->m_difficulty);
      const int botLoss = rg.get (average.second / 2, average.second);

      if (botPing <= 5) {
         botPing = rg.get (10, 23);
      }
      else if (botPing > 70) {
         botPing = rg.get (30, 40);
      }
      client.ping = getPingBitmask (client.ent, botLoss, botPing);
   }
}

void BotSupport::emitPings (edict_t *to) {
   MessageWriter msg;

   auto isThirdpartyBot = [] (edict_t *ent) {
      return !bots[ent] && (ent->v.flags & FL_FAKECLIENT);
   };

   for (auto &client : m_clients) {
      if (!(client.flags & ClientFlags::Used) || client.ent == game.getLocalEntity () || isThirdpartyBot (client.ent)) {
         continue;
      }

      // no ping, no fun
      if (!client.ping) {
         client.ping = getPingBitmask (client.ent, rg.get (5, 10), rg.get (15, 40));
      }

      msg.start (MSG_ONE_UNRELIABLE, SVC_PINGS, nullptr, to)
         .writeLong (client.ping)
         .end ();
   }
   return;
}

void BotSupport::installSendTo () {
   // if previously requested to disable?
   if (!cv_enable_query_hook.bool_ ()) {
      if (m_sendToDetour.detoured ()) {
         disableSendTo ();
      }
      return;
   }

   // do not enable on not dedicated server
   if (!game.isDedicated ()) {
      return;
   }
   using SendToHandle = decltype (sendto);
   SendToHandle *sendToAddress = sendto;

   // linux workaround with sendto
   if (!plat.win && !plat.arm) {
      SharedLibrary engineLib {};
      engineLib.locate (reinterpret_cast <void *> (engfuncs.pfnPrecacheModel));

      if (engineLib) {
         auto address = engineLib.resolve <SendToHandle *> ("sendto");

         if (address != nullptr) {
            sendToAddress = address;
         }
      }
   }
   m_sendToDetour.initialize ("ws2_32.dll", "sendto", sendToAddress);

   // enable only on modern games
   if (!game.is (GameFlags::Legacy) && (plat.nix || plat.win) && !plat.arm && !m_sendToDetour.detoured ()) {
      m_sendToDetour.install (reinterpret_cast <void *> (BotSupport::sendTo), true);
   }
}

bool BotSupport::isModel (const edict_t *ent, StringRef model) {
   return model.startsWith (ent->v.model.chars (9));
}

String BotSupport::getCurrentDateTime () {
   time_t ticks = time (&ticks);
   tm timeinfo {};

   plat.loctime (&timeinfo, &ticks);

   auto timebuf = strings.chars ();
   strftime (timebuf, StringBuffer::StaticBufferSize, "%d-%m-%Y %H:%M:%S", &timeinfo);

   return String (timebuf);
}

int32_t BotSupport::sendTo (int socket, const void *message, size_t length, int flags, const sockaddr *dest, int destLength) {
   const auto send = [&] (const Twin <const uint8_t *, size_t> &msg) -> int32_t {
      return Socket::sendto (socket, msg.first, msg.second, flags, dest, destLength);
   };

   auto packet = reinterpret_cast <const uint8_t *> (message);
   constexpr int32_t packetLength = 5;

   // player replies response
   if (length > packetLength && memcmp (packet, "\xff\xff\xff\xff", packetLength - 1) == 0) {
      if (packet[4] == 'D') {
         QueryBuffer buffer { packet, length, packetLength };
         auto count = buffer.read <uint8_t> ();

         for (uint8_t i = 0; i < count; ++i) {
            buffer.skip <uint8_t> (); // number
            auto name = buffer.readString (); // name
            buffer.skip <int32_t> (); // score

            auto ctime = buffer.read <float> (); // override connection time
            buffer.write <float> (bots.getConnectTime (name, ctime));
         }
         return send (buffer.data ());
      }
      else if (packet[4] == 'I') {
         QueryBuffer buffer { packet, length, packetLength };
         buffer.skip <uint8_t> (); // protocol

         // skip server name, folder, map game
         for (size_t i = 0; i < 4; ++i) {
            buffer.skipString ();
         }
         buffer.skip <short> (); // steam app id
         buffer.skip <uint8_t> (); // players
         buffer.skip <uint8_t> (); // maxplayers
         buffer.skip <uint8_t> (); // bots
         buffer.write <uint8_t> (0); // zero out bot count

         return send (buffer.data ());
      }
      else if (packet[4] == 'm') {
         QueryBuffer buffer { packet, length, packetLength };

         buffer.shiftToEnd (); // shift to the end of buffer
         buffer.write <uint8_t> (0); // zero out bot count

         return send (buffer.data ());
      }
   }
   return send ({ packet, length });
}

StringRef BotSupport::weaponIdToAlias (int32_t id) {
   StringRef none = "none";

   if (m_weaponAlias.exists (id)) {
      return m_weaponAlias[id];
   }
   return none;
}


// helper class for reading wave header
class WaveEndianessHelper final : public NonCopyable {
private:
#if defined (CR_ARCH_CPU_BIG_ENDIAN)
   bool little { false };
#else
   bool little { true };
#endif

public:
   uint16_t read16 (uint16_t value) {
      return little ? value : static_cast <uint16_t> ((value >> 8) | (value << 8));
   }

   uint32_t read32 (uint32_t value) {
      return little ? value : (((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | ((value & 0x00ff0000) >> 8) | ((value & 0xff000000) >> 24));
   }

   bool isWave (char *format) const {
      if (little && memcmp (format, "WAVE", 4) == 0) {
         return true;
      }
      return *reinterpret_cast <uint32_t *> (format) == 0x57415645;
   }
};

float BotSupport::getWaveLength (StringRef filename) {
   auto filePath = strings.joinPath (cv_chatter_path.str (), strings.format ("%s.wav", filename));

   MemFile fp (filePath);

   // we're got valid handle?
   if (!fp) {
      return 0.0f;
   }

   // else fuck with manual search
   struct WavHeader {
      char riff[4];
      uint32_t chunkSize;
      char wave[4];
      char fmt[4];
      uint32_t subchunk1Size;
      uint16_t audioFormat;
      uint16_t numChannels;
      uint32_t sampleRate;
      uint32_t byteRate;
      uint16_t blockAlign;
      uint16_t bitsPerSample;
      char dataChunkId[4];
      uint32_t dataChunkLength;
   } header {};

   static WaveEndianessHelper weh;

   if (fp.read (&header, sizeof (WavHeader)) == 0) {
      logger.error ("Wave File %s - has wrong or unsupported format", filePath);
      return 0.0f;
   }
   fp.close ();

   if (!weh.isWave (header.wave)) {
      logger.error ("Wave File %s - has wrong wave chunk id", filePath);
      return 0.0f;
   }

   if (weh.read32 (header.dataChunkLength) == 0) {
      logger.error ("Wave File %s - has zero length!", filePath);
      return 0.0f;
   }

   const auto length = static_cast <float> (weh.read32 (header.dataChunkLength));
   const auto bps = static_cast <float> (weh.read16 (header.bitsPerSample)) / 8;
   const auto channels = static_cast <float> (weh.read16 (header.numChannels));
   const auto rate = static_cast <float> (weh.read32 (header.sampleRate));

   return length / bps / channels / rate;
}
