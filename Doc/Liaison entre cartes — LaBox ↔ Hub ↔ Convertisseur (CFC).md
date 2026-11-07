# 🚂 Hub de distribution DCC — CFC (2 × 8 districts)

Carte ESP32 intermédiaire entre **une sortie EXT de la LaBox** et **8 convertisseurs DCC\_CAN\_RTM-Booster** (un par district). Chaque district dispose d'une sortie DCC 3,3 V **coupable individuellement** et d'une entrée « intensité » TOR (ligne d'injection du convertisseur). Les convertisseurs et boosters existants restent **inchangés**.

**Deux hubs identiques** — l'un sur EXT1, l'autre sur EXT2 — donnent **16 districts au total**, chacun avec son repli global indépendant :


| Hub   | Sortie LaBox | Injection DAC →         | Districts | Nom ID\_X\_nn                    |
| ----- | ------------ | ----------------------- | --------- | -------------------------------- |
| Hub A | EXT1         | GPIO35 (intensité EXT1) | 1…8       | **ID\_H\_01** (`HUB_INSTANCE` 0) |
| Hub B | EXT2         | GPIO39 (intensité EXT2) | 9…16      | **ID\_H\_02** (`HUB_INSTANCE` 1) |


### Nomenclature ID\_X\_nn — les hubs entrent dans la famille

Extension du tableau du DCC\_CAN\_RTM-Booster : **TYPE=5 → lettre `H`**, nommage `ID_H_01`…`ID_H_02` reconstruit côté convertisseur (et supervision PC) comme pour les boosters. À réserver dans le firmware convertisseur :


| TYPE  | Lettre  | Nom                     | Cartes                               |
| ----- | ------- | ----------------------- | ------------------------------------ |
| 0     | `C`     | `ID_C`                  | Convertisseur DCC → CAN (unique)     |
| 1     | `B`     | `ID_B_01`…`ID_B_nn`     | Boosters de puissance                |
| 2     | `S`     | `ID_S_nn`               | Aiguillages / accessoires (réservé)  |
| 3     | `D`     | `ID_D_nn`               | Détection d'occupation (réservé)     |
| **5** | **`H`** | **`ID_H_01`…`ID_H_nn`** | **Hubs de distribution (EXT1/EXT2)** |


⚠️ Les types 2/3 étant réservés à des cartes futures, le type 4 reste libre — on saute directement à 5 pour les hubs afin de garder 4 disponible. Chaque type garde son espace de 254 numéros.

Seule différence entre les deux cartes : la config (`Config.h`) — même firmware, même PCB.

## Principe : DCC direct sur GPIO (8 districts)

Avec 8 districts, l'ESP32 a assez de broches : le **GPIO Matrix** recopie le signal DCC d'entrée vers les **8 broches DCC out**, une par district (fronts d'origine, zéro charge CPU). Couper un district = déconnecter sa broche de la matrice et la forcer à 0. Les injections TOR des convertisseurs arrivent sur un **MCP23017** (I2C, 8 entrées, interruption). Pas de 74HC126, pas de 2ᵉ MCP23017.

## Architecture (par hub)

```mermaid
flowchart TB
    LB["LaBox EXT1 (ou EXT2)"] -- "DCC 3,3 V" --> XOR["XOR SN74LVC1G86"] -- "GPIO 27" --> HUB
    HUB -- "DAC GPIO 25 via 4,7–10 kΩ" --> LBI["broche intensité EXT (GPIO35/39)"]
    subgraph HUB["ESP32 Hub"]
        M["GPIO Matrix : copie DCC<br/>vers 8 broches coupables"]
    end
    HUB -- "DCC out 1" --> C1["Convertisseur 1 — gare cachée"]
    HUB -- "DCC out 8" --> C8["Convertisseur 8"]
    C1 -. "injection = défaut" .-> MCP
    C8 -. "injection" .-> MCP
    MCP["MCP23017 @ 0x20<br/>8 entrées TOR (GPA0–7)"] -- "I2C + INT" --> HUB
    HUB <--> |"CAN 500 kbit/s<br/>(bus commun aux 2 hubs)"| BUS["Bus CAN"]
```

## Brochage ESP32 (commun aux 2 hubs)


| Fonction              | GPIO     | Rôle / câblage                                                    |
| --------------------- | -------- | ----------------------------------------------------------------- |
| DCC in (hub)          | 27       | EXT → XOR SN74LVC1G86 → GPIO27, masse commune LaBox ↔ hub         |
| DCC out 1             | 13       | vers XOR du convertisseur 1                                       |
| DCC out 2             | 14       | vers XOR du convertisseur 2                                       |
| DCC out 3             | 15       | vers XOR du convertisseur 3                                       |
| DCC out 4             | 18       | vers XOR du convertisseur 4                                       |
| DCC out 5             | 19       | vers XOR du convertisseur 5                                       |
| DCC out 6             | 16       | vers XOR du convertisseur 6                                       |
| DCC out 7             | 17       | vers XOR du convertisseur 7                                       |
| DCC out 8             | 32       | vers XOR du convertisseur 8                                       |
| Injection hub → LaBox | 25 (DAC) | résistance 4,7–10 kΩ → GPIO35 (hub A) ou GPIO39 (hub B), ≈ 3,04 V |
| I2C SDA / SCL         | 21 / 22  | MCP23017 @ 0x20, 400 kHz                                          |
| INT MCP23017          | 34       | pull-up 10 kΩ externe, active bas                                 |
| CAN TX / RX           | 5 / 4    | TWAI natif 500 kbit/s, transceiver TJA1050/SN65HVD230             |
| LED WS2812            | 26       | 9 LED (1 générale + 8 districts), 100 nF près du strip            |


GPIO 33 et les entrées 35/36/39 restent libres (l'entrée 35/39 est utilisée côté LaBox, pas côté hub).

## Entrées intensité — MCP23017

- **GPA0…GPA7** = injections des districts 1…8 du hub ;
- ligne d'injection du convertisseur (son GPIO25) → résistance série 4,7–10 kΩ → GPIO du MCP23017 ;
- niveaux : `HIGH` ≈ 3,04 V = défaut confirmé, `LOW` = normal — lisible en TOR, aucun ADC nécessaire ;
- lecture **par interruption** (broche INT → GPIO34), confirmation anti-rebond ≥ 20 ms ;
- pull-ups internes du MCP : ligne lue `LOW` si un convertisseur est absent (pas de faux défaut).

## Logique à niveaux


| Niveau           | Qui agit                 | Effet                                                                                                                                                                                                                   |
| ---------------- | ------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1 — local        | chaque booster (via CAN) | coupe sa propre zone, trafic continu ailleurs                                                                                                                                                                           |
| 1.5 — **hub**    | hub de distribution      | injection du district n détectée → **coupe DCC out n** (broche déconnectée de la matrice, forcée à 0). Le convertisseur n perd son DCC &gt; 500 ms → failsafe → relâche son injection. **Aucun autre district touché.** |
| 2 — retry        | hub                      | réactivation du district après délai (5 s puis 10 s), 2 essais                                                                                                                                                          |
| 3 — repli global | hub                      | si le défaut revient : district verrouillé + **injection DAC du hub vers la broche intensité de SON EXT** → la LaBox coupe uniquement ce segment (EXT1 ou EXT2), l'autre hub et ses 8 districts continuent normalement  |


Avec deux hubs, le repli global ne coupe plus que la **moitié** du réseau — c'est le double avantage d'EXT1 + EXT2 par rapport à un hub de 16.

## Machine à états d'un district

```mermaid
stateDiagram-v2
    [*] --> ACTIF
    ACTIF --> COUPE : injection détectée (>20 ms)
    COUPE --> ATTENTE_RETRY : injection relâchée (convertisseur en failsafe)
    ATTENTE_RETRY --> ACTIF : essai 1 ou 2
    ATTENTE_RETRY --> VERROUILLE : 2 essais épuisés
    VERROUILLE --> ACTIF : acquittement CAN ou série
    VERROUILLE --> INJECTION_GLOBALE : défaut persistant
    INJECTION_GLOBALE --> VERROUILLE : tout est redevenu OK
```

## Séquence de coupure sélective

```mermaid
sequenceDiagram
    participant B as Booster RTM (district n)
    participant C as Convertisseur n
    participant H as Hub de distribution
    participant L as LaBox
    B->>C: FAULT ×3 (télémétrie 0x102)
    C->>C: confirme le défaut
    C-->>H: injection ~3,04 V sur la ligne du district n
    H->>H: coupe DCC out n (GPIO Matrix)
    Note over B,L: les autres districts (des 2 hubs) continuent normalement
    C->>C: DCC perdu > 500 ms → failsafe → relâche l'injection
    H->>C: réactive DCC out n (retry)
    alt défaut persistant
        H-->>L: injection DAC du hub → coupure du segment EXT concerné
    else district sain
        C->>H: injection LOW, télémétrie OK
    end
```

## Câblage des convertisseurs (inchangé)

Chaque convertisseur se branche **comme sur EXT1/EXT2** :


| Signal convertisseur             | Destination               | Remarque                                                 |
| -------------------------------- | ------------------------- | -------------------------------------------------------- |
| entrée DCC (via son XOR, GPIO27) | DCC out n du hub          | même format que EXT LaBox                                |
| injection DAC (GPIO25)           | entrée TOR du MCP23017    | résistance 4,7–10 kΩ, la ligne ne va **plus** à la LaBox |
| CAN TX/RX                        | bus CAN commun aux 2 hubs | supervision 0x102 côté hubs                              |
| masse                            | masse commune hub/LaBox   | obligatoire, déjà imposée par le CAN                     |


⚠️ Chaque convertisseur appartient à un hub précis : les 8 premiers sur le hub A (EXT1), les 8 suivants sur le hub B (EXT2). La nomenclature ID\_B\_xx reste globale sur le bus CAN (B\_01 à B\_16).

## Nomenclature (BOM) — par hub (×2)


| Élément                              | Qté                          |
| ------------------------------------ | ---------------------------- |
| ESP32 DevKitC (WROOM-32)             | 1                            |
| XOR SN74LVC1G86                      | 1                            |
| MCP23017 (DIP/SOIC)                  | 1                            |
| Résistance 4,7–10 kΩ                 | 9 (8 injections + 1 DAC)     |
| Pull-up 10 kΩ (INT)                  | 1                            |
| Transceiver CAN (TJA1050/SN65HVD230) | 1                            |
| Strip WS2812                         | 9 LED (1 + 8)                |
| Résistances 120 Ω                    | 2 par bus CAN (terminaisons) |
| Borniers 3 pts (DCC, GND, inj.)      | 8                            |


## Commandes série du hub

```
districts          état des 8 districts (état, dernier défaut, retries)
districts json     idem JSON pour supervision PC
cut N / on N       coupure / réactivation manuelle d'un district
inject on|off      test de l'injection DAC vers la LaBox
hub                identité du hub (EXT1/EXT2, ID CAN)
reset              redémarrage ESP32
```

## Structure du firmware (miroir de DCC\_CAN\_RMT-Booster)

```
DCC_HUB_DISTRIB-Booster/
├── .vscode/                     configuration VSCode/PlatformIO
├── src/
│   ├── Pins.h            broches et adresses I2C
│   ├── Config.h          seuils, délais, IDs CAN, identité du hub (A ou B)
│   ├── DistrictRouter.h/.cpp      copie GPIO Matrix + coupure directe par broche
│   ├── InjectionInput.h/.cpp     MCP23017 : lecture des 8 injections (INT)
│   ├── DistrictManager.h/.cpp     machine à états des 8 districts
│   ├── GlobalInject.h/.cpp        DAC → broche intensité de l'EXT du hub
│   ├── HubCanConfig.h             Provider CanUniversal (TWAI interne, TX=5/RX=4)
│   ├── CanHub.h/.cpp              télémétrie 0x102 + écoute du bus (ID_B_xx)
│   ├── StatusLed.h/.cpp           WS2812 : 1 LED générale + 8 districts
│   ├── Cli.h/.cpp                 commandes série
│   └── main.cpp                   setup + boucle
├── platformio.ini          espressif32@6.9.0, lib_deps : CanUniversal + NeoPixel
├── README.md               présentation, câblage, protocole CAN, commandes série
└── LICENSE.txt              MIT
```

## Vérification croisée avec DCC\_CAN\_RMT-Booster


| DCC\_CAN\_RMT-Booster (référence)           | DCC\_HUB\_DISTRIB-Booster (hub) | Statut                                                                       |
| ------------------------------------------- | ------------------------------- | ---------------------------------------------------------------------------- |
| `Pins.h`                                    | `Pins.h`                        | ✅ même rôle, mêmes conventions (XOR → GPIO27, TX=5/RX=4, WS2812 GPIO26)      |
| `Config.h`                                  | `Config.h`                      | ✅ + `HUB_INSTANCE` (seule différence hub A / hub B)                          |
| `BoosterCanConfig.h` (Provider CAN)         | `HubCanConfig.h`                | ✅ même découpe : Provider CanUniversal séparé                                |
| `CanBooster.h/.cpp`                         | `CanHub.h/.cpp`                 | ✅ télémétrie 0x102, DLC 5, états OK/FAULT/OFF                                |
| `BoosterWatch.h/.cpp`                       | `HubWatch.h/.cpp`               | ✅ même logique : suivi nominatif ID\_X\_nn, état, dernière vue, streak FAULT |
| `DccRmtDecoder.h/.cpp`                      | `DistrictRouter.h/.cpp`         | ✅ équivalent fonctionnel : RMT RX mesure ↔ GPIO Matrix recopie               |
| `StatusLed.h/.cpp`                          | `StatusLed.h/.cpp`              | ✅ mêmes conventions couleurs, LED 0 générale + 1 LED/district                |
| `Cli.h/.cpp`                                | `Cli.h/.cpp`                    | ✅ commandes série, + `districts`/`cut`/`on`/`hub` spécifiques                |
| `main.cpp`                                  | `main.cpp`                      | ✅ setup + boucle, tâches épinglées par cœur                                  |
| — (fonctionnalité du convertisseur)         | `DistrictManager.h/.cpp`        | ➕ nouveau : machine à états des 8 districts                                  |
| —                                           | `InjectionInput.h/.cpp`         | ➕ nouveau : MCP23017, injections TOR sur interruption                        |
| —                                           | `GlobalInject.h/.cpp`           | ➕ nouveau : DAC → broche intensité LaBox                                     |
| `README.md`                                 | `README.md`                     | ✅ rédigé (canvas README), même plan et même style                            |
| `.vscode/`, `platformio.ini`, `LICENSE.txt` | idem                            | ✅ à copier du projet convertisseur en adaptant `lib_deps`                    |


**À date, les seules fonctions du convertisseur sans équivalent :** les commandes `stats`/`can filter` (le hub n'a pas de statistiques RMT à montrer). Les commandes `boosters`/`boosters json` côté convertisseur devront aussi apprendre le TYPE=5 (`ID_H_01`/`ID_H_02`).

## 🔄 Compatibilité de fonctionnement LaBox ↔ Hub ↔ Convertisseur

### Côté LaBox — configuration `config.Labox.h`


| Action                                    | Détail                                                                                                                                                                                                                                                  |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Choisir le shield avec les 2 boosters** | `#define YOUR_MOTOR_SHIELD_TYPE LABOX_MAIN_BOOSTER_BOOSTER_MOTOR_SHIELD` → EXT1 et EXT2 deviennent deux sorties « Booster » indépendantes : une par hub. La télémétrie/ordre `<1>`/`<0>` DCC-EX reste possible par piste.                               |
| **Polarité identique**                    | les 16 districts proviennent de la même centrale : pas de problème de polarité entre districts d'un même hub. Entre hub A et hub B, vérifier une fois à l'oscillo que les deux EXT sont en phase ; sinon `INVERT_BOOSTER_OUTPUT` dans `config.Labox.h`. |
| **Seuil d'intensité EXT**                 | le hub injecte \~3,04 V, au-dessus du seuil 2500 mA ÷ 0,80 = 2,52 V → LaBox coupe son segment. Ne pas modifier le facteur `0.80` sans recalculer `DAC_INJECT_VALUE` côté hub.                                                                           |
| **Backoff LaBox**                         | après injection, LaBox réessaie de rétablir le DCC avec backoff 40 ms → 10 s. Le hub relâche son injection après retour OK : les deux mécanismes se réarment ensemble sans conflit.                                                                     |
| **RailCom**                               | si `ENABLE_LABOX_RAILCOM` est activé, le GPIO Matrix transporte le cutout intact jusqu'aux convertisseurs : aucune action côté hub. Les boosters aval le régénèrent comme aujourd'hui.                                                                  |
| **Lecture d'intensité EXT**               | inutilisable (elle voit l'injection du hub en niveau 3) — sans importance, la sortie EXT n'alimente que le hub.                                                                                                                                         |


### Côté convertisseur — firmware DCC\_CAN\_RMT-Booster


| Action                           | Détail                                                                                                                                                                                         |
| -------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Réserver TYPE=5 (lettre `H`)** | dans la nomenclature ID\_X\_nn et `BoosterWatch` : lettres `C/B/S/D/H`, pour que `boosters` affiche `ID_H_01`/`ID_H_02` comme les `ID_B_xx`.                                                   |
| **Recâblage de l'injection**     | la ligne DAC GPIO25 du convertisseur ne va **plus** à la LaBox : elle va au MCP23017 du hub (résistance 4,7–10 kΩ). Firmware inchangé.                                                         |
| **Failsafe inchangé**            | DCC perdu &gt; 500 ms → `BOOSTER_OFF` + relâchement de l'injection : c'est ce qui referme la boucle de coupure sélective du hub. Ne pas modifier ces 500 ms sans recaler `INJECT_RELEASE_MS`.  |
| **Confirmation avant injection** | le convertisseur confirme le défaut après 3 trames FAULT consécutives : le hub voit donc l'injection \~3 s après le défaut initial — pendant ce temps, le niveau 1 (booster local) a déjà agi. |


### Côté hub — réglages de coordination


| Paramètre           | Valeur                              | Coordination                                                                                                   |
| ------------------- | ----------------------------------- | -------------------------------------------------------------------------------------------------------------- |
| `INJECT_CONFIRM_MS` | 20 ms                               | « anti-rebond : bien inférieur au maintien d'injection du convertisseur.                                       |
| `INJECT_RELEASE_MS` | 50 ms                               | le convertisseur met &gt; 500 ms à relâcher (failsafe) : le hub attend un front bas **stable** avant le retry. |
| `RETRY_DELAY1/2_MS` | 5 s / 10 s                          | espacés du backoff LaBox (40 ms → 10 s) : deux retries hub avant que le niveau 3 ne soit envisagé.             |
| `LOCK_TO_GLOBAL_MS` | 30 s                                | le repli global n'arrive qu'en dernier recours, après 2 essais + délai.                                        |
| `DAC_INJECT_VALUE`  | 235 (\~3,04 V)                      | à recalculer si le facteur de shunt LaBox change (0,80).                                                       |
| Démarrage           | districts actifs dès `routerInit()` | si DCC absent au boot : silence en entrée = silence en sortie ; tout s'établit dès l'arrivée du DCC.           |


### Ordre de mise sous tension (recommandé)

1. Bus CAN sous tension (hubs + convertisseurs démarrent, `ID_H_xx` vus, `watch` s'emplit) ;
2. LaBox : EXT1/EXT2 émettent le DCC → convertisseurs sortent du failsafe → districts vivants ;
3. En cas de défaut d'un district : la chaîne locale (booster → convertisseur → hub) agit **sans jamais solliciter la LaBox**, sauf défaut persistant.

### Checklist de recette (première mise en service)

- [ ] `watch` sur le hub : tous les `ID_B_xx`, `ID_C` et l'autre `ID_H_xx` présents, pas de `[STALE]` ;
- [ ] `districts` : les 8 districts `ACTIF` ;
- [ ] couper un district (`cut 3`) : la LED 3 passe bleu, le voltmètre DCC du district 3 tombe à 0, les 7 autres restent vivants ;
- [ ] court-circuit volontaire sur un district : niveau 1 (booster local) puis niveau 1.5 (hub), vérifier l'isolement des autres districts ;
- [ ] `inject on` : voltmètre sur GPIO25 ≈ 3,04 V, LaBox coupe son segment, l'autre hub continue ;
- [ ] `inject off` : rétablissement automatique LaBox ;
- [ ] oscillo sur le district le plus éloigné : fronts DCC propres, cutout intact (RailCom si activé).