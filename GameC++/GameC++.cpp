#include<iostream>
#include <SDL.h>
#include <SDL_image.h>
#include <string>
#include <fstream>
#include<SDL_mixer.h>
#include<stdio.h>
//#include <unistd.h>

using namespace std;

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;

Mix_Chunk* gTouch = NULL;
Mix_Music* gMusicGame = NULL;

const int TOTAL_HORIZONTAL_TILE = 100;
const int TOTAL_VERTICAL_TILE = 20;
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 600;
const int LEVEL_WIDTH = 3200;
const int LEVEL_HEIGHT = 640;
const int TILE_WIDTH = 32;
const int TILE_HEIGHT = 32;
const int TOTAL_TILES = 2000;
const int TOTAL_TILE_SPRITES = 247;

class LTexture {
public:
	LTexture();
	~LTexture();
	bool loadFromFile(string path);
	void free();
	void render(int x, int y, SDL_Rect* clip = NULL);
private:
	SDL_Texture* mTexture;
	int mWidth;
	int mHeight;
};

class Tile {
public:
	Tile(int x, int y, int tileType);
	void render(SDL_Rect& camera);
	int getType() { return mType; };
	SDL_Rect getBox() { return mBox; };
private:
	SDL_Rect mBox;
	int mType;
};

class Dot {
public:
	static const int DOT_WIDTH = 20;
	static const int DOT_HEIGHT = 20;
	static const int DOT_VEL = 2;
	int getPosY() { return mBox.y; }
	int getPosX() { return mBox.x; }
	Dot();
	void handleEvent(SDL_Event& e);
	void move(Tile* tiles[]);
	void setCamera(SDL_Rect& camera);
	void render(SDL_Rect& camera);
private:
	SDL_Rect mBox;
	int mVelX, mVelY;
};

bool init();
bool loadMedia(Tile* tiles[]);
void close(Tile* tiles[]);
bool checkCollision(SDL_Rect a, SDL_Rect b);
bool touchesWall(SDL_Rect box, Tile* tiles[]);
bool setTiles(Tile* tiles[]);

LTexture::LTexture() {
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture() {
	free();
}

bool LTexture::loadFromFile(string path) {
	free();
	SDL_Texture* newTexture = NULL;
	SDL_Surface* loadedSurface = IMG_Load(path.c_str());
	if (loadedSurface == NULL) {
		cout << "LoadFromFile Surface" << path.c_str() << IMG_GetError()<<endl;
	}
	else {
		newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
		if (newTexture == NULL) {
			cout << "LoadFromFile newTexture " << path.c_str() << SDL_GetError()<<endl;
		}
		else {
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}
		SDL_FreeSurface(loadedSurface);
	}
	mTexture = newTexture;
	return mTexture != NULL;
}

void LTexture::free() {
	if (mTexture != NULL) {
		SDL_DestroyTexture(mTexture);
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::render(int x, int y, SDL_Rect* clip) {
	SDL_Rect renderQuad = { x,y,mWidth,mHeight };
	if (clip != NULL) {
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}
	SDL_RenderCopy(gRenderer, mTexture, clip, &renderQuad);
}

LTexture youLose;
LTexture k;
LTexture youWin;
LTexture gDot;
LTexture gTile;
LTexture start;

SDL_Rect gTileClips[TOTAL_TILE_SPRITES];

Dot::Dot() {
	mBox.x = 0;
	mBox.y = 160;
	mBox.w = DOT_WIDTH;
	mBox.h = DOT_HEIGHT;

	mVelX = 0;
	mVelY = 0;
}

void Dot::handleEvent(SDL_Event& e) {
	if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
		switch (e.key.keysym.sym) {
		case SDLK_UP: {
			mVelY -= DOT_VEL;
			break;
		}
		case SDLK_DOWN: {
			mVelY += DOT_VEL;
			break;
		}
		case SDLK_LEFT: {
			mVelX -= DOT_VEL; break;
		}
		case SDLK_RIGHT: {
			mVelX += DOT_VEL; break;
		}
		}
	}
	else if (e.type == SDL_KEYUP && e.key.repeat == 0) {
		switch (e.key.keysym.sym) {
		case SDLK_UP: {
			mVelY += DOT_VEL;
			break;
		}
		case SDLK_DOWN: {
			mVelY -= DOT_VEL;
			break;
		}
		case SDLK_LEFT: {
			mVelX += DOT_VEL;
			break;
		}
		case SDLK_RIGHT: {
			mVelX -= DOT_VEL;
			break;
		}
		}
	}
}

void Dot::setCamera(SDL_Rect& camera) {
	camera.x = (mBox.x + DOT_WIDTH / 2) - SCREEN_WIDTH / 2;
	camera.y = (mBox.y + DOT_HEIGHT / 2) - SCREEN_HEIGHT / 2;
	if (camera.x < 0){
		camera.x = 0;
	}
	if (camera.y < 0){
		camera.y = 0;
	}
	if (camera.x > LEVEL_WIDTH - camera.w){
		camera.x = LEVEL_WIDTH - camera.w;
	}
	if (camera.y > LEVEL_HEIGHT - camera.h){
		camera.y = LEVEL_HEIGHT - camera.h;
	}
}

void Dot::render(SDL_Rect& camera) {
	gDot.render(mBox.x - camera.x, mBox.y - camera.y);
}

Tile::Tile(int x, int y, int tileType) {
	mBox.x = x;
	mBox.y = y;
	mBox.w = TILE_WIDTH;
	mBox.h = TILE_HEIGHT;
	mType = tileType;
}

void Tile::render(SDL_Rect& camera) {
	if (checkCollision(camera, mBox)) {
		gTile.render(mBox.x - camera.x, mBox.y - camera.y, &gTileClips[mType]);
	}
}

bool init() {
	bool success = true;
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		cerr << "SDL could not initialize! SDL Error: " << SDL_GetError()<<endl;
		success = false;
	}
	else {
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
			cerr << "SetHint\n";
		}
		gWindow = SDL_CreateWindow("BuiDucTrong_23021741_LTNC", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
		if (gWindow == NULL) {
			cerr << "Window could not be created! SDL Error: " << SDL_GetError()<<endl;
			success = false;
		}
		else {
			gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
			if (gRenderer == NULL) {
				cerr << "Renderer could not be created! SDL Error: " << SDL_GetError()<<endl;
				success = false;
			}
			else {
				int imgFlags = IMG_INIT_PNG;
				if (!(IMG_Init(imgFlags) & imgFlags)) {
					cerr << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << endl;
					success = false;
				}
			}
			if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
				cerr<<"SDL_mixer could not initialize! SDL_mixer Error: "<< Mix_GetError()<<"\n";
				success = false;
			}
		}
	}
	return success;
}

bool loadMusic() {
	bool success = true;
	gTouch = Mix_LoadWAV("music/touch2.mp3");
	if (gTouch == NULL) {
		cerr << "Failed to load touch sound! SDL_mixer Error: " << Mix_GetError() << endl;
		success = false;
	}
	gMusicGame = Mix_LoadMUS("music/gameMusic2.0.mp3");
	if (gMusicGame == NULL) {
		cerr << "Failed to load Music game! SDL_mixer Error: " << Mix_GetError() << endl;
	}
	return success;
}

void closeMusic() {
	Mix_FreeChunk(gTouch);
	Mix_FreeMusic(gMusicGame);
	gTouch = NULL;
	gMusicGame = NULL;
	Mix_Quit();
}

bool loadMedia(Tile* tiles[]) {
	bool success = true;
	if (!gDot.loadFromFile("image/anhnv.bmp")) {
		cerr << "Failed to load  anhnv!\n";
		success = false;
	}
	if (!gTile.loadFromFile("map/mapreal.png")) {
		cerr << "Failed to load tile map!\n";
		success = false;
	}
	if (!start.loadFromFile("image/start.bmp")) {
		cerr << "Failed to load image start!\n";
		success = false;
	}
	if (!k.loadFromFile("image/k.bmp")) {
		cerr << "Failed to load image k!\n";
		success = false;
	}
	if (!youWin.loadFromFile("image/youWin2.0.bmp")) {
		cerr << "Failed to load image youWin!\n";
		success = false;
	}
	if (!youLose.loadFromFile("image/youLose2.bmp")) {
		cerr << "Failed to load image youLose!\n";
		success = false;
	}
	if (!setTiles(tiles)) {
		cerr << "Failed to load tile set!\n";
		success = false;
	}
	return success;
}

void close(Tile* tiles[]) {
	gDot.free();
	gTile.free();
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	gRenderer = NULL;
	IMG_Quit();
	SDL_Quit();
}

bool checkCollision(SDL_Rect a, SDL_Rect b) {
	int leftA, leftB;
	int rightA, rightB;
	int topA, topB;
	int bottomA, bottomB;

	leftA = a.x;
	rightA = a.x + a.w;
	topA = a.y;
	bottomA = a.y + a.h;

	leftB = b.x;
	rightB = b.x + b.w;
	topB = b.y;
	bottomB = b.y + b.h;

	if (bottomA <= topB) {
		return false;
	}
	if (topA >= bottomB) {
		return false;
	}
	if (rightA <= leftB) {
		return false;
	}
	if (leftA >= rightB) {
		return false;
	}
	return true;
}

bool setTiles(Tile* tiles[]) {
	bool tilesLoaded = true;
	int x = 0, y = 0;
	ifstream map("map/map3.map");
	if (map.fail()) {
		cerr<<"SetTiles: mapFile!\n";
		tilesLoaded = false;
	}
	else {
		for (int i = 0; i < TOTAL_TILES; ++i) {
			int tileType = -2;
			map >> tileType;
			if (map.fail()) {
				cerr << "SetTile: File!\n";
				tilesLoaded = false;
				break;
			}
			if ((tileType >= 0) && (tileType < TOTAL_TILE_SPRITES)) {
				tiles[i] = new Tile(x, y, tileType);
			}
			else {
				cerr << "SetTile: Type " << i << "\n";
				tilesLoaded = false;
				break;
			}
			x += TILE_WIDTH;
			if (x >= LEVEL_WIDTH) {
				x = 0;
				y += TILE_HEIGHT;
			}
		}
		if (tilesLoaded) {
			int xPos = 0, yPos = 0;
			for (int i = 0; i < 247; i++) {
				gTileClips[i].x = xPos;
				gTileClips[i].y = yPos;
				gTileClips[i].w = TILE_WIDTH;
				gTileClips[i].h = TILE_HEIGHT;
				xPos += 32;
				if (xPos >= 608) {
					xPos = 0;
					yPos += 32;
				}
			}
		}
	}
	map.close();
	return tilesLoaded;
}

bool touchesWall(SDL_Rect box, Tile* tiles[]) {
	for (int i = 0; i < TOTAL_TILES; ++i) {
		if ((tiles[i]->getType() != 40) && (tiles[i]->getType() != 134)) {
			if (checkCollision(box, tiles[i]->getBox())) {
				return true;
			}
		}
	}
	return false;
}

void Dot::move(Tile* tiles[]) {
	mBox.x += mVelX;
	if ((mBox.x < 0) || (mBox.x + DOT_WIDTH > LEVEL_WIDTH) || touchesWall(mBox, tiles)) {
		mBox.x -= mVelX;
		Mix_PlayChannel(-1, gTouch, 0);
	}
	mBox.y += mVelY;
	if ((mBox.y < 0) || (mBox.y + DOT_HEIGHT > LEVEL_HEIGHT) || touchesWall(mBox, tiles)) {
		mBox.y -= mVelY;
		Mix_PlayChannel(-1, gTouch, 0);
	}
}

void startGame() {
	bool quit = false;
	SDL_Event e;
	while (!quit) {
		start.render(0, 0);
		SDL_RenderPresent(gRenderer);
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_r) {
					quit = true;
					start.free();
				}
			}
		}
	}
}

void gameKey() {
	bool quit = false;
	SDL_Event e;
	while (!quit) {
		k.render(0, 0);
		SDL_RenderPresent(gRenderer);
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_KEYDOWN) {
				if (e.key.keysym.sym == SDLK_SPACE) {
					quit = true;
					k.free();
				}
			}
		}
	}
}

void gameOver() {
	bool quit = false;
	SDL_Event e;
	while (!quit) {
		youLose.render(0, 0);
		SDL_RenderPresent(gRenderer);
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_KEYDOWN || e.type == SDL_QUIT) {
				quit = true;
				youLose.free();
			}
		}
	}
}

void gameWin() {
	SDL_Delay(5);
	bool quit = false;
	SDL_Event e;
	while (!quit) {
		youWin.render(0, 0);
		SDL_RenderPresent(gRenderer);
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_KEYDOWN || e.type == SDL_QUIT) {
				quit = true;
				youWin.free();
			}
		}
	}
}

int main(int argc, char* args[]) {

	if (!init()) {
		cerr << "Failed to initialize!\n";
	}
	if (!loadMusic()) {
		cerr << "Failed to load Music\n";
		return 0;
	}
	else {
		Tile* tileSet[TOTAL_TILES];
		if (!loadMedia(tileSet)) {
			cerr << "Failed to load media!\n";
		}
		else {
			startGame();
			gameKey();
			bool quit = false;
			SDL_Event e;
			Dot dot;
			SDL_Rect camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
			Mix_PlayMusic(gMusicGame, -1);
			while (!quit) {
				while (SDL_PollEvent(&e) != 0) {
					if (e.type == SDL_QUIT) {
						quit = true;
					}
					dot.handleEvent(e);
				}
				dot.move(tileSet);
				dot.setCamera(camera);
				SDL_SetRenderDrawColor(gRenderer, 0xFF, 0xFF, 0xFF, 0xFF);
				SDL_RenderClear(gRenderer);
				for (int i = 0; i < TOTAL_TILES; ++i) {
					tileSet[i]->render(camera);
				}
				dot.render(camera);
				SDL_RenderPresent(gRenderer);
				if (e.type == SDL_KEYDOWN) {
					if (e.key.keysym.sym == SDLK_F5) {
						gameOver();
						break;
					}
				}
				if (dot.getPosX() == TILE_WIDTH * 100 - TILE_WIDTH || dot.getPosY() == 0) {
					gameWin();
					break;
				}
			}
		}
		close(tileSet);
		closeMusic();
	}
	return 0;
}
