// Cli.cpp — mêmes commandes que DCC_CAN_RMT-Booster + gestion des districts
#include "Cli.h"
#include "Config.h"
#include "DistrictManager.h"
#include "GlobalInject.h"
#include "HubWatch.h"

static String cliLine;

// ---------------------------------------------------------------------------
// Table de commandes : une fonction par commande, aucune succession de else if.
// Chaque entrée : { "nom", paramètre attendu (true/false), fonction }.
// Le paramètre est le reste de la ligne après le nom ("cut 3" -> "3").
// ---------------------------------------------------------------------------
typedef void (*CliCmdFn)(const String& arg);

struct CliCommand {
    const char* name;
    bool        takesArg;
    CliCmdFn    fn;
};

static void cmdDistricts(const String&);      // cf. définitions ci-dessous
static void cmdDistrictsJson(const String&);
static void cmdHub(const String&);
static void cmdCut(const String&);
static void cmdOn(const String&);
static void cmdWatch(const String&);
static void cmdWatchJson(const String&);
static void cmdInject(const String&);
static void cmdReset(const String&);
static void cmdHelp(const String&);

static const CliCommand CLI_COMMANDS[] = {
    { "districts",     false, cmdDistricts     },
    { "districts json", false, cmdDistrictsJson },
    { "hub",           false, cmdHub           },
    { "cut",           true,  cmdCut           },
    { "on",            true,  cmdOn            },
    { "watch",         false, cmdWatch         },
    { "watch json",    false, cmdWatchJson     },
    { "inject",        true,  cmdInject        },
    { "reset",         false, cmdReset         },
    { "help",          false, cmdHelp          },
};
static const uint8_t CLI_COMMANDS_COUNT = sizeof(CLI_COMMANDS) / sizeof(CLI_COMMANDS[0]);

// --- Actions ---------------------------------------------------------------

static void cmdDistricts(const String&) {
    Serial.printf("hub %c (EXT%u) :\n", 'A' + HUB_INSTANCE, HUB_INSTANCE + 1);
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++)
        Serial.printf("  %2u : %s\n", i + 1, dmStateName(i));
}

static void cmdDistrictsJson(const String&) {
    for (uint8_t i = 0; i < HUB_NB_DISTRICTS; i++)
        Serial.printf("{\"hub\":%u,\"district\":%u,\"state\":\"%s\"},\n",
                      HUB_INSTANCE, i + 1, dmStateName(i));
}

static void cmdHub(const String&) {
    Serial.printf("hub %c, ID CAN %u, version %s\n",
                  'A' + HUB_INSTANCE, HUB_CAN_ID, HUB_VERSION);
}

static void cmdCut(const String& arg) {
    int n = arg.toInt();
    if (n >= 1 && n <= HUB_NB_DISTRICTS) dmCut(n - 1);
}

static void cmdOn(const String& arg) {
    int n = arg.toInt();
    if (n >= 1 && n <= HUB_NB_DISTRICTS) dmOn(n - 1);
}

static void cmdWatch(const String&)   { watchPrint(); }
static void cmdWatchJson(const String&) { watchPrintJson(); }

static void cmdInject(const String& arg) {
    if      (arg == "on")  globalInject(true);
    else if (arg == "off") globalInject(false);   // seule branche conditionnelle
}

static void cmdReset(const String&)  { ESP.restart(); }

static void cmdHelp(const String&) {
    Serial.println("commandes : districts [json] | hub | watch [json] | cut N | on N | inject on|off | reset");
}

// --- Répartiteur -----------------------------------------------------------

static void Cli_handle(const String& line) {
    for (uint8_t i = 0; i < CLI_COMMANDS_COUNT; i++) {
        const CliCommand& c = CLI_COMMANDS[i];
        size_t len = strlen(c.name);
        if (!line.startsWith(c.name)) continue;

        if (!c.takesArg) {
            if (line.length() == len) { c.fn(""); return; }
            continue;   // ex. "districts" ne consomme pas "districts json"
        }
        if (line.length() > len + 1 && line.charAt(len) == ' ') {
            c.fn(line.substring(len + 1));   // ex. "cut 3" -> "3"
            return;
        }
        cmdHelp("");                          // "cut" sans numéro -> aide
        return;
    }
    cmdHelp("");                              // commande inconnue
}

void Cli_begin() {
    // Serial est déjà ouvert dans setup(). Réservé à une future
    // initialisation (buffer, écho, vitesse…).
    cliLine = "";
}

void Cli_task() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (cliLine.length()) { cliLine.trim(); Cli_handle(cliLine); cliLine = ""; }
        } else {
            cliLine += c;
        }
    }
}