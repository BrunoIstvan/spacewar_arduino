#include <Arduino.h>
#include <LiquidCrystal.h>

#define VELOCITY 200

#define NOTE_G4  392
#define NOTE_G5  784
#define NOTE_G6  1568

#define START_BUTTON 13
#define PANEL_POSITION 14
#define MAX_INITIAL_OBJECTS_POSITION_X 13
#define vrx A0  
#define vry A1  
#define ENERGY_POSSIBILITY 30

#define MAX_ENERGY 100
#define SCORE_TO_WIN 100
#define MIN_SCORE 0

#define ASTEROID_1 1
#define ASTEROID_2 2
#define ASTEROID_3 3
#define ASTEROID_4 4

#define SHIP 5
#define SHOOT 6
#define POWER 7
#define EXPLOSION 8

const byte asteroidForm1[8] = {B00100,B01110,B10111,B11011,B11110,B01011,B11110,B00100};
const byte asteroidForm2[8] = {B00000,B01110,B10111,B11111,B11011,B10101,B01110,B00000};
const byte asteroidForm3[8] = {B01110,B10011,B01010,B11110,B11110,B01010,B10011,B01110};
const byte asteroidForm4[8] = {B11001,B10110,B00100,B11100,B11100,B00100,B10110,B11001};
// const byte asteroidForm5[8] = {B10001,B01010,B10100,B10100,B10100,B10100,B01010,B10001};

const byte shipForm[8]      = { B11000, B01000, B01100, B11011, B11011, B01100, B01000, B11000 };
const byte explosionForm[8] = { B01010, B10101, B01010, B10001, B10001, B01010, B10101, B01010 };
const byte powerForm[8]     = { B00011, B00110, B01100, B11111, B11111, B00110, B01100, B11000 };
const byte shootForm[8]     = { B00000, B00000, B10010, B01001, B01001, B10010, B00000, B00000 };

int pyShip = 0, pxShip = 0, pyPower = 0, pxShoot = 0, pyShoot = 0, pxExplosion = 0, pyExplosion = 0;
int pxAsteroid = MAX_INITIAL_OBJECTS_POSITION_X, pyAsteroid = 0;
int pxPower = MAX_INITIAL_OBJECTS_POSITION_X;
bool game = false, hasShoot = false, hasPower = false, playSound = false;
int score = MIN_SCORE, rotationEnergy = 0;
double energy = MAX_ENERGY;
byte asteroidGenerated = 0;

int switchValue = 0, xValue = 0, yValue = 0;

int PIEZO = 12;
long freqIn, blow1, blow2;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

enum STATUS { INITIAL, WIN, LOSE, NO_ENERGY };

void setup() {
  
  // inicializar serial
  Serial.begin(9600);
  
  // inicializar display
  lcd.begin(16, 2);
  lcd.clear();

  pinMode(START_BUTTON, INPUT_PULLUP);
  pinMode(PIEZO, OUTPUT);
 
  // incluir personagens na memoria do display
  createCharacters();

}

void loop() {

  if(game) {

    lcd.clear();
    
    // imprimir pontos e energia
    drawPanel(PANEL_POSITION);

    // avalidar se o joystick esta direcionado para algum lugar e desenhar nave
    availableShip();  
    
    // avaliar se deve gerar um tiro
    availableShoot();

    // verificar se asteroid chegou no limite da tela
    availableAsteroid();

    // avaliar se vai gerar uma energia
    availablePower();
    
    // avaliar se houve colisao entre nave e asteroid
    availableShipAndAsteroidCollision();

    // avaliar se houve colisao entre tiro e asteroid
    availableShootAndAsteroidCollision();

    // avaliar se houve colisao entre nave e energia
    availableShipAndPowerCollision();

    // avaliar se jogador atingiu pontuacao maxima para vitoria
    availableScoreGame();

    // avaliar se energia acabou
    availablePowerShip();

    // avaliar se houve colisao entre tiro e energia
    availableShootAndPowerCollision();
    
    energy -= 0.25;
    
    delay(VELOCITY);
    
  } else {

    drawMessageStoppedGame(INITIAL);
    
    // se jogador apertou botao, entao iniciar o jogo
    if(digitalRead(START_BUTTON) == 0) {
      resetGame();
    }

  }

}

// avaliar se energia acabou
void availablePowerShip() {
  finishGame(score >= SCORE_TO_WIN, NO_ENERGY);
}

// avaliar se jogador atingiu pontuacao maxima para vitoria
void availableScoreGame() {
  finishGame(score >= SCORE_TO_WIN, WIN);
}

void finishGame(bool validation, int character) {
  if(validation) {
    game = false;
    drawMessageStoppedGame(character);
    delay(5000);
    lcd.clear();
  }
}

// avaliar se houve colisao entre nave e energia
void availableShipAndPowerCollision() {
  
  bool checkCollision = (pxShip == pxPower) && (pyShip == pyPower);
  bool alternativeCheckCollision = (pxShip == pxPower+1) && (pyShip == pyPower);

  // houve uma colisao, remover asteroid e nave e desenhar explosao
  if(checkCollision || alternativeCheckCollision) {
    energy+=10;
    if(energy > MAX_ENERGY) energy = 100;
    hasPower = false;
    pxPower = MAX_INITIAL_OBJECTS_POSITION_X;
    playTakePower();
  }
}

// avaliar se vai gerar uma energia
void availablePower() {

  if(!hasPower) {
    // se for 5... gerar uma energia
    if(random(0, ENERGY_POSSIBILITY) == 5) {
      hasPower = true;
      pxPower = MAX_INITIAL_OBJECTS_POSITION_X;
      pyPower = random(0, 2);
      if(pxPower == pxAsteroid && pyPower == pyAsteroid) {
        pxPower++;
      }
    }
  } else {
    pxPower--;
    // se energia atingiu limite da tela
    if(pxPower < 0) {
      hasPower = false;
      pxPower = MAX_INITIAL_OBJECTS_POSITION_X;
    } else {
       drawSomething(pxPower, pyPower, POWER);
    }
  }
  
}

// avaliar se houve colisao entre nave e asteroid
void availableShipAndAsteroidCollision() {

  bool checkCollision = (pxShip == pxAsteroid) && (pyShip == pyAsteroid);
  bool alternativeCheckCollision = (pxShip == pxAsteroid+1) && (pyShip == pyAsteroid);

  // houve uma colisao, remover asteroid e nave e desenhar explosao
  if(checkCollision || alternativeCheckCollision) {    
    game = false;
    drawSomething(pxShip, pyShip, EXPLOSION);
    playExplosionSound();
    drawMessageStoppedGame(LOSE);
    delay(5000);
    lcd.clear();
  }
  
}

// avaliar se houve colisao entre tiro e energia
void availableShootAndPowerCollision() {

  bool checkCollision = hasShoot && (pxShoot == pxPower) && (pyShoot == pyPower);
  bool alternativeCheckCollision = hasShoot && (pxShoot == pxPower+1) && (pyShoot == pyPower);

  // houve uma colisao, remover asteroid e energia e desenhar explosao
  if(checkCollision || alternativeCheckCollision) {
    playExplosionSound();
    hasShoot = hasPower = false;
    pxShoot = -1;
    drawSomething(pxPower, pyPower, EXPLOSION);
    delay(200);
    pyPower = random(0, 2);
    pxPower = MAX_INITIAL_OBJECTS_POSITION_X;
    score-=2;
  }
  
}

// avaliar se houve colisao entre tiro e asteroid
void availableShootAndAsteroidCollision() {

  bool checkCollision = hasShoot && (pxShoot == pxAsteroid) && (pyShoot == pyAsteroid);
  bool alternativeCheckCollision = hasShoot && (pxShoot == pxAsteroid+1) && (pyShoot == pyAsteroid);

  // houve uma colisao, remover asteroid e tiro e desenhar explosao
  if(checkCollision || alternativeCheckCollision) {
    playExplosionSound();
    hasShoot = false;
    pxShoot = -1;
    drawSomething(pxAsteroid, pyAsteroid, EXPLOSION);
    delay(200);
    pyAsteroid = random(0, 2);
    pxAsteroid = MAX_INITIAL_OBJECTS_POSITION_X;

    if(asteroidGenerated == 1 || asteroidGenerated == 2) {
      score += 2;
    } else if(asteroidGenerated == 3 || asteroidGenerated == 4) {
      score += 4;
    }

    asteroidGenerated = 0;
  }
}

// verificar se asteroid chegou no limite da tela e desenhar em caso negativo
void availableAsteroid() {
  
  // diminuir a posicao de um asteroid
  pxAsteroid--;
  if(pxPower == pxAsteroid && pyPower == pyAsteroid) {
    pxAsteroid++;
  }
  // desenhar asteroid
  if(asteroidGenerated == 0) {
    asteroidGenerated = random(1, 5); // será gerado um asteroid no formato de ASTEROID_1, ASTEROID_2, ASTEROID_3 (ovni) ou ASTEROID_4 (ovni)
  }
  drawSomething(pxAsteroid, pyAsteroid, asteroidGenerated);
  
  if(pxAsteroid < 0) {
    pxAsteroid = MAX_INITIAL_OBJECTS_POSITION_X;
    pyAsteroid = random(0, 2);
    asteroidGenerated = 0;
  }
}

// avaliar se deve gerar um tiro e gera caso positivo
void availableShoot() {
  if(digitalRead(START_BUTTON) == 0 && energy < MAX_ENERGY) { // se botao de tiro for pressionado, emitir um projetil
    pxShoot = 1;
    pyShoot = pyShip;
    hasShoot = playSound = true;
  }
  // se um tiro foi disparado
  if(hasShoot) {
    // desenhar tiro
    drawSomething(pxShoot, pyShoot, SHOOT);
    if(playSound) {
      playShootSound();
      playSound = false;
    }
    pxShoot++;
  }
  // avaliar a posicao do tiro
  if(hasShoot && pxShoot > MAX_INITIAL_OBJECTS_POSITION_X) {
    pxShoot = 0;
    hasShoot = false;
  }
}

// avalidar se o joystick esta direcionado para algum lugar e desenhar nave
void availableShip() {
  
  // lendo a posicao do joystick
  xValue = analogRead(vrx);
  yValue = analogRead(vry);
  
  if((yValue+10) >= 660) { // se joystick for direcionado para baixo
    pyShip = 1;  
  } else if((yValue-10) <= 220) { // se joystick for direcionado para cima
    pyShip = 0;
  }
  // desenhar nave
  drawSomething(pxShip, pyShip, SHIP); 

}

void resetGame() {
  score = MIN_SCORE;
  energy = MAX_ENERGY;
  game = true;
}

void drawExplosionShip(int px, int py) {
  lcd.clear();
  drawSomething(px, py, EXPLOSION);
  delay(1000);
  lcd.clear();
}

void drawSomething(int px, int py, int character) {
  lcd.setCursor(px, py);
  lcd.write(character);
}

void drawPanel(int px) {
  lcd.setCursor(px, 0);
  lcd.print(score);
  lcd.setCursor(px, 1);
  lcd.print(energy);
}

// desenhar a tela conforme status passado no parametro
void drawMessageStoppedGame(STATUS pStatus) { 

  String showScore = "";
  lcd.setCursor(0, 0);
  switch (pStatus) {
    case INITIAL: 
      lcd.write(SHIP);
      lcd.setCursor(4, 0);
      lcd.print("SPACEWAR");
      lcd.setCursor(15, 0);
      lcd.write(ASTEROID_4);
      lcd.setCursor(2, 1);
      lcd.print("PRESS BUTTON");
      break;
    case WIN: 
    case LOSE: 
    case NO_ENERGY: 
      lcd.clear();
      lcd.print(pStatus == WIN ? ":)  YOU WIN  :)" : (pStatus == LOSE ? ":( GAME OVER :(" : "POWER IS OVER :(") ); 
      lcd.setCursor(0, 1);
      showScore = "SCORE: " + (String)score + " PTS";
      lcd.print(showScore);
      break;
  }
  
}

void playShootSound() {
  for(freqIn = 200; freqIn < 300; freqIn = freqIn + 2){
    piezoTone(1000000/freqIn,3);
  }
}

void playExplosionSound() {
  for(int k = 0; k < 250; k++){
    blow1 = random(500,1000);
    blow2 = random(1,3);
    piezoTone(blow1,blow2);
  } 
}

void playTakePower() {
  
  tone(PIEZO, NOTE_G4, 35);
  delay(35);
  tone(PIEZO, NOTE_G5, 35);
  delay(35);
  tone(PIEZO, NOTE_G6, 35);
  delay(35);
  noTone(8);
  
}

void piezoTone(long freq, long duration){
  long aSecond = 1000000;
  long period = aSecond/freq;
  duration = duration*1000;
  duration = duration/period;
  for(long k = 0; k < duration; k++){
    digitalWrite(PIEZO, HIGH);
    delayMicroseconds(period/2);
    digitalWrite(PIEZO, LOW);
    delayMicroseconds(period/2);
  }
} 

// Cria os personagens na memoria do display
void createCharacters() {
  
  // cria o asteroid 1
  lcd.createChar(ASTEROID_1, asteroidForm1);

  // cria o asteroid 2
  lcd.createChar(ASTEROID_2, asteroidForm2);

  // cria o ovni 1
  lcd.createChar(ASTEROID_3, asteroidForm3);

  // cria o ovni 2
  lcd.createChar(ASTEROID_4, asteroidForm4);

  // cria a nave 
  lcd.createChar(SHIP, shipForm);
  
  // cria a explosao
  lcd.createChar(EXPLOSION, explosionForm);
  
  // cria a bateria
  lcd.createChar(POWER, powerForm);
  
  // cria o tiro
  lcd.createChar(SHOOT, shootForm);
  
}
