#include "RP6RobotBaseLib.h"
#define MEINEADRESSE 2
#define MainRobotAdress 1
#define BYTEBEFEHLE
#include "transmitLib.c"


void driveToField(uint8_t field)
{
    switch(field){
        case 1:
            move(160, FWD, DIST_CM(90), true);
            break;
        case 2:
            move(160, FWD, DIST_CM(90), true);
            rotate(40, RIGHT, 90, true);
            move(160, FWD, DIST_CM(30), true);
            break;
        case 3:
            move(160, FWD, DIST_CM(90), true);
            rotate(40, RIGHT, 90, true);
            move(160, FWD, DIST_CM(60), true);
            break;
        case 4:
            move(160, FWD, DIST_CM(60), true);
            break;
        case 5:
            move(160, FWD, DIST_CM(60), true);
            rotate(40, RIGHT, 90, true);
            move(160, FWD, DIST_CM(30), true);
            break;
        case 6:
            move(160, FWD, DIST_CM(60), true);
            rotate(40, RIGHT, 90, true);
            move(160, FWD, DIST_CM(60), true);
            break;
        case 7:
            move(160, FWD, DIST_CM(30), true);
            break;
        case 8:
            move(160, FWD, DIST_CM(30), true);
            rotate(40, RIGHT, 90, true);
            move(160, FWD, DIST_CM(30), true);
            break;
        case 9:
            move(160, FWD, DIST_CM(30), true);
            rotate(40, RIGHT, 90, true);
            move(160, FWD, DIST_CM(60), true);
            break;
    }
}


void driveBackFromField(uint8_t field)
{
    switch(field){
        case 1:
            move(160, BWD, DIST_CM(90), true);
            break;
        case 2:
            move(160, BWD, DIST_CM(30), true);
            rotate(40, LEFT, 90, true);
            move(160, BWD, DIST_CM(90), true);
            break;
        case 3:
            move(160, BWD, DIST_CM(60), true);
            rotate(40, LEFT, 90, true);
            move(160, BWD, DIST_CM(90), true);
            break;
        case 4:
            move(160, BWD, DIST_CM(60), true);
            break;
        case 5:
            move(160, BWD, DIST_CM(30), true);
            rotate(40, LEFT, 90, true);
            move(160, BWD, DIST_CM(60), true);
            break;
        case 6:
            move(160, BWD, DIST_CM(60), true);
            rotate(40, LEFT, 90, true);
            move(160, BWD, DIST_CM(60), true);
            break;
        case 7:
            move(160, BWD, DIST_CM(30), true);
            break;
        case 8:
            move(160, BWD, DIST_CM(30), true);
            rotate(40, LEFT, 90, true);
            move(160, BWD, DIST_CM(30), true);
            break;
        case 9:
            move(160, BWD, DIST_CM(60), true);
            rotate(40, LEFT, 90, true);
            move(160, BWD, DIST_CM(30), true);
            break;
    }
}


void drawCircle(void)
{   
    setServo(6);  // ->  |
    rotate(40, RIGHT, 360, true);
    setServo(18);  // ->  _
}


int main(void)
{
    initRobotBase();
    powerON();

    initByteReception();

    while (true) {
        task_ACS();

        if (isByteDa()) {
            uint8_t field = getByte();

            driveToField(field);
            drawCircle();
            driveBackFromField(field);

            sendByte(MainRobotAdress, 1);  // tell MainRobot that DrawRobot finished drawing
            task_ACS();
        }
    }
    return 0;
}
