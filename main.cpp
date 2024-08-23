#include <iostream>
#include <SFML/Graphics.hpp>
#include <random>
#include <vector>

using namespace std;
using namespace sf;

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 1000

#define FALL_SPEED 10

const Vector2f BIRD_START_POS(SCREEN_WIDTH/5, 0.0f);
int score;


#define PIPE_SPEED 5

RenderWindow window;

bool isClicked;


const float TIME_BETWEEN_PIPES = 2;

const Vector2f DEFAULT_PIPE_POS(750, 0);
const float DEFAULT_PIPE_DISTANCE = 600;



float getRandom(int num1, int num2) {
	float min = static_cast<float>(num1);
	float max = static_cast<float>(num2);
	static random_device randomDevice;
	static mt19937 generator(randomDevice());
	uniform_real_distribution<float> distribution(min, max);

	return (distribution(generator));
}// Method takes two numbers and gets random int between them


class Bird {
private:
	Vector2f position;
	Texture tex_bird;
	Sprite spr_bird;
	

public:

	explicit Bird() {
		tex_bird.loadFromFile("bird.png");
		spr_bird.setTexture(tex_bird);
		spr_bird.setScale(.1f, .1f);
		position = BIRD_START_POS;
		spr_bird.setPosition(position);
	}

	void update() {
		spr_bird.setPosition(position);
	}

	void fly() {
		if (position.y>0){
			position.y -= 15;
	}}
	void fall() {
		position.y += 6;
	}

	void draw(RenderWindow& window) {
		window.draw(spr_bird);
	}
	FloatRect getRect() {
		auto size = tex_bird.getSize();
		return {
			position.x, position.y,
			(float)size.x, (float)size.y
		};
	}

	float getYPos() {
		return spr_bird.getPosition().y;
	}

	Sprite getBird() { return spr_bird; }

	

};//Bird Class



class Pipe
{

private:
	Vector2f topPipePos;
	Texture tex_topPipe;
	Sprite spr_topPipe;


	Vector2f bottomPipePos;
	Texture tex_bottomPipe;
	Sprite spr_bottomPipe;

	bool isActive;
	bool isPassed = false;

public:
	Pipe() {
		tex_topPipe.loadFromFile("pipe.png");
		spr_topPipe.setTexture(tex_topPipe);
		spr_topPipe.setScale(.8, .8);
		spr_topPipe.setOrigin(0, 425);
		
		

	
		tex_bottomPipe.loadFromFile("bottomPipe.png");
		spr_bottomPipe.setTexture(tex_bottomPipe);
		spr_bottomPipe.setScale(.8, .8);

		isActive = false;
		
	}
	~Pipe(){}

	void setPosition(){
		float randomNum = getRandom(-9, 9);
		topPipePos.y = DEFAULT_PIPE_POS.y + (randomNum * 20);
		topPipePos.x = DEFAULT_PIPE_POS.x;

		bottomPipePos.x = topPipePos.x;
		bottomPipePos.y = topPipePos.y + DEFAULT_PIPE_DISTANCE;

		isPassed = false;

	}

	void update() {
		spr_topPipe.setPosition(topPipePos);
		spr_bottomPipe.setPosition(bottomPipePos);
	}
	void checkIfPassed(){
		if (BIRD_START_POS.x>spr_topPipe.getPosition().x){isPassed = true;}
	}

	bool getIfPassed() {
		return isPassed;
	}

	void draw(RenderWindow& window) {

		window.draw(spr_topPipe);
		window.draw(spr_bottomPipe); 
		isActive = true;
		
	}

	void move() {
		topPipePos.x -= PIPE_SPEED;
		bottomPipePos.x -= PIPE_SPEED;
	}


	float getPos() {
		return spr_topPipe.getPosition().x;
	}



	FloatRect getUpperRect() const {
		auto size = tex_topPipe.getSize();
		return {
				topPipePos.x, topPipePos.y,
				0, (float)size.y
		};
	}

	FloatRect getLowerRect() const {
		auto size = tex_topPipe.getSize();
		return {
			 	bottomPipePos.x, bottomPipePos.y,
				(float)size.x, (float)size.y
		};
	}
	bool checkIfActive() { if (isActive == true) { return true; } else { return false; } }
	void setIfActive(bool active) { isActive = active; }
	Sprite getUpper() { return spr_topPipe; }
	Sprite getLower() { return spr_bottomPipe; }
	
};


bool checkColision(Bird bird, Pipe pipe) {
	bool collision = false;


	//get bird bounds
	Sprite spr_bird = bird.getBird();
	Rect<float> birdRect = spr_bird.getGlobalBounds();


	//get Pipe bounds
	Sprite spr_topPipe = pipe.getUpper();
	Rect<float> topRect = spr_topPipe.getGlobalBounds();

	Sprite spr_bottomPipe = pipe.getLower();
	Rect<float>bottomRect = spr_bottomPipe.getGlobalBounds();

	//check if bird intrsects with either pipe or falls off screen
	if (pipe.checkIfActive()){
		collision = birdRect.intersects(topRect) || birdRect.intersects(bottomRect) || bird.getYPos()>SCREEN_HEIGHT;
	}

	
	if (collision) { cout << "Collision"<< endl; }

	return collision;
}


bool alive = true;
vector<Pipe> pipes;
Clock pipeGeneratingClock;


void flappyBirds() {
	RenderWindow window{ VideoMode{SCREEN_WIDTH, SCREEN_HEIGHT}, "Flappy Birds" };
	window.setFramerateLimit(60);

	score = 0;

	// Bird init
	Bird mainBird = Bird();

	// Texture Set-up
	Texture tex_background, tex_bird;
	tex_background.loadFromFile("bg.png");

	//Sprite setup
	Sprite spr_background(tex_background);
	spr_background.scale(2, 2);

	isClicked = false;

	sf::RectangleShape lowerRectangle({ (float)window.getSize().x,(float)window.getSize().y });

	lowerRectangle.setPosition(0, (float)tex_background.getSize().y);
	lowerRectangle.setFillColor({ 245, 228, 138 });
	window.draw(lowerRectangle);

	Pipe pipe;
	for (int i = 0; i < 6; i++) {
		pipe = Pipe();
		pipe.setPosition();
		pipes.push_back(pipe);

	}


	while (window.isOpen()) {
		while (alive) {


			Event newEvent;

			while (window.pollEvent(newEvent)) {
				if (newEvent.type == sf::Event::Closed)
				{
					window.close();
				}
			}

			if (newEvent.type == sf::Event::MouseButtonPressed) {
				mainBird.fly();
			}

			window.clear();




			// Drawing sprites to screen
			window.draw(spr_background);
			mainBird.fall();
			mainBird.update();
			mainBird.draw(window);



			for (auto& pipe : pipes) {

				if (checkColision(mainBird, pipe) && pipe.checkIfActive())
				{
					for (auto& pipe : pipes) {
						if (pipe.getIfPassed()) { score++; }
					}
					cout << score << endl;
					alive = false;
					window.close();

				}

				pipe.checkIfPassed();//checking is bird has passed the pipe, if so bool isPassed is set to true.

			}
			if (!alive) {
				for (auto& pipe : pipes) {


				}
			}
			


			if (pipeGeneratingClock.getElapsedTime().asSeconds() > .1 && alive) {
				pipes[0].move();
				pipes[0].update();
				pipes[0].draw(window);

				if ((pipeGeneratingClock.getElapsedTime().asSeconds() > TIME_BETWEEN_PIPES * 1)) {
					pipes[1].move();
					pipes[1].update();
					pipes[1].draw(window);

					if ((pipeGeneratingClock.getElapsedTime().asSeconds() > TIME_BETWEEN_PIPES * 2)) {
						pipes[2].move();
						pipes[2].update();
						pipes[2].draw(window);

						if (pipeGeneratingClock.getElapsedTime().asSeconds() > TIME_BETWEEN_PIPES * 2.7) {
							pipes[3].move();
							pipes[3].update();
							pipes[3].draw(window);

							if (pipeGeneratingClock.getElapsedTime().asSeconds() > TIME_BETWEEN_PIPES * 3.5) {
								pipes[4].move();
								pipes[4].update();
								pipes[4].draw(window);

								if (pipeGeneratingClock.getElapsedTime().asSeconds() > TIME_BETWEEN_PIPES * 4.2) {
									pipes[5].move();
									pipes[5].update();
									pipes[5].draw(window);

									if (pipeGeneratingClock.getElapsedTime().asSeconds() > TIME_BETWEEN_PIPES * 5.3) {
										for (auto& pipe : pipes) {
											if (pipe.getPos() <= -600)
											{
												if (pipe.getIfPassed()) { score++; }
												pipe.setPosition();
												pipe.update();
											}
										}
										pipeGeneratingClock.restart();

									}
								}
							}
						}
					}

				}

			}
			else { alive = false; }

			window.display();
		}// alive loop
		Event startEvent;
		while (window.pollEvent(startEvent)) {
			if (startEvent.type == sf::Event::Closed)
			{
				window.clear();
				
				window.close();
			}
		}


		if (startEvent.type == sf::Event::MouseButtonPressed) {
			alive = true;
			for (auto& pipe : pipes) {
				pipe.setIfActive(false);

			}
			

			window.clear();
		}

		

	}//Main loop


}

int main() {
	flappyBirds();
	return 0;
}//Main
