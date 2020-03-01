#include "RP6RobotBaseLib.h"
#define MAINEADRESSE 1
#define DrawRobotAdress 2
#define BYTEBEFEHLE 
#include "transmitLib.c" 


void copyCharArrayLen10(char* a_orig, char* a_copy)  
{
    for(uint8_t i=0; i<10; i++){
        a_copy[i] = a_orig[i];
    }
}


int random_n(uint8_t max)  
{
    task_ADC();
    uint16_t n = adcLSL;
    n = n % (max+1);
    return n;
}


int getWinner(char* board)  
{
    // Player
    if(board[1] == 'X' && board[2] == 'X' && board[3] == 'X'){return 1;}
    if(board[4] == 'X' && board[5] == 'X' && board[6] == 'X'){return 1;}
    if(board[7] == 'X' && board[8] == 'X' && board[9] == 'X'){return 1;}
    if(board[1] == 'X' && board[4] == 'X' && board[7] == 'X'){return 1;}
    if(board[2] == 'X' && board[5] == 'X' && board[8] == 'X'){return 1;}
    if(board[3] == 'X' && board[6] == 'X' && board[9] == 'X'){return 1;}
    if(board[1] == 'X' && board[5] == 'X' && board[9] == 'X'){return 1;}
    if(board[3] == 'X' && board[5] == 'X' && board[7] == 'X'){return 1;}
    
    // Computer
    if(board[1] == 'O' && board[2] == 'O' && board[3] == 'O'){return 2;}
    if(board[4] == 'O' && board[5] == 'O' && board[6] == 'O'){return 2;}
    if(board[7] == 'O' && board[8] == 'O' && board[9] == 'O'){return 2;}
    if(board[1] == 'O' && board[4] == 'O' && board[7] == 'O'){return 2;}
    if(board[2] == 'O' && board[5] == 'O' && board[8] == 'O'){return 2;}
    if(board[3] == 'O' && board[6] == 'O' && board[9] == 'O'){return 2;}
    if(board[1] == 'O' && board[5] == 'O' && board[9] == 'O'){return 2;}
    if(board[3] == 'O' && board[5] == 'O' && board[7] == 'O'){return 2;}
    
    // no winner
    return 0;
}


int boardFull(char * board)
{
    for(uint8_t i=1; i<=9; i++){
        if(board[i]==' '){
            return false;
        }
    }
    return true;
}


int newRound(void)
{
    while (getBufferLength() > 0) { readChar(); }  // clear bufferr

    writeString_P("Neue Runde? (ja/ nein): ");
    while (getBufferLength() == 0) {}  // wait until buffer is not empty
    char ans = readChar();  

    while (getBufferLength() > 0) { readChar(); }  // clear bufferr

    if (ans == 'j' || ans == 'J') {
        writeString_P("\n\n");
        return true;
    }
    return false;
}


void drawBoardInTerminal(char * board)
{
    for(uint8_t i=1; i<=7; i+=3){
        writeString_P(" ");
        writeChar(board[i]);
        writeString_P(" | ");
        writeChar(board[i+1]);
        writeString_P(" | ");
        writeChar(board[i+2]);
        writeString_P("\n");
    }
    writeString_P("\n");
}


void makeTurnPlayer(char* board)
{	
    char field_char;  
    uint8_t field_int;  

    while (getBufferLength() > 0) { readChar(); }  // clear bufferr

    while (true) {  
        writeString_P("Gib an in welches Feld du ein X setzen moechtest (1-9): ");

        while (getBufferLength() == 0) {}  // wait until buffer is not empty
        field_char = readChar();  

        while (getBufferLength() > 0) { readChar(); }  // clear bufferr

        if ('0' < field_char && field_char <= '9') {  // is ASCII number 1 - 9

            field_int = field_char - '0';  // convert to int
            
            if(board[field_int] == ' '){  
                board[field_int] = 'X'; 
                break;
            }
        }
        writeString_P("Keine gueltige Eingabe\n");
    }	
}


int makeTurnComputer(char * board)
{
    writeString_P("Der Computer ist dran...\n");
    
    char board_copy[10];
    copyCharArrayLen10(board, board_copy);
    
    uint8_t board_i;

    // can win?
    for(board_i=1; board_i<=9; board_i++){
        if(board_copy[board_i] == ' '){
            board_copy[board_i] = 'O';
            if(getWinner(board_copy) == 2){
                copyCharArrayLen10(board_copy, board);
                sendByte(DrawRobotAdress, board_i);  // tell DrawRobot to draw 'O' on that field
				while (true) {  // wait until DrawRobot finished drawing
					task_ACS();
					if (isByteDa()) {
						getByte();
						break;
					}
				}
                return 0;
            }
            copyCharArrayLen10(board, board_copy);
        }
    }

    // can prevent player to win?
    for(board_i=1; board_i<=9; board_i++){
        if(board_copy[board_i] == ' '){
            board_copy[board_i] = 'X';
            if(getWinner(board_copy) == 1){
                board_copy[board_i] = 'O';
                copyCharArrayLen10(board_copy, board);
                sendByte(DrawRobotAdress, board_i);  // tell DrawRobot to draw 'O' on that field
				while (true) {  // wait until DrawRobot finished drawing
					task_ACS();
					if (isByteDa()) {
						getByte();
						break;
					}
				}
                return 0;
            }
            copyCharArrayLen10(board, board_copy);
        }
    }
    
    // middle?
    if(board[5]==' '){
        board[5] = 'O';
        sendByte(DrawRobotAdress, 5);  // tell DrawRobot to draw 'O' on that field
		while (true) {  // wait until DrawRobot finished drawing
        task_ACS();
        if (isByteDa()) {
            getByte();
			break;
        }
    }
        return 0;
    }
    
    uint8_t moves[4];  // possible moves
    int8_t moves_i = -1;  

    // corner?
    for(board_i=1; board_i<=9; board_i++){
        if((board_i%2 != 0) && (board_i != 5)){  // i uneven and not 5 (only corners)
            if (board[board_i] == ' '){
                moves_i++;
                moves[moves_i] = board_i;
            }
        }
    }
    if(moves_i>=0){  // at least one possible move
        uint8_t field = moves[random_n(moves_i)];  // random move from moves
        board[field] = 'O';
        sendByte(DrawRobotAdress, field);  // tell DrawRobot to draw 'O' on that field
		while (true) {  // wait until DrawRobot finished drawing
        task_ACS();
        if (isByteDa()) {
            getByte();
			break;
        }
    }
        return 0;
    }
    
    // side?
    for(board_i=1; board_i<=9; board_i++){
        if((board_i%2 != 1) && (board_i != 5)){  // i even and not 5 (only sides)
            if (board[board_i] == ' '){
                moves_i++;
                moves[moves_i] = board_i;
            }
        }
    }
    uint8_t field = moves[random_n(moves_i)];  // random move from moves
    board[field] = 'O';
    sendByte(DrawRobotAdress, field);  // tell DrawRobot to draw 'O' on that field
	while (true) {  // wait until DrawRobot finished drawing
        task_ACS();
        if (isByteDa()) {
            getByte();
			break;
        }
    }
    return 0;
}


int main(void) 
{
    initRobotBase();

    powerON();
	
	initByteReception();
    
    initServo();
    setServo(18);  // ->  _

    writeString_P("Willkommen zu TicTacToe\n");

    char board[10];
    
    uint8_t winner = 0;  // (1 = player; 2 = computer)

    while (true)  
    {   // alternately player first then computer first

        // start game -----------------------------------
        // (player first)

        for(uint8_t j=0;j<10;j++){
            board[j] = ' ';
        }

        drawBoardInTerminal(board);

        winner = 0;

        while (winner == 0){  // game-loop -----
        
            makeTurnPlayer(board);
            drawBoardInTerminal(board);
            mSleep(600);
            if(boardFull(board)){break;}
            winner = getWinner(board);
            if(winner){continue;}

            makeTurnComputer(board);
            mSleep(1000);
            drawBoardInTerminal(board);
            mSleep(600);
            if(boardFull(board)){break;}
            winner = getWinner(board);
            if(winner){continue;}
        }
        
        if(winner==1){  // player won
            writeString_P("Du hast gewonnen!\n");
        }
        else if(winner==2){  // computer won
            writeString_P("Der Computer hat gewonnen!\n");
        }
        else{  // it's a draw
            writeString_P("Unentschieden!\n");
        }
        mSleep(1000);

        if(newRound()){}
        else{break;}


        // start game -----------------------------------
        // (computer first)

        for (uint8_t j = 0; j < 10; j++) {
            board[j] = ' ';
        }

        drawBoardInTerminal(board);

        winner = 0;

        while (winner == 0) {  // game-loop -----

            makeTurnComputer(board);
            mSleep(1000);
            drawBoardInTerminal(board);
            mSleep(600);
            if(boardFull(board)){break;}
            winner = getWinner(board);
            if(winner){continue;}

            makeTurnPlayer(board);
            drawBoardInTerminal(board);
            mSleep(600);
            if(boardFull(board)){break;}
            winner = getWinner(board);
            if(winner){continue;}
        }
        
        if(winner==1){  // player won
            writeString_P("Du hast gewonnen!\n");
        }
        else if(winner==2){  // computer won
            writeString_P("Der Computer hat gewonnen!\n");
        }
        else{  // it's a draw
            writeString_P("Unentschieden!\n");
        }
        mSleep(1000);

        if(newRound()){continue;}
        else{break;}
    }

    return 0;
}
