#include <Arduino.h>
#include <ArduinoBLE.h>


// numéro de pin.
using Pin = uint8_t;
// microsecondes μs
using US = uint32_t;


void assert(bool condition, const char *message) {
    if (condition) return;

    Serial.print("ECHEC D'ASSERTION : ");
    Serial.println(message);

    pinMode(LED_BUILTIN, OUTPUT);

    while (true) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    }
}


namespace util {

/**
 * Comme map de core/arduino/WMath.cpp mais templaté.
 */
template<typename T>
static T map(T x, T in_min, T in_max, T out_min, T out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static constexpr US MICROSECONDES_PAR_SECONDE = 1000000;

/**
 * Cette fonction permet de faire des maths sur, par exemple, des int8_t en
 * évitant les overflows. 
 */
template<typename T, typename U>
static U conversionClamp(T valeur, U min, U max) {
    static_assert(sizeof(T) < 8);
    static_assert(sizeof(U) < 8);
    if(static_cast<int64_t>(valeur) < static_cast<int64_t>(min))
        return min;
    else if(static_cast<int64_t>(max) < static_cast<int64_t>(valeur))
        return max;
    else return static_cast<U>(valeur);
}

} // namespace util




//ARRIERE_GAUCHE
constexpr Pin ROUE_ARRIERE_GAUCHE_AVANCER = A6;
constexpr Pin ROUE_ARRIERE_GAUCHE_RECULER = A7;
constexpr Pin ROUE_ARRIERE_GAUCHE_PWM = 3;
//AVANT_GAUCHE
constexpr Pin ROUE_AVANT_GAUCHE_AVANCER = 1; //normalement 0? // D0/RX
constexpr Pin ROUE_AVANT_GAUCHE_RECULER = 0; //normalement 1? // D1/TX
constexpr Pin ROUE_AVANT_GAUCHE_PWM = 2;
//ARRIERE_DROITE
constexpr Pin ROUE_ARRIERE_DROITE_AVANCER = 5;
constexpr Pin ROUE_ARRIERE_DROITE_RECULER = 4;
constexpr Pin ROUE_ARRIERE_DROITE_PWM = 6;
//AVANT_DROITE
constexpr Pin ROUE_AVANT_DROITE_AVANCER = 8; // normalement 7?
constexpr Pin ROUE_AVANT_DROITE_RECULER = 7; // normalement 8?
constexpr Pin ROUE_AVANT_DROITE_PWM = 9;

// mapper entre 80 à 180
constexpr uint8_t ROUE_VITESSE_MIN = 80;//80;
constexpr uint8_t ROUE_VITESSE_MAX = 180;//180;




/**
 * Cette struct représente les actions à effectuer pour une itération de la boucle
 * `loop` du programme. C'est les instructions que Controlleur lit.
 */
struct Actions {
    // Avancement / recul du robot
    int8_t avant{};
    // Rotation du robot https://fr.wikipedia.org/wiki/Lacet_(mouvement)
    int8_t lacet{};
    // Mouvement latéral du robot
    int8_t lateral{};

    /**
     * Lire l'état de la manette et retourner son Actions correspondant.
     */
    //static Actions lire(US deltaTime){
    //    return Actions{};
    //}

    constexpr Actions(int8_t avant, int8_t lacet, int8_t lateral)
      : avant(avant)
      , lacet(lacet)
      , lateral(lateral)
    {

    }

    /**
     * Affiche la struct dans le moniteur de série
     */
    void print() const{
        Serial.print("avant=");
        Serial.print(avant);
        
        Serial.print(" lacet=");
        Serial.print(lacet);
        
        Serial.print(" lateral=");
        Serial.println(lateral);
    }
};


void gererMouvement(struct Actions actions)
{
    const int16_t avant = actions.avant;
    const int16_t lacet = actions.lacet;
    const int16_t lateral = actions.lateral;

    const int8_t min = -127;
    const int8_t max = 127;
    int8_t avantGauche = util::conversionClamp(avant - lacet - lateral, min, max);
    int8_t arriereGauche = util::conversionClamp(avant - lacet + lateral, min, max);
    int8_t avantDroite = util::conversionClamp(avant + lacet + lateral, min, max);
    int8_t arriereDroite = util::conversionClamp(avant + lacet - lateral, min, max);

    const auto gererOutput = [](int8_t valeur, Pin avancerPin, Pin reculerPin, Pin pwmPin, const char *dbgNom) {
        bool avancerBool;
        bool reculerBool;

        if(valeur < 0) {
            avancerBool = LOW;
            reculerBool = HIGH;
        } else if(valeur > 0) {
            avancerBool = HIGH;
            reculerBool = LOW;
        } else {
            avancerBool = LOW;
            reculerBool = LOW;
        }
        digitalWrite(avancerPin, avancerBool);
        digitalWrite(reculerPin, reculerBool);

        const auto mapperVitesse = [](int8_t vitesse) -> uint8_t {
            uint8_t vitessePositive = abs(vitesse);
            if(vitessePositive == 0) return 0;
            return util::map<uint8_t>(vitessePositive, 0, 127, ROUE_VITESSE_MIN, ROUE_VITESSE_MAX);
        };

        const auto outputPwm = mapperVitesse(valeur);
        analogWrite(pwmPin, outputPwm);

/*
        Serial.print(dbgNom);
        Serial.println(":");
        Serial.print(" avancerPin : ");
        Serial.println(avancerBool);
        Serial.print(" reculerPin : ");
        Serial.println(reculerBool);
        Serial.print(" pwmPin : ");
        Serial.println(outputPwm);
        Serial.println("");
        */
    };

    gererOutput(avantGauche, ROUE_AVANT_GAUCHE_AVANCER, ROUE_AVANT_GAUCHE_RECULER, ROUE_AVANT_GAUCHE_PWM, "avantGauche");
    gererOutput(arriereGauche, ROUE_ARRIERE_GAUCHE_AVANCER, ROUE_ARRIERE_GAUCHE_RECULER, ROUE_ARRIERE_GAUCHE_PWM, "arriereGauche");
    gererOutput(avantDroite, ROUE_AVANT_DROITE_AVANCER, ROUE_AVANT_DROITE_RECULER, ROUE_AVANT_DROITE_PWM, "avantDroite");
    gererOutput(arriereDroite, ROUE_ARRIERE_DROITE_AVANCER, ROUE_ARRIERE_DROITE_RECULER, ROUE_ARRIERE_DROITE_PWM, "arriereDroite");
}



BLEService nusService("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
BLECharacteristic rxChar("6E400002-B5A3-F393-E0A9-E50E24DCCA9E", BLEWrite | BLEWriteWithoutResponse, 20);
BLECharacteristic txChar("6E400003-B5A3-F393-E0A9-E50E24DCCA9E", BLENotify, 20);

void setup() {
    Serial.begin(9600);
    while (!Serial);

    { // Setup BLE
        BLE.begin();
        BLE.setLocalName("Robot6");
        
        nusService.addCharacteristic(rxChar);
        nusService.addCharacteristic(txChar);
        BLE.addService(nusService);
        
        BLE.setAdvertisedService(nusService); // CRUCIAL pour la visibilité
        BLE.advertise();
    }

    pinMode(ROUE_ARRIERE_GAUCHE_AVANCER, OUTPUT);
    pinMode(ROUE_ARRIERE_GAUCHE_RECULER, OUTPUT);
    pinMode(ROUE_ARRIERE_GAUCHE_PWM, OUTPUT);
    pinMode(ROUE_AVANT_GAUCHE_AVANCER, OUTPUT);
    pinMode(ROUE_AVANT_GAUCHE_RECULER, OUTPUT);
    pinMode(ROUE_AVANT_GAUCHE_PWM, OUTPUT);
    pinMode(ROUE_ARRIERE_DROITE_AVANCER, OUTPUT);
    pinMode(ROUE_ARRIERE_DROITE_RECULER, OUTPUT);
    pinMode(ROUE_ARRIERE_DROITE_PWM, OUTPUT);
    pinMode(ROUE_AVANT_DROITE_AVANCER, OUTPUT);
    pinMode(ROUE_AVANT_DROITE_RECULER, OUTPUT);
    pinMode(ROUE_AVANT_DROITE_PWM, OUTPUT);
}

void traiterCommande(char c, struct Actions& actions) {
    constexpr int8_t testVitesse = 127;

    switch (c) {
        case 'w':
        case 'W': {
            actions = Actions(testVitesse, 0, 0);
            Serial.println(F("[avant]"));
        } break;

        case 's':
        case 'S': {
            actions = Actions(-(testVitesse), 0, 0);
            Serial.println(F("[recul]"));
        } break;

        case 'a':
        case 'A': {
            actions = Actions(0, static_cast<int8_t>(-testVitesse), 0);
            Serial.println(F("[rot. gauche]"));
        } break;

        case 'd':
        case 'D': {
            actions = Actions(0, testVitesse, 0);
            Serial.println(F("[rot. droite]"));
        } break;

        case 'q':
        case 'Q': {
            actions = Actions(0, 0, static_cast<int8_t>(-testVitesse));
            Serial.println(F("[lat. gauche]"));
        } break;

        case 'e':
        case 'E': {
            actions = Actions(0, 0, testVitesse);
            Serial.println(F("[lat. droite]"));
        } break;

        case 'x':
        case 'X': {
            actions = Actions(0, 0, 0);
            Serial.println(F("[stop]"));
        } break;

        case '\r':
        case '\n': {
             // ignorer retours de ligne
        } break;

        default: {
            Serial.print(F("Commande inconnue: "));
            Serial.println(c);
        } break;
    }
}


void loop() {
    static Actions actions(0, 0, 0);
    BLE.poll(); // Traite les événements BLE internes

    if (rxChar.written()) {
        int len = rxChar.valueLength();
        const uint8_t* val = rxChar.value();
        
        for (int i = 0; i < len; i++) {
            char c = (char)val[i];
            traiterCommande(c, actions); // ex: 'F' pour Forward, 'S' pour Stop
        }
    }

    gererMouvement(actions);
}
