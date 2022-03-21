#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>

const int WIDTH = 1100, HEIGHT = 800;

sf::Color defaultCellColor(100, 100, 100);
sf::Font defaultFont;

sf::Color contentColor(int x) {
	if (x == 2) return sf::Color(221, 160, 220);
	if (x == 4) return sf::Color(107, 90, 205);
	if (x == 8) return sf::Color(30, 144, 255);
	if (x == 16) return sf::Color(0, 206, 209);
	if (x == 32) return sf::Color(63, 177, 117);
	if (x == 64) return sf::Color(49, 205, 51);
	if (x == 128) return sf::Color(255, 141, 3);
	if (x == 256) return sf::Color(252, 127, 116);
	if (x == 512) return sf::Color(254, 68, 3);
	if (x == 1024) return sf::Color(255, 20, 147);
	if (x == 2048) return sf::Color(203, 21, 136);
	return sf::Color::Red;
}

class Cell {
private:
	sf::RectangleShape shape;
	sf::Text num;
	int content = 0;
	bool filled = false;
public:
	Cell() {}
	Cell(sf::Vector2f position, sf::Vector2f size) {
		shape.setFillColor(defaultCellColor);
		shape.setPosition(position + sf::Vector2f(10, 10));
		shape.setSize(size - sf::Vector2f(20, 20));
		num.setFont(defaultFont);
	}
	void setText(std::string txt) {
		num.setString(txt);
		num.setStyle(sf::Text::Bold);
		num.setCharacterSize(70 - 4 * txt.length());
		num.setOrigin(num.getLocalBounds().width / 2, num.getLocalBounds().height / 2 + 20);
		num.setPosition(shape.getPosition() + shape.getSize() / 2.f);
	}
	void fill(int content){
		shape.setFillColor(contentColor(content));
		setText(std::to_string(content));
		setContent(content);
	}
	void setContent(int content) {
		this->content = content;
		filled = true;
	}
	void clear() {
		shape.setFillColor(defaultCellColor);
		num.setString("");
		filled = false;
	}
	bool empty() {
		return !filled;
	}
	sf::Vector2f getPosition() {
		return shape.getPosition();
	}
	int getContent() {
		return content;
	}
	sf::RectangleShape getShape() {
		return shape;
	}
	sf::Text getNum() {
		return num;
	}
	void update(sf::RenderWindow& win) {
		win.draw(shape);
		win.draw(num);
	}
};

class Transit {
private:
	sf::RectangleShape shape;
	sf::Text num;
	Cell* fromCell = NULL, *toCell = NULL;
	sf::Vector2f from, to, delta, numDelta;
	float progress = 0;
	int result = 0;
public:
	Transit() {}
	Transit(Cell& from, Cell& to, int result) {
		fromCell = &from;
		toCell = &to;
		this->result = result;
		this->from = from.getPosition();
		this->to = to.getPosition();
		delta = this->to - this->from;
		shape = sf::RectangleShape(from.getShape());
		num = sf::Text(from.getNum());
		numDelta = num.getPosition() - shape.getPosition();
	}
	bool animationStep() {
		progress += 0.2f;
		shape.setPosition(from + delta * progress);
		num.setPosition(shape.getPosition() + numDelta);
		if (progress >= 1.f) {
			toCell->fill(result);
			return 1;
		}
		return 0;
	}
	void update(sf::RenderWindow& win) {
		win.draw(shape);
		win.draw(num);
	}
};

class Field {
private:
	sf::Vector2f cellSize, position;
	sf::Text* scoreText, *theBestText;
	std::vector<std::vector<Cell>> matrix;
	std::vector<std::vector<bool>> usedByStep;
	std::vector<Transit> transitions;
	std::ofstream fout;
	int columnsCount, rowsCount;
	int score = 0, bestScore = 0;
	sf::Vector2f toGlobalPosition(int x, int y) {
		return position + sf::Vector2f(x * cellSize.x, y * cellSize.y);
	}
public:
	bool isAnimationEnding = false;
	Field(int x, int y, int width, int height, int columns, int rows, sf::Text& scoreText, sf::Text& theBestText) {
		columnsCount = columns;
		rowsCount = rows;
		cellSize = sf::Vector2f((float)width / columns, (float)height / rows);
		position = sf::Vector2f(x, y);

		this->scoreText = &scoreText;
		this->theBestText = &theBestText;
		
		std::ifstream fin("record.rec");
		fin >> bestScore;
		fin.close();
		
		theBestText.setString(std::to_string(bestScore));
		
		matrix.resize(rows, std::vector<Cell>(columns));
		usedByStep.resize(rows, std::vector<bool>(columns));
		for (int i = 0; i < rows; i++)
			for (int j = 0; j < columns; j++)
				matrix[i][j] = Cell(toGlobalPosition(j, i), cellSize);
	}
	bool isAnimationPlaying() {
		return !transitions.empty();
	}
	void addScore(int add) {
		score += add;
		scoreText->setString(std::to_string(score));
		if (score > bestScore) {
			bestScore = score;
			
			fout.open("record.rec");
			fout << bestScore;
			fout.close();

			theBestText->setString(std::to_string(bestScore));
		}
	}
	void setBlock(int x, int y, int content) { // set block in the current cell (it's always possible)
		matrix[y][x].fill(content);
	}
	void clearBlock(int x, int y) { // Clear cell content
		matrix[y][x].clear();
	}
	void addBlock() { // add block in the random free cell on field if it possible
		std::vector<sf::Vector2i> freeCells;
		for (int i = 0; i < rowsCount; i++)
			for (int j = 0; j < columnsCount; j++)
				if (matrix[i][j].empty())
					freeCells.push_back(sf::Vector2i(j, i));
		if (freeCells.empty()) return;
		
		int it = rand() % freeCells.size();
		if (rand() % 4 == 0)
			setBlock(freeCells[it].x, freeCells[it].y, 4);
		else
			setBlock(freeCells[it].x, freeCells[it].y, 2);
	}
	void addTransit(int x1, int y1, int x2, int y2, int result) { // add cell transit animations
		transitions.push_back(Transit(matrix[y1][x1], matrix[y2][x2], result));
		clearBlock(x1, y1);
		matrix[y2][x2].setContent(result);
	}
	bool move(int dirX, int dirY) {
		bool moved = false;
		
		// move blocks in direction if it possible
		int startY = 0, endY = rowsCount - 1;
		int startX = 0, endX = columnsCount - 1;
		if (dirX > 0) std::swap(startX, endX);
		if (dirY > 0) std::swap(startY, endY);
		int dx = -dirX, dy = -dirY;
		
		if (!dx) dx = 1;
		if (!dy) dy = 1;
		for (int i = startY; i * dy <= endY; i += dy) {
			for (int j = startX; j * dx <= endX; j += dx) {
				if (matrix[i][j].empty()) continue;
				int x = j + dirX, y = i + dirY;
				while (0 <= x && x < columnsCount
					&& 0 <= y && y < rowsCount
					&& matrix[y][x].empty()) {
					x += dirX, y += dirY;
				}
				if (0 <= x && x < columnsCount &&
					0 <= y && y < rowsCount &&
					!usedByStep[y][x] &&
					matrix[y][x].getContent() == matrix[i][j].getContent()) {
					addTransit(j, i, x, y, matrix[i][j].getContent() * 2);
					if(matrix[i][j].getContent() > 2)
						addScore(matrix[i][j].getContent() * 2);
					usedByStep[y][x] = true;
					moved = true;
				}
				else if(i != y - dirY || j != x - dirX){
					y -= dirY, x -= dirX;
					addTransit(j, i, x, y, matrix[i][j].getContent());
					moved = true;
				}
			}
		}
		for (int i = 0; i < rowsCount; i++)
			for (int j = 0; j < columnsCount; j++)
				usedByStep[i][j] = false;
		return moved;
	}
	void update(sf::RenderWindow& win) {
		for (int i = 0; i < rowsCount; i++)
			for (int j = 0; j < columnsCount; j++)
				matrix[i][j].update(win);
		
		auto transit = transitions.begin();
		while (transit != transitions.end()) {
			transit->update(win);
			if (transit->animationStep()) {// if transition is ended
				transit = transitions.erase(transit);
			}
			else {
				++transit;
			}
		}
	}
};

int main() {
	srand(time(0));

	sf::ContextSettings settings;
	settings.antialiasingLevel = 4;

	sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "2048", sf::Style::Close, settings);
	window.setFramerateLimit(30);

	defaultFont.loadFromFile("Whipsmart.ttf");
	sf::Text scoreLabel("Score: ", defaultFont, 60);
	sf::Text scoreText("0", defaultFont, 50);

	sf::Text theBestLabel("Record: ", defaultFont, 60);
	sf::Text theBestText("0", defaultFont, 50);

	scoreLabel.setPosition(800, 50);
	scoreText.setPosition(845, 120);
	theBestLabel.setPosition(800, 350);
	theBestText.setPosition(845, 420);

	Field field(50, 50, 700, 700, 4, 4, scoreText, theBestText);

	field.setBlock(0, 0, 2);

	bool left = false, right = false, up = false, down = false;

	while (window.isOpen()) {
		sf::Event event;
		while (window.pollEvent(event)) {
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				if (event.key.code == sf::Keyboard::Left)
					left = true;
				else if (event.key.code == sf::Keyboard::Right)
					right = true;
				else if (event.key.code == sf::Keyboard::Up)
					up = true;
				else if (event.key.code == sf::Keyboard::Down)
					down = true;
			}
		}
		window.clear(sf::Color::Black);

		if (!field.isAnimationPlaying()) {
			if (field.isAnimationEnding) {
				field.isAnimationEnding = false;
				field.addBlock();
			}
			if (left) {
				field.move(-1, 0);
				left = false;
			}
			else if (right) {
				field.move(1, 0);
				right = false;
			}
			else if (up) {
				field.move(0, -1);
				up = false;
			}
			else if (down) {
				field.move(0, 1);
				down = false;
			}
		}
		else {
			field.isAnimationEnding = true;
		}

		field.update(window);
		window.draw(scoreLabel);
		window.draw(scoreText);
		window.draw(theBestLabel);
		window.draw(theBestText);

		window.display();
	}
	return 0;
}
