#pragma once

#include "framework.h"

#include <vector>
#include <random>

class MoveButton {
#define MIN_DISTANCE_TO_CURSOR 15
#define DISTANCE_MULT 1.4
public:
	MoveButton(HWND hwnd, RECT* rectButton, volatile POINT* pos);
	BOOL BeginMoveIfNeeded(const POINT cursorPos, const POINT lastCursorPos);
	void Step(const POINT cursorPos, const POINT lastCursorPos);
	BOOL IsMoving();
	void GetButtonRect(RECT* rect);
private:
	LONG GetDistance(POINT pt1, POINT pt2);
	void AddPoints(std::vector<POINT>& pointsVec, int xc, int yc, int x, int y);
	void GetPossiblePoints(std::vector<POINT>& pointsVec, RECT* rectScreen, int xc, int yc, int r);
	std::mt19937 randGen;
	const HWND hwndButton;
	LONG x, y;
	LONG x1, y1;
	LONG dx, dy;
	LONG sx, sy;
	LONG err;
	LONG buttonWigth;
	LONG buttonHeight;
	LONG speed;
	LONG minDistanceFromCenter;
	volatile POINT * const remotePt;
};