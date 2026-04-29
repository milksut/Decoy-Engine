#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT {
    vec2 tex;
} gs_in[];

out vec2 TexCoord;

uniform float width;
uniform float height;
uniform float tex_width;
uniform float tex_height;

void main() {
    //left bottom
    TexCoord = gs_in[0].tex;
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    //right bottom
    TexCoord = gs_in[0].tex + vec2(tex_width, 0.0);
    gl_Position = gl_in[0].gl_Position + vec4(width, 0.0, 0.0, 0.0);
    EmitVertex();

    //left top
    TexCoord = gs_in[0].tex + vec2(0.0, tex_height);
    gl_Position = gl_in[0].gl_Position + vec4(0.0, height, 0.0, 0.0);
    EmitVertex();

    //right top
    TexCoord = gs_in[0].tex + vec2(tex_width, tex_height);
    gl_Position = gl_in[0].gl_Position + vec4(width, height, 0.0, 0.0);
    EmitVertex();

    EndPrimitive();
}  
