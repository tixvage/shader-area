#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec2 uv = fragTexCoord - vec2(0.5);

    float d = length(uv);
    d -= 0.3;
    d = abs(d);
    d = 0.01 / d;
    vec3 color = vec3(d * 0.6, d * 0.5, d * 0.8);

    finalColor = vec4(color, 1);
}
