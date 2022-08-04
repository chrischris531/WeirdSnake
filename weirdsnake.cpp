// snake but it's really weird
// basically: you move and eat food, like normal, but every time you eat food enemies appear...

#include <Windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <conio.h>
#include <vector>
#include <cmath>

// stuff for making printing better
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

void setCursorPosition(int x, int y)
{
	static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	std::cout.flush();
	COORD coord = { (SHORT)x, (SHORT)y };
	SetConsoleCursorPosition(hOut, coord);
}

// a piece of a body, with coordinates and current direction
struct PieceSegment {
	int x;
	int y;
	char direction;
};

// a grid class for the game
class Grid {
private:
	int gridHeight, gridWidth; // dimensions
	int score; // score
	std::vector<PieceSegment> player, enemies; // entities with multiple segments
	PieceSegment food, power; // single entities
	std::vector<char> directionBuffer; // all the directions for a body's segments
	char newDirection; // the direction for the front of the player to move next iteration
	int frames; // the number of frames that have passed since the beginning

	// function to use direction of piece to work out where to move next
	void calculateNextCoordinate(PieceSegment * piece) {
		// left
		if (piece->direction == 'a') {
			piece->x--;
		}
		// right
		if (piece->direction == 'd') {
			piece->x++;
		}
		// up
		if (piece->direction == 'w') {
			piece->y--;
		}
		// down
		if (piece->direction == 's') {
			piece->y++;
		}
	}

	// function to check if current coord in printgrid is one of the piecesegments of player or enemy
	char showPiece(int i, int j) {
		// go through all segments of player and show 'p' if any are at current coords
		for (int a = 0; a < player.size(); a++) {
			if (player[a].x == j && player[a].y == i) {
				return 'p';
			}
		}
		// go through all enemies and show 'e' if any are at current coords
		for (int b = 0; b < enemies.size(); b++) {
			if (enemies[b].x == j && enemies[b].y == i) {
				return 'e';
			}
		}
		
		// default: 'x' means no 
		return 'x';
	}

	// function to show the grid every iteration
	void printGrid() {
		// show the score
		std::cout << "SCORE: " << score << std::endl;

		// go through height and width and print necessary values
		for (int i = 0; i < gridHeight; i++) {
			for (int j = 0; j < gridWidth; j++) {
				// player/enemy pieces
				if (showPiece(i, j) != 'x') {
					std::cout << showPiece(i, j);
				}
				// food
				else if (i == food.y && j == food.x) {
					std::cout << 'f';
				}
				// power
				else if (i == power.y && j == power.x) {
					std::cout << '?';
				}
				// sides
				else if (j == 0 || j == gridWidth - 1) {
					std::cout << '|';
				}
				// top/bottom
				else if (i == 0 || i == gridHeight - 1) {
					std::cout << '-';
				}
				// none of the above
				else {
					std::cout << ' ';
				}
				// print new line
				if (j == gridWidth - 1) {
					std::cout << std::endl;
				}
			}
		}
	}

	// function to generate food at a certain random coordinate
	void generateFood() {
		// x is random number between 1 and gridWidth - 1
		food.x = ((int)rand() % (gridWidth - 2)) + 1;
		// y is random number between 1 and gridHeight - 1
		food.y = ((int)rand() % (gridHeight - 2)) + 1;
	}

	// function to generate power at random coord
	void generatePower() {
		// same as food
		power.x = ((int)rand() % (gridWidth - 2)) + 1;
		power.y = ((int)rand() % (gridHeight - 2)) + 1;
	}

	// function to produce a new chunk behind the snake when eating
	PieceSegment produceNewSegment(std::vector<PieceSegment> Piece) {
		PieceSegment pieceSegment; // temporary pieceSegment to edit and insert
		char lastDirection = Piece[player.size() - 1].direction; // get last segment direction and make the new segment have the same

		// moving left: put to the right
		if (lastDirection == 'a') {
			pieceSegment.x = Piece[player.size() - 1].x + 1;
			pieceSegment.y = Piece[player.size() - 1].y;
		}
		// moving up: put to the bottom
		else if (lastDirection == 'w') {
			pieceSegment.x = Piece[player.size() - 1].x;
			pieceSegment.y = Piece[player.size() - 1].y + 1;
		}
		// moving right: put to the left
		else if (lastDirection == 'd') {
			pieceSegment.x = Piece[player.size() - 1].x - 1;
			pieceSegment.y = Piece[player.size() - 1].y;
		}
		// moving down: put to the top
		else if (lastDirection == 's') {
			pieceSegment.x = Piece[player.size() - 1].x;
			pieceSegment.y = Piece[player.size() - 1].y - 1;
		}

		// set this new segment's direction
		pieceSegment.direction = lastDirection;

		// return it for insertion
		return pieceSegment;
	}

	// function to handle snake passing over food
	void eat() {
		// check for any piece currently over food
		for (int i = 0; i < player.size(); i++) {
			if (player[i].x == food.x && player[i].y == food.y) {
				// increment the score
				score++;
				
				// generate new food
				generateFood();

				//generate new enemy
				generateEnemy();

				// make the player bigger
				player.push_back(produceNewSegment(player));
			}
		}
	}

	// function to handle snake passing over power: randomly remove an enemy
	void powerUp() {
		// check for any piece currently over power
		for (int i = 0; i < player.size(); i++) {
			if (player[i].x == power.x && player[i].y == power.y && enemies.size() > 0) {
				// get random enemy
				int randEnemy = (int)rand() % enemies.size();
				enemies.erase(enemies.begin() + randEnemy);
				generatePower();
			}
		}
	}

	// function to move multiple-segment body
	void processNextDirection(std::vector<PieceSegment>& piece) {
		int size = piece.size();

		// insert next move first, this acts as a sort of shift register, so directions shift along segment-by-segment and get removed every iteration
		directionBuffer.insert(directionBuffer.begin(), newDirection);

		// if too big, clear end
		if (directionBuffer.size() != size) {
			directionBuffer.erase(directionBuffer.end() - 1);
		}

		// set directions of piece indices to directions in buffer so that their next coords can be worked out elsewhere
		for (int i = 0; i < size; i++) {
			piece[i].direction = directionBuffer[i];
		}
	}

	// function to generate new random enemy
	void generateEnemy() {
		PieceSegment enemy; // temporary enemy pieceSegment
		enemy.x = ((int)rand() % (gridWidth - 4)) + 2; // random x, like food
		enemy.y = ((int)rand() % (gridHeight - 4)) + 2; // random y, like food

		// give room for player to avoid
		if (enemy.x == player[0].x) {
			enemy.x++;
		}
		if (enemy.y == player[0].y) {
			enemy.y++;
		}
		enemies.push_back(enemy); // insert into enemies array
	}

	// function to find minimum distance between enemy and player
	PieceSegment minDistSegment(PieceSegment enemy) {
		int minDist = -1; // tracks current minimum distance found
		int distance; // actual distance from enemy to segment
		PieceSegment minDistSegment; // temporary segment for minimum
		minDistSegment.x = -1; // temporary values for initialisation
		minDistSegment.y = -1; // temporary values for initialisation

		// for all segments in the player, find the cartesian distance to it
		for (int i = 0; i < player.size(); i++) {
			// sqrt(dx^2 + dy^2)
			distance = sqrt((pow((player[i].x - enemy.x), 2) + pow((player[i].y - enemy.y), 2)));

			// set new minimum distance if necesary and copy over coords
			if (distance < minDist || minDist == -1) {
				minDist = distance;
				minDistSegment = player[i];
			}
		}

		// return to use coords to work out which direction to go
		return minDistSegment;
	}

	// function to calculate direction for enemy to travel in next iteration
	void setEnemyDirection(PieceSegment& enemy) {
		// get closest segment of player to the enemy
		PieceSegment closest = minDistSegment(enemy);

		// find the minimum x gap and minimum y gap between enemy and player
		int x_gap = closest.x - enemy.x;
		int y_gap = closest.y - enemy.y;

		// further x_component to the right, move right to reduce this
		if (abs(x_gap) >= abs(y_gap) && x_gap >= 0) {
			enemy.direction = 'd';
		}
		// further x component to the left, move left to reduce this
		else if (abs(x_gap) >= abs(y_gap) && x_gap <= 0) {
			enemy.direction = 'a';
		}
		// further y component to the bottom, move down to reduce this
		else if (abs(y_gap) >= abs(x_gap) && y_gap >= 0) {
			enemy.direction = 's';
		}
		// further y component to the top, move down to reduce this
		else if (abs(y_gap) >= abs(x_gap) && y_gap <= 0) {
			enemy.direction = 'w';
		}
	}

	// function to check if any player segment is being eaten and if so change all after to enemies
	void checkEnemy() {
		int enemySize = enemies.size();

		// compare all player segments and enemy coordinates
		for (int i = 1; i < player.size(); i++) {
			for (int j = 0; j < enemySize; j++) {
				if (player[i].x == enemies[j].x && player[i].y == enemies[j].y) {
					// for all the remaining segments, remove from player and 
					for (int k = player.size() - 1; k >= j; k--) {
						enemies.push_back(player[k]);
						player.erase(player.begin() + k);
					}
					j = enemySize;
					i = player.size();
				}
			}
		}
	}

public:
	// constructor
	Grid() {
		// initialise variables
		gridHeight = 20;
		gridWidth = 30;
		score = 0;
		frames = 0;

		//piece starting coords for player in top left, moving right
		PieceSegment tempPlayer;
		tempPlayer.x = 1;
		tempPlayer.y = 1;
		tempPlayer.direction = 'd';
		player.push_back(tempPlayer);

		// generate first enemy
		generateEnemy();

		// generate first food
		generateFood();

		// generate first power
		generatePower();
	}

	// function for thread to set player's direction
	void setNewDirection(char c) {
		newDirection = c;
	}

	// function for all processing at beginning of iteration
	void startIteration() {
		// set start coord to print on terminal
		setCursorPosition(0, 0);

		// print the grid
		printGrid();
	}

	// function for all processing at end of iteration
	void endIteration() {
		// check if we're being eaten
		checkEnemy();

		// process next direction of whole snake
		processNextDirection(player);

		// make new enemy every 5 frames
		if (frames % 50 == 0) {
			generateEnemy();
		}

		// calculate all player segment next coords individually
		for (int i = 0; i < player.size(); i++) {
			calculateNextCoordinate(&(player[i]));
		}

		// move enemies every 2 frames rather than every frame because of difficulty
		if (frames % 2 == 0) {
			for (int i = 0; i < enemies.size(); i++) {
				setEnemyDirection(enemies[i]);
				calculateNextCoordinate(&(enemies[i]));
			}
		}

		// check if we're eating
		eat();

		// check if we're powering
		powerUp();

		// increment frame counter
		frames++;

		// clear buffer
		std::cout.flush();
	}

	// function to check if square moved into is invalid, so game over
	bool coordIsValid() {
		// are we in a wall?
		if (player[0].x == 0 || player[0].x == gridWidth - 1 || player[0].y == 0 || player[0].y == gridHeight - 1) {
			return false;
		}

		// are we eating ourselves?
		for (int i = 1; i < player.size(); i++) {
			if (player[0].x == player[i].x && player[0].y == player[i].y) {
				return false;
			}
		}

		// has an enemy eaten us?
		for (int j = 0; j < enemies.size(); j++) {
			if (player[0].x == enemies[j].x && player[0].y == enemies[j].y) {
				return false;
			}
		}

		// should be good
		return true;
	}

	// function to get the score to print at the end
	int getScore() {
		return score;
	}

};

// function for getting input in thread
void getMove(Grid * grid, bool* playing) {
	// continously get new character
	while (*playing) {
		grid->setNewDirection(_getch());
	}
}

int main() {
	// set seed of random number generator
	srand((int)time(0));
	char c = 'y';
	bool playing;

	while (c == 'y') {
		playing = true;

		// initialise the grid
		Grid grid;

		// open up a thread to get button presses to move player
		std::thread th1(getMove, &grid, &playing);

		// while we're not dead
		while (grid.coordIsValid()) {
			// handle processing at start of iteration
			grid.startIteration();

			// timer stuff to process at regular intervals
			std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
			std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();
			std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

			while (diff.count() <= 100) {
				end = std::chrono::steady_clock::now();
				diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
			}

			start = end;

			// handle processing at end of iteration
			grid.endIteration();
		}

		// game over
		std::cout << "GAME OVER!" << std::endl;
		std::cout << "SCORE : " << grid.getScore() << std::endl;
		std::cout << "Press any key to continue..." << std::endl;

		// stop playing
		playing = false;
		th1.join();

		// play again?
		std::cout << "Play again? (y/n)" << std::endl;
		std::cin >> c;
	}

	return 0;
}