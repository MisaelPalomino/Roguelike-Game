#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>

// 1. Mapa

class TileMap : public sf::Drawable, public sf::Transformable {
public:
    bool load(const std::string& tileset, sf::Vector2u tileSize, const int* tiles, unsigned int width, unsigned int height);

private:
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

    sf::VertexArray m_vertices;
    sf::Texture m_tileset;
};

bool TileMap::load(const std::string& tileset, sf::Vector2u tileSize, const int* tiles, unsigned int width, unsigned int height) {
    if (!m_tileset.loadFromFile(tileset))
        return false;

    m_vertices.setPrimitiveType(sf::Quads);
    m_vertices.resize(width * height * 4);

    for (unsigned int i = 0; i < width; ++i) {
        for (unsigned int j = 0; j < height; ++j) {
            int tileNumber = tiles[i + j * width];

            int tu = tileNumber % (m_tileset.getSize().x / tileSize.x);
            int tv = tileNumber / (m_tileset.getSize().x / tileSize.x);

            sf::Vertex* quad = &m_vertices[(i + j * width) * 4];

            quad[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);
            quad[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
            quad[2].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y);
            quad[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);

            quad[0].texCoords = sf::Vector2f(tu * tileSize.x, tv * tileSize.y);
            quad[1].texCoords = sf::Vector2f((tu + 1) * tileSize.x, tv * tileSize.y);
            quad[2].texCoords = sf::Vector2f((tu + 1) * tileSize.x, (tv + 1) * tileSize.y);
            quad[3].texCoords = sf::Vector2f(tu * tileSize.x, (tv + 1) * tileSize.y);
        }
    }

    return true;
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();
    states.texture = &m_tileset;
    target.draw(m_vertices, states);
}

// 2. Personaje

class Player {
public:
    Player(const std::string& filename, sf::Vector2u tileSize, unsigned int numFrames, const int* level, unsigned int width, unsigned int height);
    void handleInput();
    void setHandleInput(sf::Keyboard::Key upKey, sf::Keyboard::Key downKey, sf::Keyboard::Key leftKey, sf::Keyboard::Key rightKey);
    void update();
    void draw(sf::RenderWindow& window);
    void setTexture(const std::string& filename) {
        texture.loadFromFile(filename);
    }
    sf::Vector2f getPosition() const {
        sf::Vector2f camara = sprite.getPosition();
        camara.x += 8.0f;
        camara.y += 8.0f;
        return camara;
    };

    void takeDamage(unsigned int amount);  // Método para recibir daño
    void attack();  // Método para atacar

private:
    sf::Sprite sprite;
    sf::Texture texture;
    sf::Vector2u tileSize;
    sf::Vector2i position;
    unsigned int numFrames;
    unsigned int currentFrame;
    sf::Clock animationClock;
    sf::Clock movementClock;
    sf::Clock attackClock;  // Añadido para controlar la duración del ataque
    sf::Clock jumpClock;  // Para controlar la duración del salto
    sf::Time animationInterval;
    sf::Time movementInterval;
    sf::Time attackDuration;  // Duración del ataque
    sf::Time jumpDuration;  // Duración del salto
    int positionRectSprite; //Para el sprite
    const int* level; //Almacenar el mapa
    unsigned int width, height; //Dimension del mapa
    bool isAttacking;  // Añadido para indicar si el jugador está atacando
    bool isRunning;  // Para indicar si el jugador está corriendo
    bool isJumping;  // Para indicar si el jugador está saltando

    unsigned int health;  // Vida del jugador
    unsigned int damage;  // Daño que el jugador puede infligir
    unsigned int shield;  // Escudo del jugador para reducir el daño recibido

    sf::Keyboard::Key upKey;
    sf::Keyboard::Key downKey;
    sf::Keyboard::Key leftKey;
    sf::Keyboard::Key rightKey;

    void updateAnimation();
    bool canMoveTo(sf::Vector2i newPosition); //Verificar colision
    void startAttack();  // Añadido para iniciar el ataque
    void updateAttack();  // Añadido para actualizar el estado de ataque
    void startJump();  // Para iniciar el salto
    void updateJump();  // Para actualizar el estado del salto
};

Player::Player(const std::string& filename, sf::Vector2u tileSize, unsigned int numFrames, const int* level, unsigned int width, unsigned int height) 
    : tileSize(tileSize), position(1, 1), numFrames(numFrames), currentFrame(0),
      animationInterval(sf::seconds(0.1f)), movementInterval(sf::seconds(0.2f)),
      attackDuration(sf::seconds(0.5f)),
      jumpDuration(sf::seconds(0.5f)),
      positionRectSprite(32), level(level), width(width), height(height),
      isAttacking(false), isRunning(false), isJumping(false), // Inicializar estados
      health(100), damage(10), shield(5) { // Inicializar atributos
    if (!texture.loadFromFile(filename)) {
        std::cerr << "Error: Imposible cargar la textura " << filename << std::endl;  // Mensaje de error
        throw std::runtime_error("Imposible cargar la textura");
    }
    sprite.setTexture(texture);
    sprite.setTextureRect(sf::IntRect(32, 32, tileSize.x/2, tileSize.y/2));
    sprite.setPosition(position.x * tileSize.x, position.y * tileSize.y);
    sprite.setScale(1.0f,1.0f);
    std::cout << "Player inicializado en la posicion: (" << position.x << ", " << position.y << ")" << std::endl;  // Mensaje de inicialización
}

void Player::setHandleInput(sf::Keyboard::Key up, sf::Keyboard::Key down, sf::Keyboard::Key left, sf::Keyboard::Key right) {
    upKey = up;
    downKey = down;
    leftKey = left;
    rightKey = right;
}

void Player::handleInput() {
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        isRunning = true;
        movementInterval = sf::seconds(0.1f);  // Reducir el intervalo de movimiento para correr
    } else {
        isRunning = false;
        movementInterval = sf::seconds(0.2f);  // Intervalo normal de movimiento
    }

    if (movementClock.getElapsedTime() > movementInterval) {
        sf::Vector2i newPosition = position;
        if (sf::Keyboard::isKeyPressed(upKey)) {
            positionRectSprite = 272;
            setTexture("resources/Characters/walk.png");
            newPosition.y -= 1;
            movementClock.restart();
            std::cout << "Moviendo arriba a: (" << position.x << ", " << position.y << ")" << std::endl;  // Mensaje de movimiento
        } else if (sf::Keyboard::isKeyPressed(downKey)) {
            positionRectSprite = 192;
            setTexture("resources/Characters/walk.png");
            newPosition.y += 1;
            movementClock.restart();
            std::cout << "Moviendo abajo a: (" << position.x << ", " << position.y << ")" << std::endl;  // Mensaje de movimiento
        } else if (sf::Keyboard::isKeyPressed(leftKey)) {
            positionRectSprite = 112;
            setTexture("resources/Characters/walk.png");
            newPosition.x -= 1;
            movementClock.restart();
            std::cout << "Moviendo izquierda a: (" << position.x << ", " << position.y << ")" << std::endl;  // Mensaje de movimiento
        } else if (sf::Keyboard::isKeyPressed(rightKey)) {
            positionRectSprite = 32;
            setTexture("resources/Characters/walk.png");
            newPosition.x += 1;
            movementClock.restart();
            std::cout << "Moviendo derecha a: (" << position.x << ", " << position.y << ")" << std::endl;  // Mensaje de movimiento
        } else {
            setTexture("resources/Characters/idle.png");
        }

        if(canMoveTo(newPosition)) {
            position = newPosition; 
        }
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::M)) {
        startJump();
    }

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
        startAttack();
    }
}

bool Player::canMoveTo(sf::Vector2i newPosition) {
    if (newPosition.x < 0 || newPosition.x >= static_cast<int>(width) ||
        newPosition.y < 0 || newPosition.y >= static_cast<int>(height)) {
        return false;
    }

    if (isJumping) {
        return true;  // Permitir moverse a cualquier tile mientras se está saltando
    }

    int tileValue = level[newPosition.x + newPosition.y * width];
    return tileValue == 1 || tileValue == 2;
}

void Player::takeDamage(unsigned int amount) {
    int actualDamage = amount - shield;
    if (actualDamage < 0) actualDamage = 0;

    health -= actualDamage;
    std::cout << "Player tomo " << actualDamage << " de danio. Vida ahora: " << health << std::endl;

    if (health <= 0) {
        std::cout << "Player vida es 0" << std::endl;
        // Añadir lógica para manejar la muerte del jugador, y que pueda revivir
    }
}

void Player::startAttack() {
    if (!isAttacking) {
        isAttacking = true;
        attackClock.restart();
        std::cout << "Player atacando" << std::endl;
        // Quizás otro tipo de ataques
    }
}

void Player::updateAttack() {
    if (isAttacking) {
        if (attackClock.getElapsedTime() > attackDuration) {
            isAttacking = false;
            std::cout << "Ataque terminado" << std::endl;
        } else {
            // Actualizar el rectángulo de textura para mostrar el sprite de ataque
            setTexture("resources/Characters/damage.png");
            return;
        }
    }
}

void Player::startJump() {
    if (!isJumping) {
        isJumping = true;
        jumpClock.restart();
        std::cout << "Player saltando" << std::endl;
    }
}

void Player::updateJump() {
    if (isJumping) {
        if (jumpClock.getElapsedTime() > jumpDuration) {
            isJumping = false;
            std::cout << "Salto terminado" << std::endl;
        } else {
            setTexture("resources/Characters/jump.png");
            return;
        }
    }
}

void Player::update() {
    sprite.setPosition(position.x * 16, position.y * 16);
    std::cout << "Actualizando sprite a: (" << sprite.getPosition().x << ", " << sprite.getPosition().y << ")" << std::endl;  // Mensaje de actualización
    updateAttack();
    updateJump();
    updateAnimation();
}

void Player::draw(sf::RenderWindow& window) {
    window.draw(sprite);
}

void Player::updateAnimation() {
    if (animationClock.getElapsedTime() > animationInterval) {
        currentFrame = (currentFrame + 1) % numFrames;
        int tu = currentFrame % (texture.getSize().x / tileSize.x);
        //int tv = currentFrame / (texture.getSize().x / tileSize.x);
        sprite.setTextureRect(sf::IntRect(tu * tileSize.x + 32, positionRectSprite, tileSize.x/4, tileSize.y/4));
        animationClock.restart();
    }
}

// 3. Juego Principal

const int level[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 1, 1, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Roguelike");

    TileMap map;
    if (!map.load("resources/Tiles/grass.png", sf::Vector2u(16, 16), level, 20, 10)) {
        return -1;
    }

    Player player("resources/Characters/idle.png", sf::Vector2u(80, 80), 4, level, 20, 10);
    Player player2("resources/Characters/idle.png", sf::Vector2u(80, 80), 4, level, 20, 10);

    player.setHandleInput(sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::A, sf::Keyboard::D);
    player2.setHandleInput(sf::Keyboard::Up, sf::Keyboard::Down, sf::Keyboard::Left, sf::Keyboard::Right);

    sf::View view(sf::FloatRect(0, 0, 400, 300));
    view.setCenter(player.getPosition());

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        view.setCenter(player.getPosition());
        window.setView(view);

        player.handleInput();
        player2.handleInput();
        player.update();
        player2.update();

        window.clear();
        window.draw(map);
        player.draw(window);
        player2.draw(window);
        window.display();
    }

    return 0;
}