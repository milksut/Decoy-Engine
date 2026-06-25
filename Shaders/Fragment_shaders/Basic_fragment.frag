#version 330 core

out vec4 FragColor;
uniform vec3 ourColor;

void main()
{
    //FragColor = vec4(ourColor, 1.0);
    FragColor = vec4(0.3,0.5, 0.9,1.0);
}