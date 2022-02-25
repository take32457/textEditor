#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
typedef struct E_ROW {
	int ESCn_position;
	char* pChars; //CHAR型配列へのポインタ
	int CharsIndex;//pCharsのインデックス
	int elementCount;//prowがさすCHAR型配列の要素数
	E_ROW* next;
	E_ROW* prev;
}E_ROW;
typedef struct EDITOR {
	E_ROW* currentE_ROW;
	int Ysize;
	E_ROW* prow; //rowの連結リスト
	E_ROW* ptail;//連結リストの末尾
}EDITOR;


PCONSOLE_SCREEN_BUFFER_INFO p;
CONSOLE_SCREEN_BUFFER_INFO info;
_CONSOLE_CURSOR_INFO cursorInfo;
DWORD dwNumberOfCharsRead;
DWORD dwNumberOfCharsWrite;
EDITOR E;
int usedDrawing;
int screenMode = 1;
char currentFileName[256] = { "\0" };
int messageNum = 0;
int deleteFlag = 0;

void initEditor();
E_ROW* generateE_ROW();
void deleteRow(int);
void deleteChar(COORD,int);
void pushDelete();
void addNode(E_ROW* place, E_ROW* node);
void insertRow(int x);
void normalMode();
void insertSpace();
void backSpace();
void getInput();
void printScreen(const char*);
void insertMode();
void commandMode();
void cursorOperation();
void resizeArray();
char lpBuffer[256];
void drawing();
void writeE_ROW(int);
void characterInsert(int);
void insertChar(int x, char* str);
void remove();
int loadFromFile(const char* fileName);
void changeScreenBuffer(int*);
void printInfo();
int saveToFile(const char* fileName);
int save(const char*);
void clearEditor();
void initDrawing();


void changeScreenBuffer(int* mode)
{
	if (*mode) {  //1だったらメインバッファからサブバッファに変更
		printInfo();
		printf("----normalMode----\n");
	}
	else {
		printf("\x1b[?1049l");
	}
	*mode = (*mode + 1) % 2;
}
void printInfo() 
{
	printf("\x1b[?1049h");
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{ 0,0 });
	printf("\x1b[0J");
	printf("操作方法\n");
	printf("i 編集モードに変更\n");
	printf(": コマンドモードに変更\n");
	printf("ESC 編集モードまたはコマンドモードの時に押すとノーマルモードに変更\n");
	printf("注意:このエディタは日本語に対応していません\n");
	printf("現在開いているファイル:%s\n", currentFileName);
	cursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);	
}
int save(const char* c)
{
		if (strcmp(c, "w") == 0) {
			if (strlen(currentFileName) == 0) {
				messageNum = 1;
				return 1;
			}
			else {
				if (!saveToFile(currentFileName))messageNum = 2;
			}
		}
		else if (strncmp(c, "w ", 2) == 0) {
			char temp[256];
			strcpy(temp, c + 2);
			strcpy(currentFileName, temp);
			if (!saveToFile(currentFileName))messageNum = 2;
			else return 1;
		}
		return 0;
}
int main()
{
	GetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	p = &info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	SetConsoleDisplayMode(GetStdHandle(STD_OUTPUT_HANDLE), CONSOLE_FULLSCREEN_MODE, &p->dwCursorPosition);
	initEditor();
	int usedDrawing = 0;
	changeScreenBuffer(&screenMode);
	while (1) {
		normalMode();
	}
}
void getInput()
{
	DWORD StdInMode;
	GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &StdInMode);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_INSERT_MODE | ENABLE_VIRTUAL_TERMINAL_INPUT);
	char temp[256] = { "" };
	ReadConsole(GetStdHandle(STD_INPUT_HANDLE),
		temp,
		sizeof(temp) / sizeof(temp[0]),
		&dwNumberOfCharsRead,
		NULL);
	strcpy_s(lpBuffer, temp);
	SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), StdInMode);
}
void commandMode()
{
	char commandBuff[256] = { "\0" };
	while (1) {
		messageNum = 0;
		getInput();
		if (strcmp(lpBuffer, "\x1b") == 0) {
			printInfo();
			printf("----normalMode----\n");
			normalMode();
			return;
		}
		else if (strcmp(lpBuffer, "\r") == 0) {
			char* c = commandBuff;
			printInfo();
			printf("optionコマンドを打つと説明が表示されます。\n");
			printf("----commandMode----\n");
			if (strcmp(c, "option") == 0) {
				messageNum = 4;
			}
			else if (strcmp(c, "exit") == 0)exit(0);
			else if (strncmp(c, "c",1) == 0) {
				if (!save(c + 1)) {
					clearEditor();
					currentFileName[0] = 0;
				}
			}
			else if (strncmp(c, "r ",2) == 0) {
				if (!strlen(currentFileName)) {
					char temp[256];
					strcpy(temp, c + 2);
					if (loadFromFile(temp)) {
						messageNum = 5;
					}
					else {
						strcpy(currentFileName, temp);
					}
				}
				else messageNum = 3;
			}
			else if (c[0] == 'w') {
				save(c);
			}
			printInfo();
			printf("optionコマンドを打つと説明が表示されます。\n");
			printf("----commandMode----\n");
			if (messageNum != 0) {
				switch (messageNum) {
				case 1: printf("ファイルを開いていません\n");
					break;
				case 2: printf("保存しました\n");
					break;
				case 3: printf("ファイルを閉じてください\n");
					break;
				case 4: printf("---------------------------------------------------------------------------------\nr ファイル名:ファイルを開く(現在開いているファイルがあると使用できない)\n");
					printf("w ファイル名(省略可):ファイル名が省略されている場合-上書き保存\n");
					printf("                     ファイル名が省略されていない場合-名前を付けて保存\n");
					printf("c:ファイルを閉じる(wコマンドと組み合わせて使用できる)\n");
					printf("exit:プログラムを終了する\n---------------------------------------------------------------------------------\n");
					break;
				case 5: printf("ファイルが見つかりません\n");
				default:
					break;
				}
			}
			printf(":");
			commandBuff[0] = 0;
		}
		else if (lpBuffer[0] != '\x1b') {
			int len = strlen(commandBuff);
			if (lpBuffer[0] == 127) {
				if (len != 0) {
					commandBuff[strlen(commandBuff) - 1] = 0;
					printScreen(lpBuffer);
				}
			}
			else {
				strcat(commandBuff, lpBuffer);
				printScreen(lpBuffer);
			}
			
		}
	}
}
int saveToFile(const char* fileName) 
{
	FILE* fp = fopen(fileName, "r+");
	if (fp == NULL) {
		fp = fopen(fileName, "w");
		fclose(fp);
	}
	else {
		E_ROW* temp = E.prow->next;
		while (temp->next != NULL) {
			if (temp->next->next == NULL)temp->pChars[strlen(temp->pChars) - 1] = 0;
			fputs(temp->pChars, fp);
			temp = temp->next;
		}
		fclose(fp);
	}
	return 0;
}
int loadFromFile(const char* fileName)
{
	FILE* fp = fopen(fileName, "r+");
	if (fp == NULL)return 1;
	clearEditor();
	printf("\x1b[?1049l");
	int c;
	char temp[10];
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	while ((c = fgetc(fp)) != EOF) {
		if (c == '\n')c = '\r';
		temp[0] = c;
		temp[1] = 0;
		strcpy(lpBuffer, temp);
		characterInsert(0);
		initDrawing();
	}
	cursorInfo.bVisible = TRUE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{ 0,0 });
	E.currentE_ROW = E.prow->next;
	lpBuffer[0] = 0;
	dwNumberOfCharsRead = 0;
	drawing();
	printf("\x1b[?1049h");
	return 0;
}
void clearEditor()
{
	E_ROW* temp = E.ptail->prev;
	free(temp->next);
	while (temp->prev != NULL) {
		free(temp->pChars);
		temp = temp->prev;
		free(temp->next);
	}
	free(E.prow);
	initEditor();
	printf("\x1b[?1049l");
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{ 0,0 });
	printf("\x1b[0J");
	printf("\x1b[?1049h");
}
void insertMode()
{
	int mode = 0;
	while (1) {
		mode = 1; // 1のときエディタで入力しているとき 0のときファイルからの読み込み
		getInput();
		if (strcmp(lpBuffer, "\x1b") == 0) {
			changeScreenBuffer(&screenMode);
			normalMode();
			changeScreenBuffer(&screenMode);
			return;
		}
		writeE_ROW(mode);
	}
}
void normalMode()
{
	while (1) {
		getInput();//lpBufferに1文字+\0が入る
		if (strcmp(lpBuffer, "i") == 0) {
			changeScreenBuffer(&screenMode);
			insertMode();
			changeScreenBuffer(&screenMode);
			return;
		}
		else if (strcmp(lpBuffer, ":") == 0) {
			printInfo();
			printf("optionコマンドを打つと説明が表示されます。\n");
			printf("----commandMode----\n");
			printf("%s", lpBuffer);
			commandMode();
			return;
		}
	}
}
void printScreen(const char* lpBuffer) {
	if (lpBuffer[0] != '\x1b') {
		if (lpBuffer[0] == '\r') {
			printf("\n");
			printf("\x1b[L");
			return;
		}
		if (lpBuffer[0] == 127) {
			printf("\x1b[D");
			printf("\x1b[P");
			return;
		}
		insertSpace();
	}
	printf("%s", lpBuffer);
}
void insertSpace()
{
	printf("\x1b[@");
}
void initEditor()
{
	E.prow = (E_ROW*)malloc((int)sizeof(E_ROW));
	E_ROW* tail = (E_ROW*)malloc((int)sizeof(E_ROW));
	E.ptail = tail;
	tail->next = NULL;
	tail->prev = E.prow;
	E.prow->next = tail;
	E.prow->prev = NULL;
	E.Ysize = -1;//先頭の行を削除できるように空のポインタ変数をかませる //ここから
	addNode(E.prow, generateE_ROW());
	E.currentE_ROW = E.prow->next;
}
E_ROW* generateE_ROW() {
	E_ROW* temp = (E_ROW*)malloc((int)sizeof(E_ROW));
	temp->ESCn_position = 0;
	temp->pChars = (char*)malloc((int)sizeof(char) * 40);
	temp->pChars[0] = '\n';
	temp->pChars[1] = 0;
 	temp->elementCount = 40;
	temp->CharsIndex = 0;
	temp->next = NULL;
	temp->prev = NULL;
	return temp;
}
void writeE_ROW(int mode)
{
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	usedDrawing = 0;
	if (strcmp(lpBuffer, "\x1b[3~")==0) {
		pushDelete();
		drawing();
	}
	else if (lpBuffer[0] == '\x1b') {
		cursorOperation();
		lpBuffer[0] = 0;
		dwNumberOfCharsRead = 0;
		drawing();
	}
	else if (lpBuffer[0] == 127) {
		backSpace();
		drawing();
	}
	else {
		characterInsert(mode);
		drawing();
	}
	deleteFlag = 0;
}
void backSpace()
{
	dwNumberOfCharsRead = 0;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	if (CP.Y == 0 && CP.X == 0) return;
	if (CP.X == 0) {
		deleteRow(0); //1のとき次の行の削除 0のとき前の行の削除
		return;
	}
	deleteChar(CP,0);
	return;
}
void pushDelete()
{
	dwNumberOfCharsRead = 0;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	if (CP.X == E.currentE_ROW->ESCn_position && E.currentE_ROW->next->next == NULL) return;
	else if (CP.X == E.currentE_ROW->ESCn_position) {
		deleteRow(1);
		return;
	}
	if (CP.X < E.currentE_ROW->ESCn_position) {
		deleteChar(CP,1);
		return;
	}
	return;
}
void cursorOperation()
{
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	if (strcmp(lpBuffer, "\x1b[A") == 0) { //cursorUp
		if (CP.Y == 0)return;
		//カーソルが上に上がる状況はカーソルが下にいないと起きない
		int prevRowSize = E.currentE_ROW->prev->CharsIndex;
		if (CP.X > prevRowSize) {
			CP.X = prevRowSize;
		}
		CP.Y--;
		E.currentE_ROW = E.currentE_ROW->prev; //行が変わったので再設
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	}
	else if (strcmp(lpBuffer, "\x1b[B") == 0) { //cursorDown
		if (CP.Y < E.Ysize) { //trueになるときは現在の行に\nが必ず存在する
			int nextRowSize = E.currentE_ROW->next->CharsIndex;
			if (CP.X > nextRowSize) {
				CP.X = nextRowSize; //動くべきカーソルの位置を求める
			}
			CP.Y++;
			E.currentE_ROW = E.currentE_ROW->next; //行が変わったので再設定
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
		}
	}
	else if (strcmp(lpBuffer, "\x1b[C") == 0) { //cusorRight
		if (CP.X < E.currentE_ROW->CharsIndex) {
			printf("%s", lpBuffer);
		}
		else {
			if (CP.Y < E.Ysize) {
				CP.X = 0; //動くべきカーソルの位置を求める
				CP.Y++;
				E.currentE_ROW = E.currentE_ROW->next; //行が変わったので再設定
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
			}
		}
	}
	else if (strcmp(lpBuffer, "\x1b[D") == 0) { //cursorLeft
		if (CP.X == 0 && CP.Y == 0)return;
		if (CP.X > 0) {
			printf("%s", lpBuffer);
		}
		else {
			CP.X = E.currentE_ROW->prev->ESCn_position; //動くべきカーソルの位置を求める
			CP.Y--;
			E.currentE_ROW = E.currentE_ROW->prev; //行が変わったので再設定
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
		}
	}
	else if (strcmp(lpBuffer, "\x1b[5~") == 0) {
		E.currentE_ROW = E.prow->next;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{0,0});
	}
	else if (strcmp(lpBuffer, "\x1b[6~") == 0) {
		E.currentE_ROW = E.ptail->prev;
		CP.X = E.currentE_ROW->CharsIndex;
		CP.Y = E.Ysize;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	}
	else if (strcmp(lpBuffer, "\x1b[1;5A") == 0) {
		int i = 0;
		while (i < 4 && E.currentE_ROW->prev->prev != NULL) {
			E.currentE_ROW = E.currentE_ROW->prev;
			CP.Y--;
			i++;
		}
		if (E.currentE_ROW->CharsIndex < CP.X)CP.X = E.currentE_ROW->CharsIndex;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	}
	else if (strcmp(lpBuffer, "\x1b[1;5B") == 0) {
		int i = 0;
		while (i < 4 && E.currentE_ROW->next->next != NULL) {
			E.currentE_ROW = E.currentE_ROW->next;
			CP.Y++;
			i++;
		}
		if (E.currentE_ROW->CharsIndex < CP.X)CP.X = E.currentE_ROW->CharsIndex;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	}
	else if (strcmp(lpBuffer, "\x1b[1;5C") == 0) {
		int i = 0;
		while (i < 5 && CP.X != E.currentE_ROW->CharsIndex) {
			CP.X++;
			i++;
		}
		if (E.currentE_ROW->CharsIndex < CP.X)CP.X = E.currentE_ROW->CharsIndex;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	}
	else if (strcmp(lpBuffer, "\x1b[1;5D") == 0) {
		int i = 0;
		while (i < 5 && CP.X != 0) {
			CP.X--;
			i++;
		}
		if (E.currentE_ROW->CharsIndex < CP.X)CP.X = E.currentE_ROW->CharsIndex;
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	}
}
void characterInsert(int mode)
{
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	if (mode) {
		if (lpBuffer[0] == '{') strcat(lpBuffer, "}");
		else if (lpBuffer[0] == '(')strcat(lpBuffer, ")");
		else if (lpBuffer[0] == '[')strcat(lpBuffer, "]");
		else if (lpBuffer[0] == '\'')strcat(lpBuffer, "\'");
		else if (lpBuffer[0] == '\"')strcat(lpBuffer, "\"");
		else dwNumberOfCharsRead--;
		if (dwNumberOfCharsRead != 0)usedDrawing = 1;
		dwNumberOfCharsRead++;
	}
	if (CP.Y <= E.Ysize) {
		if (strcmp(lpBuffer, "\t") == 0) {
			strcpy(lpBuffer, "    ");
			dwNumberOfCharsRead = 4;
		}
		if (E.currentE_ROW->CharsIndex + (int)dwNumberOfCharsRead < E.currentE_ROW->elementCount) {
			if (strcmp(lpBuffer, "\r") != 0) {
				insertChar(CP.X, lpBuffer);
				E.currentE_ROW->CharsIndex += (int)dwNumberOfCharsRead;
			}
			else {
				insertRow(CP.X);
			}
		}
		else {
			resizeArray();
			characterInsert(mode);
		}
	}
}
void insertRow(int x)
{
	char* pChars = E.currentE_ROW->pChars;
	if (E.currentE_ROW->CharsIndex == x) {
		addNode(E.currentE_ROW, generateE_ROW());
		E.currentE_ROW->ESCn_position = x;
		E.currentE_ROW = E.currentE_ROW->next;
	}
	else {
		int flag = 0;
		char* temp = (char*)malloc(E.currentE_ROW->elementCount);
		strcpy(temp, pChars + x);
		pChars[x] = 0;
		strcat(pChars, "\n"); //pChars改行までの文字列 temp改行以降の文字列 
		E.currentE_ROW->CharsIndex = x;
		E.currentE_ROW->ESCn_position = x;
		addNode(E.currentE_ROW, generateE_ROW());//E_ROW構造体を生成し、双方向リストに追加する
		E.currentE_ROW = E.currentE_ROW->next;
		while (strlen(temp) > E.currentE_ROW->elementCount) {
			resizeArray();
		}
		strcpy(E.currentE_ROW->pChars, temp); 
		E.currentE_ROW->ESCn_position = strlen(temp) - 1;
		E.currentE_ROW->CharsIndex = E.currentE_ROW->ESCn_position;
		free(temp);
	}
}
void insertChar(int x,char* str)
{
	int len = strlen(str);
	char* pChars = E.currentE_ROW->pChars;
	if (E.currentE_ROW->CharsIndex + len + 1< E.currentE_ROW->elementCount) {
		E.currentE_ROW->ESCn_position += (int)dwNumberOfCharsRead;
		char* temp = (char*)malloc(E.currentE_ROW->elementCount);
		strcpy(temp, pChars + x);
		pChars[x] = 0;
		strcat(pChars, str);
		strcat(pChars, temp);
		free(temp);
	}
	else {
		resizeArray();
		insertChar(x, str);
	}
}
void deleteChar(COORD xy,int mode)
{
	char* pChars = E.currentE_ROW->pChars;
	char* temp = (char*)malloc(E.currentE_ROW->elementCount);
	strcpy(temp, pChars + xy.X + mode);
	pChars[xy.X - 1 + mode] = 0; // 1 2 3 \n
	strcat(pChars, temp);
	xy.X--;
	xy.X += mode;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), xy);
	E.currentE_ROW->CharsIndex--;
	E.currentE_ROW->ESCn_position--;
	free(temp);
}
void deleteRow(int mode)
{
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	int len;
	char* temp;
	if (!mode) {
		temp = (char*)malloc(E.currentE_ROW->elementCount);
		strcpy(temp, E.currentE_ROW->pChars);
		E.currentE_ROW = E.currentE_ROW->prev;
	}
	else {
		temp = (char*)malloc(E.currentE_ROW->next->elementCount);
		strcpy(temp, E.currentE_ROW->next->pChars);
	}
	int pos = E.currentE_ROW->ESCn_position;
	remove();	
	E.currentE_ROW->pChars[pos] = 0;
	while (E.currentE_ROW->ESCn_position + strlen(temp) > E.currentE_ROW->elementCount) {
		resizeArray();
	}
	strcat(E.currentE_ROW->pChars, temp);
	len = strlen(E.currentE_ROW->pChars);
	E.currentE_ROW->ESCn_position = len - 1;
	E.currentE_ROW->CharsIndex = len - 1;
	CP.Y--;
	CP.Y += mode;
	CP.X = pos;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	free(temp);
	deleteFlag = 1;
}
void resizeArray()
{
	char* newChars;
	E.currentE_ROW->elementCount *= 2;
	//2倍の要素数を確保
	newChars = (char*)malloc((int)sizeof(char) * (E.currentE_ROW->elementCount));
	newChars[0] = 0;
	strcpy_s(newChars, (int)sizeof(char) * E.currentE_ROW->elementCount, E.currentE_ROW->pChars);
	free(E.currentE_ROW->pChars);
	E.currentE_ROW->pChars = newChars;
}
void drawing() 
{
	cursorInfo.bVisible = FALSE;
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	int top = p->srWindow.Top;
	int bottom = p->srWindow.Bottom;

	if (strcmp(lpBuffer, "\r") != 0)	CP.X += (int)dwNumberOfCharsRead;
	else {
		printf("\x1b[L");
		CP.Y++;
		CP.X = 0;
	}
	if (deleteFlag)printf("\x1b[M");
	E_ROW* temp = E.currentE_ROW;
	E_ROW* iterator = temp;
	SHORT i = CP.Y;
	while (top != i) {
		iterator = iterator->prev;
		i--;
	}
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), COORD{ 0,i });
	while(bottom  >= i && iterator->next != NULL) {
		printf("\x1b[300@");
		if (bottom == i)iterator->pChars[strlen(iterator->pChars) - 1] = 0;
		printf("%s", iterator->pChars);
		if (bottom == i)iterator->pChars[strlen(iterator->pChars)] = '\n';
		iterator = iterator->next;
		i++;
	}
	
	E.currentE_ROW = temp;
	cursorInfo.bVisible = TRUE;
	CP.X -= usedDrawing;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursorInfo);
}
void initDrawing()
{
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), p);
	COORD CP = p->dwCursorPosition;
	if (strcmp(lpBuffer, "\r") != 0)	CP.X += (int)dwNumberOfCharsRead;
	else {
		printf("\x1b[L");
		CP.Y++;
		CP.X = 0;
	}
	CP.X -= usedDrawing;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), CP);
}
void addNode(E_ROW* place, E_ROW* node) 
{
	node->prev = place->next->prev;
	node->next = place->next;
	place->next->prev = node;
	place->next = node;
	E.Ysize++;
}
void remove() 
{
	E_ROW* temp = E.currentE_ROW->next;
	E.currentE_ROW->next = E.currentE_ROW->next->next;
	E.currentE_ROW->next->prev = E.currentE_ROW;
	free(temp);
	E.Ysize--;
}