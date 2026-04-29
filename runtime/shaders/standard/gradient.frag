#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 uColor1;
uniform vec4 uColor2;
uniform int uVertical;

void main() {
    float t = (uVertical == 1) ? TexCoord.x : TexCoord.y;
    FragColor = mix(uColor1, uColor2, t);
}
