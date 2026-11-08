# 🚂 DCC\_HUB\_DISTRIB-Booster — Chemins de Fer du Chablais (CFC)

Module **hub de distribution DCC** : une entrée venant de la sortie **EXT1/EXT2 (0–3,3 V)** de la centrale LaBox / CommandStation-EX (via un XOR SN74LVC1G86 sur GPIO 27), et **8 sorties DCC indépendantes** — une par district (gare cachée, zone visible…), chacune **coupable individuellement**. Le retour d'intensité de chaque district est un signal **tout-ou-rien** : la ligne d'injection DAC du convertisseur DCC\_CAN\_RMT-Booster du district, lue via un **MCP23017** sur interruption. Le hub écoute et émet sur le **bus CAN (TWAI natif, TX GPIO 5 / RX GPIO 4, 500 kbit/s)**.

**Deux hubs identiques** — l'un sur EXT1, l'autre sur EXT2 — donnent **16 districts**, avec le même firmware : seule `Config.h` change (`HUB_INSTANCE` 0 ou 1, noms **ID\_H\_01 / ID\_H\_02**).

Basé sur la bibliothèque prune74/CanUniversal. Projets de la même famille : prune74/DCC\_CAN-Booster (version ISR), prune74/DCC\_CAN\_RMT-Booster (convertisseur DCC → CAN).

## Pourquoi le GPIO Matrix ?

Le DCC entrant est recopié **matériellement** vers les 8 broches de sortie, sans décodage ni re-synthèse :


|                       | Distribution par décodage/re-synthèse | Ce projet (GPIO Matrix)                      |
| --------------------- | ------------------------------------- | -------------------------------------------- |
| Copie du signal       | logicielle (reconstruction paquets)   | **matérielle** (matrice de broches)          |
| Fidélité au signal    | cutout recréé, timing nominal         | **fronts d'origine, cutout conservé**        |
| Charge CPU            | décodage + 8 générations              | **aucune** (routage par le matériel)         |
| Latence               | quelques ms                           | **nulle**                                    |
| Coupure d'un district | —                                     | broche déconnectée de la matrice, forcée à 0 |


Le signal DCC de chaque district reste **exactement celui de la LaBox**, y compris les cutouts (RailCom de bout en bout). Le CPU ne s'occupe que du lent : lecture des injections TOR, machine à états des districts, CAN, LED.

## Architecture


| Élément                       | Rôle                                                                       |
| ----------------------------- | -------------------------------------------------------------------------- |
| GPIO Matrix (matériel)        | recopie le signal d'entrée (GPIO 27) vers les 8 broches DCC out            |
| `DistrictRouter`              | route / coupe chaque broche de district (fronts d'origine, coupure = 0)    |
| MCP23017 @ 0x20 (port A)      | 8 entrées TOR : injections des convertisseurs, interruption sur changement |
| `DistrictManager`             | machine à états des 8 districts (coupe / retry / verrouillage)             |
| `GlobalInject` (DAC GPIO 25)  | repli global : injection vers la broche intensité de l'EXT de la LaBox     |
| `CanHub` (TWAI, TX 5 / RX 4)  | télémétrie 0x102 du hub (TYPE=5, ID=HUB\_INSTANCE), écoute des ID\_B\_xx   |
| `StatusLed` (WS2812, GPIO 26) | 9 LED : 1 générale + 1 par district                                        |


```
LaBox (CommandStation-EX)                 ESP32 hub                        Bus CAN
┌─────────────────────┐   EXT1/EXT2    ┌────────────────────────────┐
│  Sortie booster     │  0–3.3 V  ──►  │ XOR SN74LVC1G86 → GPIO 27  │
│  DCC + cutout       │                │        │                   │
│                     │                │   GPIO Matrix (matériel)    │
└─────────────────────┘                │   8 broches coupables ──►──┼──► vers les 8
                                       │                            │    convertisseurs
┌─────────────────────┐   injection    │ MCP23017 ← 8 lignes TOR ◄──┼──◄ (DAC GPIO25
│  Convertisseurs     │   ◄──────────► │ INT → GPIO 34 (I2C 21/22)  │     des cartes)
└─────────────────────┘                │ TWAI : TX=5, RX=4           │ ──► 500 kbit/s
                                       │ DAC GPIO 25 ──► LaBox EXT   │
                                       └────────────────────────────┘
```

## Câblage


| Signal | Origine | ESP32 | Remarques |
| --- | --- | --- | --- |
| DCC logique (hub) | LaBox EXT1/EXT2 via XOR SN74LVC1G86 | GPIO 27 | **Masse commune** LaBox ↔ hub obligatoire |
| DCC out 1…8 | vers XOR des convertisseurs 1…8 | 13, 14, 15, 18, 19, 16, 17, 32 | même format que EXT LaBox |
| Injections TOR 1…8 | DAC GPIO 25 des convertisseurs, via 4,7–10 kΩ | MCP23017 @ 0x20 (GPA0–7) | \~3,04 V = défaut confirmé, lisible en TOR |
| Injection hub → LaBox | DAC natif, via résistance 4,7–10 kΩ | GPIO 25 | broche intensité EXT (GPIO 35 = EXT1, GPIO 39 = EXT2). Jamais &gt; 3,3 V |
| INT MCP23017 | interruption sur changement | GPIO 34 | pull-up 10 kΩ externe |
| CAN TX / RX | TWAI natif | GPIO 5 / 4 | transceiver TJA1050/SN65HVD230, 2 × 120 Ω en bout de bus |
| LED d'état WS2812 ×9 | strip | GPIO 26 | LED 0 = générale, LED 1…8 = districts, 100 nF près du strip |


Le fil DCC de chaque district est torsadé avec son GND ; la masse est commune LaBox ↔ hubs ↔ convertisseurs (déjà imposée par le CAN).

## Principe de la coupure sélective

Les convertisseurs DCC\_CAN\_RMT-Booster restent **inchangés** : ils injectent comme aujourd'hui sur leur ligne DAC en cas de défaut confirmé. La nouveauté : cette ligne ne va plus à la LaBox, elle arrive sur le hub — qui agit **par district** :

1. **Niveau 1 (local, normal)** : chaque booster coupe sa propre zone en cas de défaut — le trafic continue ailleurs.
2. **Niveau 1.5 (hub, coupure sélective)** : l'injection du district n est détectée (anti-rebond ≥ 20 ms) → le hub **coupe DCC out n** (broche déconnectée de la matrice, forcée à 0). Le convertisseur n voit son DCC disparaître &gt; 500 ms → failsafe → il relâche son injection. **Les autres districts, y compris ceux de l'autre hub, continuent normalement.**
3. **Niveau 2 (retry)** : réactivation du district après 5 s puis 10 s, 2 essais.
4. **Niveau 3 (repli global)** : district verrouillé + défaut persistant → le hub injecte via son **DAC (GPIO 25)** une tension \~3,04 V sur la broche intensité de **son** EXT (seuil LaBox : 2500 mA ÷ 0,80 = 2,52 V). LaBox détecte une « surintensité » et **coupe le DCC du segment EXT concerné** — l'autre hub et ses 8 districts ne sont pas touchés. L'injection est relâchée quand tout revient OK : LaBox rétablit le DCC automatiquement.

Le cutout est transporté intact par le GPIO Matrix : le RailCom éventuel de la LaBox traverse le hub sans aucune dégradation.

## Protocole CAN (IDs 11 bits)


| ID | Contenu (côté hub) |
| --- | --- |
| `0x102` | **Télémétrie du hub** : `{ ID, TYPE, mA/10, mV/100, état }` (DLC 5) — TYPE=5 (`H`), ID = `HUB_INSTANCE` (0 → **ID\_H\_01**, 1 → **ID\_H\_02**) ; le hub ne mesure ni courant ni tension (mA=0, mV=0), seul son **état** est significatif : 0=OK, 1=FAULT (districts verrouillés), 3=OFF (districts coupés) |
| `0x102` | **Écoute** des télémétries des autres modules du bus (convertisseur ID\_C, boosters ID\_B\_xx) pour le suivi nominatif `HubWatch` |


Les interactions précises entre les cartes (rôles, timings partagés, réservations de protocole côté convertisseur, configuration LaBox) sont documentées séparèrement dans le document de **liaison entre cartes**.

## LED d'état WS2812 ×9 (GPIO 26)

LED 0 (générale) puis 1 LED par district, tout visible simultanément :


| LED                      | Couleur                        | État                                         |
| ------------------------ | ------------------------------ | -------------------------------------------- |
| **LED 0 — général**      | Vert battement de cœur (1,5 s) | Hub opérationnel                             |
|                          | Rouge clignotant lent (0,5 Hz) | Injection globale active (niveau 3)          |
| **LED 1…8 — district n** | Vert discret                   | District ACTIF                               |
|                          | Rouge                          | Injection présente non encore confirmée      |
|                          | Orange                         | District coupé ou en retry                   |
|                          | Rouge clignotant (1 Hz)        | District verrouillé (2 essais épuisés)       |
|                          | Magenta                        | Injection globale déclenchée par ce district |
|                          | Bleu                           | Coupure manuelle (`cut N`)                   |


Gradation via `WS2812_BRIGHTNESS` dans `Config.h`. Mêmes conventions visuelles que DCC\_CAN\_RMT-Booster.

## Failsafe

- Perte du signal DCC en entrée → le hub le détecte et télémétrie l'état ; les sorties suivent l'entrée (silence en entrée = silence en sortie).
- Convertisseur absent d'un district : pull-ups internes du MCP23017 → injection lue `LOW`, aucun faux défaut.
- Injection maintenue à la coupure du DCC du district : le convertisseur n passe en failsafe et relâche après 500 ms (comportement DCC\_CAN\_RMT-Booster inchangé).
- Redémarrage du hub : tous les districts réactivés, retries remis à zéro.

## Compilation (VSCode + PlatformIO)

```
pio run
pio run -t upload
pio device monitor
```

`platformio.ini` épingle `espressif32@6.9.0` (arduino-esp32 2.0.x). Configuration matérielle : `src/Pins.h` (broches), `src/Config.h` (`HUB_INSTANCE`, seuils, délais, IDs CAN, LED).

Flasher le hub A avec `HUB_INSTANCE 0`, le hub B avec `HUB_INSTANCE 1` — seule différence entre les deux cartes.

## Commandes série (115200 bauds)

```
districts          état des 8 districts du hub
districts json     idem JSON, pour journalisation/supervision PC
hub                identité (A/B, EXT1/EXT2, ID_H_xx, version)
watch              modules vus sur le bus (ID_H_xx, ID_C, ID_B_xx), état, dernière vue
watch json         idem JSON, pour supervision PC
cut N / on N       coupure / réactivation manuelle d'un district
inject on|off      test manuel de l'injection DAC → LaBox (voltmètre sur GPIO 25)
reset              redémarrage ESP32
```

## Limites connues

- 8 districts par hub (broches ESP32) — 16 au total avec les deux hubs EXT1/EXT2.
- Le hub ne mesure ni courant ni tension : la télémétrie 0x102 envoie `mA=0, mV=0` — seuls ID/TYPE/état sont significatifs.
- La lecture d'intensité réelle de la sortie EXT de LaBox devient inutilisable (elle voit l'injection du hub en niveau 3) — sans importance, cette sortie n'alimente que le hub.
- Les GPIO 12 (MTDI), 0, 2 sont évités pour ne pas perturber le boot.

## Auteur

Bruno Rivera (prune74) — Projet Chemins de Fer du Chablais (CFC)