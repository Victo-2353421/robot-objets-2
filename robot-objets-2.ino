#include <Arduino.h>


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
constexpr Pin ROUE_AVANT_GAUCHE_AVANCER = 0; // D0/RX
constexpr Pin ROUE_AVANT_GAUCHE_RECULER = 1; // D1/TX
constexpr Pin ROUE_AVANT_GAUCHE_PWM = 2;
//ARRIERE_DROITE
constexpr Pin ROUE_ARRIERE_DROITE_AVANCER = 5;
constexpr Pin ROUE_ARRIERE_DROITE_RECULER = 4;
constexpr Pin ROUE_ARRIERE_DROITE_PWM = 6;
//AVANT_DROITE
constexpr Pin ROUE_AVANT_DROITE_AVANCER = 7;
constexpr Pin ROUE_AVANT_DROITE_RECULER = 8;
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
    static Actions lire(US deltaTime){
        return Actions{};
    }

    /**
     * Affiche la struct dans le moniteur de série
     */
    void print() const{}
};


void gererMouvement(struct Actions actions)
{
    const int16_t avant = actions.avant;
    const int16_t lacet = actions.lacet;
    const int16_t lateral = actions.lateral;

    const int8_t min = ROUE_VITESSE_MIN;
    const int8_t max = ROUE_VITESSE_MAX;
    int8_t avantGauche = util::conversionClamp(avant - lacet - lateral, min, max);
    int8_t arriereGauche = util::conversionClamp(avant - lacet + lateral, min, max);
    int8_t avantDroite = util::conversionClamp(avant + lacet + lateral, min, max);
    int8_t arriereDroite = util::conversionClamp(avant + lacet - lateral, min, max);

    const auto gererOutput = [](int8_t valeur, Pin avancerPin, Pin reculerPin, Pin pwmPin) {
        if(valeur < 0) {
            digitalWrite(avancerPin, LOW);
            digitalWrite(reculerPin, HIGH);
        } else if(valeur > 0) {
            digitalWrite(avancerPin, HIGH);
            digitalWrite(reculerPin, LOW);
        } else {
            digitalWrite(avancerPin, LOW);
            digitalWrite(reculerPin, LOW);
        }

        const auto mapperVitesse = [](int8_t vitesse){
            uint8_t vitessePositive = abs(vitesse);
            if(vitessePositive == 128) vitessePositive = 127;
            //if(vitessePositive == 0) return 0;
            return util::map<uint8_t>(vitessePositive, 0, 127, ROUE_VITESSE_MIN, ROUE_VITESSE_MAX);
        };

        analogWrite(pwmPin, mapperVitesse(valeur));
    };

    gererOutput(avantGauche, ROUE_AVANT_GAUCHE_AVANCER, ROUE_AVANT_GAUCHE_RECULER, ROUE_AVANT_GAUCHE_PWM);
    gererOutput(arriereGauche, ROUE_ARRIERE_GAUCHE_AVANCER, ROUE_ARRIERE_GAUCHE_RECULER, ROUE_ARRIERE_GAUCHE_PWM);
    gererOutput(avantDroite, ROUE_AVANT_DROITE_AVANCER, ROUE_AVANT_DROITE_RECULER, ROUE_AVANT_DROITE_PWM);
    gererOutput(arriereDroite, ROUE_ARRIERE_DROITE_AVANCER, ROUE_ARRIERE_DROITE_RECULER, ROUE_ARRIERE_DROITE_PWM);
}


void setup() {
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

void loop() {
}
