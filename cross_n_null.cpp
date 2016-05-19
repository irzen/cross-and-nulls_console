// cross_n_null.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <Windows.h>
#include <time.h>
#include <vector>

using namespace std;

// some constants
const byte sizeX = 10;	// width of the field
const byte sizeY = 10;	// height of the field
const byte offsetX = 5;	// offset from the left border
const byte offsetY = 1;	// offset from the top border

enum ECellState { eCellStateEmpty, eCellStateCross, eCellStateNull };	// states of the cells on the field

// some globals
HANDLE g_hIn, g_hCon;	// input and output handles
COORD cursor;		// position of the players cursor

// the way to make life easer
// TODO: add varargs and pass them to printf
void print(COORD pos, const char* msg)
{
	SetConsoleCursorPosition(g_hCon, pos);
	printf(msg);
};

// gamefield. 'S' means type, 's' means object
struct SField
{
	ECellState cell[sizeX*sizeY];
} sField;

// initialize field with empty states
void clearField()
{
	for (int i = 0; i < sizeX*sizeY; ++i)
	{
		sField.cell[i] = eCellStateEmpty;
	}
}

// draw field. supports currently unused features
void drawField()
{
	char c[2];
	c[1] = '\0';
	for (int i = 0; i < sizeX; ++i)
	{
		for (int j = 0; j < sizeY; ++j)
		{
			switch (sField.cell[i + j*sizeX])
			{
			case eCellStateEmpty:
				c[0] = '+';
				break;
			case eCellStateCross:
				c[0] = 'X';
				break;
			case eCellStateNull:
				c[0] = 'O';
				break;
			}
			print({i * 2 + offsetX, j + offsetY}, &c[0]);
		}
	}
};

// set position of player's cursor
// clear "><" from previous location, draw them on new location
void setMyCursor(COORD pos)
{
	print({cursor.X * 2 + offsetX - 1, cursor.Y + offsetY}, " ");
	print({cursor.X * 2 + offsetX + 1, cursor.Y + offsetY}, " ");

	switch (sField.cell[cursor.X + cursor.Y*sizeX])
	{
	case eCellStateEmpty:
		print({cursor.X * 2 + offsetX, cursor.Y + offsetY}, "+");
		break;
	case eCellStateCross:
		print({cursor.X * 2 + offsetX, cursor.Y + offsetY}, "X");
		break;
	case eCellStateNull:
		print({cursor.X * 2 + offsetX, cursor.Y + offsetY}, "O");
		break;
	}

	cursor = pos;
	
	if (sField.cell[pos.X + pos.Y*sizeX] == eCellStateEmpty)
		print({pos.X * 2 + offsetX, pos.Y + offsetY}, "x");

	print({pos.X * 2 + offsetX - 1, pos.Y + offsetY}, ">");
	print({pos.X * 2 + offsetX + 1, pos.Y + offsetY}, "<");	
};

// clear field and reposition cursor
void restartGame()
{
	clearField();
	drawField();
	setMyCursor({0, 0});
}

// change state of the cell under given location with given state
// do nothing if the cell is not empty
void setField(COORD pos, ECellState state)
{
	if (sField.cell[pos.X + pos.Y*sizeX] != eCellStateEmpty)
		return;

	sField.cell[pos.X + pos.Y*sizeX] = state;
	switch (state)
	{
	case eCellStateEmpty:
		print({pos.X * 2 + offsetX, pos.Y + offsetY}, "+");
		break;
	case eCellStateCross:
		print({pos.X * 2 + offsetX, pos.Y + offsetY}, "X");
		break;
	case eCellStateNull:
		print({pos.X * 2 + offsetX, pos.Y + offsetY}, "O");
		break;
	}
};

// the way how AI takes it's turns
// just get all available positions and choose randomly
COORD decisionAI()
{
	const int size = sizeX*sizeY;
	vector<int> available;
	for (int i = 0; i < size; ++i)
		if (sField.cell[i] == eCellStateEmpty)
			available.push_back(i);

	if (available.empty())
		return {0, 0};
	const int rnd = rand() % available.size();
	return {(available[rnd] % sizeX), (available[rnd] / sizeX)};
};

// find the 5+ row of the crosses
// horisontal, vertical and diagonal
// TODO: add argument that allows to check noughts
int checkWin()
{
	int longestX = 0, longestY = 0, longestDR = 0, longestDL = 0;
	for (int i = 0; i < sizeX; ++i)
	{
		for (int j = 0; j < sizeY; ++j)
		{
			if (sField.cell[i + j*sizeX] == eCellStateCross)
				++longestX;
			else
				longestX = 0;
			if (longestX == 5)
				return 1;
			if (i == sizeX - 1)
				longestX = 0;

			if (sField.cell[j + i*sizeY] == eCellStateCross)
				++longestY;
			else
				longestY = 0;
			if (longestY == 5)
				return 1;
			if (j == sizeY - 1)
				longestY = 0;
			
			for (int k = 0; k < 5; ++k)
			{
				if (i < sizeX - 4 && j < sizeY - 4 && sField.cell[(i + k) + (j + k)*sizeX] == eCellStateCross)
					++longestDR;
				else
					longestDR = 0;
				if (longestDR == 5)
					return 1;

				if (k == 4)
					longestDR = 0;

				if (i > 3 && j < sizeY - 4 && sField.cell[(i - k) + (j + k)*sizeX] == eCellStateCross)
					++longestDL;
				else
					longestDL = 0;
				if (longestDL == 5)
					return 1;

				if (k == 4)
					longestDL = 0;
			}
		}		
	}
	return 0;
}

// clear screen
void clrScr()
{
	DWORD len;
	FillConsoleOutputCharacter(g_hCon, ' ', 80 * 60, {0, 0}, &len);
	SetConsoleCursorPosition(g_hCon, {0, 0});
}

// simple menu
// 
int menu() // -1 - quit, 1 - solo
{
	clrScr();
	const COORD posTop = {5, 2};	// offset from top-left corner
	const char* btnSolo = "Single";	// well, that pretty ugly but currently is ok
	const char* btnQuit = "Quit";	
	const int menuLastItem = 1;		// the way to know where is the menu's end
	const char* cursor = "=>>";		// local override - is bad idea; TODO: fix that
	byte posCursor = 0;		// position of the cursor that chooses menus

	// print menu items and cursor
	print({posTop.X, posTop.Y}, btnSolo);
	print({posTop.X, posTop.Y + 1}, btnQuit);
	print({posTop.X - strlen(cursor), posTop.Y+posCursor}, cursor);

	while (true)
	{
		INPUT_RECORD inrec;
		DWORD        count;

		// Wait for user input
		do ReadConsoleInput(g_hIn, &inrec, 1, &count);
		while ((inrec.EventType != KEY_EVENT) || !inrec.Event.KeyEvent.bKeyDown);

		switch (inrec.Event.KeyEvent.wVirtualKeyCode)
		{
		case VK_UP:
			if (posCursor > 0)
			{
				print({posTop.X - strlen(cursor), posTop.Y + posCursor}, "   ");
				--posCursor;
			}
			break;
		case VK_DOWN:
			if (posCursor < menuLastItem)
			{
				print({posTop.X - strlen(cursor), posTop.Y + posCursor}, "   ");
				++posCursor;
			}
			break;
		case VK_SPACE:
			clrScr();
			if (posCursor == 0)
				return 1;
			if (posCursor == 1)
				return -1;
			break;
		case VK_ESCAPE:
			clrScr();
			return -1;
		};
		print({posTop.X - strlen(cursor), posTop.Y + posCursor}, cursor);
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	g_hIn = GetStdHandle(STD_INPUT_HANDLE);
	g_hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleMode(g_hCon, ENABLE_PROCESSED_INPUT);

	CONSOLE_CURSOR_INFO     cursorInfo;
	GetConsoleCursorInfo(g_hCon, &cursorInfo);
	cursorInfo.bVisible = false;
	SetConsoleCursorInfo(g_hCon, &cursorInfo);

	FlushConsoleInputBuffer(g_hIn);
	srand(time(NULL));
	
	int retMenuCode = menu();
	if (retMenuCode == -1)
		return 0;

	restartGame();
	
	while (true)
	{
		INPUT_RECORD inrec;
		DWORD        count;
		COORD		 nullPos;

		do ReadConsoleInput(g_hIn, &inrec, 1, &count);
		while ((inrec.EventType != KEY_EVENT) || !inrec.Event.KeyEvent.bKeyDown);

		switch (inrec.Event.KeyEvent.wVirtualKeyCode)
		{
		case VK_LEFT:
			if (cursor.X > 0)
				setMyCursor({cursor.X - 1, cursor.Y});
			break;
		case VK_RIGHT:
			if (cursor.X < sizeX - 1)
				setMyCursor({cursor.X + 1, cursor.Y});
			break;
		case VK_UP:
			if (cursor.Y > 0)
				setMyCursor({cursor.X, cursor.Y - 1});
			break;
		case VK_DOWN:
			if (cursor.Y < sizeY - 1)
				setMyCursor({cursor.X, cursor.Y + 1});
			break;
		case VK_SPACE:
			setField(cursor, eCellStateCross);
			setField(decisionAI(), eCellStateNull);
			if (checkWin() == 1)
			{
				SetConsoleCursorPosition(g_hCon, {0, sizeY + offsetY});
				system("pause");
				clrScr();
				retMenuCode = menu();
				if (retMenuCode == -1)
					return 0;

				restartGame();
			}
			break;
		case VK_ESCAPE:
			return 0;
		};
	}

	SetConsoleCursorPosition(g_hCon, {0, sizeY + offsetY});
	system("pause");
	return 0;
}

