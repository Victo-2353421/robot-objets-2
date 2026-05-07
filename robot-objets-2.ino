#include <Arduino.h>
#include <ArduinoBLE.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoSound.h>


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
constexpr Pin ROUE_ARRIERE_DROITE_RECULER = A0;
constexpr Pin ROUE_ARRIERE_DROITE_PWM = 6;
//AVANT_DROITE
constexpr Pin ROUE_AVANT_DROITE_AVANCER = 8; // normalement 7?
constexpr Pin ROUE_AVANT_DROITE_RECULER = 7; // normalement 8?
constexpr Pin ROUE_AVANT_DROITE_PWM = 9;

// mapper entre 80 à 180
constexpr uint8_t ROUE_VITESSE_MIN = 80;//80;
constexpr uint8_t ROUE_VITESSE_MAX = 180;//180;

constexpr Pin SD_CS_PIN = 10;
SDWaveFile waveFile;

constexpr Pin LUMINOSITE_PIN = A1;

constexpr Pin YEUX_PIN = A4;



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

    constexpr Actions(int8_t avant, int8_t lacet, int8_t lateral)
      : avant(avant)
      , lacet(lacet)
      , lateral(lateral)
    {}

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
BLEIntCharacteristic lumChar("6E400004-B5A3-F393-E0A9-E50E24DCCA9E",
                                       BLERead | BLENotify);
void setup() {
    Serial.begin(9000);
    while (!Serial);

    { // Setup BLE
        BLE.begin();
        BLE.setLocalName("Robot6");
        
        nusService.addCharacteristic(rxChar);
        nusService.addCharacteristic(txChar);
        nusService.addCharacteristic(lumChar);
        BLE.addService(nusService);
        
        BLE.setAdvertisedService(nusService); // CRUCIAL pour la visibilité
        BLE.advertise();
    }

    { // setup SD et audio
        assert(SD.begin(SD_CS_PIN), "echec SD.begin");
        waveFile = SDWaveFile("audio.wav");

        assert(waveFile, "Format de fichier invalide");

        // Reglage du volume (0 a 100)
        AudioOutI2S.volume(50);

        // Lancement de la lecture en boucle
        AudioOutI2S.loop(waveFile);
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

    pinMode(LUMINOSITE_PIN, INPUT);
    pinMode(YEUX_PIN, OUTPUT);
}

class ByteSlice {
public:
    const uint8_t *ptr;
    size_t len;

    constexpr ByteSlice(const uint8_t *ptr, size_t len)
      : ptr(ptr), len(len) {}
      
    ByteSlice() = default;

    const uint8_t &operator[] (size_t i) const{
        assert(i < len, "ByteSlice OOB");
        return ptr[i];
    }
    
    uint8_t operator[] (size_t i) {
        assert(i < len, "ByteSlice OOB");
        return ptr[i];
    }

    void popFront(size_t n = 1) {
        assert(n <= len, "popFront OOB");
        len -= n;
        ptr += n;
    }

    explicit operator String() const{
        return String((const char*)ptr, len);
    }
    
    operator bool() const{
        return len && ptr;
    }
};

void allumerYeux(bool etat) {
    digitalWrite(YEUX_PIN, etat);
}

bool jouerFichierAudio(const char *nomFichier) {
    Serial.print("Lecture de l'audio : ");
    Serial.println(nomFichier);

    SDWaveFile nouveauFichier(nomFichier);
    if (!nouveauFichier) {
        Serial.print(F("Fichier audio invalide: "));
        Serial.println(nomFichier);
        return false;
    }

    waveFile = nouveauFichier;
    AudioOutI2S.loop(waveFile);
    return true;
}

int8_t vitesseRoues = 127;
void traiterCommande(class ByteSlice &slice, struct Actions& actions) {
    switch (slice[0]) {
        case 'f':
        case 'F': {
            actions = Actions(vitesseRoues, 0, 0);
            Serial.println(F("[avant]"));
            slice.popFront();
        } break;

        case 'b':
        case 'B': {
            actions = Actions(-(vitesseRoues), 0, 0);
            Serial.println(F("[recul]"));
            slice.popFront();
        } break;

        case 'l':
        case 'L': {
            actions = Actions(0, 0, static_cast<int8_t>(-vitesseRoues));
            Serial.println(F("[lat. gauche]"));
            slice.popFront();
        } break;

        case 'r':
        case 'R': {
            actions = Actions(0, 0, vitesseRoues);
            Serial.println(F("[lat. droite]"));
            slice.popFront();
        } break;

        case 'v':
        case 'V': {
            actions = Actions(0, static_cast<int8_t>(-vitesseRoues), 0);
            Serial.println(F("[rot. gauche]"));
            slice.popFront();
        } break;

        case 'x':
        case 'X': {
            actions = Actions(0, vitesseRoues, 0);
            Serial.println(F("[rot. droite]"));
            slice.popFront();
        } break;

        case 's':
        case 'S': {
            actions = Actions(0, 0, 0);
            Serial.println(F("[stop]"));
            slice.popFront();
        } break;
        
        case '-': {
            int16_t nouvelleVitesse = static_cast<int16_t>(vitesseRoues) - 10;
            vitesseRoues = util::conversionClamp<int16_t, int8_t>(nouvelleVitesse, 0, 127);
            Serial.println(F("[- vitesse]"));
            slice.popFront();
        } break;
        
        case '+': {
            int16_t nouvelleVitesse = static_cast<int16_t>(vitesseRoues) + 10;
            vitesseRoues = util::conversionClamp<int16_t, int8_t>(nouvelleVitesse, 0, 127);
            Serial.println(F("[+ vitesse]"));
            slice.popFront();
        } break;

        case 'P': {
            constexpr size_t tailleMaxNomFichier = 20;
            if (slice.len > 2 && slice[1] == ':') { // lire fichier audio
                size_t nomLen = slice.len - 2;
                for (size_t i = 2; i < slice.len; ++i) {
                    if (slice[i] == '\r' || slice[i] == '\n') {
                        nomLen = i - 2;
                        break;
                    }
                }

                if (nomLen > 0 && nomLen <= tailleMaxNomFichier) {
                    String nomFichier(reinterpret_cast<const char*>(slice.ptr + 2), nomLen);
                    jouerFichierAudio(nomFichier.c_str());
                }

                size_t consomme = 2 + nomLen;
                while (consomme < slice.len && (slice[consomme] == '\r' || slice[consomme] == '\n')) {
                    ++consomme;
                }
                slice.popFront(consomme);
            } else {
                jouerFichierAudio("audio.wav");
                slice.popFront();
            }
        } break;
        
        case 'D': {
            if (slice.len >= 4) {
                ByteSlice cmd(slice.ptr, 4);
                if (memcmp(cmd.ptr, "D:ON", 4) == 0){
                    allumerYeux(true);
                    slice.popFront(4);
                } else if(slice.len >= 5){
                    ByteSlice cmd(slice.ptr, 5);
                    if (memcmp(cmd.ptr, "D:OFF", 5) == 0){
                        allumerYeux(false);
                        slice.popFront(5);
                    } 
                }
            }
        } break;

        case '\r':
        case '\n': {
             // ignorer retours de ligne
               slice.popFront();
        } break;

        default: {
            Serial.print(F("Commande inconnue: "));
            Serial.println(slice[0]);
            slice.popFront();
        } break;
    }
}


void loop() {
    static Actions actions(0, 0, 0);
    BLE.poll(); // Traite les événements BLE internes

    if (rxChar.written()) {
        size_t len = rxChar.valueLength();
        const uint8_t* val = rxChar.value();
        
        ByteSlice slice{val, len};

        while (slice.len) {
            traiterCommande(slice, actions); // ex: 'F' pour Forward, 'S' pour Stop
        }
    }

    { // luminosité
        int valeurBrute = analogRead(A1);
        Serial.println(valeurBrute);
        // À FAIRE: convertir en pourcentage

        float pourcentage = util::conversionClamp<int, float>(valeurBrute, 0.0f, 1023.0f) / 10.23f;
        static float lastLumSent = valeurBrute;

        constexpr float CHANGE_THRESHOLD = 10.0f;

        // Envoyer seulement si la valeur a changé d'au moins CHANGE_THRESHOLD % 
        if (abs(pourcentage - lastLumSent) > CHANGE_THRESHOLD) {
            lumChar.writeValue(static_cast<int>(pourcentage + 0.5f));
            lastLumSent = pourcentage;
            Serial.println("Luminosité : " + String(pourcentage) + "%");
        }
    }

    gererMouvement(actions);
}
