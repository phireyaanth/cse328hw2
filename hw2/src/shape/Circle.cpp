#include <glm/glm.hpp>

#include "shape/Circle.h"
#include "util/Shader.h"

Circle::Circle(Shader * shader, const std::vector<glm::vec3> & parameters)
    : GLShape(shader), parameters(parameters)
{
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(glm::vec3),
                          reinterpret_cast<void *>(0));

    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizei>(parameters.size() * sizeof(glm::vec3)),
                 parameters.data(),
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Circle::setParameters(const std::vector<glm::vec3> & newParameters)
{
    parameters = newParameters;

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizei>(parameters.size() * sizeof(glm::vec3)),
                 parameters.data(),
                 GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Circle::render(float, bool)
{
    pShader->use();
    pShader->setMat3("model", model);

    glBindVertexArray(vao);
    glPatchParameteri(GL_PATCH_VERTICES, 1);

    std::cout << "Circle::render count = " << parameters.size() << "\n";

    if (!parameters.empty())
    {
        glDrawArrays(GL_PATCHES, 0, static_cast<GLsizei>(parameters.size()));
    }

    glBindVertexArray(0);
}