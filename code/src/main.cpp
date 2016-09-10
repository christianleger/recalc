

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "SDL2/SDL_events.h"
#include "SDL2/SDL_timer.h"
#include "SDL2/SDL_opengl.h"
#include "SDL2/SDL_video.h"


using namespace std;

SDL_Window* p_sdlWindow = NULL;

void init() {
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	p_sdlWindow = SDL_CreateWindow(
		"My Game",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		512,
		512,
		SDL_WINDOW_OPENGL
	);

	SDL_GLContext glContext = SDL_GL_CreateContext(p_sdlWindow);

	glClearColor(1.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(p_sdlWindow);
}

int main(int argc, char** argv) {

	init();

	printf("EHlloo. ");

	int hello = 0;
	//cout << "tell us your favorite number (under 2 to the 31st power):";
	//cin >> hello ; 

	bool loop = true;

	while (loop)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
				loop = false;

			if (event.type == SDL_KEYDOWN)
			{
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					loop = false;
					break;
				case SDLK_r:
					// Cover with red and update
					glClearColor(1.0, 0.0, 0.0, 1.0);
					glClear(GL_COLOR_BUFFER_BIT);
					SDL_GL_SwapWindow(p_sdlWindow);
					break;
				case SDLK_g:
					// Cover with green and update
					glClearColor(0.0, 1.0, 0.0, 1.0);
					glClear(GL_COLOR_BUFFER_BIT);
					SDL_GL_SwapWindow(p_sdlWindow);
					break;
				case SDLK_b:
					// Cover with blue and update
					glClearColor(0.0, 0.0, 1.0, 1.0);
					glClear(GL_COLOR_BUFFER_BIT);
					SDL_GL_SwapWindow(p_sdlWindow);
					break;
				default:
					break;
				}
			}
		}

	}

	//SDL_Delay(3000);
	//exit(0);
}