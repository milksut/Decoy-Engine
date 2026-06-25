#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

layout(std140) uniform projectionXview_block { mat4 projectionXview; };

out vec4 ourColor;

void main()
{
    ourColor = aColor;
    gl_Position = projectionXview * vec4(aPos, 1.0);
}