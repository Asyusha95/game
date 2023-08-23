
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

//#define DEBUG
// упаковка двух однобайтовых переменных в двухбайтовую переменную
#define PACK(target_x, target_y)  (unsigned short)((target_y << 8) | (target_x)); 

#define PLAYER_1 0
#define PLAYER_2 ~PLAYER_1

// i = numner lines
typedef enum // статус хода программы
{
	INIT = 0, // инициализация
	DRAW, // перерисовка
	PLAYER1,
	PLAYER2,
	GAME_OVER
} eGameState;

typedef enum // состояние обьекта на игровом поле
{
	EMPTY = 0, // пусто
	SHOT, // выстрел
	STRIKE, // ранен
	KILL, // убит
	SHIP, // корабль
	TEST_SHIP, // предварительная расстановка
	EFIELD_INFO_END
} eFieldInfo;

char draw_symbol[EFIELD_INFO_END] =
{
	'.', // EMPTY пусто
	'*', // SHOT выстрел
	'X', // STRIKE ранен
	'#', // KILL убит
	'1' // SHIP корабль
};


#define ARROW_KEY_PRESEND  224 //0xE0
#define KEY_ENTER 10
#define KEY_UP 0x30 + 8
#define KEY_RIGHT 0x30 + 6
#define KEY_DOWN 0x30 + 2
#define KEY_LEFT 0x30 + 4


#define TARGET '+' //прицел

#define LINES 16
#define FIELD_SIZE 10
char* bufscr[LINES] = {0};// видеобуфер

eFieldInfo p1_data[FIELD_SIZE * FIELD_SIZE] = { EMPTY }; // player 1
eFieldInfo p2_data[FIELD_SIZE * FIELD_SIZE] = { EMPTY }; // player 2

int main();
void chip_generate(eFieldInfo *); // генератор игровых полей

bool get_target_position(unsigned short*); // получить позицию с клавиатуры
bool generate_target_position(unsigned short*);  // получить позицию рандомно
void print_player(const char*); // напечатать имя текущего игрока в буфер

void init_buffer(); // инициализировать видеобуфер
void render(); // обновить экран

// игрок 1
void print_p1_score(unsigned int); // напечатать счёт в буфер
void redraw_p1_field(); // перерисовать игровое поле 1 в буфере
bool player_p1(); // ваш ход
eFieldInfo tell_position_p1(unsigned short); // сообщить позицию игроку 2
// возвращает ответ противника: мимо, ранен или убит
void draw_target_p1(unsigned short); // отобразить позицию в буфере

// игрок 2
void print_p2_score(unsigned int); // напечатать счёт в буфер
void redraw_p2_field(); // перерисовать игровое поле 2 в буфере
bool player_p2(); // ход компьютера
eFieldInfo tell_position_p2(unsigned short); // сообщить позицию игроку 2
// возвращает ответ противника: мимо, ранен или убит


//---------------------------------------------------------------------------------------------
void init_buffer()
{
const char* templ[] = // шаблон видеобуфера
{
//     012345678901234567890123456789
      "  ABCDEFGHIJ      ABCDEFGHIJ \0"         //0
	, " *----------*    *----------*\0"        //1
	, "0|..........|   0|..........|\0"        //2 
	, "1|..........|   1|..........|\0"        //3
	, "2|..........|   2|..........|\0"        //4
	, "3|..........|   3|..........|\0"        //5
	, "4|..........|   4|..........|\0"        //6
	, "5|..........|   5|..........|\0"        //7
	, "6|..........|   6|..........|\0"        //8
	, "7|..........|   7|..........|\0"        //9
	, "8|..........|   8|..........|\0"        //10
	, "9|..........|   9|..........|\0"        //11
	, " *----------*    *----------*\0"        //12
	, "      You           Lamer    \0"        //13
	, " Score: 0       Score: 0     \0"        //14
	, " Player:                     \0"        //15	
};

     for (int y = 0; y < LINES; y++) 
     {
         bufscr[y] = (char*)malloc(30); // создаю видеобуфер в динамической памяти
         memcpy(bufscr[y], templ[y], 30); 
     }
};

//---------------------------------------------------------------------------------------------
void redraw_p1_field()
{
	 for (int y = 0; y < FIELD_SIZE; y++) 
        for (int x = 0; x < FIELD_SIZE; x++) // вывод вашего игрового поля (с обозначением всех обьектов)
            bufscr[2+y][2+x] = draw_symbol[p1_data[y * FIELD_SIZE + x]];
    render();
}

//---------------------------------------------------------------------------------------------
void redraw_p2_field()
{
	    for (int y = 0; y < FIELD_SIZE; y++)
#ifndef DEBUG
	    	for (int x = 0; x < FIELD_SIZE; x++) // вывод игрового поля CPU (с обозначением только убитых или раненых)
    		    if 	((p2_data[y * FIELD_SIZE + x] == STRIKE) || 
                    (p2_data[y * FIELD_SIZE + x]== KILL) ||
                    p2_data[y * FIELD_SIZE + x] == SHOT)
    		        bufscr[2+y][18+x] = draw_symbol[p2_data[y * FIELD_SIZE + x]];
    		    else	
    	             bufscr[2+y][18+x] = draw_symbol[EMPTY];
#else
		    for (int x = 0; x < FIELD_SIZE; x++)
			    bufscr[2+y][18+x] = draw_symbol[p2_data[y * FIELD_SIZE + x]];	
#endif
      render();
}

//---------------------------------------------------------------------------------------------
void render()
{
     printf("\033c");
     for (int i = 0; i < LINES; i++) 
         printf("\r%s\n", bufscr[i]);
}

//---------------------------------------------------------------------------------------------
void print_player(const char* player)
{
     sprintf(&bufscr[15][9], "%s",  player);
     render();
}

//---------------------------------------------------------------------------------------------
void print_p1_score(unsigned int score)
{
     char* tmp = &bufscr[14][8] +
     sprintf(&bufscr[14][8], "%u", score);
     *tmp = ' ';
     render();
}

//---------------------------------------------------------------------------------------------
void print_p2_score(unsigned int score)
{
    sprintf(&bufscr[14][23], "%u", score);
     render();
}

//---------------------------------------------------------------------------------------------
void draw_target_p1(unsigned short a_target)
{    
    unsigned char target_x = 0;
	unsigned char target_y = 0;
   
    redraw_p2_field();   
// обозначаем позицию
	target_x = a_target & 0x00FF;          //
	target_y = a_target >> 8;  
	bufscr[2 + target_y][18 + target_x] = TARGET;
// обновляем экран
    render();     
}

//---------------------------------------------------------------------------------------------
int getch() {
	int character;
	struct termios orig_term_attr;
	struct termios new_term_attr;
	/* set the terminal to raw mode */
	tcgetattr(fileno(stdin), &orig_term_attr);
	memcpy(&new_term_attr, &orig_term_attr, sizeof(struct termios));
	new_term_attr.c_lflag &= ~(ECHO|ICANON);
	new_term_attr.c_cc[VTIME] = 0;
	new_term_attr.c_cc[VMIN] = 1;
	tcsetattr(fileno(stdin), TCSANOW, &new_term_attr);
	/* read a character from the stdin stream without blocking */
	/*   returns EOF (-1) if no character is available */
	character = fgetc(stdin);
	/* restore the original terminal attributes */
	tcsetattr(fileno(stdin), TCSANOW, &orig_term_attr);
	return character;
}

//---------------------------------------------------------------------------------------------
int main()
{
	srand((unsigned)time(NULL));//инициализация генератора случайных чисел

	eGameState game_state = INIT;
	bool isRun = true;
	while (isRun)
	{
		switch (game_state)
		{
		case INIT:
		{
			chip_generate(p1_data); 
			chip_generate(p2_data);
			game_state = DRAW;
			break;
		}
		case DRAW: 
		{	
            init_buffer();		          
			redraw_p1_field();
#ifdef DEBUG
            redraw_p2_field();
#endif
			game_state = PLAYER1;
			break;
		}
		case PLAYER1: 
		{			          
			if (player_p1())
			   game_state = PLAYER2;
            else
                game_state = GAME_OVER;
			break;
		}

		case PLAYER2:
		{
			if (player_p2())
			   game_state = PLAYER1;
			else
                game_state = GAME_OVER;
			break;
		}
		case GAME_OVER:
		{
             print_player("GAME OVER");
             isRun = false;
			break;
		}
		}
	}
    getch();
	return 0;
}

//----------------------------------------------------------------------------------------------
bool player_p2()
{
     static int score = 0; // счёт
     if (score == (4*1 + 3*2 + 2*3 + 1*4)) return false;
     eFieldInfo response = EMPTY;
     unsigned short a_target = 0;
    print_player("Lamer");
	while (!generate_target_position(&a_target)); // получаю рандомно позицию
    response = tell_position_p1(a_target); // сообщаю позицию противнику и получаю ответ: мимо, ранен или убит
    if (response == STRIKE) // если убил или ранил, прибавляю счёт
       print_p2_score(++score);    
    return true;
}

//----------------------------------------------------------------------------------------------
bool player_p1()
{
     static int score = 0;
     if (score == (4*1 + 3*2 + 2*3 + 1*4)) return false;
     eFieldInfo response = EMPTY;
     unsigned short a_target = 0;
     print_player("You");
    while (!get_target_position(&a_target)); // получаю позицию с клавиатуры
       response = tell_position_p2(a_target); // сообщаю позицию противнику и получаю ответ: мимо, ранен или убит
    if (response == STRIKE) // если убил или ранил, прибавляю счёт
      print_p1_score(++score);
    return true; 
}

//----------------------------------------------------------------------------------------------
eFieldInfo tell_position_p1(unsigned short a_target)
{
    unsigned char target_x = 0;
	unsigned char target_y = 0;
	target_x = a_target & 0x00FF;          //
	target_y = a_target >> 8; // 10
    eFieldInfo& _p1_data = p1_data[target_y * FIELD_SIZE + target_x];
	if (_p1_data == SHIP) _p1_data = STRIKE;
    if (_p1_data == EMPTY) _p1_data = SHOT;
	redraw_p1_field(); // перерисовка игрового поля
    return _p1_data;    // возврат состояния
}

//----------------------------------------------------------------------------------------------
eFieldInfo tell_position_p2(unsigned short a_target)
{
    unsigned char target_x = 0;
	unsigned char target_y = 0;
	target_x = a_target & 0x00FF;          //
	target_y = a_target >> 8; // 10
    eFieldInfo& _p2_data = p2_data[target_y * FIELD_SIZE + target_x];
	if (_p2_data == SHIP) _p2_data = STRIKE;
	if (_p2_data == EMPTY) _p2_data = SHOT;   
	redraw_p2_field(); // перерисовка игрового поля
    return _p2_data;    // возврат состояния
}
//----------------------------------------------------------------------------------------------
void chip_generate(eFieldInfo *ap_data)
{
	//очистка поля
	 for (int y = 0; y < FIELD_SIZE; y++) 
        for (int x = 0; x < FIELD_SIZE; x++)
		    ap_data[y * FIELD_SIZE + x] = EMPTY;

	int x, y;

	int dir = 0;

	int count_ship = 1; 
	while (count_ship <= 4)
	{
		for (int current_ship = 0; current_ship < count_ship;)
		{

			// ------initial position---------
			x = rand() % FIELD_SIZE;
			y = rand() % FIELD_SIZE;

			//		int tmp_x = x;
			//		int tmp_y = y;
					//------direction generation-------

			dir = rand() % 4;
			bool setting_is_possible = 1;

			//----------   checking the possibility of installing the ship----
			for (int i = 0; i < 5 - count_ship; i++)
			{
				if (x < 0 || y < 0 || x >= FIELD_SIZE || y >= FIELD_SIZE)
				{
					setting_is_possible = 0;
					break;
				}

				if (ap_data[x + FIELD_SIZE*y] == SHIP ||
					ap_data[x + FIELD_SIZE*(y + 1)] == SHIP ||
					ap_data[x + FIELD_SIZE*(y - 1)] == SHIP ||
					ap_data[x + 1 + FIELD_SIZE*y] == SHIP ||
					ap_data[x + 1 + FIELD_SIZE*(y + 1)] == SHIP ||
					ap_data[x + 1 + FIELD_SIZE*(y - 1)] == SHIP ||
					ap_data[x - 1 + FIELD_SIZE*y] == SHIP ||
					ap_data[x - 1 + FIELD_SIZE*(y + 1)] == SHIP ||
					ap_data[x - 1 + FIELD_SIZE*(y - 1)] == SHIP)
				{
					setting_is_possible = 0;
					break;
				}
				if (setting_is_possible == 1)	//если можно ставить, ставим
				{
					ap_data[x + FIELD_SIZE*y] = TEST_SHIP;
				}

				switch (dir)
				{
				case 0:
					x++;
					break;

				case 1:
					y++;
					break;

				case 2:
					x--;
					break;

				case 3:
					y--;
					break;
				}
			}
			if (setting_is_possible == 1)	//если корабль поставлен полностью
			{
				current_ship++;		//следующий n-палубник
				for (int i = 0; i < FIELD_SIZE*FIELD_SIZE; i++)	//заменить тестовую расстановку на постоянную
				{
					if (ap_data[i] == TEST_SHIP)
						ap_data[i] = SHIP;
				}
			}
			else//если не стал
			{
				for (int i = 0; i < FIELD_SIZE*FIELD_SIZE; i++)	//убрать тестовую расстановку
				{
					if (ap_data[i] == TEST_SHIP)
						ap_data[i] = EMPTY;
				}

			}

			//-------------------------------
			/*	if (setting_is_possible == 1)
			{
				ap_data[x + FIELD_SIZE*y] = TEST_SHIP;
				// x = tmp_x;
				// y = tmp_y;


				for (int i = 0; i < 4; i++)
				{
					ap_data[x * 10 + y] = SHIP;
					switch (dir)
					{
					case 0:
						x++;
						break;

					case 1:
						y++;
						break;

					case 2:
						x--;
						break;

					case 3:
						y--;
						break;

					}
				}



			}*/
		}
		count_ship++;
	}
	//}
}

//--------------------------------------------------------------------------------------------

bool generate_target_position(unsigned short* ptr_a_target)
{
    unsigned char target_x = 0;
	unsigned char target_y = 0;
    static bool was[FIELD_SIZE * FIELD_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}; // посещённые позиции
    target_x = rand() % FIELD_SIZE; // рандомно получаем позицию
	target_y = rand() % FIELD_SIZE; 	
	if (was[target_y *FIELD_SIZE + target_x]) return false;	// проверяем, не были ли мы здесь
	was[target_y *FIELD_SIZE + target_x] = true;
	*ptr_a_target = PACK(target_x, target_y); 
    for (int i = 0; i < 256 * 256 - 1; i++) // задержка
    for (int j = 0; j < 256 - 1; j++);	
	return true; //позиция разрешена
}

//--------------------------------------------------------------------------------------------

bool get_target_position(unsigned short* ptr_a_target)
{
    static unsigned char target_x = 0;
	static unsigned char target_y = 0;      
    static bool was[FIELD_SIZE * FIELD_SIZE] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,}; // посещённые позиции       
	char key = 0;
    while(1)
    {
    	*ptr_a_target = PACK(target_x, target_y);
		draw_target_p1(*ptr_a_target); // перерисовка позиции
        key = getch();
        if (key == KEY_ENTER) break;
        //if (key == ARROW_KEY_PRESEND)
		switch (key)
		{
		case KEY_DOWN:
		{
			if (target_y < (FIELD_SIZE - 1))
				(target_y)++;
			break;
		}

		case KEY_UP:
		{
			if (target_y > 0)
				(target_y)--;
			break;
		}

		case KEY_LEFT:
		{
			if (target_x > 0)
				(target_x)--;
			break;
		}

		case KEY_RIGHT:
		{
			if (target_x < (FIELD_SIZE - 1))
				(target_x)++;
			break;
		}
		}
	}

   if (was[target_y *FIELD_SIZE + target_x]) return false;
   was[target_y *FIELD_SIZE + target_x] = true;
   return true;
	//getch();
}

//---------------------------------------------------------------------------------------------
