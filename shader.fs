#version 330 core
out vec4 FragColor;

in vec3 viewPos;

uniform vec3 color;
void main()
{
    // work out the face direction from screen-space derivatives so each
    // face gets slightly different shading (no normals needed)
    vec3 n = normalize(cross(dFdx(viewPos), dFdy(viewPos)));
    float shade = abs(dot(n, normalize(vec3(0.4, 0.7, 1.0))));
    FragColor = vec4(color * (0.35 + 0.65 * shade),1.0);
}