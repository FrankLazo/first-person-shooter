#include <iostream>
#include <chrono>
#include <math.h>
#include <vector>
#include <algorithm>
using namespace std;

#include <Windows.h>

// Screen size
int nScreenWidth = 120;	// Screen columns
int nScreenHeight = 40;	// Screen rows

// Player data
float fPlayerX = 4.0f; 	// Player's X position
float fPlayerY = 14.0f; // Player's Y position
float fPlayerA = 0.0f; 	// Player's direction angle

// Minimap size
int nMapHeight = 16; 	// Minimap's height
int nMapWidth = 16; 	// Minimap's width

// Field of View
float fFOV = 3.14159 / 4.0;	// Equal to 45º
float fDepth = 16.0;		// Depth of FoV

int main()
{
	// Create Screen Buffer
	wchar_t *screen = new wchar_t[ nScreenWidth * nScreenHeight ];
	HANDLE hConsole = CreateConsoleScreenBuffer( GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL ); // Create a new screen buffer
	SetConsoleActiveScreenBuffer( hConsole ); // Set the new screen buffer active
	DWORD dwBytesWritten = 0;

	// Minimap
	wstring map;

	// #: Wall
	// .: Empty space
	map += L"#########.......";
	map += L"#...............";
	map += L"#.......########";
	map += L"#..............#";
	map += L"#......##......#";
	map += L"#......##......#";
	map += L"#..............#";
	map += L"###............#";
	map += L"##.............#";
	map += L"#......####..###";
	map += L"#......#.......#";
	map += L"#......#.......#";
	map += L"#..............#";
	map += L"#......#########";
	map += L"#..............#";
	map += L"################";

	// Time points
	auto tp1 = chrono::system_clock::now();
	auto tp2 = chrono::system_clock::now();

	// Game Loop
	while ( 1 )
	{
		// Elapsed time
		// For smooth movement
		tp2 = chrono::system_clock::now();
		chrono::duration< float > elapsedTime = tp2 - tp1;
		tp1 = tp2;
		float fElapsedTime = elapsedTime.count();

		// Controls
		// Handle CCW Rotation
		if ( GetAsyncKeyState( (unsigned short)'A' ) & 0x8000 )
			fPlayerA -= (0.8f) * fElapsedTime;

		if ( GetAsyncKeyState( (unsigned short)'D' ) & 0x8000 )
			fPlayerA += (0.8f) * fElapsedTime;

		if ( GetAsyncKeyState( (unsigned short)'W' ) & 0x8000 )
		{
			fPlayerX += cosf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY += sinf(fPlayerA) * 5.0f * fElapsedTime;

			if ( map[ (int)fPlayerY * nMapWidth + (int)fPlayerX ] == '#')
			{
				fPlayerX -= cosf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY -= sinf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		if ( GetAsyncKeyState( (unsigned short)'S' ) & 0x8000 )
		{
			fPlayerX -= cosf(fPlayerA) * 5.0f * fElapsedTime;
			fPlayerY -= sinf(fPlayerA) * 5.0f * fElapsedTime;

			if ( map[ (int)fPlayerY * nMapWidth + (int)fPlayerX ] == '#')
			{
				fPlayerX += cosf(fPlayerA) * 5.0f * fElapsedTime;
				fPlayerY += sinf(fPlayerA) * 5.0f * fElapsedTime;
			}
		}

		// For each column in the screen
		for ( int x = 0; x < nScreenWidth; x++ )
		{
			// For each column, calculate the projected ray angle into world space
			float fRayAngle = ( fPlayerA - fFOV / 2.0f ) + ( (float)x / (float)nScreenWidth ) * fFOV;

			float fDistanceToWall = 0;
			bool bHitWall = false;
			bool bBoundary = false;

			// Unit vector for ray in player space
			float fEyeX = cosf( fRayAngle );
			float fEyeY = sinf( fRayAngle );

			while ( !bHitWall && fDistanceToWall < fDepth )
			{
				fDistanceToWall += 0.1f;

				// Calculate distance to wall: abscissa and ordinate
				int nTestX = (int)( fPlayerX + fEyeX * fDistanceToWall );
				int nTestY = (int)( fPlayerY + fEyeY * fDistanceToWall );

				// Test if ray is out of bounds
				if ( nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight )
				{
					bHitWall = true;
					fDistanceToWall = fDepth; // Just set distance to maximum depth
				}
				else
				{
					// Ray is inbounds so test to see if the ray cell is a wall block
					if ( map[ nTestY * nMapWidth + nTestX ] == '#' )
					{
						bHitWall = true;

						// Detect corners of blocks
						vector< pair< float, float > > p; // distance, dot product

						for ( int tx = 0; tx < 2; tx++ )
							for ( int ty = 0; ty < 2; ty++ )
							{
								// Vector from player to perfect corner
								float vy = (float)nTestY + ty - fPlayerY;
								float vx = (float)nTestX + tx - fPlayerX;

								float distance = sqrt( vx * vx + vy * vy );
								float dotProduct = ( fEyeX * vx / distance ) + ( fEyeY * vy / distance );

								p.push_back( make_pair( distance, dotProduct ) );
							}

						// Sort pairs from closest to farthest
						sort( p.begin(), p.end(), []( const pair< float, float > &left, const pair< float, float > &right ){ return left.first < right.first; } );

						// Maximum angle between perfect corner and ray
						float fBound = 0.01;

						// Just two closest corners
						if ( acos( p.at( 0 ).second ) < fBound ) bBoundary = true;
						if ( acos( p.at( 1 ).second ) < fBound ) bBoundary = true;
						// if ( acos( p.at( 2 ).second ) < fBound ) bBoundary = true;
					}
				}
			}

			// Calculate distance to ceiling and floor
			int nCeiling = (float)( nScreenHeight / 2.0 ) - nScreenHeight / ( (float)fDistanceToWall );
			int nFloor = nScreenHeight - nCeiling;

			// Shades of wall
			short nShade = ' ';

			if ( fDistanceToWall <= fDepth / 4.0f )		nShade = 0x2588;	// Very close
			else if ( fDistanceToWall < fDepth / 3.0f ) nShade = 0x2593;	// Close
			else if ( fDistanceToWall < fDepth / 2.0f ) nShade = 0x2592;	// Medium
			else if ( fDistanceToWall < fDepth ) 		nShade = 0x2591;	// Far
			else 										nShade = ' ';		// Too far away

			if ( bBoundary )							nShade = ' ';		// Black it out

			// Draw column: ceiling, wall and floor
			for ( int y = 0; y < nScreenHeight; y++ )
			{
				if ( y <= nCeiling )
					screen[ y * nScreenWidth + x ] = ' ';
				else if ( y > nCeiling && y < nFloor )
					screen[ y * nScreenWidth + x ] = nShade;
				else
				{
					// Shade floor based on distance
					float b = 1.0f - ( ( (float)y - nScreenHeight / 2.0f ) / ( (float)nScreenHeight / 2.0f ) );

					if ( b < 0.25 )			nShade = '#';
					else if ( b < 0.5 )		nShade = 'x';
					else if ( b < 0.75 )	nShade = '.';
					else if ( b < 0.9 )		nShade = '-';
					else					nShade = ' ';

					screen[ y * nScreenWidth + x ] = nShade;
				}
			}
		}

		// Display Stats
		swprintf_s( screen, 40, L"X = %3.2f, Y = %3.2f, A = %3.2f FPS = %3.2f", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime );

		// Display Map
		for ( int nx = 0; nx < nMapWidth; nx++ )
			for ( int ny = 0; ny < nMapHeight; ny++ )
			{
				screen[ ( ny + 1 ) * nScreenWidth + nx ] = map[ ny * nMapWidth + nx ];
			}

		// Display player
		screen[ ( (int)fPlayerY + 1 ) * nScreenWidth + (int)fPlayerX ] = 'P';

		screen[ nScreenWidth * nScreenHeight - 1 ] = '\0';
		WriteConsoleOutputCharacterW( hConsole, screen, nScreenWidth * nScreenHeight, { 0, 0 }, &dwBytesWritten ); // Write characters in console
	}

	return 0;
}
