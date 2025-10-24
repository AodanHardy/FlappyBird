#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <deque>
#include <random>
#include <string>

using namespace sf;
using std::deque;

// ---------------------------- Screen ----------------------------
static const int SCREEN_WIDTH  = 720;
static const int SCREEN_HEIGHT = 1000;

// ---------------------------- RNG -------------------------------
struct RNG {
    std::mt19937 gen{std::random_device{}()};
    float uniform(float a, float b) {
        std::uniform_real_distribution<float> d(a, b);
        return d(gen);
    }
} rng;

// ------------------------ Digit display ------------------------
struct DigitDisplay {
    Texture digits[10];
    Sprite  spr;
    bool ok{true};

    DigitDisplay() {
        for (int i = 0; i < 10; ++i) {
            if (!digits[i].loadFromFile("assets/sprites/" + std::to_string(i) + ".png"))
                ok = false;
            digits[i].setSmooth(false);
        }
        spr.setScale(1.f, 1.f);
    }

    void setScale(float s) { spr.setScale(s, s); }

    void drawCenter(RenderWindow& window, int value, float y) {
        if (!ok) return;
        std::string s = std::to_string(value);
        float totalW = 0.f;
        for (char c : s) {
            auto& t = digits[c - '0'];
            totalW += t.getSize().x * spr.getScale().x + 6.f;
        }
        float x = (window.getSize().x - totalW) * 0.5f;
        for (char c : s) {
            spr.setTexture(digits[c - '0']);
            spr.setPosition(x, y);
            window.draw(spr);
            x += spr.getTexture()->getSize().x * spr.getScale().x + 6.f;
        }
    }
};

// ---------------------------- Bird ------------------------------
struct Bird {
    Texture frames[3];
    Sprite  sprite;
    float   animTime{0.f};
    float   velY{0.f};

    void init(float scale, float startX, float startY) {
        frames[0].loadFromFile("assets/sprites/yellowbird-upflap.png");
        frames[1].loadFromFile("assets/sprites/yellowbird-midflap.png");
        frames[2].loadFromFile("assets/sprites/yellowbird-downflap.png");
        for (auto& t : frames) t.setSmooth(false);

        sprite.setTexture(frames[1]);
        sprite.setScale(scale, scale);
        sprite.setOrigin(sprite.getLocalBounds().width * 0.5f,
                         sprite.getLocalBounds().height * 0.5f);
        sprite.setPosition(startX, startY);
    }

    void flap(float impulse) { velY = impulse; }

    void update(float dt, float gravity) {
        velY += gravity * dt;
        sprite.move(0.f, velY * dt);

        animTime += dt;
        int f = (int)(animTime * 12.f) % 3; // 12 fps
        sprite.setTexture(frames[f]);

        float angle = std::max(-30.f, std::min(70.f, velY * 0.08f));
        sprite.setRotation(angle);
    }

    FloatRect bounds() const {
        FloatRect b = sprite.getGlobalBounds();
        const float insetX = b.width * 0.12f;
        const float insetY = b.height * 0.10f;
        b.left  += insetX;  b.width  -= insetX * 2.f;
        b.top   += insetY;  b.height -= insetY * 2.f;
        return b;
    }
};

// ---------------------------- Pipes -----------------------------
// Fixed-gap model: randomize the TOP EDGE of the bottom pipe (bottomTopY)
// then place the BOTTOM EDGE of the top pipe at bottomTopY - GAP_PIX - bandPad.
struct PipePair {
    static Texture& tex() {
        static Texture t;
        static bool ok = false;
        if (!ok) { t.loadFromFile("assets/sprites/pipe-green.png"); t.setSmooth(false); ok = true; }
        return t;
    }

    Sprite top, bot;
    float  x{0.f};
    float  bottomTopY{0.f}; // top edge (Y) of the bottom pipe
    float  gapPx{160.f};    // visual gap
    bool   counted{false};

    float  pipeScale{0.72f}; // pipe thickness relative to world SCALE
    float  bandPad{0.f};     // small visual pad so the collars don’t enter the gap

    void init(float startX, float bottomTopY_, float worldScale, float gapPixels) {
        x = startX;
        bottomTopY = bottomTopY_;
        gapPx = gapPixels;
        counted = false;

        pipeScale *= worldScale;
        bandPad = 8.f * worldScale; // tweak 6–12 to taste

        auto& t = tex();
        top.setTexture(t);
        bot.setTexture(t);

        top.setScale(pipeScale, -pipeScale); // flip vertically
        bot.setScale(pipeScale,  pipeScale);

        top.setOrigin(0.f, (float)t.getSize().y); // bottom-left
        bot.setOrigin(0.f, 0.f);                  // top-left

        updateSprites();
    }

    void updateSprites() {
        float topBottomY = bottomTopY - gapPx - bandPad; // bottom edge of top pipe
        top.setPosition(x, topBottomY);
        bot.setPosition(x, bottomTopY + bandPad);        // push bottom down a hair
    }

    float width() const { return tex().getSize().x * pipeScale; }
    void  move(float dx) { x += dx; updateSprites(); }
    bool  offLeft() const { return x + width() < -50.f; }
    bool  passed(float birdX) const { return (x + width()) < birdX; }

    bool  intersects(const FloatRect& bird) const {
        FloatRect a = top.getGlobalBounds();
        FloatRect b = bot.getGlobalBounds();
        float inset = width() * 0.08f;
        a.left += inset; a.width -= inset * 2.f;
        b.left += inset; b.width -= inset * 2.f;
        return bird.intersects(a) || bird.intersects(b);
    }
};

// ---------------------------- State -----------------------------
enum class State { Ready, Playing, GameOver };

// ----------------------------- Main -----------------------------
int main() {
    RenderWindow window(VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "Flappy Bird");
    window.setFramerateLimit(60);

    // Background — scale by WIDTH so no black bars
    Texture bgTex; bgTex.loadFromFile("assets/sprites/background-day.png");
    bgTex.setSmooth(false);
    Sprite  bg(bgTex);
    const float SCALE = (float)SCREEN_WIDTH / (float)bgTex.getSize().x;
    bg.setScale(SCALE, SCALE);
    float bgHeight = bgTex.getSize().y * SCALE;
    bg.setPosition(0.f, (SCREEN_HEIGHT - bgHeight) * 0.5f);

    const float GRAVITY      = 1500.f * SCALE;
    const float FLAP_IMPULSE = -400.f * SCALE;
    const float PIPE_SPEED   = 220.f  * SCALE;
    const float PIPE_SPACING = 260.f  * SCALE;
    const float GAP_PIX      = 320.f  * SCALE;
    const float BIRD_X       = SCREEN_WIDTH * 0.28f;

    // Ground (two tiles, scrolling)
    Texture baseTex; baseTex.loadFromFile("assets/sprites/base.png");
    baseTex.setSmooth(false);
    Sprite  base1(baseTex), base2(baseTex);
    base1.setScale(SCALE, SCALE);
    base2.setScale(SCALE, SCALE);

    const float groundH = baseTex.getSize().y * SCALE;
    const float groundY = (float)SCREEN_HEIGHT - groundH;

    base1.setPosition(0.f, groundY);
    const float groundTileW = base1.getGlobalBounds().width;
    base2.setPosition(groundTileW, groundY);
    const float GROUND_SPEED = PIPE_SPEED * 0.8f;

    // Sounds
    SoundBuffer wingBuf, hitBuf, pointBuf;
    wingBuf.loadFromFile("assets/audio/wing.wav");
    hitBuf.loadFromFile("assets/audio/hit.wav");
    pointBuf.loadFromFile("assets/audio/point.wav");
    Sound wing(wingBuf), hit(hitBuf), point(pointBuf);

    // Score
    DigitDisplay digits; digits.setScale(0.9f * SCALE);

    // Bird
    Bird bird; bird.init(1.0f * SCALE, BIRD_X, SCREEN_HEIGHT * 0.45f);

    // Pipes
    deque<PipePair> pipes;

    // SAFE spawn helper: compute a valid random range so min < max on 1000px height
    auto spawnPipeAt = [&](float startX){
        // Keep the gap comfortably on-screen.
        const float topMargin    = 40.f * SCALE; // distance from top
        const float bottomMargin = 60.f * SCALE; // distance from ground

        // bottomTopY is the TOP of the bottom pipe. It must be >= (gap + pad + topMargin)
        // and <= (groundY - bottomMargin).
        float minBottomTop = (GAP_PIX + 8.f * SCALE) + topMargin;
        float maxBottomTop = groundY - bottomMargin;

        // Clamp to be safe (in case window sizes change)
        if (minBottomTop > maxBottomTop) {
            float mid = (groundY + topMargin) * 0.5f;
            minBottomTop = mid - 10.f * SCALE;
            maxBottomTop = mid + 10.f * SCALE;
        }

        float bottomTop = rng.uniform(minBottomTop, maxBottomTop);

        PipePair p; p.init(startX, bottomTop, SCALE, GAP_PIX);
        pipes.emplace_back(p);
    };

    // Seed a few columns ahead of time
    float xCarry = (float)SCREEN_WIDTH + 220.f * SCALE;
    for (int i = 0; i < 3; ++i) {
        spawnPipeAt(xCarry);
        xCarry += PIPE_SPACING + pipes.back().width();
    }

    // Timing
    Clock  clock;
    float  accumulator = 0.f;
    const float dt = 1.f / 120.f;
    float  spawnTimer = 0.f;
    const float SPAWN_INTERVAL = 1.35f; // seconds

    // State
    State state = State::Ready;
    int   score = 0;

    auto reset = [&]{
        state = State::Ready;
        score = 0;
        bird.init(1.0f * SCALE, BIRD_X, SCREEN_HEIGHT * 0.45f);
        pipes.clear();
        float x = (float)SCREEN_WIDTH + 220.f * SCALE;
        for (int i=0;i<3;i++) { spawnPipeAt(x); x += PIPE_SPACING + 64.f * SCALE; }
        spawnTimer = 0.f;
        base1.setPosition(0.f, groundY);
        base2.setPosition(groundTileW, groundY);
    };

    auto wantsFlap = [](const Event& e){
        return (e.type == Event::MouseButtonPressed) ||
               (e.type == Event::KeyPressed && (e.key.code == Keyboard::Space || e.key.code == Keyboard::Up));
    };

    // --------------------------- Loop ---------------------------
    while (window.isOpen()) {
        // Events
        Event e;
        while (window.pollEvent(e)) {
            if (e.type == Event::Closed) window.close();
            if (state == State::Ready) {
                if (wantsFlap(e)) { state = State::Playing; bird.flap(FLAP_IMPULSE); wing.play(); }
            } else if (state == State::Playing) {
                if (wantsFlap(e)) { bird.flap(FLAP_IMPULSE); wing.play(); }
            } else if (state == State::GameOver) {
                if (wantsFlap(e)) { reset(); }
            }
        }

        // Fixed update
        float frame = clock.restart().asSeconds();
        accumulator += frame;
        while (accumulator >= dt) {
            if (state == State::Playing) {
                bird.update(dt, GRAVITY);

                float dx = -PIPE_SPEED * dt;
                for (auto& p : pipes) p.move(dx);

                spawnTimer += dt;
                if (spawnTimer >= SPAWN_INTERVAL) {
                    float lastX = pipes.empty() ? (float)SCREEN_WIDTH : pipes.back().x;
                    float nextX = lastX + PIPE_SPACING + (pipes.empty() ? 64.f * SCALE : pipes.back().width());
                    spawnPipeAt(nextX);
                    spawnTimer = 0.f;
                }

                while (!pipes.empty() && pipes.front().offLeft()) pipes.pop_front();

                for (auto& p : pipes) {
                    if (!p.counted && p.passed(bird.sprite.getPosition().x)) {
                        p.counted = true; score++; point.play();
                    }
                }

                // Ground scroll/wrap
                base1.move(-GROUND_SPEED * dt, 0.f);
                base2.move(-GROUND_SPEED * dt, 0.f);
                if (base1.getPosition().x + base1.getGlobalBounds().width <= 0.f)
                    base1.setPosition(base2.getPosition().x + base2.getGlobalBounds().width, (float)groundY);
                if (base2.getPosition().x + base2.getGlobalBounds().width <= 0.f)
                    base2.setPosition(base1.getPosition().x + base1.getGlobalBounds().width, (float)groundY);

                // Collisions
                FloatRect bb = bird.bounds();
                bool hitPipe = false;
                for (auto& p : pipes) if (p.intersects(bb)) { hitPipe = true; break; }
                bool hitGround = (bb.top + bb.height) >= groundY;
                bool outTop    = (bb.top <= 0.f);
                if (hitPipe || hitGround || outTop) { hit.play(); state = State::GameOver; }
            }
            accumulator -= dt;
        }

        // Draw
        window.clear();
        window.draw(bg);
        for (auto& p : pipes) { window.draw(p.top); window.draw(p.bot); }
        window.draw(base1); window.draw(base2);
        window.draw(bird.sprite);
        if (state != State::Ready) digits.drawCenter(window, score, 60.f * SCALE);
        window.display();
    }

    return 0;
}