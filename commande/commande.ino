
#include <Servo.h>

int degre_horizontal, degre_vertical, fin, PAS_SERVO = 15, ETAT_ALARM=1,SEUIL_DIODE=30;
String chaine, ID_SERVO = "S",ID_POUSSOIR = "P",ID_DIODE = "D",ID_BUZZER = "B", id_composant, temp, valeur;
char separateur = ',';
Servo servo_horizontal, servo_vertical;
int pin_poussoir = 8;
int state_poussoir = 0;
int stateold_poussoir;
int pin_buzzer=13;
int state_buzzer;
int pin_diode=A5;
int value_diode;
int valueold_diode;
int ack_alarm;
int a_message;
unsigned long it_debut,it_fin,it_diff=5000000;//5 sec

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(500);
  servo_horizontal.attach(2, 550, 2450);
  servo_horizontal.write(90);
  delay(10);
  servo_vertical.attach(4, 550, 2450);
  servo_vertical.write(90);
  delay(10);
  degre_horizontal = 90;
  degre_vertical = 90;
  pinMode(pin_poussoir, INPUT);
  pinMode(pin_buzzer, OUTPUT);
  pinMode(pin_diode, INPUT);
  init_var();
  it_debut=micros();
  a_message=0;
}

void loop() {
  boutton_pressoir();
  lecture_diode();
  fonction_buzzer();
  lecture_commande();
}

void init_var(){
  stateold_poussoir = 0;//1 si le boutton a ete presser | 0 sinon
  valueold_diode=0;//1 si le capteur a detecter | 0 sinon
  ack_alarm=0;// 1 si j'ai recu un ack d e la raspi qu'il a recu le message d'alarm | 0 sinon
}

void boutton_pressoir(){
  if(ETAT_ALARM==1){
    state_poussoir = digitalRead(pin_poussoir);//HIGH (presser) ou LOW
    //ne changer la valeur de stateold_poussoir que si aucune precedente intrusion n'a ete detecter
    stateold_poussoir=(stateold_poussoir==1)?stateold_poussoir:(state_poussoir==HIGH)?1:0;
  }
}

void lecture_diode(){
  if(ETAT_ALARM==1){
    value_diode = analogRead(pin_diode);//entre 0 et 1023 : <SEUIL -> lumiere
    //ne changer la valeur de valueold_diode que si aucune precedente intrusion n'a ete detecter
    valueold_diode=(valueold_diode==1)?valueold_diode:(value_diode<SEUIL_DIODE)?1:0;
  }
}

void fonction_buzzer(){
  if(ETAT_ALARM==1)
    if(stateold_poussoir == 1 || valueold_diode==1){
      tone(pin_buzzer, 1300,100);
      delay(100);
      noTone(pin_buzzer);
      if(a_message==0)
        delay(100); 
      else
        a_message=0;
      it_fin=micros();
      if(ack_alarm==0){
        if(it_fin-it_debut>it_diff){//ENVOYER UNE ALERTE TOUTES LES 5 SEC
          chaine="";
          chaine.concat("ALERT");
          chaine.concat(separateur);
          chaine.concat(ID_POUSSOIR);
          chaine.concat(separateur);
          chaine.concat(String(stateold_poussoir));
          chaine.concat(separateur);
          chaine.concat(ID_DIODE);
          chaine.concat(separateur);
          chaine.concat(String(valueold_diode));
          Serial.println(chaine);
          it_debut=it_fin;
        }
      }
    }
}

void lecture_commande(){
  if (Serial.available() > 0) {
    chaine = Serial.readString();
    //je regarde la nature du message
    fin = chaine.indexOf(separateur);
    temp = chaine.substring(0, fin);
    //message recu : HELLO|ID_COMPOSANT
    if (temp.equals("HELLO")) { //format du message hello : HELLO ->  HELLO
      Serial.println("HELLO");
    }
    else if(temp.equals("ON")){
      ETAT_ALARM=1;
      Serial.println("ON");
    }
    else if(temp.equals("OFF")){
      ETAT_ALARM=0;
      //remettre la valeur de detection des capteur a zero
      init_var();
      Serial.println("OFF");
    }
    else if(temp.equals("ACK")){
      ack_alarm=1;
    }
    else if(temp.equals("FAKE")){
      init_var();
      Serial.println("FAKE:OK");
    }
    else { //id_composant donc   //format du message commande : ID_COMPOSANT:[VALEUR] -> OK| ID_COMPOSANT VALEUR | ERROR
      id_composant = temp;
      fin = chaine.indexOf(separateur);
      if (id_composant.equals(ID_SERVO)) { //la commande est pour les servo moteur
        if (fin != -1) {
          valeur = chaine.substring(fin + 1);
          if (valeur.equals("G")) {
            if (degre_horizontal - PAS_SERVO > 0) {
              degre_horizontal -= PAS_SERVO;
              servo_horizontal.write(degre_horizontal);
            }
          }
          else if (valeur.equals("D")) {
            if (degre_horizontal + PAS_SERVO < 180) {
              degre_horizontal += PAS_SERVO;
              servo_horizontal.write(degre_horizontal);
            }
          }
          else if (valeur.equals("H")) {
            if (degre_vertical - PAS_SERVO > 0) {
              degre_vertical -= PAS_SERVO;
              servo_vertical.write(degre_vertical);
            }
          }
          else if (valeur.equals("B")) {
            if (degre_vertical + PAS_SERVO < 180) {
              degre_vertical += PAS_SERVO;
              servo_vertical.write(degre_vertical);
            }
          }
          Serial.println("S:OK");
        }
        else { //je renvoie les info du servo
          chaine = "";
          chaine.concat(ID_SERVO);
          chaine.concat(separateur);
          chaine.concat(String(servo_horizontal.read()));
          chaine.concat(separateur);
          chaine.concat(String(servo_vertical.read()));
          Serial.println(chaine);
        }
      }
      else if (id_composant.equals(ID_POUSSOIR)){
        chaine = "";
        chaine.concat(ID_POUSSOIR);
        chaine.concat(separateur);
        chaine.concat(String(stateold_poussoir));
        Serial.println(chaine);
      }
      else if (id_composant.equals(ID_BUZZER)){
        chaine = "";
        chaine.concat(ID_BUZZER);
        chaine.concat(separateur);
        int v=(valueold_diode==1 || stateold_poussoir == 1)?1:0;
        chaine.concat(String(v));
        Serial.println(chaine);
      }
      else if (id_composant.equals(ID_DIODE)){
        chaine = "";
        chaine.concat(ID_DIODE);
        chaine.concat(separateur);
        chaine.concat(String(valueold_diode));
        Serial.println(chaine);
      }
      else
        Serial.println("ERROR");
    }
    a_message=1;
  }
}







