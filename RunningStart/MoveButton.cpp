#include "MoveButton.h"

MoveButton::MoveButton(HWND hwnd, RECT* rectButton, volatile POINT* pos) : hwndButton(hwnd), remotePt(pos), randGen{ std::random_device()() } {	
	buttonWigth = rectButton->right - rectButton->left;
	buttonHeight = rectButton->bottom - rectButton->top;
	x = x1 = rectButton->left;
	y = y1 = rectButton->top;
	minDistanceFromCenter = max(buttonWigth / 2, buttonHeight / 2) + MIN_DISTANCE_TO_CURSOR;
}

BOOL MoveButton::BeginMoveIfNeeded(const POINT cursorPos, const POINT lastCursorPos) {
	POINT buttonCenter = { x + buttonWigth / 2, y + buttonHeight / 2 };
	int distanceCurrent = GetDistance(buttonCenter, cursorPos);
	if (distanceCurrent >= minDistanceFromCenter) return FALSE;	
	POINT buttonCenterNew = { buttonCenter.x + (LONG)((buttonCenter.x - lastCursorPos.x) * DISTANCE_MULT), buttonCenter.y + (LONG)((buttonCenter.y - lastCursorPos.y) * DISTANCE_MULT) };
	RECT rectScreen = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
	InflateRect(&rectScreen, x - buttonCenter.x, y - buttonCenter.y);
	if (!PtInRect(&rectScreen, buttonCenterNew)) {
		static std::vector<POINT> points;
		GetPossiblePoints(points, &rectScreen, lastCursorPos.x, lastCursorPos.y, GetDistance(buttonCenterNew, lastCursorPos));
		if (points.size()) {
			std::uniform_int_distribution<size_t> distrib(0, points.size() - 1);
			size_t randIndex = distrib(randGen);
			buttonCenterNew = points[randIndex];
		}
		else return FALSE;		
		speed = 10;
	}
	else speed = 0;
	LONG offsetX = buttonCenter.x - buttonCenterNew.x;
	LONG offsetY = buttonCenter.y - buttonCenterNew.y;
	// Using Bresenham's line algorithm
	x1 = x - offsetX;
	y1 = y - offsetY;
	dx = abs(offsetX);
	dy = abs(offsetY);
	sx = x < x1 ? 1 : -1;
	sy = y < y1 ? 1 : -1;
	err = (dx > dy ? dx : -dy) / 2;
	return TRUE;
}

void MoveButton::Step(const POINT cursorPos, const POINT lastCursorPos) {
	speed = max(speed, GetDistance(cursorPos, lastCursorPos));
	for (int i = 0; i < speed; ++i) {
		if (!IsMoving()) break;
		// Using Bresenham's line algorithm
		LONG e = err;
		if (e > -dx) {
			err -= dy;
			x += sx;
		}
		if (e < dy) {
			err += dx;
			y += sy;
		}
	}
	remotePt->x = x;
	remotePt->y = y;
	SetWindowPos(hwndButton, NULL, x, y, 0, 0, SWP_NOSENDCHANGING | SWP_DEFERERASE | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER | SWP_NOCOPYBITS | SWP_NOREDRAW);
}
BOOL MoveButton::IsMoving() {
	return (x != x1 || y != y1);
}

void MoveButton::GetButtonRect(RECT* rect) {
	*rect = { x, y, x + buttonWigth, y + buttonHeight };
};

LONG MoveButton::GetDistance(POINT pt1, POINT pt2) {
	LONG a = pt1.x - pt2.x;
	LONG b = pt1.y - pt2.y;
	return (LONG)sqrt(a * a + b * b);
}

void MoveButton::AddPoints(std::vector<POINT>& pointsVec, int xc, int yc, int x, int y) {
	pointsVec.push_back({ x + xc, y + yc });
	pointsVec.push_back({ x + xc, -y + yc });
	pointsVec.push_back({ -x + xc, y + yc });
	pointsVec.push_back({ -x + xc, -y + yc });
	pointsVec.push_back({ y + xc, x + yc });
	pointsVec.push_back({ y + xc, -x + yc });
	pointsVec.push_back({ -y + xc, x + yc });
	pointsVec.push_back({ -y + xc, -x + yc });
}

void MoveButton::GetPossiblePoints(std::vector<POINT>& pointsVec, RECT* rectScreen, int xc, int yc, int r) {
	pointsVec.clear();
	// Using Bresenham’s circle drawing algorithm
	int x = 0;
	int y = r;
	int d = 3 - (2 * r);
	AddPoints(pointsVec, xc, yc, x, y);
	while (x <= y) {
		if (d <= 0) {
			d = d + (4 * x) + 6;
		}
		else {
			d = d + (4 * x) - (4 * y) + 10;
			--y;
		}
		++x;
		AddPoints(pointsVec, xc, yc, x, y);
	}
	std::sort(pointsVec.begin(), pointsVec.end(), [](const POINT& a, const POINT& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
	// Removing duplicates and points out of screen rect
	auto itSlow = pointsVec.begin(), itFast = itSlow;
	for (; itFast != pointsVec.end() - 1; ++itFast) {
		if ((itFast + 1)->x == itFast->x && (itFast + 1)->y == itFast->y) continue;
		if (!PtInRect(rectScreen, *itFast)) continue;
		if (itSlow != itFast) {
			*itSlow = *itFast;
		}
		++itSlow;
	}
	if (PtInRect(rectScreen, *itFast)) *itSlow++ = *(itFast);
	pointsVec.erase(itSlow, pointsVec.end());
}