#include <SFML/Graphics.hpp> 
#include <SFML/Audio.hpp>// Include SFML for rendering graphics
#include <iostream>
#include <ctime>              // For random number generation using srand, rand
using namespace std;

// Define the size of the Tetris game board: 18 rows and 12 columns
const int BOARD_ROWS = 18;
const int BOARD_COLS = 12;

// These rendering values will be initialized in the Board class constructor
int TILE_SIZE;                // Size of each square tile in pixels
float offsetX;                // Horizontal offset (used to position the board on screen)
float offsetY;                // Vertical offset (used to position the board on screen)

// The 2D array that represents the game grid (0 = empty, any other value = colored tile)
int board[BOARD_ROWS][BOARD_COLS] = { 0 };

// Structure to represent the (x, y) coordinates of a tile/block in space
struct Point {
    int x, y;
};

// Abstract base class that represents a generic Tetris piece
class Piece {
protected:
    Point current[4], backup[4]; // current = actual blocks; backup = used to revert if move is invalid
    int colorIndex;              // Determines which color the piece uses when rendered

public:
    // Initialize the piece with a shape type (e.g., I, T, L, etc.)
    virtual void initialize(int type) = 0;

    // Rotate the piece 90 degrees (to be defined in subclasses)
    virtual void rotate() = 0;

    // Checks if the current piece is in a valid position
    // Valid = inside bounds and not colliding with existing blocks
    bool check()
    {
        for (int i = 0; i < 4; ++i)
        {
            if (current[i].x < 0 || current[i].x >= BOARD_COLS ||
                current[i].y >= BOARD_ROWS || board[current[i].y][current[i].x])
                return false; // Out of bounds or overlapping
        }
        return true; // Valid position
    }
    // Tries to move the piece down by one cell
    // If move is successful, returns true (still falling)
    // If blocked, locks the piece in place and returns false
    bool tryMoveDownAndLock() {
        for (int i = 0; i < 4; ++i)
        {
            backup[i] = current[i];   // Save current position
            current[i].y += 1;        // Move each block down
        }

        if (!check()) // If new position is invalid
        {
            // Lock the 
            // piece at its previous position into the board
            for (int i = 0; i < 4; ++i)
            {
                if (backup[i].y >= 0) // Only lock if within visible board
                    board[backup[i].y][backup[i].x] = colorIndex;
            }
            return false; // Piece is locked
        }

        return true; // Still falling
    }


    // Moves the piece horizontally (left or right)
    // dx = -1 for left, dx = 1 for right
    void move(int dx)
    {
        for (int i = 0; i < 4; ++i)
        {
            backup[i] = current[i];   // Save original position
            current[i].x += dx;       // Apply move
        }

        if (!check()) // Invalid move
        {
            for (int i = 0; i < 4; ++i)
                current[i] = backup[i]; // Undo the move
        }
    }

    // Draws the current piece on the window using the given sprite
    void draw(sf::RenderWindow& window, sf::Sprite& sprite)
    {
        for (int i = 0; i < 4; ++i)
        {
            // Set the texture to the appropriate color tile
            sprite.setTextureRect(sf::IntRect(colorIndex * 18, 0, 18, 18));

            // Calculate and set the screen position for each tile
            sprite.setPosition(
                current[i].x * TILE_SIZE + offsetX,
                current[i].y * TILE_SIZE + offsetY
            );

            // Render the tile
            window.draw(sprite);
        }
    }

    // Getter for the 4 blocks of the current piece (used in previews)
    const Point* getBlocks() const {
        return current;
    }

    // Getter for the color index of the piece
    int getColor() const {
        return colorIndex;
    }
};


// Tetrimino class is a concrete implementation of a Tetris piece
class Tetrimino : public Piece {
public:
    // Constructor: initializes the piece with a random shape and color
    Tetrimino() {
        colorIndex = 1 + rand() % 7; // Random color from 1 to 7
        initialize(rand() % 7);      // Random shape (0 to 6)
    }

    // Initialize the shape of the Tetrimino based on the given type (0-6)
    void initialize(int type) override {
        // Predefined shape data for each of the 7 possible Tetriminos
        static const int shapes[7][4] = {
            {1, 3, 5, 7}, // I shape
            {2, 4, 5, 7}, // Z shape
            {3, 5, 4, 6}, // S shape
            {3, 5, 4, 7}, // T shape
            {2, 3, 5, 7}, // L shape
            {3, 5, 7, 6}, // J shape
            {2, 3, 4, 5}  // O shape
        };

        // Convert shape data into x, y coordinates
        for (int i = 0; i < 4; ++i) {
            current[i].x = shapes[type][i] % 2;  // x-position based on column
            current[i].y = shapes[type][i] / 2;  // y-position based on row
        }

        // Align piece to the left wall (adjust x-position)
        int minX = current[0].x;
        for (int i = 1; i < 4; ++i) {
            minX = min(minX, current[i].x); // Find the minimum x-coordinate
        }
        for (int i = 0; i < 4; ++i) {
            current[i].x -= minX; // Shift the piece to the left
        }
    }

    // Rotates the piece 90 degrees around the second block (current[1])
    void rotate() override {
        // Backup the current positions in case the rotation is invalid
        for (int i = 0; i < 4; ++i) {
            backup[i] = current[i];
        }

        // Set the second block as the pivot point
        Point pivot = current[1];

        // Rotate each block 90 degrees around the pivot
        for (int i = 0; i < 4; ++i) {
            int xOffset = current[i].y - pivot.y;
            int yOffset = current[i].x - pivot.x;

            // Apply 90-degree rotation transformation
            current[i].x = pivot.x - xOffset;
            current[i].y = pivot.y + yOffset;
        }

        // If rotation results in an invalid position, revert to the backup
        if (!check()) {
            for (int i = 0; i < 4; ++i)
                current[i] = backup[i];
        }
    }
};


class Board {
private:
    sf::RenderWindow window;           // The window where the game will be rendered
    sf::Texture tileTexture, bgTexture; // Textures for the tiles and background
    sf::Sprite tileSprite, backgroundSprite; // Sprites to display tiles and background
    Tetrimino currentPiece;             // The current falling Tetrimino piece
    float timer = 0, delay = 0.3f;     // Timer to control the speed of falling pieces
    int dx = 0;                         // Horizontal movement (left or right)
    bool rotateFlag = false;            // Flag to control piece rotation

    sf::Font font;                      // Font for displaying text
    sf::Text gameOverText;              // Text displayed when the game is over

    int score = 0;                      // Player's score
    Tetrimino nextPiece;                // The next Tetrimino piece for preview
    sf::RectangleShape sideBar;         // Sidebar shape to hold next piece and score
    sf::Text scoreText;                 // Text displaying the score
    sf::Text nextLabel;                 // Text displaying the "Next" label for the upcoming piece
    bool isPaused = false;              // Flag to check if the game is paused
    sf::Text pauseText;                 // Text displayed when the game is paused
    sf::Text pauseScoreText;            // Text displaying the score during the pause menu
    sf::Text pauseExitText;             // Text for exit option in the pause menu

    string playerName = "Player"; // Default player name
    sf::Text menuText;
    sf::Text playerNameText;
    bool gameStarted = false;
    sf::Text nameInputPrompt;      // "Enter your name:" label
    sf::Text nameInputField;       // Live typed name
    std::string nameInputBuffer;   // Input storage
    bool enteringName = false;    // Entry mode test
    sf::Text pauseLabel;
    bool gameOver = false;


public:
    // Constructor: Initializes the board, window, textures, fonts, and initial setup
    Board()
        : window(sf::VideoMode(900, 1000), "Tetris Game") // Create the game window
    {
        srand((unsigned)time(nullptr));  // Seed random number generator for piece randomness

        // Load textures for tiles and background
        if (!tileTexture.loadFromFile("tiles.png") || !bgTexture.loadFromFile("background (3).png"))
        {
            throw std::runtime_error("Failed to load textures");
        }

        // Load font for text rendering
        if (!font.loadFromFile("OpenSans-ExtraBold.ttf"))
        {
            throw std::runtime_error("Failed to load font");
        }

        // Initialize tile and background sprites
        tileSprite = sf::Sprite(tileTexture);
        backgroundSprite = sf::Sprite(bgTexture);

        // Scale background to match window size
        auto winSize = window.getSize(); //auto keyword will automatically deduce type
        backgroundSprite.setScale(
            float(winSize.x) / bgTexture.getSize().x,
            float(winSize.y) / bgTexture.getSize().y
        );

        // Set borders and calculate inner width and height for the play area
        const int leftBorder = 80;
        const int rightBorder = 2;
        const int topBorder = 50;
        const int bottomBorder = 190;
        int innerWidth = winSize.x - leftBorder - rightBorder;
        int innerHeight = winSize.y - topBorder - bottomBorder;

        // Set the tile size based on the available space
        TILE_SIZE = std::min(innerWidth / BOARD_COLS, innerHeight / BOARD_ROWS);
        offsetX = float(leftBorder);  // Horizontal offset to align the board
        offsetY = float(topBorder);   // Vertical offset to align the board

        // Scale the tile sprite to match the tile size
        float scaleFactor = TILE_SIZE / 18.f;
        tileSprite.setScale(scaleFactor, scaleFactor);

        // Set up the sidebar for the score and next piece display
        float panelX = offsetX + BOARD_COLS * TILE_SIZE;
        float panelWidth = winSize.x - panelX;
        sideBar.setPosition(panelX + 65, 0);
        sideBar.setSize({ panelWidth, float(winSize.y) });
        sideBar.setFillColor(sf::Color(200, 220, 225, 200));  // Set sidebar color

        //Player name display
        playerNameText.setFont(font);
        playerNameText.setString("Player: " + playerName);  // Display the player's name
        playerNameText.setCharacterSize(30);  // Set font size
        playerNameText.setFillColor(sf::Color::Black);  // Set text color
        playerNameText.setPosition(panelX + 70, offsetY + 20);
        // Set up the score text
        scoreText.setFont(font);
        scoreText.setCharacterSize(30);
        scoreText.setFillColor(sf::Color::Black);
        scoreText.setPosition(panelX + 70, offsetY + 60);
        scoreText.setString("Score: 0");

        // Set up the "Next" label text
        nextLabel.setFont(font);
        nextLabel.setCharacterSize(40);
        nextLabel.setFillColor(sf::Color::Black);
        nextLabel.setString("Next:");
        nextLabel.setPosition(panelX + 70, offsetY + 150);


        pauseLabel.setFont(font);
        pauseLabel.setCharacterSize(25);
        pauseLabel.setFillColor(sf::Color::Black);
        pauseLabel.setString("Press P To Pause");
        pauseLabel.setPosition(panelX + 70, offsetY + 400);

        // Generate the next piece for the preview
        nextPiece = Tetrimino();

        // Set up the "Game Over" text
        gameOverText.setFont(font);
        gameOverText.setString("Game Over\nScore: " + to_string(score));
        gameOverText.setCharacterSize(60);
        gameOverText.setFillColor(sf::Color::Red);
        auto gameOverBounds = gameOverText.getLocalBounds();
        gameOverText.setPosition(
            winSize.x / 2.f - gameOverBounds.width / 2.f,
            winSize.y / 2.f - gameOverBounds.height / 2.f
        );

        // ===== Pause Menu Texts =====
        // Pause message
        pauseText.setFont(font);
        pauseText.setString("Game Paused (Press P to resume)");
        pauseText.setCharacterSize(50);
        pauseText.setFillColor(sf::Color(255, 255, 150));
        auto pauseBounds = pauseText.getLocalBounds();
        pauseText.setPosition(
            winSize.x / 2.f - pauseBounds.width / 2.f,
            winSize.y / 2.f - pauseBounds.height - 40
        );

        // Score during pause
        pauseScoreText.setFont(font);
        pauseScoreText.setCharacterSize(50);
        pauseScoreText.setFillColor(sf::Color(200, 255, 255));
        pauseScoreText.setString("Score: 0");
        auto pauseScoreBounds = pauseScoreText.getLocalBounds();
        pauseScoreText.setPosition(
            winSize.x / 2.f - pauseScoreBounds.width / 2.f,
            pauseText.getPosition().y + 70
        );

        // Exit option during pause
        pauseExitText.setFont(font);
        pauseExitText.setCharacterSize(50);
        pauseExitText.setFillColor(sf::Color(255, 100, 100));
        pauseExitText.setString("Exit (Press E to Exit)");
        auto exitBounds = pauseExitText.getLocalBounds();
        pauseExitText.setPosition(
            winSize.x / 2.f - exitBounds.width / 2.f,
            pauseScoreText.getPosition().y + 50
        );

        menuText.setFont(font);
        menuText.setString("Press S to Start\nPress E to Exit");
        menuText.setCharacterSize(40);
        menuText.setFillColor(sf::Color::White);
        menuText.setPosition(winSize.x / 2.f - 100, winSize.y / 2.f - 40);

    }

    // Checks if the game is over (i.e., if the top row is filled with blocks)
    bool checkGameOver() {
        for (int j = 0; j < BOARD_COLS; ++j)
        {
            if (board[1][j] != 0)
                gameOver = true; // If there's any block in the top row
            return gameOver;        // Game is over
        }
        return gameOver;  // Game is not over
    }

    // Function to show the menu and get the player name
    void showMenu() {
        while (window.isOpen())
        {
            sf::Event e;
            while (window.pollEvent(e))
            {
                if (e.type == sf::Event::Closed || e.type == sf::Event::KeyPressed)
                {
                    if (e.type == sf::Event::KeyPressed)
                    {
                        if (e.key.code == sf::Keyboard::S)
                        {
                            // Ask for player name
                            askPlayerName();
                            gameStarted = true;
                            return; // Exit menu and start game
                        }
                        else if (e.key.code == sf::Keyboard::E)
                        {
                            window.close(); // Exit the game
                        }
                    }
                }
            }

            window.clear();
            window.draw(backgroundSprite);
            window.draw(menuText); // Draw menu text
            window.display();
        }
    }

    // Function to ask for player name
        // You can implement a way to get player input through a text box
        // For simplicity, we will just prompt the user to enter their name in the console
    void askPlayerName() {
        enteringName = true;
        nameInputBuffer.clear();

        nameInputPrompt.setFont(font);
        nameInputPrompt.setCharacterSize(40);
        nameInputPrompt.setFillColor(sf::Color::White);
        nameInputPrompt.setString("Enter your name:");
        nameInputPrompt.setPosition(window.getSize().x / 2.f - 180, window.getSize().y / 2.f - 100);

        nameInputField.setFont(font);
        nameInputField.setCharacterSize(40);
        nameInputField.setFillColor(sf::Color::Yellow);
        nameInputField.setString("");  // Start empty
        nameInputField.setPosition(window.getSize().x / 2.f - 180, window.getSize().y / 2.f - 40);
    }

    void run()
    {
    playAgain:
        gameOver = false;
        showMenu(); // Show the main menu before starting the game
        sf::Clock clock; // Clock to measure frame time

        while (window.isOpen())
        {
            // ========== Handle Game Over ==========
            if (checkGameOver())
            {

                gameOverText.setString("Game Over\n\nScore: " + to_string(score));
                window.clear();
                window.draw(gameOverText);
                window.draw(pauseExitText);
                window.display();

                // Wait until player presses a key or closes the window
                while (window.isOpen())
                {
                    sf::Event e;
                    while (window.pollEvent(e))
                    {
                        if (e.type == sf::Event::Closed || e.type == sf::Event::KeyPressed)
                            if (e.key.code == sf::Keyboard::E) {
                                return;
                            }
                    }
                }
            }
            // ========== Handle Name Input Before Game Starts ==========
            if (gameStarted && enteringName)
            {
                handleEvents();

                // Draw the name input screen
                window.clear();
                window.draw(backgroundSprite);
                window.draw(nameInputPrompt);
                window.draw(nameInputField);
                window.display();
                continue; // Skip game logic while entering name
            }


            // ========== Game Loop Logic ==========
            float dt = clock.restart().asSeconds(); // Calculate time since last frame
            timer += dt; // Add to timer

            handleEvents(); // Handle player input events
            if (!gameStarted)
            {
                handleEvents(); // Handle keyboard input
                window.clear();
                window.draw(backgroundSprite);
                window.draw(nameInputField); // Show "Enter Name:" text

                window.display(); // Skip the rest of the loop while entering name
            }

            // ========== Pause Menu ==========
            if (isPaused)
            {
                // Update paused score
                pauseScoreText.setString("Score: " + to_string(score));

                // Draw paused screen
                window.clear();
                window.draw(backgroundSprite);
                drawField();
                window.draw(sideBar);
                window.draw(pauseText);
                window.draw(pauseScoreText);
                window.draw(pauseExitText);
                window.display();
                continue; // Skip updates while paused
            }

            // ========== Fast Drop with Down Arrow ==========
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
                delay = 0.05f;

            // ========== Piece Fall and Lock ==========
            if (timer > delay)
            {
                if (!currentPiece.tryMoveDownAndLock())
                {
                    // Lock current piece and generate next
                    currentPiece = nextPiece;
                    nextPiece = Tetrimino();
                    scoreText.setString("Score: " + to_string(score)); // Update score
                }
                timer = 0; // Reset fall timer
            }

            // ========== Game Updates ==========
            clearLines();  // Clear full rows
            draw();        // Render everything

            // Reset input state
            dx = 0;
            rotateFlag = false;
            delay = 0.3f; // Restore default fall delay
        }
    }


    // Handle user input and game events (movement, rotation, pause, name input, etc.)
    void handleEvents()
    {
        sf::Event e;
        while (window.pollEvent(e))
        {
            // Handle window close event
            if (e.type == sf::Event::Closed)
            {
                window.close();
                return;
            }

            // If the player is entering their name before the game starts
            if (enteringName)
            {
                if (e.type == sf::Event::TextEntered)
                {
                    if (e.text.unicode == '\b') // Handle backspace
                    {
                        if (!nameInputBuffer.empty())
                            nameInputBuffer.pop_back();
                    }
                    else if (e.text.unicode == '\r' || e.text.unicode == '\n') // Enter key pressed
                    {
                        // Finalize name and start the game
                        playerName = nameInputBuffer.empty() ? "Player" : nameInputBuffer;
                        playerNameText.setString("Player: " + playerName);
                        enteringName = false;
                    }
                    else if (e.text.unicode < 128 && nameInputBuffer.size() < 15) // Character input
                    {
                        nameInputBuffer += static_cast<char>(e.text.unicode);
                    }

                    // Update on-screen input field
                    nameInputField.setString("Enter Name: " + nameInputBuffer);
                }
                continue; // Skip rest of input handling while typing
            }

            // Handle menu key input before the game starts
            if (!gameStarted)
            {
                if (e.type == sf::Event::KeyPressed)
                {
                    if (e.key.code == sf::Keyboard::S)
                    {
                        // Start name input mode
                        enteringName = true;
                        nameInputBuffer.clear();
                        nameInputField.setString("Enter Name: ");
                    }
                    else if (e.key.code == sf::Keyboard::E)
                    {
                        window.close(); // Exit the game from main menu
                    }
                }
                continue; // Skip further input if game hasn't started
            }

            // Handle pause toggle
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::P)
            {
                isPaused = !isPaused;
            }

            // Allow exit during pause
            if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::E && isPaused)
            {
                window.close();
            }

            // Handle movement and rotation only if not paused
            if (!isPaused && e.type == sf::Event::KeyPressed)
            {
                if (e.key.code == sf::Keyboard::Up)
                {
                    rotateFlag = true; // Rotate the piece
                }
                else if (e.key.code == sf::Keyboard::Left)
                {
                    dx = -1; // Move left
                }
                else if (e.key.code == sf::Keyboard::Right)
                {
                    dx = 1; // Move right
                }
            }
        }

        // Apply movement and rotation outside of event loop if not paused
        if (!isPaused)
        {
            if (dx) currentPiece.move(dx);       // Apply horizontal move
            if (rotateFlag) currentPiece.rotate(); // Apply rotation
        }
    }


    // Draw the game field with all the tiles (filled cells)
    void drawField()
    {
        for (int i = 0; i < BOARD_ROWS; ++i) // Iterate over rows
        {
            for (int j = 0; j < BOARD_COLS; ++j) // Iterate over columns
            {
                if (!board[i][j])
                {
                    continue; // Skip empty cells
                }

                // Set the texture for the current tile
                tileSprite.setTextureRect(sf::IntRect(board[i][j] * 18, 0, 18, 18));
                tileSprite.setPosition(j * TILE_SIZE + offsetX, i * TILE_SIZE + offsetY); // Set the position
                window.draw(tileSprite); // Draw the tile on the window
            }
        }
    }


    void draw()
    {
        window.clear(); // Clear the window
        window.draw(backgroundSprite); // Draw the background
        drawField(); // Draw the main game field

        currentPiece.draw(window, tileSprite); // Draw the current piece

        // Draw sidebar (score, next piece)
          // Draw the sidebar with player name above the score
        window.draw(sideBar);
        window.draw(playerNameText);
        window.draw(scoreText); //Display score
        window.draw(nextLabel);
        window.draw(pauseLabel);// Display next 

        // Draw nextPiece preview
        float prevScale = (TILE_SIZE * 0.6f) / 18.f;
        tileSprite.setScale(prevScale, prevScale);
        const Point* blocks = nextPiece.getBlocks();
        int color = nextPiece.getColor();
        float px = nextLabel.getPosition().x + 20;
        float py = nextLabel.getPosition().y + 100;
        for (int i = 0; i < 4; ++i)
        {
            tileSprite.setTextureRect(sf::IntRect(color * 18, 0, 18, 18));
            tileSprite.setPosition(
                px + blocks[i].x * TILE_SIZE * 0.6f,
                py + blocks[i].y * TILE_SIZE * 0.6f
            );
            window.draw(tileSprite);
        }

        if (enteringName) {
            window.draw(nameInputPrompt);
            window.draw(nameInputField);
        }

        // Restore full scale for the tile
        tileSprite.setScale(TILE_SIZE / 18.f, TILE_SIZE / 18.f);

        // Draw pause text and score if game is paused
        if (isPaused)
        {
            pauseScoreText.setString("Score: " + to_string(score));
            auto sb = pauseScoreText.getLocalBounds();
            pauseScoreText.setPosition(
                window.getSize().x / 2.f - sb.width / 2.f,
                pauseText.getPosition().y + 70
            );

            window.draw(pauseText);
            window.draw(pauseScoreText);
            window.draw(pauseExitText);
        }

        window.display(); // Update the display
    }

    void clearLines()
    {
        int rowToCheck = BOARD_ROWS - 1;  // Start checking from the bottom row
        int linesCleared = 0;

        // Loop through each row starting from the bottom
        for (int row = BOARD_ROWS - 1; row >= 0; --row)
        {
            int filledBlocks = 0;

            // Count the number of filled blocks in the current row
            for (int col = 0; col < BOARD_COLS; ++col)
            {
                if (board[row][col])  // Check if there is a block in this cell
                {
                    filledBlocks++;
                }
            }

            // If the row is completely filled
            if (filledBlocks == BOARD_COLS)
            {
                linesCleared++;  // Increase the cleared lines count
            }
            else
            {
                // If the row isn't filled, move the blocks down
                for (int col = 0; col < BOARD_COLS; ++col)
                {
                    board[rowToCheck][col] = board[row][col];
                }
                rowToCheck--;  // Move the row down
            }
        }

        // If any lines were cleared, update the score
        if (linesCleared)
        {
            score += 100 * linesCleared;  // Award points for each cleared line
            scoreText.setString("Score: " + to_string(score));  // Update the score display
        }
    }

};
int main()
{
    sf::Music bgMusic;
    if (!bgMusic.openFromFile("sound.ogg")) {
        std::cerr << "Failed to load music!" << std::endl;
        return 0;
    }

    bgMusic.setLoop(true);
    bgMusic.play();
    try
    {
        Board board;  // Initialize the board
        board.run();  // Run the game
    }
    catch (exception& e)
    {
        cerr << "Error: " << e.what() << "\n"; // Catch and print any errors
        return 1;
    }
    return 0;
}