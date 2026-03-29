#ifndef APP_H
#define APP_H

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include "app/Window.h"

class Shader;
class Circle;

class App : private Window
{
public:
    static App & getInstance();
    void run();

private:
    struct BallState
    {
        glm::vec2 centerNdc {0.0f, 0.0f};
        glm::vec2 velocityNdc {0.0f, 0.0f};
        float radiusNdc {0.1f};
    };

    struct BallConfig
    {
        float radiusNdc {0.1f};
        glm::vec2 velocityNdc {0.25f, 0.15f};
    };

    static void cursorPosCallback(GLFWwindow *, double, double);
    static void framebufferSizeCallback(GLFWwindow *, int, int);
    static void keyCallback(GLFWwindow *, int, int, int, int);
    static void mouseButtonCallback(GLFWwindow *, int, int, int);
    static void scrollCallback(GLFWwindow *, double, double);

    static void perFrameTimeLogic(GLFWwindow *);
    static void processKeyInput(GLFWwindow *);

    static constexpr char kWindowName[] {WINDOW_NAME};
    static constexpr int kWindowWidth {1000};
    static constexpr int kWindowHeight {1000};
    
    int currentWidth {kWindowWidth};
    int currentHeight {kWindowHeight};

private:
    App();

    void update();
    void render();

    BallConfig readBallConfig() const;
    glm::vec2 screenToNdc(const glm::dvec2 & p) const;
    glm::vec3 ballToCircleParameter(const BallState & ball) const;
    void syncCircleRenderer();
    bool canSpawnBall(const BallState & candidate) const;
    void trySpawnBall();
    void resolveWallCollision(BallState & ball);
    void resolveBallCollision(BallState & a, BallState & b);

    std::unique_ptr<Shader> pCircleShader {nullptr};
    std::unique_ptr<Circle> pCircleShape {nullptr};

    std::vector<BallState> balls;

    bool animationEnabled {true};
    bool bouncingBallMode {false};

    double timeElapsedSinceLastFrame {0.0};
    double lastFrameTimeStamp {0.0};

    bool mousePressed {false};
    glm::dvec2 mousePos {0.0, 0.0};
    glm::dvec2 lastMouseLeftClickPos {0.0, 0.0};
    glm::dvec2 lastMouseLeftPressPos {0.0, 0.0};
};

#endif  // APP_H