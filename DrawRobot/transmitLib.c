#include "RP6RobotBaseLib.h"
//#define DEBUG
#define MOVEATSPEED 0
#define MOVE 1
#define ROTATE 2
#define STOP 3
#define CHANGEDIRECTION 4
#define BLINK 5
#ifdef DEBUG
void nl(void){
  writeChar('\n');
}
#endif
#ifndef MEINEADRESSE
#define MEINEADRESSE 1
#endif

void sendByte(uint8_t adr, uint8_t data) //konvertiert data in einen vernünftigen
															 //RC5-Code und übergibt data dann mit 
														    //adr und togglebit an IRCOMM_sendRC5()
{    uint8_t RC5data;
    uint8_t RC5adr;
    RC5data = data & 0b111111;    //die ersten sechs bit von data in RC5data schreiben.
    RC5adr=((data & (0b11<<6)) >> 2 ) | (adr & 0b1111);
    IRCOMM_sendRC5 (RC5data, RC5adr);
}

uint8_t IRCOMM_decodeData(RC5data_t RC5data)
{/*RC5data.data enthält 16 Bit. Die höchstwertigen 4 sind Schrott.
dann folgen 6 databit (die als niederwertige Bits der Daten interpretiert werden)
dann 6 Adressbits, wovon die höchstwertigen als höchstwertige Datenbits interpretiert
werden, die niederwertigen 4 Bit sind die eigentlich Adresse
*/
    uint8_t data = (0b111111000000 & RC5data.data) >> 6;
	 data |= (uint8_t)((0b110000 & RC5data.data) << 2);
    return data;
}

#ifdef BEWEGUNGSBEFEHLE
typedef 
  struct {
	 unsigned type:4; //einer der oben definierten Befehlstypen
	 unsigned direction:4;
	 union {
		struct {
		  uint8_t left;
		  uint8_t right;//für den MoveAtSpeed Befehl
		} ;
		struct {
		  uint8_t speed;
		  uint16_t distance;//für den move befehl
		} ;
		struct {
		  uint8_t speed;
		  uint16_t angle;//für den rotate befehl
		} ;
		struct {
		  uint8_t LEDs;  //die blinkenden LEDs
		  uint16_t interval;//Blinkintervall
		} ;
	 } ;
  } command_t;
uint8_t transmitSize(uint8_t commandType){
  switch (commandType){
	 case MOVE:;
	 case BLINK:;
	 case ROTATE : return 4; break;
	 case STOP : ;
	 case CHANGEDIRECTION : return 1 ; break ;
	 default : return 3 ;
  }
}
command_t tcommand; //der zu übertragende Befehl.
struct {
	 uint8_t * data;//zeiger auf das nächste zu übertragende Byte
	 int8_t count; //anzahl der noch zu übertragenden Byte. Darf erst dann auf 0 sein,
		  //wenn die Übertragung abgeschlossen ist.
	 uint8_t adr;
} transmitInfo; //wird von sendCommand gesetzt.

#define commandSent() (transmitInfo.count==-1)
 

void task_Transmitter(void){
  /* muss wie task_Motioncontrol in der Hauptschleife aufgerufen werden
     und sendet alle 120 ms das Byte, auf das transmitInfo.data zeigt, solange bis
	  transmitInfo.count==0 ist es wird die stopWatch8 verwendet.*/
  if(!commandSent() && getStopwatch8()>120){
	 if (transmitInfo.count-- > 0){
		sendByte(transmitInfo.adr,*(transmitInfo.data++));
		setStopwatch8(0);
	 }else{//letzes Byte wurde bereits übertragen.
		stopStopwatch8();
		transmitInfo.count = -1;
	 }
  }
}

void sendCommand(uint8_t adr,uint8_t blocking){
/*setzt die Variable transmitInfo und startet die Übertragung an adr. adr<32 ! */
  transmitInfo.data=(uint8_t*)&tcommand;
  transmitInfo.adr=adr&0b1111;
  transmitInfo.count=transmitSize(tcommand.type);
  #ifdef DEBUG 
  writeString_P("habe einen Befehl mit soviel Bytes zu uebertragen: ");
  writeInteger(transmitInfo.count,10);
  nl();
  #endif
  startStopwatch8();
  setStopwatch8(0);
  while( blocking && (! commandSent() )){
	 task_Transmitter();
	 task_RP6System();
  }  
}

//die folgenden Befehle erlauben eine besonders einfache Fernsteuerung.
void send_moveAtSpeed(uint8_t adr,uint8_t left,uint8_t right,uint8_t blocking){
  tcommand.type=MOVEATSPEED;
  tcommand.left=left;
  tcommand.right=right;
  sendCommand(adr,blocking);
}
void send_changeDirection(uint8_t adr,uint8_t direction,uint8_t blocking){
  tcommand.type=CHANGEDIRECTION;
  tcommand.direction=direction;
  sendCommand(adr,blocking);
}
void send_move(uint8_t adr, uint8_t speed, uint8_t direction, uint16_t distance, uint8_t blocking ){
  tcommand.type=MOVE;
  tcommand.speed=speed;
  tcommand.direction=direction;
  tcommand.distance=distance;
  sendCommand(adr,blocking);
}
void send_rotate(uint8_t adr, uint8_t speed, uint8_t direction, uint16_t angle,uint8_t blocking){
  tcommand.type=ROTATE;
  tcommand.speed=speed;
  tcommand.direction=direction;
  tcommand.angle=angle;
  sendCommand(adr,blocking);
}

void send_stop(uint8_t adr,uint8_t blocking){
  tcommand.type=STOP;
  sendCommand(adr,blocking);
}
void send_blink(uint8_t adr, uint8_t LEDs, uint16_t interval,uint8_t blocking){
  tcommand.type=BLINK;
  tcommand.LEDs=LEDs;
  tcommand.interval=interval;
  sendCommand(adr,blocking);
}
/*---------------hier die Empfangsroutinen----------------------------------------*/
command_t rcommand; //der empfangene Befehl.
//RC5data_t IRSignal;
struct {
  uint8_t leds;
  uint16_t interval;
} blinkInfo = {0,1000}; //struct fürs Blinken.

#define isBlinking() (blinkInfo.leds != 0)

void task_blink(void){//lässt die LEDS gemäß der globalen Variablen blinkInfo blinken.
  if (isBlinking() && (getStopwatch7()>blinkInfo.interval)){
	 setLEDs(statusLEDs.byte ^ blinkInfo.leds);
	 setStopwatch7(0);
  }
}



struct {
  char * buffer; //dorthin wird das nächste Byte geschrieben.
  uint8_t count; //soviele Bytes müssen noch geschrieben werden.
  enum {receiving,completed,ready} state;/*completed bedeutet: Befehl vollständig
    empfangen, aber noch nicht abgearbeitet, ready bedeutet: abgearbeitet*/
} receiveInfo={(char*)&rcommand,0,ready}; // Information über den Emfpang von Befehlen

#define commandReceptionCompleted() (receiveInfo.state==completed)
#define initCommandReception() IRCOMM_setRC5DataReadyHandler(receiveCommands) 

void receiveCommands(RC5data_t IRSignal)/*dieser Handler muss mit IRCOMM_setRC5DataReadyHandler
		  registriert werden.*/
{  
  #ifdef DEBUG
    writeString_P("empfange etwas ");
  #endif
    if ((uint8_t)(IRSignal.data & 0b1111) != MEINEADRESSE)
	   return;
  #ifdef DEBUG
    writeString_P("fuer mich: ");
	 uint8_t data=IRCOMM_decodeData(IRSignal);
	 writeInteger(data,2);writeChar('=');
	 writeInteger(data,10);
	 nl();
  #endif
	 switch (receiveInfo.state){
		case ready : //voriger Befehl komplett abgearbeitet.
		  receiveInfo.buffer=(char*)&rcommand;
		  *receiveInfo.buffer=IRCOMM_decodeData(IRSignal);
		  receiveInfo.count=transmitSize(rcommand.type)-1;
		  #ifdef DEBUG 
		  writeString_P("bekomme Befehl mit soviel Bytes: ");
		  writeInteger(receiveInfo.count+1,10);
		  nl();
		  #endif
		  if (receiveInfo.count==0){
			 #ifdef DEBUG
			   writeString_P(" habe einen ein-Byte-Befehl bekommen!\n");
			 #endif
			 receiveInfo.state=completed;
		  }else{
			 receiveInfo.state=receiving;
			 receiveInfo.buffer++;
		  }
		  break;
		case receiving : 
		  *receiveInfo.buffer=IRCOMM_decodeData(IRSignal);
		  if (receiveInfo.count-- == 1){//letztes Byte empfangen
			 #ifdef DEBUG
			   writeString_P("letztes Byte empfangen\n");
			 #endif
			 receiveInfo.state=completed;
		  }else
			 receiveInfo.buffer++;
		  break;
		case completed : /*dieser Fall darf eigentlich nicht auftreten. Das hieße,
		  dass ein Befehl noch nicht abgearbeitet wurde, während der nächste schon eintrudelt.*/
		  writeString_P("Da kommt ein neuer Befehl. Der Alte wurde noch nicht abgearbeitet.\n");
		  /*hier könnte man versuchen, einen vollständigen Befehl abzuwarten bis der nächste
		  gespeichert wird.*/
		  break;
	 }
}

void doCommand(void){
  receiveInfo.state=ready;
  switch (rcommand.type){
	 case MOVEATSPEED :
		moveAtSpeed(rcommand.left,rcommand.right);
		break;
	 case MOVE :
		move(rcommand.speed,rcommand.direction,rcommand.distance,true);
		break;
	 case ROTATE :
		rotate(rcommand.speed,rcommand.direction,rcommand.angle,true);
		break;
	 case STOP :
		stop();break;
	 case CHANGEDIRECTION :
		changeDirection(rcommand.direction);break;
	 case BLINK : //writeString_P("Blink-Befehl kommt an\n");
		blinkInfo.leds=rcommand.LEDs;
		blinkInfo.interval=rcommand.interval;
		startStopwatch7();break;
  }  
}

#endif
/*------------nun folgen die Textsendebefehle------------------------------------*/
#ifdef TEXTBEFEHLE
#define BYTEBEFEHLE
typedef struct {
  char string[MAXTEXTLENGTH+1]; 
  uint8_t pos; //speichert die Position des nächsten zu übertragenden Buchstabens
  uint8_t sending;
  uint8_t adr;
} message_t;
message_t tmessage={"",0,false,0};
#define textSent() (tmessage.sending==false)
void task_Messenger(void){
  /* muss wie task_Motioncontrol in der Hauptschleife aufgerufen werden
     und sendet alle 120 ms einen Buchstaben der message */
  if(tmessage.sending && getStopwatch8()>120){
	 uint8_t zeichen=tmessage.string[tmessage.pos++];
	 if(zeichen==0){
	   tmessage.sending=false;
		stopStopwatch8();
	 }else{
		sendByte( tmessage.adr,zeichen);
		setStopwatch8(0);
	 }
  }
}

void sendText(uint8_t adr, char*amessage,uint8_t blocking){
/*setzt die Variable message und startet die übertragung  */ 
  uint8_t i=0;
  while((amessage[i]!=0)&&(i<MAXTEXTLENGTH)){
	 tmessage.string[i]=amessage[i]; //message nach messageString kopiert
	 i+=1;
  }
  tmessage.string[i]=0;
  startStopwatch8();
  setStopwatch8(0);
  tmessage.pos=0;
  tmessage.sending=true;
  tmessage.adr=adr;
  while(blocking && !textSent()){
	 task_Messenger();
	 task_RP6System();
  }
}

#endif

/*----------nun folgen die Byte-Empfangsbefehle-------------------*/
#ifdef BYTEBEFEHLE
uint8_t rByte, byteDa=false;
#define isByteDa() byteDa
void receiveBytes(RC5data_t IRSignal)/*dieser Handler muss mit IRCOMM_setRC5DataReadyHandler
		  registriert werden.*/
{  
    if ((uint8_t)(IRSignal.data & 0b1111) != MEINEADRESSE)
	   return;
	 rByte=IRCOMM_decodeData(IRSignal);
	 byteDa=true;
}
#define initByteReception() IRCOMM_setRC5DataReadyHandler(receiveBytes)
#define initTextReception() IRCOMM_setRC5DataReadyHandler(receiveBytes)
uint8_t getByte(void){
  byteDa=false;
  return rByte;
}
#endif
