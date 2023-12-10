#version 110

varying vec2 texCoord;
#define outputColor gl_FragData[0]

float checker(vec2 c) {
    vec2 ic = floor(c * vec2(16.0));
    return float(mod(ic.x + ic.y, 2.0));
}

uniform int render_mode;

void main() {
    if (render_mode == 0)
        outputColor = vec4(vec3(1.0, 1.0, 0.0) * checker(texCoord), 1.0);
    else if (render_mode == 1)
        outputColor = vec4(texCoord, 0.0, 1.0);
    else
        outputColor = vec4(1.0, 0.0, 0.0, 1.0);
}
