#ifndef LOCALIZATION_H
#define LOCALIZATION_H

#include <stdint.h> 
#include <stddef.h> 

// Define supported languages
enum class Language : uint8_t {
    ENGLISH = 0,
    FRENCH = 1,
    _LANGUAGE_COUNT 
};

enum class StringKey {
    // Stats Scene (0-12)
    STATS_TITLE, STAT_AGE, STAT_HEALTH, STAT_HAPPY, STAT_DIRTY, STAT_SICKNESS, STAT_TIRED, STAT_WEIGHT, STAT_PLAYTIME, STAT_SLEEP, STAT_POOP, STAT_HUNGER, HINT_EXIT,
    // Sickness States (13-19)
    SICKNESS_NONE, SICKNESS_COLD, SICKNESS_HOT, SICKNESS_DIARRHEA, SICKNESS_VOMIT, SICKNESS_HEADACHE, SICKNESS_UNKNOWN,
    // Yes/No (20-21)
    YES, NO,
    // Parameters Menu (22-38)
    PARAMS_TITLE, PARAMS_SUB_BLUETOOTH, PARAMS_SUB_WIFI, PARAMS_SUB_RESET, PARAMS_SUB_LANGUAGE, PARAMS_BT_DISCOVERY, PARAMS_BT_DEVICE, PARAMS_BT_CONNECT_NEW, PARAMS_BT_TOGGLE, PARAMS_BT_DISCONNECT, PARAMS_BT_FORGET, PARAMS_WIFI_SSID, PARAMS_WIFI_PASSWORD, PARAMS_WIFI_STATUS, PARAMS_RESET_TAMA, PARAMS_RESET_SETTINGS, PARAMS_RESET_ALL, PARAMS_LANG_SELECT,
    // Language Names (39-40) 
    LANG_ENGLISH, LANG_FRENCH,
    // Action Scene (41-49)
    ACTION_TITLE, ACTION_CLEANING, ACTION_MEDICINE, ACTION_FEEDING, ACTION_PLAYING, ACTION_SLEEP, ACTION_PLACEHOLDER5, ACTION_PLACEHOLDER6, ACTION_PLACEHOLDER7,
    // Cleaning Menu (50-56)
    CLEANING_MENU_TITLE, CLEANING_DOUCHE, CLEANING_TOILET, CLEANING_BROSSE, CLEANING_PAPIER, CLEANING_DENTIFRICE, CLEANING_SERVIETTE,
    // Medicine Menu (57-63)
    MEDICINE_MENU_TITLE, MEDICINE_FIOLE, MEDICINE_SERINGUE, MEDICINE_PANSEMENT, MEDICINE_BANDAGE, MEDICINE_GELULE, MEDICINE_THERMOMETRE,
    // Dialog Messages (64-66)
    DIALOG_ALREADY_SLEEPING,
    DIALOG_NOT_TIRED_ENOUGH,
    DIALOG_STILL_TOO_TIRED,
    // Prequel Related Strings (Starting at 67) 
    PREQUEL_LANG_SELECT_TITLE,     
    PREQUEL_STAGE1_TITLE,          
    PREQUEL_STAGE1_INST_LINE1,     
    PREQUEL_STAGE1_INST_LINE2,     
    PREQUEL_STAGE1_INST_LINE3,     
    PREQUEL_STAGE1_INST_LINE4,     
    PREQUEL_STAGE2_TITLE,          
    PREQUEL_STAGE2_INST_BOND1,     
    PREQUEL_STAGE2_INST_BOND2,     
    PREQUEL_STAGE2_INST_BOND3,     
    PREQUEL_STAGE2_INST_BOND4,     
    PREQUEL_STAGE2_INST_SPEC1,     
    PREQUEL_STAGE2_INST_SPEC2,     
    PREQUEL_STAGE2_INST_SPEC3,     
    PREQUEL_STAGE2_INST_SPEC4,     
    PREQUEL_STAGE3_TITLE,          
    PREQUEL_STAGE3_INST_LINE1,     
    PREQUEL_STAGE3_INST_LINE2,     
    PREQUEL_STAGE3_INST_LINE3,     
    PREQUEL_STAGE3_QTE_PROMPT,     
    PREQUEL_STAGE3_GATE_PASSED,    
    PREQUEL_STAGE3_GATE_FAILED,    
    // Boot Screen Text (89-91) 
    BOOT_BOOTING,                  
    BOOT_WAKING_UP,                
    BOOT_SLEPT_FOR,                
    // Prequel Stage 4 (92-95)
    PREQUEL_STAGE4_TITLE,
    PREQUEL_STAGE4_INST_LINE1,
    PREQUEL_STAGE4_INST_LINE2,
    PREQUEL_STAGE4_INST_LINE3,
    // Weather Names (NEW KEYS starting at 96)
    WEATHER_NONE,
    WEATHER_SUNNY,
    WEATHER_CLOUDY,
    WEATHER_RAINY,
    WEATHER_HEAVY_RAIN,
    WEATHER_SNOWY,
    WEATHER_HEAVY_SNOW,
    WEATHER_STORM,
    WEATHER_RAINBOW,
    WEATHER_WINDY,
    WEATHER_FOG,
    WEATHER_AURORA,
    WEATHER_UNKNOWN,


    _STRING_KEY_COUNT 
};

const char* const translations_en_arr[] = {
    // ... (previous translations up to PREQUEL_STAGE4_INST_LINE3) ...
    "Stats", "Age", "Health", "Happy", "Dirty", "Sick", "Tired", "Weight", "Play", "Sleep", "Poop", "Hunger", "(Long OK=exit)",
    "None", "Cold", "Hot", "Diarrhea", "Vomit", "Headache", "Unknown",
    "Yes", "No",
    "Parameters", "Bluetooth", "WiFi", "Reset", "Language", "Discovery", "Device", "Connect New", "Toggle Discovery", "Disconnect", "Forget Devices", "SSID", "Password", "Status", "Reset Tama Stats", "Reset Settings", "Reset All", "Language",
    "English", "French",
    "Actions", "Clean", "Medicine", "Feed", "Play", "Sleep", "??? 5", "??? 6", "??? 7",
    "Cleaning", "Shower", "Toilet", "Brush", "Paper", "Toothpaste", "Towel",
    "Medicine", "Vial", "Syringe", "Band-Aid", "Bandage", "Pill", "Thermometer",
    "Already sleeping!", "Not tired enough!", "Still too tired!",
    "Select Language", "Stage 1: Awakening",
    "A new spark flickers...",
    "Hold LEFT to Attract.",
    "Press RIGHT to Absorb.",
    "Press OK to Pulse.",
    "Stage 2: Formation",
    "Guide cells, avoid voids.",
    "LEFT/RIGHT nudges.",
    "OK near cells -> Bond Game!",
    "Press OK in the zone!",
    "Specialization: A:2 B:2 C:1",
    "LEFT cycles type.",
    "RIGHT selects cell.",
    "OK applies type.",
    "Stage 3: The Journey",          
    "Navigate the abstract flow.",   
    "Buttons adapt to zones.",       
    "OK (Long) to close dialog.",    
    "Match Sequence!",               
    "Gate Opened!",                  
    "Gate Sequence Failed!",         
    "Booting...",
    "Waking up...",
    "Slept",                          
    "Stage 4: Shell Weave",
    "Collect A, B, C fragments.",
    "Match target pattern above.",
    "L/R moves, OK no action yet.",
    // Weather Names
    "Clear", "Sunny", "Cloudy", "Rainy", "Heavy Rain", "Snowy", "Heavy Snow", "Storm", "Rainbow", "Windy", "Fog", "Aurora", "Unknown"
};

const char* const translations_fr_arr[] = {
    // ... (previous translations up to PREQUEL_STAGE4_INST_LINE3) ...
    "Stats", "Age", "Sante", "Bonheur", "Salete", "Malade", "Fatigue", "Poids", "Jeu", "Dodo", "Caca", "Faim", "(OK Long=sortir)",
    "Aucune", "Rhume", "Chaud", "Diarrhee", "Vomi", "Migraine", "Inconnue",
    "Oui", "Non",
    "Parametres", "Bluetooth", "WiFi", "Reset", "Langue", "Decouverte", "Appareil", "Connecter Nouv.", "Activer Decouv.", "Deconnecter", "Oublier App.", "SSID", "Mot de passe", "Statut", "Reset Stats Tama", "Reset Parametres", "Reset Tout", "Langue",
    "Anglais", "Francais",
    "Actions", "Nettoyer", "Soigner", "Nourrir", "Jouer", "Dormir", "??? 5", "??? 6", "??? 7",
    "Nettoyage", "Douche", "Toilette", "Brosse", "Papier", "Dentifrice", "Serviette",
    "Medicaments", "Fiole", "Seringue", "Pansement", "Bandage", "Gelule", "Thermometre",
    "Dort deja !", "Pas assez fatigue !", "Encore trop fatigue !",
    "Choisir Langue", "Etape 1: Eveil",
    "Une etincelle s'eveille...",
    "MAINTENIR GAUCHE: Attirer.",
    "APPUYER DROITE: Absorber.",
    "APPUYER OK: Impulsion.",
    "Etape 2: Formation",
    "Guidez cellules, evitez vides.",
    "GAUCHE/DROITE pousse.",
    "OK pres cellules -> Jeu Lien!",
    "Appuyez OK dans la zone!",
    "Specialisation: A:2 B:2 C:1",
    "GAUCHE change type.",
    "DROITE select. cellule.",
    "OK applique type.",
    "Etape 3: Le Voyage",            
    "Naviguez le flux abstrait.",    
    "Boutons adaptes aux zones.",    
    "OK (Long) ferme dialogue.",     
    "Suivez la Sequence!",           
    "Porte Ouverte!",                
    "Sequence Porte Echouee!",       
    "Demarrage...",
    "Reveil...",
    "Dormi",
    "Etape 4: Tissage",
    "Collectez fragments A, B, C.",
    "Suivez le patron ci-dessus.",
    "G/D bouge, OK rien pour l'instant.",
    // Weather Names
    "Degage", "Ensoleille", "Nuageux", "Pluvieux", "Forte Pluie", "Neigeux", "Fortes Neiges", "Orage", "Arc-en-ciel", "Venteux", "Brouillard", "Aurore", "Inconnu"
};

const char* const* const language_tables[] = {
    translations_en_arr, 
    translations_fr_arr, 
};

extern Language currentLanguage;
inline const char* loc(StringKey key) {
    size_t keyIndex = static_cast<size_t>(key); size_t langIndex = static_cast<size_t>(currentLanguage);
    if (keyIndex >= static_cast<size_t>(StringKey::_STRING_KEY_COUNT)) { return "INV_KEY"; }
    if (langIndex >= static_cast<size_t>(Language::_LANGUAGE_COUNT)) { langIndex = static_cast<size_t>(Language::ENGLISH); }
    if (currentLanguage == static_cast<Language>(255)) { 
        langIndex = static_cast<size_t>(Language::ENGLISH);
    }
    const char* localizedString = language_tables[langIndex][keyIndex];
    if (localizedString == nullptr || localizedString[0] == '\0') { if (currentLanguage != Language::ENGLISH && currentLanguage != static_cast<Language>(255)) { langIndex = static_cast<size_t>(Language::ENGLISH); localizedString = language_tables[langIndex][keyIndex]; } }
    if (localizedString == nullptr || localizedString[0] == '\0') { return "???"; } return localizedString;
}

#endif // LOCALIZATION_H