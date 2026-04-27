#include <Arduino.h>



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




// numéro de pin.
using Pin = uint8_t;
// microsecondes μs
using US = uint32_t;

//ARRIERE_GAUCHE
constexpr Pin ROUE_1_AVANCER_PIN = A6;
constexpr Pin ROUE_1_RECULER_PIN = A7;
constexpr Pin ROUE_1_PWM_PIN = 3;
//AVANT_GAUCHE
constexpr Pin ROUE_2_AVANCER_PIN = 0; // D0/RX
constexpr Pin ROUE_2_RECULER_PIN = 1; // D1/TX
constexpr Pin ROUE_2_PWM_PIN = 2;
//ARRIERE_DROITE
constexpr Pin ROUE_3_AVANCER_PIN = 5;
constexpr Pin ROUE_3_RECULER_PIN = 4;
constexpr Pin ROUE_3_PWM_PIN = 6;
//AVANT_DROITE
constexpr Pin ROUE_4_AVANCER_PIN = 7;
constexpr Pin ROUE_4_RECULER_PIN = 8;
constexpr Pin ROUE_4_PWM_PIN = 9;

// mapper entre 80 à 180
constexpr int8_t ROUES_VITESSE_MIN = -100;//80;
constexpr int8_t ROUES_VITESSE_MAX = 100;//180;




namespace utils {

/**
 * Comme map de core/arduino/WMath.cpp mais templaté.
 */
template<typename T>
T map(T x, T in_min, T in_max, T out_min, T out_max)
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

} // namespace utils




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
    static Actions lire(US deltaTime){}

    /**
     * Affiche la struct dans le moniteur de série
     */
    void print() const{}
};




void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}
