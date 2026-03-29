#version 410 core

layout (quads, equal_spacing, ccw) in;

const float kPi = 3.14159265358979323846f;

uniform mat3 model;
uniform float windowWidth;
uniform float windowHeight;

void main()
{
    vec4 params = gl_in[0].gl_Position;

    vec3 centerH = model * vec3(2.0f * params.x / windowWidth - 1.0f,
                                2.0f * params.y / windowHeight - 1.0f,
                                1.0f);

    vec2 center = centerH.xy;

    float rx = 2.0f * params.z / windowWidth;
    float ry = 2.0f * params.z / windowHeight;

    float radiusFactor = gl_TessCoord.x;
    float theta = 2.0f * kPi * gl_TessCoord.y;

    vec2 offset = vec2(rx * radiusFactor * cos(theta),
                       ry * radiusFactor * sin(theta));

    gl_Position = vec4(center + offset, 0.0f, 1.0f);
}