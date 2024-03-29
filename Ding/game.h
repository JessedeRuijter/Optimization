#pragma once

#define SCRWIDTH	 800
#define SCRHEIGHT	 640

namespace Tmpl8 {

struct Task
{
	int x1, y1, x2, y2, scale;
};

class Surface;
class Game
{
public:
	void SetTarget( Surface* _Surface ) { screen = _Surface; }
	void Init();
	void Shutdown();
	void HandleInput( float dt ) {}
	void Set( int x, int y, byte value );
	byte Get( int x, int y );
	void Push( int x1, int y1, int x2, int y2, int scale )
	{
		task[taskPtr].x1 = x1, task[taskPtr].x2 = x2;
		task[taskPtr].y1 = y1, task[taskPtr].y2 = y2;
		task[taskPtr++].scale = scale;
	}
	void Subdivide( int x1, int y1, int x2, int y2, int scale );
	void Tick( float dt );
	void MouseUp( int _Button ) { /* implement if you want to detect mouse button presses */ }
	void MouseDown( int _Button ) { /* implement if you want to detect mouse button presses */ }
	void MouseMove( int _X, int _Y ) { /* implement if you want to detect mouse movement */ }
	void KeyUp( int a_Key ) { /* implement if you want to handle keys */ }
	void KeyDown( int a_Key ) { /* implement if you want to handle keys */ }
private:
	Surface* screen;
	Memory* memory;
	Cache* cache1;
	Cache* cache2;
	Cache* cache3;
	Task task[512];
	int taskPtr, c;
	//real-time visualization
	int data[SCRWIDTH][DATAHEIGHT];
};

}; // namespace Tmpl8