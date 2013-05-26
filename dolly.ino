#include <LiquidCrystal.h>
//#define _DEBUG_
#define min(a, b) ((a) < (b) ? (a) : (b))

// pin
LiquidCrystal lcd(2, 3, 4, 5, 6 ,7);
const int SW_Enter = 11;
const int SW_Up = 13;
const int SW_Down = 12;
const int STM_A = 8;
const int STM_B = 9;
const int STM_PS = 10;
const int STM_VS2B = 1;
//const int SHOT_SW1 = 0; // AF
const int SHOT_SW2 = 0; // Release 

// menu
//enum Menu_State {MENU_INTERVAL, MENU_TIMES, MENU_TV, MENU_DISTSTEP, MENU_SHOT};
//enum Menu_State menu;
const int MENU_MOTOR = 0;
const int MENU_INTERVAL = 1;
const int MENU_TIMES = 2;
const int MENU_TV = 3;
const int MENU_DIST_STEP = 4;
const int MENU_DIR = 5;
const int MENU_SHOT = 6;
const int N_Menu = 7;

// around_step=200(=1.8[deg]), pulley_r=40[mm]
// 40*pi/200 = 0.628[mm] 
const unsigned int STM_STEP_DIST = 628*1; 

// global
unsigned int g_menu = 0;
unsigned int g_interval = 3000;
unsigned int g_tv = 1000;
unsigned int g_dist_step = STM_STEP_DIST;
unsigned int g_times = 3;
unsigned int g_dir = 0;
unsigned int g_stm_state = 0;

/**********************************************************/
void setup()
{
  #ifdef _DEBUG_
    Serial.begin(9600);
  #endif
  lcd.begin(16,2);
  Serial.println(1);
  
  //pinMode(SHOT_SW1, OUTPUT);
  pinMode(SHOT_SW2, OUTPUT);
  pinMode(SW_Enter, INPUT);
  pinMode(SW_Up, INPUT);
  pinMode(SW_Down, INPUT);
  pinMode(STM_A, OUTPUT);
  pinMode(STM_B, OUTPUT);
  pinMode(STM_PS, OUTPUT);  
  pinMode(STM_VS2B, OUTPUT);
  
  // init
  digitalWrite(SHOT_SW2, LOW);
  digitalWrite(STM_A, LOW);
  digitalWrite(STM_B, LOW); 
  digitalWrite(STM_PS, HIGH);
  digitalWrite(STM_VS2B, LOW);
  
  lcd.print("Motolized Camera");  
  lcd.setCursor(0, 1);
  lcd.print("Dolly     Ver1.0");  
  lcd.setCursor(0, 1);
  delay(1000);
}

// digital read in cosideration to chattering
int digitalRead_psw(int pin)
{
  int state = LOW;
  int pre_state = HIGH;
  int is_valid = 0;
  
  while( !is_valid ) {
    pre_state = digitalRead(pin);
    delay(10);
    state = digitalRead(pin);

    if( pre_state == state ) {
      is_valid = 1;
    }
  }
  delay(10);
  
  return state;
}

// __   ___________
//  |___|
//      ~
int digitalRead_once(int pin)
{
  int state = LOW;
  int pre_state = HIGH;
  int stable_cnt = 0;
  
  while( digitalRead(pin) == LOW ) {
    delay(20);
    stable_cnt++;
  }
  //Serial.println("stable_cnt");
  //Serial.println(stable_cnt);
  
  if( stable_cnt > 3 ) {
    return LOW;
  } else {
    return HIGH;
  }
}

int count_updown(unsigned int count, unsigned int step, unsigned int min, unsigned int max)
{
   while( digitalRead_once(SW_Enter) == HIGH ) {
    if( digitalRead(SW_Up) == LOW ) {
      count += step;
      if( count <= min ) {
        count = min;
      } else if( count >= max ) {
        count = max;
      }
      
      lcd.setCursor(0, 1);
      lcd.print("            ");
      lcd.setCursor(0, 1);
      lcd.print(count);
      Serial.println("Up");
      Serial.println("Max");
      Serial.println(max);
    } else if( digitalRead(SW_Down) == LOW ) {
      count -= step;
      if( count <= min ) {
        count = min;
      } else if( count >= max ) {
        count = max;
      }

      lcd.setCursor(0, 1);
      lcd.print("            ");
      lcd.setCursor(0, 1);
      lcd.print(count);
      Serial.println("Down");
    }
    delay(100);
    Serial.println(count);
  }

  return count;
}

unsigned int menu_disp(String title, String unit, unsigned int val, unsigned int step, unsigned int min, unsigned int max)
{
  lcd.clear();
  lcd.print(title);
  lcd.setCursor(0, 1);
  lcd.print(val);
  lcd.setCursor(16-4, 1);
  lcd.print(unit);
  
  Serial.println("menu disp");
  return count_updown(val, step, min, max);
}

void menu_motor()
{
  lcd.clear();
  lcd.print("Rotate Motor");
  
   while( digitalRead_once(SW_Enter) == HIGH ) {
    if( digitalRead(SW_Up) == LOW ) { // clockwise
      digitalWrite(STM_PS, LOW);
      digitalWrite(STM_VS2B, HIGH);
      motor_clockwise(20);
      lcd.setCursor(0, 1);
      lcd.print("<--");
      digitalWrite(STM_PS, HIGH);
      digitalWrite(STM_VS2B, LOW);
    } else if( digitalRead(SW_Down) == LOW ) { // counter clockwise
      digitalWrite(STM_PS, LOW);
      digitalWrite(STM_VS2B, HIGH);
      motor_cclockwise(20);
      lcd.setCursor(0, 1);
      lcd.print("-->");
      digitalWrite(STM_PS, HIGH);
      digitalWrite(STM_VS2B, LOW);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("   ");
    }    
//    lcd.setCursor(0, 1);
//    lcd.print("   ");
  }  
  
//  delay(100);
}

void menu_dir()
{
  lcd.clear();
  lcd.print("Direction");
  
   while( digitalRead_once(SW_Enter) == HIGH ) {
    if( digitalRead(SW_Up) == LOW ) { // clockwise
      g_dir = 0;
    } else if( digitalRead(SW_Down) == LOW ) { // counter clockwise
      g_dir = 1;
    }    
    
    lcd.setCursor(0, 1);
    if( g_dir == 0 ) {
      lcd.print("<--");
    } else {
      lcd.print("-->");
    }
    
    delay(100);
  }  
  
}

//２－２相励磁, 正転
void motor_clockwise(const int t)
{
  switch(g_stm_state) {
    case 0:
      digitalWrite(STM_B, LOW);
      break;
    case 1:
      digitalWrite(STM_A, LOW);
      break;
    case 2:
      digitalWrite(STM_B, HIGH);
      break;
    case 3:
      digitalWrite(STM_A, HIGH);
      break;
    default:
      break;
  }  
  delay(t);
  g_stm_state++;
  g_stm_state = g_stm_state % 4;
}

//２－２相励磁, 反転
void motor_cclockwise(const int t)
{
  switch(g_stm_state) {
    case 0:
      digitalWrite(STM_A, HIGH);
      break;
    case 1:
      digitalWrite(STM_B, HIGH);
      break;
    case 2:
      digitalWrite(STM_A, LOW);
      break;
    case 3:
      digitalWrite(STM_B, LOW);
      break;
    default:
      break;
  }
  delay(t);
  g_stm_state++;
  g_stm_state = g_stm_state % 4;
}

void move_motor(const unsigned int dist_step)
{
  int i;
  
  Serial.println("dist_step ");
  i = dist_step;
  while( i > 0 ) {
    Serial.println(dist_step);
    if( g_dir == 0 ) {
      motor_clockwise(20);
    } else {
      motor_cclockwise(20);
    }
    i -= STM_STEP_DIST;
  }
}

void shot_each()
{
  Serial.println("g_tv ");
  Serial.println(g_tv);
  digitalWrite(SHOT_SW2, HIGH);
  delay(g_tv);
  digitalWrite(SHOT_SW2, LOW);
//  delay(g_interval - g_tv);
}

void shot()
{
  int count = 0;
  //int stable_delay;
  
  lcd.clear();
  lcd.print("Push Enter Key");
  delay(100);
  
  while( 1 ) {
    if( digitalRead_once(SW_Enter) == LOW ) {
      break;
    } else if( digitalRead_once(SW_Up) == LOW ) {
      return;
    } else if( digitalRead_once(SW_Down) == LOW ) {
      return;
    }
  }
  delay(500);
  
  lcd.clear();
  lcd.print("Shot !");
  lcd.setCursor(0, 1);

  // parameter check
  if( g_tv > g_interval ) {
    g_tv = g_interval;
  }

  // move & shot
  g_stm_state = 0;
  digitalWrite(STM_PS, LOW);
  digitalWrite(STM_VS2B, HIGH);
  move_motor(STM_STEP_DIST*10);  // init tention

  while( count < g_times ) {
    lcd.setCursor(0, 1);
    lcd.print(count);
    lcd.print("/");
    lcd.print(g_times);

    move_motor(g_dist_step);
    delay((g_interval - g_tv));
      
    //delay((g_interval - g_tv)/2);
    shot_each();

    count += 1;
  }
  digitalWrite(STM_PS, HIGH);
  digitalWrite(STM_VS2B, LOW);

}

void loop()
{
  //lcd.home();
  //lcd.print("abcedfg");
  //if( digitalRead_once(SW_Enter) == LOW ) {
  //}
  g_menu = g_menu % N_Menu;
  
  //Serial.println(g_menu);
  switch(g_menu) {
    case MENU_MOTOR:
      menu_motor();
      g_menu++;
      break;
    case MENU_INTERVAL:
      g_interval = menu_disp("Interval", "[ms]", g_interval, 100, 1000 , 32767);
      g_menu++;
      break;
    case MENU_TV:
      g_tv = menu_disp("Tv (enable@bulb)", "[ms]", g_tv, 100, 100, 32767);
      g_menu++;
      break;
    case MENU_DIST_STEP:
      g_dist_step = menu_disp("Dist Step", "[um]", g_dist_step, STM_STEP_DIST, 0, STM_STEP_DIST*52);
      g_menu++;
      break;
    case MENU_TIMES:
      g_times = menu_disp("Times", "", g_times, 1, 1, 32767);
      g_menu++;
      break;
    case MENU_DIR:
      menu_dir();
      g_menu++;
    case MENU_SHOT:
      shot();
      g_menu = 0;
    default:
      break;
  }
}

void loop_2()
{
  digitalWrite(0, 1);
  delay(1000);
  digitalWrite(0, 0);
  delay(3000);
}
