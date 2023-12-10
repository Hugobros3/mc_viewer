#version 110

attribute vec3 vertexIn;
attribute vec2 texCoordIn;

uniform mat4 myMatrix;
uniform ivec3 chunk_position;

varying vec2 texCoord;

void main() {
    gl_Position = myMatrix * vec4(vertexIn + vec3(chunk_position * ivec3(16)), 1.0);
    texCoord = texCoordIn;
}
