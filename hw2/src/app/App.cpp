#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "app/App.h"
#include "shape/Circle.h"
#include "util/Shader.h"

namespace
{
constexpr float kEps = 1.0e-6f;
}

App & App::getInstance()
{
    static App instance;
    return instance;
}

void App::run()
{
    while (!glfwWindowShouldClose(pWindow))
    {
        perFrameTimeLogic(pWindow);
        processKeyInput(pWindow);
        update();

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        render();

        glfwSwapBuffers(pWindow);
        glfwPollEvents();
    }
}

void App::cursorPosCallback(GLFWwindow * window, double xpos, double ypos)
{
    App & app = *reinterpret_cast<App *>(glfwGetWindowUserPointer(window));

    app.mousePos.x = xpos;
    app.mousePos.y = static_cast<double>(app.currentHeight) - ypos;

    if (app.mousePressed)
    {
        app.lastMouseLeftPressPos = app.mousePos;
    }
}

void App::framebufferSizeCallback(GLFWwindow * window, int width, int height)
{
    App & app = *reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
    app.currentWidth = width;
    app.currentHeight = height;
    glViewport(0, 0, width, height);
}

void App::keyCallback(GLFWwindow * window, int key, int, int action, int)
{
    App & app = *reinterpret_cast<App *>(glfwGetWindowUserPointer(window));

    if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_A)
        {
            app.animationEnabled = !app.animationEnabled;
        }
        else if (key == GLFW_KEY_1)
        {
            app.bouncingBallMode = true;
        }
    }
}

void App::mouseButtonCallback(GLFWwindow * window, int button, int action, int)
{
    App & app = *reinterpret_cast<App *>(glfwGetWindowUserPointer(window));

    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            double xpos = 0.0;
            double ypos = 0.0;
            glfwGetCursorPos(window, &xpos, &ypos);

            app.mousePressed = true;
            app.mousePos.x = xpos;
            app.mousePos.y = static_cast<double>(app.currentHeight) - ypos;
            app.lastMouseLeftClickPos = app.mousePos;
            app.lastMouseLeftPressPos = app.mousePos;

            if (app.bouncingBallMode)
            {
                app.trySpawnBall();
            }
        }
        else if (action == GLFW_RELEASE)
        {
            app.mousePressed = false;
        }
    }
}

void App::scrollCallback(GLFWwindow *, double, double)
{
}

void App::perFrameTimeLogic(GLFWwindow * window)
{
    App & app = *reinterpret_cast<App *>(glfwGetWindowUserPointer(window));

    double currentFrame = glfwGetTime();
    app.timeElapsedSinceLastFrame = currentFrame - app.lastFrameTimeStamp;
    app.lastFrameTimeStamp = currentFrame;
}

void App::processKeyInput(GLFWwindow * window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

App::App() : Window(kWindowWidth, kWindowHeight, kWindowName, nullptr, nullptr)
{
    glfwSetWindowUserPointer(pWindow, this);
    glfwSetCursorPosCallback(pWindow, cursorPosCallback);
    glfwSetFramebufferSizeCallback(pWindow, framebufferSizeCallback);
    glfwSetKeyCallback(pWindow, keyCallback);
    glfwSetMouseButtonCallback(pWindow, mouseButtonCallback);
    glfwSetScrollCallback(pWindow, scrollCallback);

    glfwGetFramebufferSize(pWindow, &currentWidth, &currentHeight);

    glViewport(0, 0, currentWidth, currentHeight);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(6.0f);
    glPointSize(1.0f);

    pCircleShader = std::make_unique<Shader>("src/shader/circle.vert.glsl",
                                             "src/shader/circle.tesc.glsl",
                                             "src/shader/circle.tese.glsl",
                                             "src/shader/circle.frag.glsl");

    pCircleShape = std::make_unique<Circle>(pCircleShader.get(), std::vector<glm::vec3>{});
}

App::BallConfig App::readBallConfig() const
{
    BallConfig cfg;

    std::ifstream fin("etc/config.txt");
    if (!(fin >> cfg.radiusNdc >> cfg.velocityNdc.x >> cfg.velocityNdc.y))
    {
        throw std::runtime_error("config.txt must contain: r vx vy");
    }

    return cfg;
}

glm::vec2 App::screenToNdc(const glm::dvec2 & p) const
{
    return glm::vec2(
        static_cast<float>((2.0 * p.x) / static_cast<double>(currentWidth) - 1.0),
        static_cast<float>((2.0 * p.y) / static_cast<double>(currentHeight) - 1.0)
    );
}

glm::vec3 App::ballToCircleParameter(const BallState & ball) const
{
    const float xPx = (ball.centerNdc.x + 1.0f) * 0.5f * static_cast<float>(currentWidth);
    const float yPx = (ball.centerNdc.y + 1.0f) * 0.5f * static_cast<float>(currentHeight);
    const float rPx = ball.radiusNdc * 0.5f * static_cast<float>(currentWidth);

    return glm::vec3(xPx, yPx, rPx);
}

void App::syncCircleRenderer()
{
    std::vector<glm::vec3> params;
    params.reserve(balls.size());

    for (const auto & ball : balls)
    {
        params.emplace_back(ballToCircleParameter(ball));
    }

    pCircleShape->setParameters(params);
}

bool App::canSpawnBall(const BallState & candidate) const
{
    if (candidate.centerNdc.x - candidate.radiusNdc < -1.0f ||
        candidate.centerNdc.x + candidate.radiusNdc >  1.0f ||
        candidate.centerNdc.y - candidate.radiusNdc < -1.0f ||
        candidate.centerNdc.y + candidate.radiusNdc >  1.0f)
    {
        std::cout << "Rejected: overlaps wall\n";
        std::cout << "center = (" << candidate.centerNdc.x << ", " << candidate.centerNdc.y << ")\n";
        std::cout << "radius = " << candidate.radiusNdc << "\n";
        return false;
    }

    for (const auto & ball : balls)
{
    glm::vec2 d = candidate.centerNdc - ball.centerNdc;
    float dist2 = glm::dot(d, d);
    float dist = std::sqrt(dist2);
    float minDist = candidate.radiusNdc + ball.radiusNdc;

    std::cout << "candidate center = (" << candidate.centerNdc.x
              << ", " << candidate.centerNdc.y << ")\n";
    std::cout << "existing center  = (" << ball.centerNdc.x
              << ", " << ball.centerNdc.y << ")\n";
    std::cout << "dist = " << dist << "  minDist = " << minDist << "\n";

    if (dist2 < minDist * minDist)
    {
        std::cout << "Rejected: overlaps another ball\n";
        return false;
    }
}

    return true;
}

void App::trySpawnBall()
{
    BallConfig cfg;
    try
    {
        cfg = readBallConfig();
    }
    catch (const std::exception & ex)
    {
        std::cerr << ex.what() << '\n';
        return;
    }

    BallState candidate;
    candidate.centerNdc = screenToNdc(lastMouseLeftClickPos);
    candidate.velocityNdc = cfg.velocityNdc;
    candidate.radiusNdc = cfg.radiusNdc;

    if (!canSpawnBall(candidate))
    {
        std::cout << "Spawn rejected.\n";
        return;
    }

    balls.emplace_back(candidate);
    syncCircleRenderer();
}

void App::resolveWallCollision(BallState & ball)
{
    if (ball.centerNdc.x + ball.radiusNdc > 1.0f)
    {
        ball.centerNdc.x = 1.0f - ball.radiusNdc;
        ball.velocityNdc.x = -std::abs(ball.velocityNdc.x);
    }
    else if (ball.centerNdc.x - ball.radiusNdc < -1.0f)
    {
        ball.centerNdc.x = -1.0f + ball.radiusNdc;
        ball.velocityNdc.x = std::abs(ball.velocityNdc.x);
    }

    if (ball.centerNdc.y + ball.radiusNdc > 1.0f)
    {
        ball.centerNdc.y = 1.0f - ball.radiusNdc;
        ball.velocityNdc.y = -std::abs(ball.velocityNdc.y);
    }
    else if (ball.centerNdc.y - ball.radiusNdc < -1.0f)
    {
        ball.centerNdc.y = -1.0f + ball.radiusNdc;
        ball.velocityNdc.y = std::abs(ball.velocityNdc.y);
    }
}

void App::resolveBallCollision(BallState & a, BallState & b)
{
    glm::vec2 delta = b.centerNdc - a.centerNdc;
    float dist2 = glm::dot(delta, delta);
    float minDist = a.radiusNdc + b.radiusNdc;

    if (dist2 > minDist * minDist)
    {
        return;
    }

    float dist = std::sqrt(std::max(dist2, kEps));
    glm::vec2 n = (dist > kEps) ? delta / dist : glm::vec2(1.0f, 0.0f);
    glm::vec2 t(-n.y, n.x);

    // Position correction if overlapping
    float penetration = minDist - dist;
    if (penetration > 0.0f)
    {
        glm::vec2 correction = 0.5f * penetration * n;
        a.centerNdc -= correction;
        b.centerNdc += correction;
    }

    // Only handle collision if moving toward each other
    float rel = glm::dot(b.velocityNdc - a.velocityNdc, n);
    if (rel >= 0.0f)
    {
        return;
    }

    float aN = glm::dot(a.velocityNdc, n);
    float aT = glm::dot(a.velocityNdc, t);
    float bN = glm::dot(b.velocityNdc, n);
    float bT = glm::dot(b.velocityNdc, t);

    // Equal-mass elastic collision: swap normal components
    a.velocityNdc = bN * n + aT * t;
    b.velocityNdc = aN * n + bT * t;
}

void App::update()
{
    if (!animationEnabled || !bouncingBallMode)
    {
        return;
    }

    float dt = static_cast<float>(timeElapsedSinceLastFrame);

    for (auto & ball : balls)
    {
        ball.centerNdc += ball.velocityNdc * dt;
        resolveWallCollision(ball);
    }

    for (std::size_t i = 0; i < balls.size(); ++i)
    {
        for (std::size_t j = i + 1; j < balls.size(); ++j)
        {
            resolveBallCollision(balls[i], balls[j]);
        }
    }

    syncCircleRenderer();
}

void App::render()
{
    pCircleShader->use();
    pCircleShader->setFloat("windowWidth", static_cast<float>(currentWidth));
    pCircleShader->setFloat("windowHeight", static_cast<float>(currentHeight));

    pCircleShape->render(static_cast<float>(timeElapsedSinceLastFrame), animationEnabled);
}