#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "mandelbrot_calc.h"

// #define double float

#define eprintf(...) fprintf(stderr, ##__VA_ARGS__)

#define CONTINUOUS_KEY_SPEED (9e-10)

enum {
	KDOWN_IDX = 0,
	KUP_IDX,
	KLEFT_IDX,
	KRIGHT_IDX,
	KZOOM_INC_IDX,
	KZOOM_DEC_IDX,
	KLEN
};

typedef struct SDLState {
	SDL_Window 	*window;
	SDL_Renderer 	*renderer;
	SDL_Texture 	*texture;
	int width;
	int height;

	struct MandelbrotState set_state;

	bool key_used[KLEN];
	uint64_t key_timestamps[KLEN];
} SDLState;

int sdl_init(SDLState *state, int width, int height) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		eprintf("SDL_Init Error: %s\n", SDL_GetError());
		return -1;
    	}

	if (!SDL_CreateWindowAndRenderer("Mandelbrot", width, height,
				  0, &state->window, &state->renderer)) {
		SDL_Quit();
		eprintf("Window/Renderer Error: %s\n", SDL_GetError());
		return -1;
	}

	state->texture = SDL_CreateTexture(state->renderer,
				SDL_PIXELFORMAT_XRGB8888, 
                                SDL_TEXTUREACCESS_STREAMING, width, height);

	if (!state->texture) {
		SDL_DestroyRenderer(state->renderer);
		SDL_DestroyWindow(state->window);
		SDL_Quit();

		eprintf("Texture Creation Error: %s\n", SDL_GetError());
		return -1;
	}

	return 0;
}

void sdl_destroy(SDLState *state) {
	SDL_DestroyTexture(state->texture);
	SDL_DestroyRenderer(state->renderer);
	SDL_DestroyWindow(state->window);
	SDL_Quit();
}


int renderMandelbrotTexture(SDLState *state, SDL_Texture *texture) {
	void *pixels;
        int pitch;

        if (!SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
		return -1;
	}

	uint32_t *pixels_ptr = pixels;

	int height = texture->h;
	int width = texture->w;

	calcMandelbrotSet(pixels_ptr, pitch, height, width, &state->set_state);

	SDL_UnlockTexture(texture);
	return 0;
}


int map_key_scancode_idx(unsigned int sdl_idx) {
	switch (sdl_idx) {
		case SDL_SCANCODE_DOWN:
			return KDOWN_IDX;
		case SDL_SCANCODE_UP:
			return KUP_IDX;
		case SDL_SCANCODE_LEFT:
			return KLEFT_IDX;
		case SDL_SCANCODE_RIGHT:
			return KRIGHT_IDX;
		case SDL_SCANCODE_Z:
			return KZOOM_INC_IDX;
		case SDL_SCANCODE_A:
			return KZOOM_DEC_IDX;
		default:
			return -1;
	}
}

int continuous_key_event(SDLState *state, int keyid, bool pressed, uint64_t timestamp) {
	bool *key_used = &state->key_used[keyid];
	uint64_t *key_last_timestamp = &state->key_timestamps[keyid];

	double timediff = 0;

	if (pressed) {
		if (*key_used) {
			timediff = timestamp - *key_last_timestamp;
		} else {
			*key_used = true;
		}

		*key_last_timestamp = timestamp;
	} else if (*key_used) {
		timediff = timestamp - *key_last_timestamp;
		*key_used = false;
	} else {
		return 0;
	}

	timediff *= CONTINUOUS_KEY_SPEED;
	double shift_bias = timediff * state->set_state.zoom;
	double zoom_bias = exp(-timediff);


	switch (keyid) {
		case KDOWN_IDX:
			state->set_state.shift_up -= shift_bias;
			break;
		case KUP_IDX:
			state->set_state.shift_up += shift_bias;
			break;
		case KLEFT_IDX:
			state->set_state.shift_left += shift_bias;
			break;
		case KRIGHT_IDX:
			state->set_state.shift_left -= shift_bias;
			break;
		case KZOOM_INC_IDX:
			state->set_state.zoom *= zoom_bias;
			break;
		case KZOOM_DEC_IDX:
			state->set_state.zoom /= zoom_bias;
			break;
		default:
			break;
	}


	return 0;
}

int key_event(SDLState *state, SDL_KeyboardEvent *key) {
	int map_idx = map_key_scancode_idx(key->scancode);
	if (map_idx >= 0) {
		return continuous_key_event(state,
			       	map_idx, key->down,
			       	SDL_GetTicksNS());
	}

	return 0;
}

int poll_keyboard(SDLState *state) {
	const bool *kbs = SDL_GetKeyboardState(NULL);
	uint64_t ts = SDL_GetTicksNS();

	continuous_key_event(state, KDOWN_IDX, kbs[SDL_SCANCODE_DOWN], ts);
	continuous_key_event(state, KUP_IDX, kbs[SDL_SCANCODE_UP], ts);
	continuous_key_event(state, KLEFT_IDX, kbs[SDL_SCANCODE_LEFT], ts);
	continuous_key_event(state, KRIGHT_IDX, kbs[SDL_SCANCODE_RIGHT], ts);
	continuous_key_event(state, KZOOM_INC_IDX, kbs[SDL_SCANCODE_Z], ts);
	continuous_key_event(state, KZOOM_DEC_IDX, kbs[SDL_SCANCODE_A], ts);

	return 0;
}

int main() {
	SDLState state = {0};
	SDL_Event event = {0};
	int width = 800;
	int height = 600;

	if (sdl_init(&state, width, height)) {
		return 1;
	}

	state.set_state.shift_left = 1.;
	state.set_state.shift_up = 0.;
	state.set_state.zoom = 1.;

	int running = 1;

	uint64_t fps = 0;
	uint64_t fpsTimer = SDL_GetTicks();

	int down_used = 0;
	uint64_t lastDownTimestamp = 0;

	while (running) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT ||
				(event.type == SDL_EVENT_KEY_DOWN &&
       				(event.key.key == SDLK_ESCAPE ||
				 event.key.key == SDLK_Q))) {
				running = 0;

				break;
			}

			if (event.type == SDL_EVENT_KEY_DOWN ||
				event.type == SDL_EVENT_KEY_UP) {
				SDL_KeyboardEvent *key = &event.key;

				key_event(&state, key);
			}
		}

		poll_keyboard(&state);

		renderMandelbrotTexture(&state, state.texture);

		SDL_RenderClear(state.renderer);
		SDL_RenderTexture(state.renderer, state.texture, NULL, NULL);
		SDL_RenderPresent(state.renderer);

		if (SDL_GetTicks() - fpsTimer >= 1000) {
			printf("FPS: %lu \n", fps);
			fpsTimer = SDL_GetTicks();
			fps = 0;
		} else {
			fps++;
		}
	}

	sdl_destroy(&state);

	return 0;
}
