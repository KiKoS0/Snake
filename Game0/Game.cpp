// Game0.cpp : définit le point d'entrée pour l'application console.
//

#include "stdafx.h"


enum Direction { LEFT = 0x00, UP = 0x01, RIGHT = 0x02, DOWN = 0x03};


constexpr unsigned nScreenWidth = 100;
constexpr unsigned nScreenHeight = 100;
 

constexpr unsigned nplayFieldWidth = 50;
constexpr unsigned nplayFieldHeight = 35;

constexpr unsigned defaultConsoleScreenWidth = 80;

constexpr unsigned leftFieldPadding = 0;
constexpr unsigned upFieldPadding =0;

constexpr unsigned leftConsolePadding = 13;
constexpr unsigned upConsolePadding = 5;

constexpr unsigned playerIntialX = 25;
constexpr unsigned playerIntialY = 17;


bool keyPressed[4] = { false,false,false,false };

bool gameOver = false;
int bordersColor = 0x1;

bool spawnNode = false;
bool spawnedFruit = false;

std::atomic<bool> stepFlag = false;
Direction oldDirection;


using namespace std::chrono_literals;
std::chrono::milliseconds gameSpeed = 70ms;
int gameScore = -1;

struct Node {
	Node* next;
	unsigned x, y;
	Node(unsigned a,unsigned b) :next(NULL), x(a),y(b) {}
};

struct Player {
	Node* head;
	Node* tail;
	std::atomic<Direction> direction;
	Player(unsigned x, unsigned y):direction(UP){
		tail = head = new Node(x, y);
	}
	~Player() {
		Node* temp = head;
		while (temp) {
			delete temp;
			temp = temp->next;
		}
	}
};

void game_over(wchar_t* screen, HANDLE& hConsole, WORD* attributes) {
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
		screen[i] = L' ';
		attributes[i] = FOREGROUND_RED ;
	}
	DWORD	 dwBytesWritten = 0;
	DWORD	 dwAttribWritten = 0;
	WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	std::wstring gameOverString[7];
	gameOverString[0] = L" _______  _______  __   __  _______    _______  __   __  _______  ______   ";
	gameOverString[1] = L"|       ||   _   ||  |_|  ||       |  |       ||  | |  ||       ||    _ |  ";
	gameOverString[2] = L"|    ___||  |_|  ||       ||    ___|  |   _   ||  |_|  ||    ___||   | ||  ";
	gameOverString[3] = L"|   | __ |       ||       ||   |___   |  | |  ||       ||   |___ |   |_||_ ";
	gameOverString[4] = L"|   ||  ||       ||       ||    ___|  |  |_|  ||       ||    ___||    __  |";
	gameOverString[5] = L"|   |_| ||   _   || ||_|| ||   |___   |       | |     | |   |___ |   |  | |";
	gameOverString[6] = L"|_______||__| |__||_|   |_||_______|  |_______|  |___|  |_______||___|  |_|";


	for (int i = 0; i < 7; i++) {
		for (int j = 0; j < gameOverString[i].length(); j++) {
			screen[j+defaultConsoleScreenWidth* i] = gameOverString[i][j];
		}
	}

	WriteConsoleOutputAttribute(hConsole, attributes, nScreenWidth * nScreenHeight, { 0,0 }, &dwAttribWritten);
	WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 3,15 }, &dwBytesWritten);
}

void render(wchar_t* screen,HANDLE& hConsole ,WORD* attributes, std::vector<std::vector<wchar_t>>& gameField) {
	if (gameOver) {
		game_over(screen,hConsole,attributes);
		return;
	}
	DWORD	 dwBytesWritten = 0;
	DWORD	 dwAttribWritten = 0;

	unsigned firstLine = upFieldPadding * defaultConsoleScreenWidth;

	for (int i = 0; i < nplayFieldWidth; i++) {
		for (int j = 0; j < nplayFieldHeight; j++) {
			if (gameField[i][j]) {
				int screenCoord = firstLine + j * defaultConsoleScreenWidth + i;
				screen[screenCoord] = gameField[i][j];
				attributes[screenCoord + upConsolePadding * defaultConsoleScreenWidth + leftConsolePadding] = FOREGROUND_RED;
				if (gameField[i][j] == L'•') {
					attributes[screenCoord + upConsolePadding * defaultConsoleScreenWidth + leftConsolePadding] = FOREGROUND_GREEN;

				}
			}
		}
	}

	for (int i = leftFieldPadding + firstLine; i < leftFieldPadding + nplayFieldWidth + firstLine; i++) {
		screen[i] = L'-';
		attributes[i + upConsolePadding * defaultConsoleScreenWidth + leftConsolePadding] = bordersColor;
	}
	unsigned lastLine = firstLine + nplayFieldHeight * defaultConsoleScreenWidth;
	for (int i = lastLine + leftFieldPadding; i < lastLine + leftFieldPadding + nplayFieldWidth; i++) {
		screen[i] = L'-';
		attributes[i + upConsolePadding * defaultConsoleScreenWidth + leftConsolePadding] = bordersColor;
	}
	unsigned secondLine = firstLine + defaultConsoleScreenWidth;

	for (int i = secondLine + leftFieldPadding; i < (nplayFieldHeight + upFieldPadding)*defaultConsoleScreenWidth; i += defaultConsoleScreenWidth) {
		screen[i-1] = screen[i + nplayFieldWidth ] = L'#';
		attributes[i + upConsolePadding * defaultConsoleScreenWidth + leftConsolePadding-1] =
			attributes[i + nplayFieldWidth  + upConsolePadding * defaultConsoleScreenWidth + leftConsolePadding] = bordersColor;
	}
	WriteConsoleOutputAttribute(hConsole, attributes, nScreenWidth * nScreenHeight, { 0,0 }, &dwAttribWritten);
	WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { leftConsolePadding,upConsolePadding }, &dwBytesWritten);

}


void grow_player() {
	spawnNode = true;
	bordersColor++;
	if (gameSpeed > 60ms)
		gameSpeed-=2ms;
	gameScore++;
}
bool check_fruit(wchar_t f) {
	return f == L'•';
}

bool detect_collision(std::vector<std::vector<wchar_t>>& gameField,Player& player) {
	int debug;
	switch (player.direction) {
		case UP: {
			gameOver = player.head->y == 1;
			if (!gameOver && check_fruit(gameField[player.head->x][player.head->y - 1])) {
				spawnedFruit = false;
				grow_player();
				break;
			}
			gameOver = gameOver ? gameOver : gameField[player.head->x][player.head->y - 1] != L' ';
			debug = 1;
		}break;
		case DOWN: {
			gameOver = player.head->y == nplayFieldHeight-1;
			if (!gameOver && check_fruit(gameField[player.head->x][player.head->y + 1]) ){
				spawnedFruit = false;
				grow_player();
				break;
			}
			gameOver = gameOver ? gameOver : check_fruit(gameField[player.head->x][player.head->y + 1]);
			debug = 2;
		}break;
		case LEFT: {
			gameOver = player.head->x == 0;
			if (!gameOver && check_fruit(gameField[player.head->x - 1][player.head->y])) {
				spawnedFruit = false;
				grow_player();
				break;
			}
			gameOver = gameOver ? gameOver : gameField[player.head->x-1][player.head->y] != L' ';
			debug = 3;

		}break;
		case RIGHT: {
			gameOver = player.head->x == nplayFieldWidth-1;
			if (!gameOver && check_fruit(gameField[player.head->x + 1][player.head->y])) {
				spawnedFruit = false;
				grow_player();
				break;
			}
			gameOver = gameOver ? gameOver : gameField[player.head->x+1][player.head->y] != L' ';
			debug = 4;

		}break;
		default: throw(0);
	}
	if (gameOver)
	{
		int a = 0;
	}
	return gameOver;
}


void advance(std::vector<std::vector<wchar_t>>& gameField,Player& player) {
	if (!detect_collision(gameField,player)) {
		stepFlag = true;
		unsigned lastPosX = player.head->x;
		unsigned lastPosY = player.head->y;

		switch (player.direction) {
		case UP: {
			gameField[player.head->x][player.head->y - 1] = L'H';
			gameField[player.head->x][player.head->y--] = L' ';
		}break;
		case DOWN: {
			gameField[player.head->x][player.head->y + 1] = L'H';
			gameField[player.head->x][player.head->y++] = L' ';
		}break;
		case LEFT: {
			gameField[player.head->x - 1][player.head->y] = L'H';
			gameField[player.head->x--][player.head->y] = L' ';
		}break;
		case RIGHT: {
			gameField[player.head->x + 1][player.head->y] = L'H';
			gameField[player.head->x++][player.head->y] = L' ';
		}break;
		}
		if (spawnNode) {
			gameField[lastPosX][lastPosY] = L'H';
			Node* newNode = new Node(lastPosX, lastPosY);
			player.tail->next = newNode;
			player.tail = newNode;
			spawnNode = false;
		}
		Node* temp = player.head->next;
		Node* last = player.head;
		bool hey = false;
		while (temp) {
			hey = true;
			gameField[lastPosX][lastPosY] = L'S';
			std::swap(lastPosX, temp->x);
			std::swap(lastPosY, temp->y);
			temp = temp->next;
			last = last->next;
		}
		if (hey ) {
			gameField[last->x][last->y] = L' ';
		}
	}
}
void player_input(Player& player) {
	using namespace std::chrono_literals;
	Direction playerRTDirection = stepFlag ? (Direction)(player.direction) : oldDirection;
	for (int i = 0x25; i < 0x29; i++)
		if (GetAsyncKeyState(i) && (i - 0x25 - playerRTDirection != 2 && i - 0x25 - playerRTDirection != -2)) {
			oldDirection = player.direction;
			player.direction = (Direction)(i - 0x25);
			stepFlag = false;
		}
}
int times = 0;
int nTimes = 1000;
void auto_grow_player(std::vector<std::vector<wchar_t>>& gameField,Player& player) {
	times++;
	if (times == 3&&nTimes--) {
		times = 0;
		spawnNode = true;
	}
}

void spawn_fruits(std::vector<std::vector<wchar_t>>& gameField, WORD* attributes,Player& player) {
	if (!spawnedFruit) {
		spawnedFruit = true;
		std::random_device generatorX;
		std::random_device generatorY;
		int X, Y;
		std::uniform_int_distribution<int> distributionX(0, nplayFieldWidth-1);
		std::uniform_int_distribution<int> distributionY(1, nplayFieldHeight-1);
		bool flag = true;
		while (flag) {
			flag = false;
			X = distributionX(generatorX);
			Y = distributionY(generatorY);
			for (Node* it = player.head; it; it = it->next) {
				if (X == it->x && Y == it->y) {
					flag = true;
					break;
				}
			}
		}
		gameField[X][Y] = L'•';
	}
}

void game_loop(wchar_t *screen,HANDLE& hConsole, WORD* attributes,std::vector<std::vector<wchar_t>>& gameField,Player& player) {
	auto handle = std::async(std::launch::async,
		player_input,std::ref(player));
	while (!gameOver) {
		player_input(player);
		std::this_thread::sleep_for(gameSpeed);
		spawn_fruits(gameField, attributes,player);
		player_input(player);
		advance(gameField,player);
		render(screen, hConsole, attributes, gameField);
	}
	//handle.get();
}

void game_start() {
	wchar_t *screen = new wchar_t[nScreenWidth *nScreenHeight];
	WORD* attributes = new WORD[nScreenWidth * nScreenHeight]{ 0x0 };
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
		screen[i] = L' ';
		attributes[i] = 0xF;
	}
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(hConsole, &info);
	SetConsoleActiveScreenBuffer(hConsole);
	std::vector<std::vector<wchar_t>> gameField(nplayFieldWidth,std::vector<wchar_t>(nplayFieldHeight,L' '));
	gameField[playerIntialX][playerIntialY] = L'H';


	Player player(playerIntialX, playerIntialY);
	player.direction = Direction::LEFT;
	game_loop(screen,hConsole, attributes, gameField,player);
}

int main()
{
	game_start();
}

